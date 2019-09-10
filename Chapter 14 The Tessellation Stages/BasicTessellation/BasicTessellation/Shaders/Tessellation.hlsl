#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;
	
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.PosL = vin.PosL;

    return vout;
}
 
struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor; // 边缘曲面细分因子
    float InsideTess[2] : SV_InsideTessFactor; // 内部曲面细分因子
};

// 常量外壳着色器,针对每个面片进行处理,输出网格的曲面细分因子,在HS中指定函数名
// 常量外壳着色器以面片的所有控制点作为输入,控制点会首先传至顶点着色器,因此他们的类型由VS的输出类型来确定
PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, 
					 uint patchID : SV_PrimitiveID) // 面片的ID
{
    PatchTess pt;

    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;

    float d = distance(centerW, gEyePosW);

	// 根据网格与观察点的距离来对面片进行镶嵌处理
	// 如果d>=d1,镶嵌份数为0,若d<=d0,镶嵌份数为64
	// [d0, d1]区间定义了执行镶嵌操作的距离范围

    const float d0 = 20.0f;
    const float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	// 对面片的各方面(边缘,内部)进行统一的镶嵌化处理

    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;

    return pt;
}

struct HullOut
{
    float3 PosL : POSITION;
};

// 控制点外壳着色器,以大量的控制点作为输入与输出
[domain("quad")] // 面片的类型
[partitioning("integer")] // 细分模式
[outputtopology("triangle_cw")] // 通过细分所创的
[outputcontrolpoints(4)] // 外壳着色器执行的次数,每次执行都输出1个控制点
[patchconstantfunc("ConstantHS")] // 指定常量外壳着色器函数名称的字符串
[maxtessfactor(64.0f)] // 曲面细分因子的最大值
HullOut HS(InputPatch<VertexOut, 4> p, // InputPatch: 将面片的所有点都传至外壳着色器
			uint i : SV_OutputControlPointID, // 正在被外壳着色器所处理的输出控制点
			uint patchId : SV_PrimitiveID)
{
    HullOut hout;

    hout.PosL = p[i].PosL;

    return hout;
}


struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
};

// 每当镶嵌器创建顶点时都会调用域着色器
// 可以把它看做镶嵌处理阶段后的"顶点着色器"
[domain("quad")]
DomainOut DS(PatchTess patchTess, // 曲面细分因子
			 float2 uv : SV_DomainLocation, // 镶嵌化处理后的顶点位于面域空间(patch domain space)内的参数坐标
			 const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;

	// 双线性差值
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);

    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosW = posW;
    dout.PosH = mul(posW, gViewProj);

    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    float3 colorH = normalize(pin.PosH.xyz);
    float3 colorW = normalize(pin.PosW);
    float3 color = colorH + colorW;
    return float4(color, 1.0f);
}
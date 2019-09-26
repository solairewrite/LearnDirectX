#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

	// 使用局部顶点位置作为立方体贴图查找向量
    vout.PosL = vin.PosL;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// 总是以相机作为天空盒的中心
    posW.xyz += gEyePosW;

	// 设置z=w,使天空盒位于远平面
	// 猜测: 规范化设备坐标系中,深度范围(0,1)
    vout.PosH = mul(posW, gViewProj).xyww;

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL) * gSkyCubeMapRatio;
}

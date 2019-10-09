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

    vout.PosL = vin.PosL;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// 以摄像机为中心
    posW.xyz += gEyePosW;

	// 令 z = 1, 使天空盒总在远平面
    vout.PosH = mul(posW, gViewProj).xyww;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// cpp 中天盒只是一个放大的球体,中心在原点,可能查找向量不需要规范化
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}

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

    posW.xyz += gEyePosW;

    vout.PosH = mul(posW, gViewProj).xyww;

    return vout;
}

// 这里要是误写成float,会返回红色的天空
float4 PS(VertexOut pin) : SV_Target
{
    float4 litColor = gCubeMap.Sample(gsamLinearWrap, pin.PosL);
    //litColor += float4(0.7f, 0, 0, 0);
    return litColor;
}
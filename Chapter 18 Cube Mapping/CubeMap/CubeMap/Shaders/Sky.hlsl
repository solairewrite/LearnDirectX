//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================

// Include common HLSL code.
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

	// Use local vertex position as cubemap lookup vector.
	// 用局部顶点的位置作为立方体图的查找向量
    vout.PosL = vin.PosL;
	
	// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Always center sky about camera.
	// 总是以摄像机作为天空球的中心
    posW.xyz += gEyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	// 设置z=w,即令球面总是位于远平面
	// ??为什么,我猜测z~[0,1]位于远近平面之间
    vout.PosH = mul(posW, gViewProj).xyww;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}

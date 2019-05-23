//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting. 默认着色器,目前已支持光照
//***************************************************************************************

// Defaults for number of lights. 光源数量的默认值
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting. 包含了光照所用的结构体与函数
#include "LightingUtil.hlsl"

// Constant data that varies per frame. 每帧都有变化的常量数据

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
};

// 每种材质的不同常量数据
cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float  gRoughness;
	float4x4 gMatTransform;
};

// Constant data that varies per material. 绘制过程中所用的杂项常量数据
cbuffer cbPass : register(b2)
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

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light gLights[MaxLights];
};

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;

	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
	// 假设这里进行的是等比缩放,否则需要使用世界矩阵的逆转置矩阵
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	// Transform to homogeneous clip space. 将顶点变换到齐次裁剪空间
	vout.PosH = mul(posW, gViewProj);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so renormalize it. 
	// 对法线进行差值可能导致其非规范化,因此需要再次对它进行规范化处理
	pin.NormalW = normalize(pin.NormalW);

// Vector from point being lit to eye. 表面到观察点向量
float3 toEyeW = normalize(gEyePosW - pin.PosW);

// Indirect lighting. 间接光照
float4 ambient = gAmbientLight * gDiffuseAlbedo;

// 直接光照
const float shininess = 1.0f - gRoughness;
Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
float3 shadowFactor = 1.0f;
float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
	pin.NormalW, toEyeW, shadowFactor);

float4 litColor = ambient + directLight;

// Common convention to take alpha from diffuse material. 从漫反射材质中获取alpha值的常见手段
litColor.a = gDiffuseAlbedo.a;

return litColor;
}



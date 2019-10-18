#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
    uint DiffuseMapIndex;
    uint NormalMapIndex;
    uint MatPad1;
    uint MatPad2;
};

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);

Texture2D gTextureMaps[10] : register(t2);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6); // 比较采样器,CPU中设置比较函数为 <=

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint gMaterialIndex;
    uint gObjPad0;
    uint gObjPad1;
    uint gObjPad2;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowTransform; // 将物体从世界空间变换到纹理空间
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    Light gLights[MaxLights];
};

// 将法线图采样变换到世界空间
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 uintNormalW, float3 tangentW)
{
	// 解压缩[0,1] -> [-1,1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;

	// 构建规范化正交基
    float3 N = uintNormalW; // 世界空间中的法线
    float3 T = normalize(tangentW - dot(tangentW, N) * N); // 世界空间中的切线
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

	// 从纹理空间变换到世界空间
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

// PCF: Percentage Closer Filtering, 百分比渐近过滤
// 阴影因子[0,1],对于一个点,0表示位于阴影之中,1表示阴影之外,0~1表示部分处于阴影之中
float CalcShadowFactor(float4 shadowPosH)
{
	// 通过除以w来完成投影操作
    shadowPosH.xyz /= shadowPosH.w;

	// 在NDC空间中的深度值
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips); // 获取贴图尺寸信息,输出参数

	// 纹素大小
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
	// 以这个点为中心的9个点
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

	[unroll]
    for (int i = 0; i < 9; ++i)
    {
		// 使用比较采样器,LevelZero意味着只能在最高的mipmap层级中才能执行此函数
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
    }

    return percentLit / 9.0f;
}
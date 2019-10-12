#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"

TextureCube gCubeMap : register(t0);
Texture2D gTextureMaps[10] : register(t1);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint gMaterialIndex; // 指定了材质结构体缓存中的索引
    uint gObjPad0;
    uint gObjPad1;
    uint gObjPad2;
};

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
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

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
	
    Light gLights[MaxLights];
};

// 将一个法线图样本从切线空间变换至世界空间,传入的已经是世界空间中的法线与切向量(VS中处理的)
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 uintNormalW, float3 tangentW)
{
	// 构建正交规范基
    float3 N = uintNormalW; // 法线
    float3 T = normalize(tangentW - dot(tangentW, N) * N); // 切线
    float3 B = cross(N, T); // 副切线
	// 切线空间 -> 世界空间 变换矩阵
    float3x3 TBN = float3x3(T, B, N);
	
	// 法向量的压缩: 单位向量的每个元素都在[-1,1]区间内,将其变换至[0,1],再*255得到RGB值
	// 法向量的解压缩过程: 将每个坐标分量由范围[0,1]解压至[-1,1]区间
    float3 normalT = 2.0f * normalMapSample - 1.0f;

	// 将法线从切线空间变换到世界空间
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}
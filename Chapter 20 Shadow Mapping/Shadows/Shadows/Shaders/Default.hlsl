#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    MaterialData matData = gMaterialData[gMaterialIndex];

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);

    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);

    vout.PosH = mul(posW, gViewProj);

    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;

	// 为场景阴影图而生成的投影纹理坐标(世界空间 -> 阴影图纹理空间)
    vout.ShadowPosH = mul(posW, gShadowTransform);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
    uint normalMapIndex = matData.NormalMapIndex;

    diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamLinearWrap, pin.TexC);

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.abort-0.1f);
#endif

    pin.NormalW = normalize(pin.NormalW);

    float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamLinearWrap, pin.TexC);
    // 将法线贴图采样转换到世界空间
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

	// 取消注释以关闭法线贴图
    //bumpedNormalW = pin.NormalW;

	// 像素点到眼睛的向量
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// 环境光影响
    float4 ambient = gAmbientLight * diffuseAlbedo;

	// 只有主光源才投射物体阴影
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
	// 阴影因子[0,1],对于一个点,0表示位于阴影之中,1表示阴影之外,0~1表示部分处于阴影之中
	// 将此点的深度值,分别与以此点为中心的9个点的阴影图采样值比较,计算平均值
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    const float shininess = (1.0f - roughness) * normalMapSample.a; // TODO 使用bumpedNormalW
    Material mat = { diffuseAlbedo, fresnelR0, shininess };

	// 光源影响
	// 这里使用法线贴图采样
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// 天空盒高光反射
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    litColor.a = diffuseAlbedo.a;

    return litColor;
}
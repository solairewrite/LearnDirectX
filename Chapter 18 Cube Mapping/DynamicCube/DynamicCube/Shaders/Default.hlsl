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
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

// 根据 VertexIn 计算 VertexOut, 作为顶点着色器的输入参数
VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    MaterialData matData = gMaterialData[gMaterialIndex];
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW;
	
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
	
    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;

    return vout;
}

// 1,环境光 = gAmbientLight * diffuseAlbedo(贴图 & UV 采样)
// 2,光源 = ComputeLighting()
// 3,天空盒颜色加成 = shininess * fresnelFactor(反射系数-石里克) * reflectionColor(立方体图 & 查找向量采样)
float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;

	// 漫反射反照率,通过贴图 uv 采样获得,影响环境光
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);
	
    pin.NormalW = normalize(pin.NormalW);

	// 像素点到相机的向量
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// 环境光对颜色的影响
    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;

	// 光源对颜色的影响
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
	pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// 查找向量
    float3 r = reflect(-toEyeW, pin.NormalW);
	// 天空盒的颜色影响,通过立方体贴图 查找向量 采样获得
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, pin.NormalW, r);
	// 天空盒采样颜色需要 * 反射系数(石里克) * 反射系数
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    litColor.a = diffuseAlbedo.a;

    return litColor;
}

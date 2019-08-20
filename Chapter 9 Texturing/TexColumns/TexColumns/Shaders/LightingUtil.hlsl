// 公式参考 Notes/第8章 光照.md

#define MaxLights 16

struct Light
{
	float3 Strength;
	float FalloffStart; // point/spot light only
	float3 Direction;   // directional/spot light only
	float FalloffEnd;   // point/spot light only
	float3 Position;    // point light only
	float SpotPower;    // spot light only
};

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess; // 光泽度 = 1 - 粗糙度
};

// 线性衰减因子
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	// saturate: 映射到[0,1]
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// 计算反射光,R0(介质的一种属性) = ((n-1)/(n+1))^2,n是折射率
// 菲涅尔方程以数学方法描述了入射光线被反射的百分比
// 采用石里克近似法代替菲涅尔方程
// RF(θi)=RF(0°) + (1 - RF(0°))(1 - cosθ)^5
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));

	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

	return reflectPercent;
}

// 漫反射+镜面反射(菲涅尔效应&粗糙度)
// Cs=max(L·n,0)·BL (x) RF(αh) * (m+8)/8 * (n·h)^m
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
	// m由光泽度推导而来,光泽度根据粗糙度求得
	const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec); // αh=<lightVec, halfVec>

	float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = fresnelFactor * roughnessFactor;

	// 防止结果>1,适当缩小
	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

	return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

// 方向光源
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
	// 光向量与光传播的方向相反
	float3 lightVec = -L.Direction;

	// 朗伯余弦定律
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// 点光源
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	// The vector from the surface to the light.
	float3 lightVec = L.Position - pos;

	// The distance from surface to light.
	float d = length(lightVec);

	// Range test.
	if (d > L.FalloffEnd)
		return 0.0f;

	// Normalize the light vector.
	lightVec /= d;

	// Scale light down by Lambert's cosine law.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	// Attenuate light by distance.
	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// 聚光灯
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	// The vector from the surface to the light.
	float3 lightVec = L.Position - pos;

	// The distance from surface to light.
	float d = length(lightVec);

	// Range test.
	if (d > L.FalloffEnd)
		return 0.0f;

	// Normalize the light vector.
	lightVec /= d;

	// Scale light down by Lambert's cosine law.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = L.Strength * ndotl;

	// Attenuate light by distance.
	// max(cosφ,0)^s
	float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	// Scale by spotlight
	float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
	lightStrength *= spotFactor;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// 多种光照叠加
float4 ComputeLighting(Light gLights[MaxLights], Material mat,
	float3 pos, float3 normal, float3 toEye,
	float3 shadowFactor)
{
	float3 result = 0.0f;

	int i = 0;

#if (NUM_DIR_LIGHTS > 0)
	for (i = 0; i < NUM_DIR_LIGHTS; ++i)
	{
		result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
	}
#endif

#if (NUM_POINT_LIGHTS > 0)
	for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
	{
		result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
	}
#endif

#if (NUM_SPOT_LIGHTS > 0)
	for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
	{
		result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
	}
#endif 

	return float4(result, 0.0f);
}

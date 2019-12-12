#pragma once

#include "../../../Common/d3dUtil.h"
#include "../../../Common/MathHelper.h"
#include "../../../Common/UploadBuffer.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	Light Lights[MaxLights];
};

struct SsaoConstants
{
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4X4 ProjTex; // 观察空间,投影到屏幕空间的uv坐标
	DirectX::XMFLOAT4   OffsetVectors[14]; // 环境光遮蔽采样点,立方体的8个顶点和6个面的中心点

	// For SsaoBlur.hlsl
	DirectX::XMFLOAT4 BlurWeights[3]; // 模糊权重12个值,打包成3个Float4

	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	// 观察空间中的坐标
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f; // 遮挡开始衰减的距离
	float OcclusionFadeEnd = 2.0f; // 遮挡衰减最远的距离
	float SurfaceEpsilon = 0.05f; // 一个很小的值,距离大于这和值,才会被遮挡
};

struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.5f;

	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT NormalMapIndex = 0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
	DirectX::XMFLOAT3 TangentU;
};

struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;

	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	UINT64 Fence = 0;
};

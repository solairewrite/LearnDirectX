#pragma once

#include "../../../Common/d3dUtil.h"
#include "../../../Common/MathHelper.h"
#include "../../../Common/UploadBuffer.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };
	float cbPerObjectPad1 = 0.0f; // ?
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f,0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f,0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f,0.0f,0.0f,1.0f };

	// 方向光源: [0, NUM_DIR_LIGHTS)
	// 点光源: [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS)
	// 聚光灯: [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	Light Lights[MaxLights];
};

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC; // 纹理坐标
};

// 存储CPU为一帧构建命令列表所需的资源
struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	// GPU处理完命令之前不能重置命令分配器,所以每一帧都需要自己的命令分配器
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// GPU处理完命令前不能更新常量缓存,所以每一帧都需要自己的常量缓存
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// 围栏值标记命令到达这个围栏点,允许检测CPU是否还在使用这些帧资源
	UINT64 Fence = 0;
};

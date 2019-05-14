#pragma once

#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

// 物体常量
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

// 过程常量
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f,0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f,0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
};

// 顶点结构体
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};


// 帧资源:存有CPU为构建每帧命令列表所需的资源
struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	// 在CPU处理完此命令分配器相关的命令之前,不能对它进行重置
	// 因此每帧都要有自己的命令分配器
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// 在CPU执行完引用此常量缓冲区的命令之前,不能对它进行更新
	// 所以每帧都要有自己的常量缓冲区
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr; // 过程常量缓冲区
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr; // 物体常量缓冲区

	// 通过围栏值将命令标记到此围栏点
	// 这使我们可以检查到GPU是否还在引用这些资源
	UINT64 Fence = 0;
};
#pragma once

#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

// ���峣��
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

// ���̳���
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4(); // Inverse
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

// ����ṹ��
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};


// ֡��Դ:����CPUΪ����ÿ֡�����б��������Դ
struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	// ��CPU������������������ص�����֮ǰ,���ܶ�����������
	// ���ÿ֡��Ҫ���Լ������������
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// ��CPUִ�������ô˳���������������֮ǰ,���ܶ������и���
	// ����ÿ֡��Ҫ���Լ��ĳ���������
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr; // ���̳���������
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr; // ���峣��������

	std::unique_ptr<UploadBuffer<Vertex>> WavesVB = nullptr;

	// ͨ��Χ��ֵ�������ǵ���Χ����
	// ��ʹ���ǿ��Լ�鵽GPU�Ƿ���������Щ��Դ
	UINT64 Fence = 0;
};
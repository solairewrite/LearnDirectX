#pragma once

#include "d3dUtil.h"

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
		:mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		// 常量缓冲区对硬件有特别的要求,大小必为硬件最小分配空间(256B)的整数倍
		if (isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // 常量缓冲区通常由CPU每帧更新一次,在上传堆
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		// 获得指向欲更新资源数据的指针
		// para1: 子资源的索引,指定了欲映射的子资源.对于缓冲区来说,它自身就是唯一的子资源,设为0
		// para2: 内存的映射范围, nullptr对整个资源进行映射
		// para3: 待映射资源数据的目标内存块
		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		// 当常量缓冲区更新完成后,应该在释放映射内存之前对其进行取消映射操作
		// para1: 子资源索引,指定了将被取消映射的子资源,缓冲区设为0
		// para2: 取消映射的内存范围, nullptr取消整个资源的映射
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

	// 通过CPU修改上传缓冲区中数据(eg, 观察矩阵变化)
	void CopyData(int elementIndex, const T& data)
	{
		// 将数据从系统内存复制到常量缓冲区
		memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};
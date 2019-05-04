#pragma once

#include "d3dUtil.h"

template<typename T>
class UploadBuffer // 上传缓冲区辅助函数
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		// 常量缓冲区的大小为 256 bytes 的整数倍
		// 因为硬盘只能按 m*256B的偏移量和n*256B的数据长度 查看常量数据 
		// 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // 上传堆
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData))); // 获取指向想要更新资源数据的指针

		// 只要还会修改当前的资源,就不取消映射
		// 在资源被GPU使用期间,不能向资源进行写操作(使用同步技术)
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr); // 取消映射

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T)); // 将数据从系统内存复制到常量缓冲区
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer; // 上传缓冲区
	BYTE* mMappedData = nullptr; // 想要更新资源数据的指针

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false; // 是否是常量缓冲区
};
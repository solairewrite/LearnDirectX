#include "../Public/d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{

}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
	return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}
// 离线将着色器编译到.cso(compiled shader object)文件,加载到程序中
ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
	std::ifstream fin(filename, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

// 将数据 内存->上传堆->常量缓存区(GPU)
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData, // 泛型数据顶点缓冲区/索引缓冲区
	UINT64 byteSize, // 数据大小
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource. 创建实际的默认缓冲区资源
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // 默认堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 为了将CPU内存中的数据复制到默认缓冲区,还需要创建一个处于中介位置的上传堆
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // 上传堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// Describe the data we want to copy into the default buffer. 描述希望复制到默认缓冲区中的数据
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData; // 指向某个系统内存块的指针,其中有初始化缓冲区所用的数据
	subResourceData.RowPitch = byteSize; // 对于缓冲区而言,此参数为想要复制数据的字节数
	subResourceData.SlicePitch = subResourceData.RowPitch; // 对于缓冲区而言,此参数为想要复制数据的字节数

	// 将数据复制到默认缓冲区的流程(Schedule)  
	// 辅助函数 UpdateSubresources 会将CPU内存中的数据复制到位于中介位置的上传堆
	// 然后调用 ID3D12CommandList::CopySubresourceRegion 把上传堆的数据复制到 mBuffer
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 注意: 调用上述函数后,不能立即销毁uploadBuffer
	// 因为命令列表中的复制操作可能未执行
	// 调用者得知复制完成后,可以释放uploadBuffer
	return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,	// .hlsl文件路径
	const D3D_SHADER_MACRO* defines,// 本书nullptr
	const std::string& entrypoint,	// 着色器的入口函数名
	const std::string& target)		// 指定着色器类型和版本的string
{
	UINT compileFlags = 0; // 如果处于调试模式,则使用调试标志
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // 调试模式/跳过优化
#endif

	HRESULT hr = S_OK;
	// ID3DBlob类型描述一段普通的内存块,两个方法
	// GetBufferPointer(),返回数据void*类型的指针,使用前要转换类型.
	// GetBufferSize(),返回数据大小
	ComPtr<ID3DBlob> byteCode = nullptr; // 储存编译好的着色器对象字节码
	ComPtr<ID3DBlob> errors; // 如果编译报错,储存报错的字符串
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);
	// 将错误信息输出到调试窗口
	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer()); // GetBufferPointer:返回指向 ID3DBlob 对象数据的 void* 类型的指针

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}



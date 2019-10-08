#include "CubeRendertarget.h"

CubeRenderTarget::CubeRenderTarget(ID3D12Device* device,
	UINT width, UINT height,
	DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

ID3D12Resource* CubeRenderTarget::Resource()
{
	return mCubeMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE CubeRenderTarget::Srv()
{
	return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE CubeRenderTarget::Rtv(int faceIndex)
{
	return mhCpuRtv[faceIndex];
}

D3D12_VIEWPORT CubeRenderTarget::Viewport() const
{
	return mViewport;
}

D3D12_RECT CubeRenderTarget::ScissorRect() const
{
	return mScissorRect;
}

void CubeRenderTarget::BuildDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6])
{
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;

	for (int i = 0; i < 6; ++i)
		mhCpuRtv[i] = hCpuRtv[i];

	BuildDescriptors();
}

void CubeRenderTarget::OnResize(UINT newWidth, UINT newHeight)
{
	if (mWidth != newWidth || mHeight != newHeight)
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		BuildDescriptors();
	}
}

// 为立方体贴图整体创建 SRV, 为每一个面创建 RTV
void CubeRenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // srv指定了资源维度是立方体贴图
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	// 为整个立方体图资源创建srv
	md3dDevice->CreateShaderResourceView(mCubeMap.Get(), &srvDesc, mhCpuSrv);

	// 为每个立方体面创建RTV
	for (int i = 0; i < 6; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY; // rtv指定了视图维度是2D贴图数组
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		// 为第i个元素创建渲染目标视图
		rtvDesc.Texture2DArray.FirstArraySlice = i; // 猜测这指明在mhCpuRtv[]中的索引

		// 仅为数组中的每一个元素创建一个视图
		rtvDesc.Texture2DArray.ArraySize = 1;

		// 为立方体贴图的第i个面创建rtv
		// 将资源映射到 rtv
		md3dDevice->CreateRenderTargetView(mCubeMap.Get(), &rtvDesc, mhCpuRtv[i]);
	}
}

// 构建立方体图资源,这里只是将资源指针关联到视图中,资源还没有数据
// DynamicCubeMapApp::DrawSceneToCubeMap() 将不透明物体渲染到 mhCpuRtv[] 中
void CubeRenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mCubeMap)));
}

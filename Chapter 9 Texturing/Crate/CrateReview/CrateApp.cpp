// 请先看 README.md: CrateReview 项目资源的传递
#include "CrateApp.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CrateApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 载入贴图,存入mTextures,传入GPU
	LoadTextures();

	// 根签名(根参数数组)指定了register,constant buffer 通过指明根签名中的RootParameterIndex对应register
	BuildRootSignature();

	// 创建SRV堆,储存在mSrvDescriptorHeap,描述mTextures中的资源,通过句柄偏移绑定不同的tex
	BuildDescriptorHeaps();

	// 载入Shader,存在mShaders,指明VS,PS函数名
	// mInputLayout通过字符串和Shader中的struct VertexIn绑定,通过顺序和C++中的struct Vertex绑定
	BuildShadersAndInputLayout();

	// 将顶点/索引数据存入mGeometries,mGeometries中的每个元素都是一个SubmeshGeometry的map
	// struct MeshGeometry提供函数,将顶点数据放入内存,再通过上传堆传到默认堆,获取顶点/索引视图
	BuildShapeGeometry();

	// 将材质存入mMaterials[],设置MatCBIndex,DiffuseSrvHeapIndex对应mSrvDescriptorHeap中的句柄偏移(tex)
	BuildMaterials();

	// 渲染项存入mAllRitems和mOpaqueRitems,设置ObjectCB(World,TexTransform),ObjCBIndex,mat,Geo,顶点/索引起始位置
	// 默认 NumFramesDirty = gNumFrameResources,所以UpdateObjectCBs(),UpdateMaterialCBs()会修改每个帧资源
	// ObjCBIndex: BuildFrameResources()中,创建了空的ObjectCB数组
	//		UpdateObjectCBs()中,将mAllRitems中的struct ObjectConstants存入mCurrFrameResource->ObjectCB
	//		通过UploadBuffer<>提供的CopyData()函数传至GPU
	//		DrawRenderItems()中,通过objectCB->GetGPUVirtualAddress()和ObjCBIndex偏移找到渲染项的objCBAddress
	// mat: 储存了材质属性和MatCBIndex(作用类似ObjCBIndex),tex在mSrvDescriptorHeap中的句柄偏移
	// Geo: (struct MeshGeometry)储存了顶点/索引,提供函数,返回顶点/索引视图
	BuildRenderItems();

	// 创建帧资源储存在mFrameResources[],指明passCount,objectCount,materialCount
	// 帧资源的构造函数将这些常量储存在UploadBuffer<>中
	BuildFrameResources();

	// 创建PSO,存入mPSOs["opaque"],设置了 InputLayout, pRootSignature, Shader
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}

// 更新 mProj
void CrateApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void CrateApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt); // do nothing
	UpdateCamera(gt); // 更新mView

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// 如果GPU没有完成'当前帧资源'的处理,CPU等待GPU处理完命令,到达这个围栏点
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt); // do nothing
	UpdateObjectCBs(gt); // 更新每个帧资源的每个ObjectConstant,传至GPU
	UpdateMaterialCBs(gt); // 更新每个帧资源的每个材质MaterialConstants, 传至GPU
	UpdateMainPassCB(gt); // 修改了mCurrFrameResource->PassCB, struct PassConstants,传至GPU
}

void CrateApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	// mPSOs["opaque"],设置了 InputLayout, pRootSignature, Shader
	// mInputLayout通过字符串和Shader中的struct VertexIn绑定,通过顺序和C++中的struct Vertex绑定
	// mRootSignature,指定了register: t0(SRV,tex), b0,b1,b2(CB)
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 指明要渲染的缓存
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// mSrvDescriptorHeap,描述了1张贴图,用句柄偏移区分
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// mRootSignature, 指定了register: t0(SRV, tex), b0, b1, b2(CB)
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 根签名(根参数数组)指定了寄存器插槽,constant buffer 通过指明根签名中的根参数索引对应register
	// RootParameterIndex:2, cbuffer cbPass : register(b1)
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	// 获取当前帧资源的objectCB,matCB
	// 对于每一个渲染项,进行绘制
	// 设置顶点/索引视图和偏移
	// tex在mSrvDescriptorHeap中的handle偏移,register
	// objCBAddress,matCBAddress偏移,register
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// mCurrFrameResource->Fence只属于当前帧资源,每次+=3, mCurrentFence作用于所有帧资源,每次++
	mCurrFrameResource->Fence = ++mCurrentFence;

	// CPU为GPU设置围栏命令: 1,2,3,4... GPU依次达到1,2,3,4...
	// 如果CPU再一次处理第一个帧资源,eg(mCurrFrameResource->Fence=4)
	// CPU就会等待GPU(mFence->GetCompletedValue())增长:1->2->3->4
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 2.5f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInput(const GameTimer& gt)
{

}

// 更新mView
void CrateApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void CrateApp::AnimateMaterials(const GameTimer& gt)
{
	bool bShowAnim = true;
	if (!bShowAnim)
		return;

	auto mat = mMaterials["woodCrate"].get();

	// 变换矩阵的第四行控制平移
	float& du = mat->MatTransform(3, 0);
	float& dv = mat->MatTransform(3, 1);

	du += 0.05f*gt.DeltaTime();
	dv += 0.07f*gt.DeltaTime();

	if (du >= 1.0f)
		du -= 1.0f;
	if (dv >= 1.0f)
		dv -= 1.0f;

	mat->MatTransform(3, 0) = du;
	mat->MatTransform(3, 1) = dv;

	mat->NumFramesDirty = gNumFrameResources;
}

// 更新每个帧资源的每个ObjectConstant,传至GPU
void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			// 上传GPU
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

// 更新每个帧资源的每个材质MaterialConstants, 传至GPU
void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			// 上传GPU
			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

// 修改了mCurrFrameResource->PassCB, struct PassConstants,传至GPU
void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	// 上传GPU
	currPassCB->CopyData(0, mMainPassCB);
}

// 载入贴图,存入mTextures
void CrateApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../../../Textures/WoodCrate01.dds";
	// 上传GPU
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

// 根签名(根参数数组)指定了register,constant buffer 通过指明根签名中的RootParameterIndex对应register
void CrateApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // register t0

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // register b0
	slotRootParameter[2].InitAsConstantBufferView(1); // register b1
	slotRootParameter[3].InitAsConstantBufferView(2); // register b2

	auto staticSamplers = GetStaticSamplers(); // register s0 ~ s6

	// 根签名是根参数数组
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 创建具有4个槽位的根签名,第一个指向含有单个着色器资源视图的描述符表,其它三个各指向一个常量缓冲区视图
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

// 创建SRV堆,储存在mSrvDescriptorHeap,描述mTextures中的资源
void CrateApp::BuildDescriptorHeaps()
{
	// 创建SRV堆
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	// Fill out the heap with actual descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto woodCrateTex = mTextures["woodCrateTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	// 提供方法,可以将采样时所返回的纹理向量中的分量进行重新排序
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = woodCrateTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0; // 此视图中图像细节最详尽的mipmap层级的索引
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels; // 此视图的mipmap层级数量,以MostDetailedMip为起始值
	srvDesc.Texture2D.ResourceMinLODClamp = 1.0f; // 可以访问的最小mipmap层级

	md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
}

// 载入Shader,存在mShaders,指明VS,PS函数名
// mInputLayout通过字符串和Shader中的struct VertexIn绑定,通过顺序和C++中的struct Vertex绑定
void CrateApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
}

// 将所有顶点/索引数据存入mGeometries,并记录单个几何体的偏移,mGeometries中的每个元素都是一个SubmeshGeometry的map
// struct MeshGeometry提供函数,将顶点数据放入内存,再通过上传堆传到默认堆,获取顶点/索引视图
void CrateApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;

	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	// 将顶点数据放入内存
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// 将顶点数据通过上传堆传到默认堆,上传GPU
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

// 创建PSO,存入mPSOs["opaque"],设置了 InputLayout, pRootSignature, Shader
void CrateApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// PSO for opaque objects
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));
}

// 创建帧资源储存在mFrameResources[],指明passCount,objectCount,materialCount
// 帧资源的构造函数将这些常量储存在UploadBuffer<>中
void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

// 将材质存入mMaterials[],设置MatCBIndex,DiffuseSrvHeapIndex对应tex
void CrateApp::BuildMaterials()
{
	auto woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	mMaterials["woodCrate"] = std::move(woodCrate);
}

// 渲染项存入mAllRitems和mOpaqueRitems,设置ObjectCB(World,TexTransform),ObjCBIndex,mat,Geo,顶点/索引起始位置
// 默认 NumFramesDirty = gNumFrameResources,所以UpdateObjectCBs(),UpdateMaterialCBs()会修改每个帧资源
// ObjCBIndex: BuildFrameResources()中,创建了空的ObjectCB数组
// UpdateObjectCBs()中,将mAllRitems中的struct ObjectConstants存入mCurrFrameResource->ObjectCB
// 通过UploadBuffer<>提供的CopyData()函数传至GPU
// DrawRenderItems()中,通过objectCB->GetGPUVirtualAddress()和ObjCBIndex偏移找到渲染项的objCBAddress
// mat: 储存了材质属性和MatCBIndex(作用类似ObjCBIndex),tex在mSrvDescriptorHeap中的句柄偏移
// Geo: (struct MeshGeometry)储存了顶点/索引,提供函数,返回顶点/索引视图
void CrateApp::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = mMaterials["woodCrate"].get();
	boxRitem->Geo = mGeometries["boxGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	// all render items are opaque
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

// 获取当前帧资源的objectCB,matCB
// 对于每一个渲染项,进行绘制
// 设置顶点/索引视图和偏移
// tex在mSrvDescriptorHeap中的handle偏移,register
// objCBAddress,matCBAddress偏移,register
void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// for each render items
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		// BuildShapeGeometry()中,d3dUtil::CreateDefaultBuffer()将所有顶点传到GPU默认堆
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		// BuildRenderItems()中设置了Mat,struct Material内含有DiffuseSrvHeapIndex
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		// BuildRenderItems()中,为每个几何体设置ObjCBIndex,ObjectConstants
		// UpdateObjectCBs()中,读取每个mAllRenderItems,赋值给mCurrFrameResource->ObjectCB
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		// 根签名(根参数数组)指定了寄存器插槽,constant buffer 通过指明根签名中的根参数索引对应register
		// RootParameterIndex:0, Texture2D gDiffuseMap : register(t0)
		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		// RootParameterIndex:1, cbuffer cbPerObject : register(b0)
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		// RootParameterIndex:3, cbuffer cbMaterial : register(b2)
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
{
	// 应用程序一般只会用到这些采样器的一部分
	// 所以就将它们全部提前定义好,并作为根签名的一部分保留下来

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister 着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter 过滤器类型 点过滤
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU U轴方向上所用的寻址模式 重复寻址模式
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU 钳位寻址模式
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter 线性过滤
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias mipmap层级的偏置值
		8);                               // maxAnisotropy 最大各向异性值

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}
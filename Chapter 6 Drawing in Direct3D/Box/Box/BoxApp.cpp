//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

// RTV,DSV				: 有描述符堆, 有描述符(视图)
// 顶点缓冲区,索引缓冲区: 无描述符堆, 有描述符(视图), 默认堆, 通过输入布局描述指定着色器寄存器
// 常量缓冲区			: 有描述符堆, 有描述符(视图), 上传堆, 通过根签名指定着色器寄存器, CPU每帧更新

//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//
//using Microsoft::WRL::ComPtr;
//using namespace DirectX;
//using namespace DirectX::PackedVector;
//
//struct Vertex
//{
//	XMFLOAT3 Pos;
//	XMFLOAT4 Color;
//};
//
//struct ObjectConstants
//{
//	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
//};
//
//class BoxApp : public D3DApp
//{
//public:
//	BoxApp(HINSTANCE hInstance);
//	BoxApp(const BoxApp& rhs) = delete;
//	BoxApp& operator=(const BoxApp& rhs) = delete;
//	~BoxApp();
//
//	virtual bool Initialize()override;
//
//private:
//	virtual void OnResize()override;
//	virtual void Update(const GameTimer& gt)override;
//	virtual void Draw(const GameTimer& gt)override;
//
//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//	void BuildDescriptorHeaps();
//	void BuildConstantBuffers();
//	void BuildRootSignature();
//	void BuildShadersAndInputLayout();
//	void BuildBoxGeometry();
//	void BuildPSO();
//
//private:
//
//	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
//
//	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
//
//	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
//
//	ComPtr<ID3DBlob> mvsByteCode = nullptr;
//	ComPtr<ID3DBlob> mpsByteCode = nullptr;
//
//	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//	ComPtr<ID3D12PipelineState> mPSO = nullptr;
//
//	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//	float mTheta = 1.5f*XM_PI;
//	float mPhi = XM_PIDIV4;
//	float mRadius = 5.0f;
//
//	POINT mLastMousePos;
//};
#include "BoxApp.h"
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
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

BoxApp::BoxApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return 0;

	// 重置命令列表,准备初始化
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	// 执行初始化命令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsList), cmdsList);

	// 等待初始化完成
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view*proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	// (CPU每帧)更新常量缓冲区
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 在顶点缓冲区及其对应视图创建完成后,将它与渲染流水线上的一个输入槽相绑定
	// 这样就能向流水线中的输入装配器阶段传递顶点数据了
	// para1: 在绑定多个顶点缓冲区时,起始输入槽0~15
	// para2: 将要与输入槽绑定的顶点缓冲区数量
	// para3: 指向顶点缓冲区视图数组中第一个元素的指针
	// 将顶点缓冲区设置到输入槽上并不会对其执行实际的绘制操作,而是仅为顶点数据送至渲染流水线做好准备
	// 最后一步是通过 ID3D12GraphicsCommandList::DrawInstanced 方法真正的绘制顶点
	// 在使用索引的时候,用 ID3D12GraphicsCommandList::DrawIndexedInstanced 绘制
	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	// 将索引缓冲区绑定到输入装配器阶段
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 根签名只定义了应用程序要绑定到渲染流水线的资源,却没有真正地执行任何资源绑定操作
	// 只有率先通过命令列表设置好根签名
	// 就能用 SetGraphicsRootDescriptorTable 方法令描述符表与渲染流水线相绑定
	// para1: 将根参数按此索引(欲绑定到的寄存器槽号)进行设置
	// para2: 要向着色器绑定的描述符表中第一个描述符位于描述符堆中的句柄
	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	// 在使用索引的时候,用 ID3D12GraphicsCommandList::DrawIndexedInstanced 绘制
	// para1: 每个实例将要绘制的索引数量
	// para2: 用于实现被称作实例化的高级技术.目前只绘制一个实例,暂设为1
	// para3: 起始索引
	// para4: 在本次绘制调用读取顶点之前,要为每个索引都加上此整数值
	// para5: 用与实现被称作实例化的高级技术,暂设为0
	mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 着色器可见
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
	// 绘制1个物体所需的常量数据
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);
	// 计算常量对象大小,配合index用于在GPU虚拟地址中偏移
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// 缓冲区的起始地址
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	// 偏移到常量缓冲区中绘制第i个物体所需的常量数据
	int boxCBIndex = 0;
	cbAddress += boxCBIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = objCBByteSize;

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{






	// 着色器程序一般需要以资源作为输入(eg,常量缓冲区,纹理,采样器等)
	// 根签名定义了着色器程序所需的具体资源
	// 如果把着色器程序看做一个函数,而将输入的资源看做向函数传递的参数数据
	// 那么便可类似地认为根签名定义的是函数签名

	// 根签名以一组描述绘制调用过程中,着色器所需资源的根参数定义而成 


	// 根参数可以是根常量,根描述符,或根描述符表
	// 描述符表指定的是描述符堆中存有描述符的一块连续区域
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];


	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	// para1: 描述符表的类型
	// para2: 表中描述符的数量
	// para3: 将这段描述符区域绑定至此基址着色器寄存器
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	// para1: 描述符区域的数量
	// para2: 指向描述符区域数组的指针
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// 根签名是根参数数组
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	// Direct3D 12规定,必须先将根签名的描述布局进行序列化处理,待其转换为以 ID3DBlob 接口表示的序列化数据格式后,才可以将它传入 CreateRootSignature 方法
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
	//HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		// para1: 语义,通过语义可以将顶点结构体中的元素与顶点着色器的输入签名中的元素一一映射
		// para2: 附到语义上的索引.可在不引入新语义的情况下区分元素(eg,POSITION0,POSITION1)
		// para3: DXGI_FORMAT 顶点元素的数据类型
		// para4: 输入槽,支持0~15
		// para5: 特定输入槽中,顶点结构体的首地址到某元素起始地址的偏移量(用字节表示)
		// para6: 暂定如此
		// para7: 暂定如此
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
}


void BoxApp::BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices = // 顶点,基于局部坐标
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f ,-1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";
	// 创建顶点和索引在内存中的副本
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	// 输入布局描述 结构体 D3D12_INPUT_LAYOUT_DESC
	// para1: D3D12_INPUT_ELEMENT_DESC *pInputElementDescs
	// para2: UINT NumElements
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX; // 对所有的采样点进行采样
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1; // 同时所用的渲染目标数量(即 RTVFormats 数组中渲染目标格式的数量)
	psoDesc.RTVFormats[0] = mBackBufferFormat; // 渲染目标的格式
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
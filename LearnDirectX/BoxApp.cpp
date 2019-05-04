// 用 Direct3D 12 绘制 Box
// 控制:
// 按下鼠标左键移动: 旋转
// 按下鼠标右键移动: 缩放

// 需要提前编译Shader文件,LearnDirectX\LearnDirectX\Shaders\color.hlsl
// 报错: FXC : error X3501: 'main': entrypoint not found
// 解决: 右键项目 Add -> Existing Item: color.hlsl;  右键color.hlsl文件 Properties -> General -> Item Type: Do not participate in build
// 原因: VS HLSL编译器默认入口文件是main,而shader的入口文件可以自定义
#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX; // XMFLOAT3
using namespace DirectX::PackedVector;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants // 对象常量结构体: 世界视图投影矩阵
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4(); // 世界-视图-投影 矩阵
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeaps();		// 创建描述符堆
	void BuildConstantBuffers();		// 创建常量缓冲区
	void BuildRootSignature();			// 创建根签名
	void BuildShadersAndInputLayout();	// 创建着色器和输入布局
	void BuildBoxGeometry();			// 创建Box几何体
	void BuildPSO();					// 创建流水线状态对象 PSO: Pipeline State Object 前面几个函数都是为此准备

private:

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr; // 根签名
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr; // 常量缓冲区描述符堆

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr; // 上传缓冲区辅助函数

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr; // 几何体网格

	ComPtr<ID3DBlob> mvsByteCode = nullptr; // 顶点着色器(vertex shader)字节码
	ComPtr<ID3DBlob> mpsByteCode = nullptr; // 像素着色器(pixed shader)字节码

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; // 输入布局

	ComPtr<ID3D12PipelineState> mPSO = nullptr; // 流水线状态对象 PSO: Pipeline State Object

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4(); // 世界矩阵
	XMFLOAT4X4 mView = MathHelper::Identity4x4(); // 视图矩阵
	XMFLOAT4X4 mProj = MathHelper::Identity4x4(); // 投影矩阵

	float mTheta = 1.5f * XM_PI; // 相机水平旋转, 相机以球坐标表示,后面转化为笛卡尔坐标,以Box为原点
	float mPhi = XM_PIDIV4; // 相机竖直旋转
	float mRadius = 5.0f; // 相机距离物体的距离 ???

	POINT mLastMousePos; // 上一帧鼠标位置
};
// 程序入口
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// 调试版本开启运行时内存检测
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

BoxApp::BoxApp(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 重置命令列表,为执行初始化命令做好准备工作
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();			// 创建描述符堆
	BuildConstantBuffers();			// 创建常量缓冲区
	BuildRootSignature();			// 创建根签名
	BuildShadersAndInputLayout();	// 创建着色器和输入布局
	BuildBoxGeometry();				// 创建Box几何体
	BuildPSO();						// 创建流水线状态对象 PSO: Pipeline State Object

	// 执行初始化命令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 等待初始化完成
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// 如果用户调整了窗口尺寸,则更新纵横比并重新计算投影矩阵 param: 垂直角度,宽高比,近点,远点
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	// 将球坐标转换为笛卡尔坐标
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// 构建观察矩阵
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld); // 世界矩阵: 物体的移动,旋转,缩放
	XMMATRIX proj = XMLoadFloat4x4(&mProj); // 投影矩阵: 窗口大小
	XMMATRIX worldViewProj = world * view * proj;

	// 用 世界-观察-投影 矩阵 更新常量缓存区
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
	// 复用记录命令所用的内存
	// 只有当GPU中的命令列表执行完毕后,才能对其进行重置
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 将命令列表加入命令队列(ExecuteCommandList),然后将其重置
	// 复用命令列表即复用其相应的内存
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get())); // Pipeline State Object

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 按照资源的用途指示其状态的改变,这里将资源 呈现状态 -> 渲染目标状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 清除后台缓冲区和深度缓存区
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 指定将要渲染的目标缓冲区
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get()); // 设置根签名

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView()); // 输入装配顶点缓存
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView()); // 输入装配索引缓存
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 输入装配图元拓扑

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart()); // 令描述符表与渲染流水线相绑定

	mCommandList->DrawIndexedInstanced( // 使用索引进行绘制
		mBoxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);

	// 改变资源状态 渲染目标 -> 呈现
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// 完成命令的记录
	ThrowIfFailed(mCommandList->Close());

	// 向命令队列添加命令列表
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区与前台缓冲区
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待绘制此帧的命令执行完毕
	// 这种等待简单低效
	// 后面重新组织渲染代码,使不必在每帧等待
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
	if ((btnState & MK_LBUTTON) != 0) // & 按位与,只有两个参数都为1才返回1
	{
		// 根据鼠标移动距离计算旋转角度,令每个 像素按照此角度的1/4旋转
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// 根据鼠标的输入来更新摄像机绕立方体旋转的角度
		mTheta += dx;
		mPhi += dy;

		// 限制垂直角度的范围
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// 使场景中的每个像素按鼠标移动的0.005倍进行缩放
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// 根据鼠标的输入更新相机的可视范围半径
		mRadius += dx - dy;

		// 限制可视半径的范围
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
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	// 偏移到常量缓冲区中第i个物体所对应的常量数据
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
	// 着色器程序一般以资源作为输入(常量缓冲区,纹理,采样器等)
	// 根签名定义了着色器程序所需的具体资源
	// 如果把着色器程序看做一个函数,输入的资源作为参数
	// 那么可以将根签名看做函数签名
	// 

	// 根签名可以是描述符表,跟描述符或根常量
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// 创建由单个CBV(Const Buffer View)所组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// 根签名是由根参数组成的数组
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 用单个寄存器来创建一个根签名,该槽位指向一个仅含有单个常量缓冲区的描述符区域
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
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0"); // 顶点着色器字节码
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0"); // 像素着色器字节码

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices = // 顶点数组
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices = // 索引数组
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

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU)); // 创建Blob类型的顶点数组
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize); // 顶点数组复制内存?

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU)); // 创建Blob类型的索引数组
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize); // 索引数组复制内存?

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
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
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
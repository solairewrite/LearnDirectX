#include "d3dApp.h"
#include <windowsx.h> // GET_X_LPARAM

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr; // 应用实例
D3DApp* D3DApp::GetApp()
{
	return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance)
	:mhAppInst(hInstance)
{
	// 只有一个D3DApp可以被构造
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp()
{
	// 在销毁GPU引用的资源以前,必须等待GPU处理完队列中的所有命令.否则可能造成应用程序在退出时崩溃
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

HINSTANCE D3DApp::AppInst() const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd() const
{
	return mhMainWnd;
}

float D3DApp::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState() const
{
	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// 配合多重采样,重新创建交换链和缓存区
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset(); // 开启消息循环之前,重置游戏时间

	while (msg.message != WM_QUIT)
	{
		// 如果有Window消息,处理
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// 否则,处理游戏逻辑
		else
		{
			mTimer.Tick();

			if (!mAppPaused) // 游戏主要逻辑
			{
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	// 初始化resize
	OnResize();

	return true;
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// 创建RTV描述符堆,SwapChainBufferCount个
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// 创建DSV描述符堆,1个
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// 改变资源前,先Flush
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 释放将要重新创建的之前的资源
	for (int i = 0; i < SwapChainBufferCount; i++)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// resize 交换链
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	// 资源不能与渲染流水线中的阶段直接绑定,所以必须先为资源创建视图(描述符)
	// 描述符堆句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// 为了将后台缓冲区绑定到流水线的输出合并阶段
		// 需要为后台缓冲区创建RTV
		// 第一步就是:获取交换链中的缓冲区资源
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]))); // 后台缓冲区的索引:i
		// 为获取的后台缓冲区创建渲染目标视图
		// para2: D3D12_RENDER_TARGET_VIEW_DESC,描述了资源中元素的数据类型,如果该资源在创建时已指定了具体格式,就可以设为nullptr
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// 偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// 深度/模板缓冲区描述结构体
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 资源维度
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth; // 纹理宽度
	depthStencilDesc.Height = mClientHeight; // 纹理高度
	depthStencilDesc.DepthOrArraySize = 1; // 纹理深度(对于2D纹理,是纹理数组的大小)
	depthStencilDesc.MipLevels = 1; // mipmap层级数量,对于深度/模板缓冲区,1
	depthStencilDesc.Format = mDepthStencilFormat; // DXGI_FORMAT_D24_UNORM_S8_UINT. 24位深度,映射到[0,1], 8位模板,映射到[0,255]
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 指定纹理布局
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 与资源有关的杂项标志

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	// GPU资源都存于堆(heap)中,本质是具有特定属性的GPU显存快
	// ID3D12Device::CreateCommittedResource方法,根据我们提供 的属性创建一个资源与一个堆,并把该资源提交到这个堆中

	// 创建深度模板缓冲区
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // (资源欲提交至的)堆所具有的属性,深度模板缓冲区上传至默认堆.默认堆的资源只有GPU可以访问
		D3D12_HEAP_FLAG_NONE, // 与堆有关的额外选项标志
		&depthStencilDesc, 
		D3D12_RESOURCE_STATE_COMMON, // 资源创建时的初始状态
		&optClear, // 描述用于清除资源的优化值
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// 创建DSV
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// 将资源从初始状态转换为深度缓冲
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 执行 resize 命令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists); // 将命令列表里的命令添加到命令队列中

	// 等待 resize 完成
	FlushCommandQueue();

	// 更新视口位置
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0,0,mClientWidth,mClientHeight };
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// 窗口被激活或反激活时,WM_ACTIVATE被发送
		// 反激活时暂停游戏,激活时取消暂停
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// 用户resize窗口
	case WM_SIZE:
		// 保存新的尺寸
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// 从最小化恢复
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}
				// 从最大化恢复
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// 交给WM_EXITSIZEMOVE处理,否则会不断触发
				}
				else // 调用API,如SetWindowPos,mSwapChain->SetFullscreenState
				{
					OnResize();
				}
			}
		}
		return 0;

		// 用户开始拖拽
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// 用户结束拖拽
		// 重置基于窗口尺寸的属性,如缓冲区,视图等
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// 窗口被摧毁
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// 激活菜单,并且用户按下的键不响应助记键或加速键
	case WM_MENUCHAR:
		// alt-enter 不发出哔哔声
		return MAKELRESULT(0, MNC_CLOSE);

		// 抓住这条消息以防窗口变得太小
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE) // 按下Esc
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2) // 按下F2(这里程序崩溃了)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	// 填写WNDCLASS结构体
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc; // 关联窗口过程函数的指针
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd"; // 引用这个窗口类结构体的名字

	// 注册结构体
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	// 计算窗口尺寸
	RECT R = { 0,0,mClientWidth,mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	// 创建窗口
	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	// 启用D3D12 Debug
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	// 为创建WARP适配器(也可用于创建交换链),需要先创建一个IDXGIFactory4对象
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory))); 

	// 1,尝试创建硬件设备
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,	// 指定创建设备时所用的显示适配器(显卡). nullptr:使用主显示适配器
		D3D_FEATURE_LEVEL_11_0, // 应用程序需要硬件所支持的最低功能级别.如果适配器不支持此功能级别,则设备创建失败.这里是Direct3D11
		IID_PPV_ARGS(&md3dDevice));

	// 回滚到WARP设备
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWrapAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWrapAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWrapAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	// 2-1,创建围栏
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, // 初始值:0
		IID_PPV_ARGS(&mFence)));
	// 2-2,获取描述符的大小
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 3,检查 4X MSAA 质量支持
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	// 4,创建命令队列,命令列表,命令分配器
	CreateCommandObjects();
	// 5,创建交换链
	CreateSwapChain();
	// 6,7,8 创建描述符堆,缓冲区,RTV,DSV
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	// 创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// 创建命令分配器
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, // 可供GPU直接执行的命令
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// 创建命令列表
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0, // node mask, 指定与所建命令列表相关联的物理GPU, 对于单GPU的系统,设置为0
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),	// 关联的command allocator
		nullptr,					// 初始PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// 首次引用命令列表时需要重置,而重置前需要关闭
	mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// 释放之前的交换链
	mSwapChain.Reset();

	// 填写DXGI_SWAP_CHAIN_DESC结构体
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth; // 缓冲区分辨率的宽度
	sd.BufferDesc.Height = mClientHeight; // 缓冲区分辨率的高度
	sd.BufferDesc.RefreshRate.Numerator = 60;	// 分子
	sd.BufferDesc.RefreshRate.Denominator = 1;	// 分母
	sd.BufferDesc.Format = mBackBufferFormat; // 缓冲区的显示格式,这里是DXGI_FORMAT_R8G8B8A8_UNORM
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // 逐行扫描 vs 隔行扫描
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // 图像如何相对于屏幕进行拉伸
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1; // 多重采样每个像素的采样次数
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0; // 多重采样的质量级别
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 由于我们要将数据渲染至后台缓冲区(即用它作为渲染目标),因此这样指定
	sd.BufferCount = SwapChainBufferCount; // 交换链中所用的缓冲区数量
	sd.OutputWindow = mhMainWnd; // 渲染窗口的句柄
	sd.Windowed = true; // 窗口模式 vs 全屏模式
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 切换全屏时,选择最适合于当前应用程序窗口尺寸的显示模式

	// 创建交换链
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	mCurrentFence++;

	// 向命令队列加入指令,以设置新的围栏值
	// 直到GPU完成所有优先于Signal()的命令,才设置
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// 等待直到GPU完成所有到这个围栏值的命令
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// 当GPU到达当前围栏值时(执行到Signal()指令,修改了围栏值),触发预定事件
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// 等待直到GPU到达当前围栏值,触发事件
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
	// 创建描述符堆后,访问其中所存的描述符
	// 通过句柄引用描述符
	// 创建了SwapChainBufferCount个RTV
	// 用偏移量找到当前后台缓冲区的RTV描述符,必须知道RTV的大小
	// 伪代码: 目标描述符句柄 = GetCPUDescriptorHandleForHeapStart() + mCurrBackBuffer * mRtvDescriptorSize
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(), // 获得描述符堆中第一个描述符的句柄
		mCurrBackBuffer, // 偏移至后台缓冲区描述符句柄的索引
		mRtvDescriptorSize); // 描述符所占字节的大小
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView() const
{
	// 只创建了1个DSV
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// 统计数据置于窗口说明栏
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// TotalTime每帧递增,以总时间增加1s为周期统计
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;	// frameCnt/1
		float mspf = 1000.0f / fps;		// 每帧多少ms

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"		fps: " + fpsStr +
			L"	   mspf: " + mspfStr;

		// 设置窗口左上角文字 
		SetWindowText(mhMainWnd, windowText.c_str());

		// 重置带下一个周期
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// 获取count
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}
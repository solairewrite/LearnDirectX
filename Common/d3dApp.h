#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// 链接DirectX库
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

	D3DApp(HINSTANCE hInstance); // HINSTANCE是句柄数据类型,相当于装入到了内存的资源id,实际是无符号长整数
	D3DApp(const D3DApp& rhs) = delete; // 禁止使用编译器默认生成的函数
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:

	static D3DApp* GetApp();

	HINSTANCE AppInst() const; // 返回应用程序实例句柄
	HWND MainWnd() const; // 返回主窗口句柄 HWND:窗口句柄
	float AspectRatio() const; // 后台缓冲区的宽高比

	bool Get4xMsaaState() const; // 是否开启4X MSAA(多重采样抗锯齿)
	void Set4xMsaaState(bool value);

	int Run(); // 封装应用程序的消息循环,使用Win32的PeekMessage函数,当没有窗口消息时,就会处理游戏逻辑



	virtual bool Initialize(); // 初始化,如分配资源,初始化对象,建立3D场景等
	// 应用程序主窗口的窗口过程函数 LRESULT: 窗口程序或回调函数返回的32位值,WPARAM,LPARAM: 消息响应机制
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps(); // 创建RTV和DSV描述符堆
	virtual void OnResize(); // 当D3DApp::MsgProc函数接收到WM_SIZE消息时调用
	virtual void Update(const GameTimer& gt) = 0; // 每帧调用
	virtual void Draw(const GameTimer& gt) = 0; // 每帧调用,发出渲染命令,将当前帧绘制到后台缓冲区



	// 处理鼠标输入
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {} // WPARAM:鼠标事件发生时,哪个键按下
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:

	bool InitMainWindow(); // 初始化应用程序主窗口
	bool InitDirect3D();
	void CreateCommandObjects(); // 创建命令队列,命令分配器和命令列表
	void CreateSwapChain(); //创建交换链

	void FlushCommandQueue(); // 强制CPU等待GPU,直到GPU处理完队列中的所有命令

	ID3D12Resource* CurrentBackBuffer() const; // 返回当前后台缓冲区的ID3D12Resource
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const; // 返回当前后台缓冲区的RTV(渲染目标视图)
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const; // 返回主深度/模板缓冲区的DSV(深度/模板视图)

	void CalculateFrameStats(); // 计算每秒的平均帧数,每帧平均的毫秒数

	void LogAdapters(); // 枚举系统中所有的适配器(显卡)
	void LogAdapterOutputs(IDXGIAdapter* adapter); // 枚举指定适配器的全部显示输出(显示器)
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format); // 枚举某个显示输出对特定格式支持的所有显示模式

protected:

	static D3DApp* mApp;

	HINSTANCE	mhAppInst = nullptr;	// 应用实例句柄
	HWND		mhMainWnd = nullptr;	// 主窗口句柄
	bool		mAppPaused = false;		// 程序是否暂停
	bool		mMinimized = false;		// 程序是否最小化
	bool		mMaximized = false;		// 程序是否最大化
	bool		mResizing = false;		// 是否正在拖拽尺寸栏
	bool		mFullScreenState = false; // 是否允许全屏
	bool		m4xMsaaState = false;	// 是否允许4X MSAA(4倍多重采样抗锯齿)
	UINT		m4xMsaaQuality = 0;		// 4X MSAA等级

	GameTimer mTimer;

	// COM:Component Object Model 组件对象模型,是一种令DirectX不受编程语言束缚,并使之向后兼容的技术
	// 为管理COM对象的生命周期,Windows运行时库(WRL:Windows Runtime Library)提供了Microsoft::WRL::ComPtr类
	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory; // 用于创建交换链,创建WARP(Windows高级光栅化平台)
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain; // 交换链
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice; // 显示适配器(显卡)

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence; // 围栏
	UINT64 mCurrentFence = 0; // 当前围栏值

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue; // GPU命令队列
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc; // 命令分配器
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList; // CPU命令列表

	static const int SwapChainBufferCount = 2; // 有几个缓冲区(前+后)
	int mCurrBackBuffer = 0; // 当前后台缓冲区
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount]; // 渲染目标视图缓冲区
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer; // 深度/模板缓冲区

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap; // RTV描述符堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap; // DSV描述符堆

	D3D12_VIEWPORT mScreenViewport; // 视口
	D3D12_RECT mScissorRect; // 裁剪矩形

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	// 继承类应该在构造器中设置这些值
	std::wstring mMainWndCaption = L"d3d App"; // wstring:宽字符, caption:说明文字
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800; // 窗口宽度
	int mClientHeight = 600; // 窗口高度
};
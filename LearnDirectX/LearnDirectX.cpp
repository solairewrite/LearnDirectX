#include "d3dApp.h"
#include <DirectXColors.h> // using namespace DirectX

using namespace DirectX; // Colors::LightSteelBlue









class InitDirect3DApp :public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize() override; // 框架方法,初始化Windows应用程序

private:
	virtual void OnResize() override; // 框架方法
	virtual void Update(const GameTimer& gt) override; // 框架方法
	virtual void Draw(const GameTimer& gt) override; // 框架方法

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// 启用运行时内存检查
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitDirect3DApp theApp(hInstance);
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

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{

}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	// 重复使用记录命令的相关内存
	// 只有当与GPU关联的命令列表执行完时,才重置

	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 将命令列表加入命令队列后,可重置
	// 复用命令列表及其内存

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 转换资源状态,呈现状态->渲染目标状态

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	// 设置视口和裁剪矩形,他们需要随着命令列表的重置而重置
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 在每帧为了刷新场景而绘制之前,需要清除后台缓冲区和深度缓冲区
	// 将指定的渲染目标清理为指定的颜色

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	// 清除深度/模板缓冲区
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	// 指定要渲染的缓冲区
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());


	// 再次转换资源状态,渲染目标->呈现
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	// 完成命令记录
	ThrowIfFailed(mCommandList->Close());

	// 将执行的命令列表加入命令队列

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区和前台缓冲区

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount; // 更新索引

	// 等待此帧的命令完成,为了简单,当前代码的效率低
	// 后面将重新组织渲染部分的代码,以免在每一帧等待



	FlushCommandQueue();
}

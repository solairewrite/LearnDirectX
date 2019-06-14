#include <windows.h> // Win32 API结构体,数据类型,函数声明

HWND ghMainWnd = 0; // 主窗口的句柄

// 初始化Windows应用程序
bool InitWindowsApp(HINSTANCE instanceHandle, int show);

// 消息循环
int Run();

// 窗口过程:处理窗口所接受到的消息
LRESULT CALLBACK
WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 在Windows应用程序中WinMain相当于大多数语言的main()
// 如果WinMain函数成功运行,那么在其终止时,应返回WM_QUIT的wParam成员
// 如果函数在退出时还没进入消息循环,返回0
// #define WINAPI __stdcall 指明函数的调用约定,关乎函数参数的入栈顺序等
// hInstance:当前应用程序的实例句柄
// hPrevInstance:Win32编程用不到
// pCmdLine:运行此程序所用的命令函参数字符串
// nCmdShow:应用程序如何显示(按照窗口当前的大小与位置,窗口最大化...)
int WINAPI
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// 创建并初始化应用程序主窗口
	if (!InitWindowsApp(hInstance, nShowCmd))
	{
		return 0;
	}
	// 开启消息循环.直到接收到消息WM_QUIT(应用程序关闭)
	return Run();
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	// 填写WNDCLASS结构体,根据其中描述的特征来创建一个窗口
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW; // 窗口类的样式,当工作区的宽度或高度发生改变时就重绘窗口
	wc.lpfnWndProc = WndProc; // 关联得窗口过程函数的指针
	wc.cbClsExtra = 0; // 分配额外的内存空间
	wc.cbWndExtra = 0; // 分配额外的内存空间
	wc.hInstance = instanceHandle; // 当前应用程序实例的句柄
	wc.hIcon = LoadIcon(0, IDI_APPLICATION); // 指定一个图标的句柄,这里采用默认
	wc.hCursor = LoadCursor(0, IDC_ARROW); // 指定光标样式句柄
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // 指出画刷的句柄,以此指定窗口工作区的背景颜色
	wc.lpszMenuName = 0; // 指定窗口的菜单
	wc.lpszClassName = L"BasicWndClass"; // 引用这个窗口类结构体的名字 CreateWindow(para1, ...)

	// 在Windows系统中,为上述WNDCLASS注册一个实例
	if (!RegisterClass(&wc))
	{
		// 消息框所属的窗口的句柄, 内容文本, 标题文本, 样式
		MessageBox(0, L"RegisterClass FAILED", 0, 0);
		return false;
	}

	// 调用CreateWindow()创建窗口,返回创建窗口的句柄(HWND类型),创建失败返回0
	// 窗口句柄是一种窗口的引用方式
	ghMainWnd = CreateWindow(
		L"BasicWndClass", // 采用前面注册的WNDCLASS实例
		L"Win32Basic", // 窗口标题
		WS_OVERLAPPEDWINDOW, // 窗口的样式标志
		CW_USEDEFAULT, // x坐标
		CW_USEDEFAULT, // y坐标
		CW_USEDEFAULT, // 窗口宽度
		CW_USEDEFAULT, // 窗口高度
		0, // 父窗口句柄
		0, // 菜单句柄
		instanceHandle, // 应用程序实例句柄
		0 // 其他参数
	);

	if (ghMainWnd == 0)
	{
		MessageBox(0, L"Create Window FAILED", 0, 0);
		return false;
	}

	// 显示并更新窗口
	ShowWindow(ghMainWnd, show);
	UpdateWindow(ghMainWnd);

	return true;
}

int Run()
{
	MSG msg = { 0 };

	// 在获取WM_QUIT消息之前,该函数会一直保持循环.
	// GetMessage函数只有在收到WM_QUIT消息时才会返回0,若发生错误,返回-1
	// 没有消息到来之时,GetMessage会令此应用程序栈进入休眠状态
	BOOL bRet = 1; // boolReturn
	while ((bRet = GetMessage(&msg, 0, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			MessageBox(0, L"GetMessage FAILED", L"ERROR", MB_OK);
			break;
		}
		else
		{
			TranslateMessage(&msg); // 实现键盘按键的转换,特别是将虚拟按键消息转换为字符消息
			DispatchMessage(&msg); // 把消息分派给指定的窗口过程
		}
	}

	return (int)msg.wParam; // 如果应用程序根据WM_QUIT消息顺利退出,则WinMain返回WM_QUIT消息的参数wParam(消息代码)
}

// LRESULT: 整形数,表示函数是否调用成功
// CALLBACK: 指明这是一个回调函数,不显示的调用,Windows系统(在需要处理消息的时候)自动调用此窗口过程
// hWnd: 接收此消息的窗口句柄
// msg: 标识此消息的预定值
// wParam, lParam: 与具体消息相关的额外信息
LRESULT CALLBACK
WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 处理一些特定的消息,处理完一个消息后,应返回0
	switch (msg)
	{
		// 按下鼠标左键,弹出消息框
	case WM_LBUTTONDOWN:
		MessageBox(0, L"Hello, World", L"Hello", MB_OK);
		return 0;

		// 按下Esc后,销毁应用程序主窗口
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(ghMainWnd);
		return 0;

		// 处理销毁消息的方法是发出退出消息,这样会终止消息循环
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// 将上面没有处理的消息转发给默认的窗口过程
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
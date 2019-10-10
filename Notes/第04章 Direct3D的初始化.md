# 学习笔记
2019-05-08 学习笔记,在学习的过程中,发现基础不牢固.从Direct3D的初始化开始复习  
## 第4章 Direct3D的初始化
### 4.1 预备知识
#### 4.1.2 组件对象模型
COM: Component Object Model 组件对象模型,是一种令DirectX不受编程语言束缚,并使之向后兼容的技术  
为了管理COM对象的生命周期,Windows运行时库(WRL: Windows Runtime Library)提供了Microsoft\::WRL\::ComPtr类  
COMPtr 3个方法 Get: 返回指向此底层COM接口的指针  
GetAddressOf: 返回指向此底层COM接口指针的地址  
Reset  
#### 4.1.6 资源描述符(descripor描述符即视图view)
发出绘制命令之前,需要将与本次绘制调用相关的资源绑定到渲染流水线上  
GPU资源并非直接与渲染流水线相绑定,而是要通过描述符对它间接引用  
描述符作用: 指定资源数据, 告知资源被绑定在流水线的哪个阶段上, 指定欲绑定资源中的局部数据, 指明类型  
常用的描述符:  
<font color="#FF8000">CBV(constant buffer view)</font>: 常量缓冲视图  
<font color="#FF8000">SRV(shader resource view)</font>: 着色器资源视图  
<font color="#FF8000">UAV(unordered access view)</font>: 无视访问视图  
<font color="#FF8000">RTV(render target view)</font>: 渲染目标视图  
<font color="#FF8000">DSV(depth/stencil view)</font>深度/模板视图  
描述符堆descriptor heap(可以看做描述符数组),需要为每一种类型的描述符都创建出单独的描述符堆  
也可为同一种描述符类型创建出多个描述符堆  
#### 4.1.10 DirectX图形基础结构
DXGI: DirectX Graphics Infrastructure DirectX图形基础结构: 提供通用API  
显示适配器(显卡)IDXGIAdaptor, 显示输出(显示器)IDXGIOutput
### 4.2 CPU与GPU的交互
#### 4.2.1 命令队列和命令列表
GPU维护命令队列(command queue,本质上是环形缓冲区)**ID3D12CommandQueue**  
CPU可以利用命令列表(command list,封装了一系列图形渲染命令)**ID3D12GraphicsCommandList**将命令提交到这个队列中去  
填写结构体描述队列: **D3D12_COMMAND_QUEUE_DESC**  
创建队列: **ID3D12Device::CreateCommandQueue**  
将命令列表里的命令添加到命令队列: **ID3D12CommandQueue::ExecuteCommandLists**  
将命令加入命令列表后,调用 **ID3D12GraphicsCommandList::Close**  
在调用 ID3D12CommandQueue:: ExecuteCommandLists 提交命令列表前,一定要将其关闭  
命令分配器(command allocator)**ID3D12CommandAllocator**,内存管理类接口  
记录在命令列表的命令,实际上是储存在与之关联的命令分配器上  
创建命令分配器: **ID3D12Device::CreateCommandAllocator**  
创建命令列表: **ID3D12Device::CreateCommandList**  
在调用ID3D12CommandQueue:: ExecuteCommandLists之后,可以通过**ID3D12GraphicsComandList::Reset**方法,复用命令列表占用的相关底层内存(相关的命令分配器仍在维护内存中被命令队列引用的系列命令)  
复用命令分配器的内存(GPU执行完命令分配器的命令)**ID3D12CommandAllocator::Reset**  
#### 4.2.2 CPU与GPU间的同步
创建围栏: **ID3D12Device::CreateFence**  
#### 4.2.3 资源转换
资源转换屏障结构体**D3D12_RESOURCE_BARRIER**  
### 4.3 初始化Direct3D
#### 4.3.1 创建设备
显示适配器(显卡)**ID3D12Device**  
**D3D12CreateDevice**
#### 4.3.2 创建围栏并获取描述符的大小
#### 4.3.3 检测对4X MSAA质量级别的支持
#### 4.3.4 创建命令队列和命令列表
#### 4.3.5 描述并创建交换链
先填写结构体**DXGI_SWAP_CHAIN_DESC**,再调用**IDXGIFactory::CreateSwapChain**  
#### 4.3.6 创建描述符堆(描述符<=>视图)
**ID3D12Device::CreateDescriptorHeap**  
#### 4.3.7 创建渲染目标视图(描述符)
获取交换链中的缓冲区资源**IDXGISwapChain::GetBuffer**  
为获取的后台缓冲区创建渲染目标视图**ID3D12Device::CreateRenderTargetView**
#### 4.3.8 创建深度/模板缓冲区及其视图
填写**D3D12_RESOURCE_DESC**结构体,**ID3D12Device::CreateCommittedResource**  
#### 4.3.9 设置视口(后台缓冲区的绘制范围)
填写**D3D12_VIEWPORT**结构体,**ID3D12GraphicsCommandList::RSSetViewports**  
#### 4.3.10 设置裁剪矩形
**D3D12_RECT**, **ID3D12GraphicsCommandList::RSSetScissorRects**  
## 示例代码
#### 1, WinMain函数中,根据应用句柄HINSTANCE实例化D3DApp(的子类)theApp  
#### 2, 初始化theApp.Initialize()包括创建窗口,初始化DX  
##### 2.1, 创建窗口:填写WNDCLASS结构体,注册结构体RegisterClass,创建窗口CreateWindow,ShowWindow(),UpdateWindow()  
##### 2.2, 初始化DX: InitDirect3D(),参考4.3节,这里4.3.1-4.3.8  
##### 2.3, 调用OnResize()  
改变资源前先Flush **FlushCommandQueue()**  
重置命令列表 **mCommandList->Reset()**  
释放要重新创建的之前的资源 **mSwapChainBuffer[i].Reset() mDepthStencilBuffer.Reset()**  
resize交换链 **mSwapChain->ResizeBuffers()**  
获取描述符堆句柄 **CD3DX12_CPU_DESCRIPTOR_HANDLE** rtvHeapHandle  
获取交换链中的缓冲区资源 **mSwapChain->GetBuffer()**  
为获取的后台缓冲区创建渲染目标视图 **md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle)**  

深度/模板缓冲区描述结构体 **D3D12_RESOURCE_DESC** depthStencilDesc  
创建深度模板缓冲区 **md3dDevice->CreateCommittedResource()**  
创建DSV **md3dDevice->CreateDepthStencilView()**  
将资源从初始状态转换为深度缓冲 **mCommandList->ResourceBarrier()**  
执行 resize 命令 **mCommandList->Close() mCommandQueue->ExecuteCommandLists()**  
等待 resize 完成 **FlushCommandQueue()**  
更新视口位置 mScreenViewport, mScissorRect  
#### 3, 运行应用theApp.Run()
开启消息循环之前,重置游戏时间 **mTimer.Reset()**  
如果有Window消息,处理 if(**PeekMessage(&msg, 0, 0, 0, PM_REMOVE)**)  
否则,处理游戏逻辑 **Update(mTimer) Draw(mTimer)**  
#### 4, Run()中每帧调用Draw()
重复使用记录命令的相关内存 **mDirectCmdListAlloc->Reset()**  
复用命令列表及其内存 **mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)**  
转换资源状态 **mCommandList->ResourceBarrier**  
设置视口和裁剪矩形,他们需要随着命令列表的重置而重置 **mCommandList->RSSetViewports mCommandList->RSSetScissorRects**  
在每帧为了刷新场景而绘制之前,需要清除后台缓冲区和深度缓冲区,将指定的渲染目标清理为指定的颜色  
**mCommandList->ClearRenderTargetView()**  
清除深度/模板缓冲区 **mCommandList->ClearDepthStencilView**  
指定要渲染的缓冲区 **mCommandList->OMSetRenderTargets**  
再次转换资源状态 **mCommandList->ResourceBarrier**  
完成命令记录 **mCommandList->Close()**  
将执行的命令列表加入命令队列 **mCommandQueue->ExecuteCommandLists()**  
交换后台缓冲区和前台缓冲区 **mSwapChain->Present(0, 0)**  
等待此帧的命令完成 **FlushCommandQueue()**  

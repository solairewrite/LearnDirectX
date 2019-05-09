# 学习笔记
2019-05-08 学习笔记,在学习的过程中,发现基础不牢固.从Direct3D的初始化开始复习  
## 第4章 Direct3D的初始化
### 4.1 预备知识
#### 4.1.2 组件对象模型
COM: Component Object Model 组件对象模型,是一种令DirectX不受编程语言束缚,并使之向后兼容的技术  
为了管理COM对象的生命周期,Windows运行时库(WRL: Windows Runtime Library)提供了Microsoft::WRL::ComPtr类  
COMPtr 3个方法 Get: 返回指向此底层COM接口的指针  
GetAddressOf: 返回指向此底层COM接口指针的地址  
Reset  
#### 4.1.6 资源描述符
发出绘制命令之前,需要将与本次绘制调用相关的资源绑定到渲染流水线上  
GPU资源并非直接与渲染流水线相绑定,而是要通过描述符对它间接引用  
常用的描述符: CBV(constant buffer view), SRV(shader resource view), UAV(unordered access view)  
RTV(render target view), DSV(depth/stencil view)  
描述符堆(可以看做描述符数组)  
#### 4.1.10 DirectX图形基础结构
DXGI: DirectX Graphics Infrastructure DirectX图形基础结构: 提供通用API  
显示适配器(显卡)IDXGI, 显示输出(显示器)IDXGIOutput
### 4.2 CPU与GPU的交互
#### 4.2.1 命令队列和命令列表
GPU维护命令队列(command queue,本质上是环形缓冲区)**ID3D12CommandQueue**  
CPU可以利用命令列表(command list,封装了一系列图形渲染命令)**ID3D12GraphicsCommandList**将命令提交到这个队列中去  
填写结构体描述队列: **D3D12_COMMAND_QUEUE_DESC**  
创建队列: **ID3D12Device::CreateCommandQueue**  
将命令列表里的命令添加到命令队列: **ID3D12CommandQueue::ExecuteCommandLists**  
将命令加入命令列表后,调用 **ID3D12GraphicsCommandList::Close**  
在调用 ID3D12CommandQueue::ExecuteCommandLists 提交命令列表前,一定要将其关闭  
命令分配器(command allocator)**ID3D12CommandAllocator**,内存管理类接口  
记录在命令列表的命令,实际上是储存在与之关联的命令分配器上
创建命令分配器: **ID3D12Device::CreateCommandAllocator**  
创建命令列表: **ID3D12Device::CreateCommandList**  
在调用ID3D12CommandQueue::ExecuteCommandLists之后,可以通过**ID3D12GraphicsComandList::Reset**方法,复用命令列表占用的相关底层内存(相关的命令分配器仍在维护内存中被命令队列引用的系列命令)  
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
#### 4.3.6 创建描述符堆
**ID3D12Device::CreateDescriptorHeap**  
#### 4.3.7 创建渲染目标视图(描述符)
获取交换链中的缓冲区资源**IDXGISwapChain::GetBuffer**  
为获取的后台缓冲区创建渲染目标视图**ID3D12Device::CreateRenderTargetView
#### 4.3.8 创建深度/模板缓冲区及其视图
填写**D3D12_RESOURCE_DESC**结构体,**ID3D12Device::CreateCommittedResource**  
#### 4.3.9 设置视口(后台缓冲区的绘制范围)
填写**D3D12_VIEWPORT**结构体,**ID3D12GraphicsCommandList::RSSetViewports**  
#### 4.3.10 设置裁剪矩形
**D3D12_RECT**, **ID3D12GraphicsCommandList::RSSetScissorRects**
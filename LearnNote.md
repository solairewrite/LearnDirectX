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
CPU可以利用命令列表(command list)将命令提交到这个队列中区**ID3D12GraphicsCommandList**(封装了一系列图形渲染命令)  
填写结构体描述队列: D3D12_COMMAND_QUEUE_DESC  
创建队列: ID3D12Device::CreateCommandQueue  
将命令列表里的命令添加到命令队列: ID3D12CommandQueue::ExecuteCommandLists  
将命令加入命令列表后,调用 ID3D12GraphicsCommandList::Close  
在调用 ID3D12CommandQueue::ExecuteCommandLists 提交命令列表前,一定要将其关闭  

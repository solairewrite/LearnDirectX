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
#### 代码: 创建窗口的流程
##### 1, WinMain函数中,根据应用句柄HINSTANCE实例化D3DApp(的子类)theApp  
##### 2, 初始化theApp.Initialize()包括创建窗口,初始化DX  
###### 2.1, 创建窗口:填写WNDCLASS结构体,注册结构体RegisterClass,创建窗口CreateWindow,ShowWindow(),UpdateWindow()  
###### 2.2, 初始化DX: InitDirect3D(),参考4.3节,这里4.3.1-4.3.8  
###### 2.3, 调用OnResize()  
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
##### 3, 运行应用theApp.Run()
开启消息循环之前,重置游戏时间 **mTimer.Reset()**  
如果有Window消息,处理 if(**PeekMessage(&msg, 0, 0, 0, PM_REMOVE)**)  
否则,处理游戏逻辑 **Update(mTimer) Draw(mTimer)**  
##### 4, Run()中每帧调用Draw()
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
## 第5章 渲染流水线
### 5.4 渲染流水线概述
**输入装配器阶段 IA: Input Assembler**  
**顶点着色器阶段 VS: Vertex Shader**  
**曲面细分阶段 Tessellator**  
**几何着色器阶段 GS: Geometry Shader**  
**裁剪**  
**光栅化阶段 RS: Rasterizer**  
**像素着色器阶段 PS: Pixel Shader**  
**输出合并阶段 OM: Output Merge**  
### 5.5 输入装配器阶段
输入装配器阶段(IA: Input Assembler)阶段从显存中读取几何数据(顶点和索引,vertex and index),再将他们装配为几何图元(geometric primitive)  
#### 5.5.2 图元拓扑
通过顶点缓冲区(vertex buffer)将顶点与渲染流水线绑定,指定图元拓扑(primitive topology)告知DX如何利用顶点数据来表示几何图元  
**ID3D12GrapgicsCommandList::IASetPrimitiveTopology**  
### 5.6 顶点着色器阶段
图元被装配完毕后,其顶点就会被送入顶点着色器阶段(vertex shader stage, VS).可以吧顶点着色器看做一种输入与输出数据皆为单个顶点的函数(对顶点的操作由GPU执行)  
#### 5.6.1 局部空间和世界空间
将局部坐标转为全局场景坐标系的过程叫做世界变换(world mmatrix),所使用的变换矩阵是世界矩阵(world matrix, W=SRT)  
#### 5.6.2 观察空间
观察空间,在此坐标系中,虚拟摄像机位于远点,沿Z轴的正向观察,X轴指向摄像机的右侧,Y轴指向摄像机的上方.  
由世界空间至观察空间的坐标变换称为观察变换(view transform),变换矩阵称为观察矩阵(view matrix)  
给定相机位置,观察目标点,世界空间中向上方向,就能构建摄像机局部坐标系,推导观察矩阵  
**XMMatrixLookAtLH**  
#### 5.6.3 投影和齐次裁剪空间
透视投影变换(perspective projection transform): 将3D顶点V变换至其投影线(V与观察点的连线)与2D投影平面V'  
用垂直视场角,纵横比,到近平面的距离,到远平面的距离,构建透视投影矩阵  
**XMMatrixPerspectiveFovLH**  
## 第6章 利用Direct3D绘制几何体
### 6.1 顶点与输入布局
为了自定义顶点格式,首先创建结构体来容纳选定的顶点数据 struct Vertex  
输入布局描述: 定义顶点结构体后,向DX提供顶点结构体的描述,使它了解怎样处理结构体中的每个成员  
**D3D12_INPUT_LAYOUT_DESC**  
输入布局描述由两部分组成:**D3D12_INPUT_ELEMENT_DESC**构成的数组,及数组长度  
**D3D12_INPUT_ELEMENT_DESC**数组中的元素依次描述了顶点结构体中所对应的成员  
结构体的**SemanticName**(语义) == **顶点着色器输入签名(vertex shader input signature)**  
### 6.2 顶点缓冲区
为了使GPU可以访问顶点数组,需要将它们放置在成为缓冲区(buffer)的**GPU资源**(**ID3D12Resource**)里,把储存顶点的缓冲区称为顶点缓冲区(vertex buffer)  
创建ID3D12Resource对象: 填写D3D12_RESOURCE_DESC(**CD3DX12_RESOURCE_DESC**, C++包装类)结构体,调用ID3D12Device::CreateCommittedResource  
对于静态几何体(每一帧都不变化),将其顶点缓冲区置于默认堆(**D3D12_HEAP_TYPE_DEFAULT**)来优化性能  
用**D3D12_HEAP_TYPE_UPLOAD**堆类型来创建上传缓冲区(upload buffer)资源.  
将顶点数据从系统内存复制到上传缓冲区,再从上传缓冲区复制到真正的顶点缓冲区  
需要利用上传缓冲区来初始化默认缓冲区参考d3dUtil :: CreateDefaultBuffer()  
为了将顶点缓冲区绑定到渲染流水线上,需创建点点缓冲区视图(vertex buffer view,不需要描述符堆)  
**D3D12_VERTEX_BUFFER_VIEW**,属性BufferLocation: 待创建的顶点缓冲区资源虚拟地址,通过**ID3D12Resource :: GetGPUVirtualAddress**获取  
在顶点缓冲区及其对应视图创建完成后,将它与渲染流水线上的一个输入槽(input slot)相绑定.  
这样一来,就能向流水线的输入装配阶段传递顶点数据了  
**ID3D12GraphicsCommandList :: IASetVertexBuffers**  
绘制顶点: **ID3D12GraphicsCommandList :: DrawInstanced**  
### 6.3 索引和索引缓冲区
为了使GPU可以访问索引数组,需要将它们放置于GPU的缓冲区资源(ID3D12Resource)内,称为索引缓冲区(index buffer)  
为了使索引缓冲区与渲染流水线绑定,需要给索引缓冲区资源创建索引缓冲区视图(index buffer view,不需要描述符堆)  
**D3D12_INDEX_BUFFER_VIEW**,属性BufferLocation:待创建视图的索引缓冲区资源虚拟地址,通过ID3D12Resource::GetGPUVirtualAddress获取  
将索引缓冲区绑定到输入装配器阶段: ID3D12GraphicsCommandList :: IASetIndexBuffer  
绘制: **ID3D12GraphicsCommandList :: DrawIndexedInstand**  
### 6.6 常量缓冲区
#### 6.6.1 创建常量缓冲区
常量缓冲区(constant buffer)也是一种GPU资源(ID3D12Resource),其数据内容可供着色器程序所引用  
常量缓冲区由CPU每帧更新一次,创建到一个上传堆中,大小为256B的整数倍  
#### 6.6.2 更新常量缓冲区
**ComPtr\<ID3D12Resource\>::Map**获取指向欲更新资源数据的指针  
**memcpy**()将数据从系统内存复制到常量缓冲区  
参考UploadBuffer.h  
#### 6.6.4 常量缓冲区描述符
利用描述符将常量缓存区绑定至渲染流水线  
填写**D3D12_DESCRIPTOR_HEAP_DESC**结构体  
Type=**D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV**  
Flags=**D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE**  
创建描述符堆: **CreateDescriptorHeap()**  
填写**D3D12_CONSTANT_BUFFER_VIEW_DESC**结构体  
创建常量缓冲区**ID3D12Device::CreateConstantBufferView()**  
#### 6.6.5 根签名和描述符表
不同类型的资源会被绑定到特定的寄存器槽(register slot)上,供着色器程序访问  
根签名(root signature): 绑定到渲染流水线上的资源,映射到着色器的对应输入寄存器  
根签名由**ID3D12RootSignature**接口表示,以一组描述着色器所需要资源的根参数(root parameter)定义  
根参数(**CD3DX12_ROOT_PARAMETER**)可以是根常量(root constant),根描述符(root descriptor),描述符表(descriptor table,**CD3DX12_DESCRIPTOR_RANGE**)  
**CD3DX12_DESCRIPTOR_RANGE.Init()**, **ID3D12RootSignature.InitAsDescriptorTable()**  
根签名只定义了应用程序要绑定到渲染流水线的资源,没有执行绑定操作  
令描述符表与渲染流水线绑定: **ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable()**  
### 6.7 编译着色器
**D3DCompileFromFile()**,参考d3dUtil::CompileShader()  
**ID3DBlob**类型描述一段内存,两个方法  
**GetBufferPointer**(返回数据void*指针,使用前需转换类型),**GetBufferSize**()  
### 6.8 光栅器状态
**D3D12_RASTERIZER_DESC**  
### 6.9 流水线状态对象
PSO: Pipeline State Object, **ID3D12PipelineState**  
填写**D3D12_GRAPHICS_PIPELINE_STATE_DESC**结构体  
创建PSO: **ID3D12Device::CreateGraphicsPipelineState()**  

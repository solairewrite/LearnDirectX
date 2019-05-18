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
为了将顶点缓冲区绑定到渲染流水线上,需创建顶点缓冲区视图(vertex buffer view,不需要描述符堆)  
**D3D12_VERTEX_BUFFER_VIEW**,属性BufferLocation: 待创建的顶点缓冲区资源虚拟地址,通过**ID3D12Resource :: GetGPUVirtualAddress**获取  
在顶点缓冲区及其对应视图创建完成后,将它与渲染流水线上的一个输入槽(input slot)相绑定.  
这样一来,就能向流水线的输入装配阶段传递顶点数据了  
**ID3D12GraphicsCommandList :: IASetVertexBuffers**  
绘制顶点: **ID3D12GraphicsCommandList :: DrawInstanced**  
### 6.3 索引和索引缓冲区
为了使GPU可以访问索引数组,需要将它们放置于GPU的缓冲区资源(ID3D12Resource)内,称为索引缓冲区(index buffer)  
为了使索引缓冲区与渲染流水线绑定,需要给索引缓冲区资源创建索引缓冲区视图(index buffer view,不需要描述符堆)  
**D3D12_INDEX_BUFFER_VIEW**,属性BufferLocation:待创建视图的索引缓冲区资源虚拟地址,通过ID3D12Resource::GetGPUVirtualAddress获取  
将索引缓冲区绑定到输入装配器阶段: **ID3D12GraphicsCommandList :: IASetIndexBuffer**  
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
## 示例代码
#### 1.
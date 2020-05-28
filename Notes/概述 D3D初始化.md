## 预备知识
COM: Component Object Model 组件对象模型,是一种令DirectX不受编程语言束缚,并使之向后兼容的技术  

### 资源描述符`descriptor`,即视图`view`
发出绘制命令之前,需要将与本次绘制调用相关的资源绑定到渲染流水线上  
GPU资源并非直接与渲染流水线相绑定,而是要通过描述符对它间接引用  

描述符作用: 指定资源数据, 告知资源被绑定在流水线的哪个阶段上, 指定欲绑定资源中的局部数据, 指明类型  

常用的描述符:  
+ `CBV(constant buffer view)`: 常量缓冲视图  
+ `SRV(shader resource view)`: 着色器资源视图  
+ `UAV(unordered access view)`: 无序访问视图  
+ `RTV(render target view)`: 渲染目标视图  
+ `DSV(depth/stencil view)`: 深度/模板视图  

描述符堆`descriptor heap`(可以看做描述符数组),需要为每一种类型的描述符都创建出单独的描述符堆  

### CPU与GPU的交互

#### 命令队列和命令列表
GPU维护命令队列`command queue`  

CPU可以利用命令列表`command list`(封装了一系列图形渲染命令)将命令提交到这个队列中去  

命令分配器`command allocator`,内存管理类接口,记录在命令列表的命令,实际上是储存在与之关联的命令分配器上  

#### CPU与GPU间的同步
围栏`Fence`  

## 初始化Direct3D
+ 创建设备(显示适配器,即显卡)  
+ 创建围栏,获取描述符大小  
+ 创建命令队列和命令列表  
+ 创建交换链  
+ 创建描述符堆  
+ 创建渲染目标视图  
+ 创建深度/模板缓冲区及其视图  
+ 设置视口  
+ 设置裁剪矩形  

## 第7章 利用Direct3D绘制几何体(续)-Shapes
### Initialize()
D3DApp::Initialize()  
mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)  
#### BuildRootSignature() // 修改了mRootSignature
#### BuildShadersAndInputLayout() // 修改了mShaders, mInputLayout
#### BuildShapeGeometry() // 修改了mGeometries
将所有的几何数据合并到一个大的顶点/索引缓冲区中  
对合并顶点缓冲区中每个物体的顶点偏移量进行缓存  
对合并索引缓冲区中每个物体的起始索引进行缓存  
定义的多个SubmeshGeometry结构体中包含了顶点/索引缓冲区内不同几何体的子网格数据  
提取顶点元素,将所有网格的顶点装进一个顶点缓冲区 vertices, indices  
...  
#### BuildRenderItems() // 修改了mAllRitems, mOpaqueRitems
构建了多个RenderItem,设置他们的位置,缩放,索引偏移  
#### BuildFrameResources() // 创建帧资源,修改了mFrameResources
#### BuildDescriptorHeaps() // 修改了mCbvHeap
为每个帧资源中的每一个物体创建一个CBV描述符  
每个帧资源的渲染过程一个CBV描述符  
...  
#### BuildConstantBufferViews()
...  
#### BuildPSOs() // 修改了mPSOs
这里建立了solid和wireframe两种pso  
mCommandList->Close()  
ID3D12CommandList* cmdsLists[] = { mCommandList.Get() }
mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists)  
FlushCommandQueue()  
### Run()
#### Update(mTimer)
&emsp;OnKeyboardInput() 检测按键: if (GetAsyncKeyState('1'))  
&emsp;UpdateCamera()  
&emsp;循环往复的获取帧资源数组中的元素  
&emsp;检查GPU端是否已执行完处理当前帧资源的所有命令  
&emsp;如果还没有就令CPU等待,直到GPU完成命令的执行,并抵达这个围栏点  
&emsp;**if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)**  
##### &emsp;UpdateObjectCBs()
##### &emsp;UpdateMainPassCB()
#### Draw(mTimer)
&emsp;...  
##### &emsp;DrawRenderItems()  

&emsp;...  
&emsp;增加围栏值,将命令标记到此围栏点  
&emsp;mCurrFrameResource->Fence = ++mCurrentFence  
&emsp;向命令队列添加一条指令来设置一个新的围栏点  
&emsp;**mCommandQueue->Signal(mFence.Get(), mCurrentFence)**  
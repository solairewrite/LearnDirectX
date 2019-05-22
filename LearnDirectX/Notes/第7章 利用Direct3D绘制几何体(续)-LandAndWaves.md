## 第7章 利用Direct3D绘制几何体(续)-LandAndWaves
### Initialize()
D3DApp::Initialize()  
mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)  
mWaves = std\::make_unique\<Waves>()  
#### BuildRootSignature() 
// 修改了 mRootSignature  
#### BuildShadersAndInputLayout()
// 修改了 mShaders, mInputLayout  
#### BuildLandGeometry()
// **设置land的网格体 mGeometries["landGeo"]->DrawArgs["grid"]**  
// 创建grid  
// 创建临时顶点数组 vertices,对每个顶点应用高度函数**GetHillsHeight**,再根据高度设置颜色  
// 创建临时索引数组 indices  
// 创建临时变量 MeshGeometry geo  
// 设置 geo 的Name landGeo  
// 将geo的顶点数组和索引数组赋值为刚刚创建的 vertices, indices  
// 将顶点数组和索引数组上传至GPU  
// 创建临时变量 SubmeshGeometry submesh  
// 设置 submeah 的 顶点偏移和索引偏移,这里只绘制land,所以都为0  
// 设置 geo->DrawArgs\["grid"](单独绘制的网格体) = submesh   
// 设置 mGeometries["landGeo"] = geo  
// mGeometries是一个MeshGeometry的map  
// MeshGeometry->DrawArgs(用于绘制的网格体)是一个SubmeshGeometry的map  
#### BuildWavesGeometryBuffers()
#### BuildRenderItems()
#### BuildFrameResources()
#### BuildPSOs()
mCommandList->Close()  
ID3D12CommandList* cmdLists[] = { mCommandList.Get() }  
mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists)  
FlushCommandQueue()  
### Run()
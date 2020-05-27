## 第7章 利用Direct3D绘制几何体(续)-LandAndWaves
### Initialize()
D3DApp::Initialize()  
mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)  
// 创建波浪,为 mWaves赋值  
<font color="#FF8000">mWaves</font> = std\::make_unique\<Waves>()  
#### BuildRootSignature() 
// 修改了 <font color="#FF8000">mRootSignature</font>  
#### BuildShadersAndInputLayout()
// 修改了 <font color="#FF8000">mShaders, mInputLayout</font>  
#### BuildLandGeometry()
// 设置land的网格体 <font color="#FF8000">mGeometries["landGeo"]->DrawArgs["grid"]</font>  
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
// 设置wave的网格体 <font color="#FF8000">mGeometries["waterGeo"]->DrawArgs["grid"]</font>  
// 创建临时索引数组 indices  
// 创建临时 MeshGeometry geo  
// 设置geo的Name waterGeo  
// 将geo的顶点数组赋值为刚刚创建的indices  
// 创建临时 SubmeshGeometry submesh  
// 设置submeah 的 顶点偏移和索引偏移,这里只绘制wave,所以都为0  
// 设置 geo->DrawArgs\["grid"](单独绘制的网格体) = submesh  
// 设置 mGeometries["landGeo"] = geo  
#### BuildRenderItems()
// 修改了 <font color="#FF8000">mWavesRitem, mRitemLayer, mAllRitems</font>  
// 创建临时 RenderItem wavesRitem  
// 设置 wavesRitem->Geo = mGeometries["waterGeo"]  
// 为 mWavesRitem 赋值为刚创建的 wavesRitem  
// 为 mRitemLayer[0] push_back 刚创建的 wavesRitem  
// 创建临时 RenderItem gridRitem  
// 设置 gridRitem->Geo = mGeometries["landGeo"]  
// 为 mRitemLayer[0] push_back 刚创建的 gridRitem ?? 这里没搞懂std::vector的逻辑  
// mAllRitems 添加 wavesRitem 和 gridRitem  
#### BuildFrameResources()
// 修改 mFrameResources  
#### BuildPSOs()
// 设置 <font color="#FF8000">mPSOs["opaque"], mPSOs["opaque_wireframe"]</font>  
  
mCommandList->Close()  
ID3D12CommandList* cmdLists[] = { mCommandList.Get() }  
mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists)  
FlushCommandQueue()  
### Run()
#### Update()
OnKeyboardInput() 检测按下'1': GetAsyncKeyState('1') // 修改了 <font color="#FF8000">mIsWireframe</font>  
UpdateCamera() // 修改了 <font color="#FF8000">mView</font>  
##### UpdateObjectCBs()  
// 将 mCurrFrameResource 和其它帧资源的 <font color="#FF8000">ObjectCB</font> 更新为 mAllRitems 的 World  
##### UpdateMainPassCB()
// 修改了 <font color="#FF8000">mMainPassCB, mCurrFrameResource->PassCB</font>  
##### UpdateWaves()
// 每一帧模拟波浪,更新顶点缓冲区  
// 修改了 <font color="#FF8000">mWaves, mCurrFrameResource->WavesVB, mWavesRitem->Geo->VertexBufferGPU</font>  
#### Draw()
cmdListAlloc->Reset()  
根据 mIsWireframe 决定绘制solid或线框  
mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get())  
mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get())
  
mCommandList->RSSetViewports(1, &mScreenViewport);  
mCommandList->RSSetScissorRects(1, &mScissorRect);  
  
mCommandList->ResourceBarrier  
  
mCommandList->ClearRenderTargetView  
mCommandList->ClearDepthStencilView  
  
mCommandList->OMSetRenderTargets  
  
mCommandList->SetGraphicsRootSignature(mRootSignature.Get())  
  
// 绑定渲染过程所用的常量缓冲区  
auto passCB = mCurrFrameResource->PassCB->Resource()  
mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress())  
##### DrawRenderItems()  
&emsp;// 对于每个渲染项  
&emsp;cmdList->IASetVertexBuffers  
&emsp;cmdList->IASetIndexBuffer  
&emsp;cmdList->IASetPrimitiveTopology  
&emsp;cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress)  
&emsp;cmdList->DrawIndexedInstanced  
  
mCommandList->ResourceBarrier  
  
mCommandList->Close()  

ID3D12CommandList* cmdsLists[] = { mCommandList.Get() }  
mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists)  
  
mSwapChain->Present(0, 0)  
mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount  
  
mCurrFrameResource->Fence = ++mCurrentFence  
  
mCommandQueue->Signal(mFence.Get(), mCurrentFence)  

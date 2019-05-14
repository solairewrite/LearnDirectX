



// 按'1'以线框模式显示

#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3; // 帧资源数量


// 储存绘制图形所需参数的轻量结构体,随应用不同而有所差别
struct RenderItem
{
	RenderItem() = default;

	// 描述物体局部空间相对于世界空间的世界矩阵
	// 它定义了物体在世界空间中的位置,旋转及缩放

	XMFLOAT4X4 World = MathHelper::Identity4x4();

	// 用已更新标志dirtyflag来表示物体的相关数据已发生改变,需要更新常量缓冲区
	// 因为每个帧资源都有一个物体常量缓冲区,所以对每个帧资源否进行更新
	// 当修改数据的时候,设置NumFramesDirty = gNumFrameResources,从而使每个帧资源都得到更新

	int NumFramesDirty = gNumFrameResources;

	// 指向GPU常量缓存区的索引,对应于当前渲染项
	UINT ObjCBIndex = 1;

	MeshGeometry* Geo = nullptr;

	// 图元拓扑
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced方法的参数
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	ShapesApp(const ShapesApp& rhs) = delete;
	ShapesApp& operator=(const ShapesApp& rhs) = delete;
	~ShapesApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:

	// 由3个帧资源构成的向量
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	// 当前的帧资源
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr; // 根签名
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr; // 常量描述符堆

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr; // 着色器资源shader resource描述符堆

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries; // map: string -> MeshGeometry
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders; // map: string -> ID3DBlob
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs; // map: string -> ID3D12PipelineState

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// 渲染项目列表
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	UINT mPassCbvOffset = 0;

	bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = 0.2f * XM_PI;
	float mRadius = 15.0f;

	POINT mLastMousePos;
};

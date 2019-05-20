#include "../Public/d3dApp.h"
#include "../Public/MathHelper.h"
#include "../Public/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX; // XMFLOAT3
using namespace DirectX::PackedVector;

// 顶点结构体
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// 对象常量结构体
struct ObjectConstants
{
	// 世界-视图-投影 矩阵
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	// = delete 禁用函数
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	// 处理Windows消息时调用
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	// 创建描述符堆
	void BuildDescriptorHeaps();
	// 创建常量缓冲区
	void BuildConstantBuffers();
	// 创建根签名
	void BuildRootSignature();
	// 创建着色器和输入布局
	void BuildShadersAndInputLayout();
	// 创建Box几何体
	void BuildBoxGeometry();
	// 创建流水线状态对象 PSO : Pipeline State Object 前面几个函数都是为此准备
	void BuildPSO();

private:

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;	// 根签名
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;		// 常量缓冲区描述符堆

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr; // 上传缓冲区辅助函数

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr; // 几何体网格

	ComPtr<ID3DBlob> mvsByteCode = nullptr;	// 顶点着色器(vertex shader)字节码(HLSL编译文件)
	ComPtr<ID3DBlob> mpsByteCode = nullptr;	// 像素着色器(pixed shader)字节码(HLSL编译文件)

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; // 输入布局

	ComPtr<ID3D12PipelineState> mPSO = nullptr;		// 流水线状态对象 PSO: Pipeline State Object

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();	// 世界矩阵
	XMFLOAT4X4 mView = MathHelper::Identity4x4();	// 视图矩阵
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();	// 投影矩阵

	float mTheta = 1.5f * XM_PI; // 相机水平旋转, 相机以球坐标表示,后面转化为笛卡尔坐标,以Box为原点
	float mPhi = XM_PIDIV4;	// 相机竖直旋转
	float mRadius = 5.0f;	// 相机距离观察对象的距离

	POINT mLastMousePos;	// 上一帧鼠标位置
};
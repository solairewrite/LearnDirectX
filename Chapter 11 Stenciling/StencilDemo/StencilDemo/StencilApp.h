



#include "../../../Common/d3dApp.h"
#include "../../../Common/MathHelper.h"
#include "../../../Common/UploadBuffer.h"
#include "../../../Common/GeometryGenerator.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")

const int gNumFrameResources = 3;



struct RenderItem
{
	RenderItem() = default;




	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();





	int NumFramesDirty = gNumFrameResources;


	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;


	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque=0,
	Mirrors,
	Reflected,
	Transparent,
	Shadow,
	Count
};

class StencilApp :public D3DApp
{
public:
	StencilApp(HINSTANCE hInstance);
	StencilApp(const StencilApp& rhs) = delete;
	StencilApp& operator=(const StencilApp& rhs) = delete;
	~StencilApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
};
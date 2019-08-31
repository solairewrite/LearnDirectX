#include "../TreeBillboardsApp.h"

using namespace DirectX;
using namespace std;

class PetalDancing
{
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	struct SMotionInfo
	{
		XMFLOAT3 loc;
		XMFLOAT3 speed;
	};

private:
	UINT PetalCount;
	float PetalSize;
	float GroundSize;
	float MinHeight;
	float HeightRange;

	vector<SMotionInfo> motionInfos;
	vector<unique_ptr<RenderItem>> petalRitems;
	vector<RenderItem*> ritems;

public:
	PetalDancing(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		unordered_map<string, unique_ptr<MeshGeometry>>& mGeometries,
		Material* mat,
		vector<RenderItem*>& mRitemLayer,
		vector<unique_ptr<RenderItem>>& mAllRitems
	);

	PetalDancing(const PetalDancing& rhs) = delete;
	PetalDancing& operator=(const PetalDancing& rhs) = delete;
	~PetalDancing();

	void BuildGeometry(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		unordered_map<string, unique_ptr<MeshGeometry>>& mGeometries
	);

	void BuildRenderItems(
		Material* mat,
		MeshGeometry* geo,
		vector<RenderItem*>& mRitemLayer, 
		vector<unique_ptr<RenderItem>>& mAllRitems
	);

	void Update(const GameTimer& gt);
};
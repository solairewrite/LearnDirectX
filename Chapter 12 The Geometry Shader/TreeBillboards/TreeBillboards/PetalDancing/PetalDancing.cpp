#include "PetalDancing.h"

PetalDancing::PetalDancing(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	unordered_map<string, unique_ptr<MeshGeometry>>& mGeometries,
	Material* mat,
	vector<RenderItem*>& mRitemLayer,
	vector<unique_ptr<RenderItem>>& mAllRitems)
{
	PetalCount = 200;
	PetalSize = 8;
	GroundSize = 100.0f;
	MinHeight = 20.0f;
	HeightRange = 50.0f;

	BuildGeometry(device, cmdList, mGeometries);
	BuildRenderItems(mat, mGeometries["PetalDancing"].get(), mRitemLayer, mAllRitems);
}

PetalDancing::~PetalDancing()
{

}

void PetalDancing::BuildGeometry(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	unordered_map<string, unique_ptr<MeshGeometry>>& mGeometries
)
{
	vector<Vertex> vertices(PetalCount);

	motionInfos.resize(PetalCount);

	for (UINT i = 0; i < PetalCount; ++i)
	{
		motionInfos[i].loc.x = MathHelper::RandF(-GroundSize, GroundSize);
		motionInfos[i].loc.y = MathHelper::RandF(MinHeight, MinHeight + HeightRange);
		motionInfos[i].loc.z = MathHelper::RandF(-GroundSize, GroundSize);;
		vertices[i].Pos = XMFLOAT3(motionInfos[i].loc.x, motionInfos[i].loc.y, motionInfos[i].loc.z);
		vertices[i].Size = XMFLOAT2(PetalSize, PetalSize);

		motionInfos[i].speed.x = MathHelper::RandF(-3.0f, 3.0f);
		motionInfos[i].speed.z = MathHelper::RandF(-3.0f, 3.0f);
		motionInfos[i].speed.y = MathHelper::RandF(-10.0, -5.0f);
	}
	vector<uint16_t> indices;
	indices.resize(PetalCount);
	for (UINT i = 0; i < PetalCount; ++i)
	{
		indices[i] = i;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "PetalDancing";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device, cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device, cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	for (UINT i = 0; i < PetalCount; ++i)
	{
		SubmeshGeometry submesh;
		submesh.IndexCount = 1;
		submesh.StartIndexLocation = i;
		submesh.BaseVertexLocation = 0;
		geo->DrawArgs["points" + to_string(i)] = submesh;
	}

	mGeometries[geo->Name] = move(geo);
}

void PetalDancing::BuildRenderItems(
	Material* mat,
	MeshGeometry* geo,
	vector<RenderItem*>& mRitemLayer,
	vector<unique_ptr<RenderItem>>& mAllRitems
)
{
	petalRitems.resize(PetalCount);
	ritems.resize(PetalCount);

	static int ObjCBIndex = 4;
	for (UINT i = 0; i < PetalCount; ++i)
	{
		petalRitems[i] = make_unique<RenderItem>();
		petalRitems[i]->World = MathHelper::Identity4x4();
		petalRitems[i]->ObjCBIndex = ObjCBIndex++;
		petalRitems[i]->Mat = mat;
		petalRitems[i]->Geo = geo;
		petalRitems[i]->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		petalRitems[i]->IndexCount = petalRitems[i]->Geo->DrawArgs["points" + to_string(i)].IndexCount;
		petalRitems[i]->StartIndexLocation = petalRitems[i]->Geo->DrawArgs["points" + to_string(i)].StartIndexLocation;
		petalRitems[i]->BaseVertexLocation = petalRitems[i]->Geo->DrawArgs["points" + to_string(i)].BaseVertexLocation;

		ritems[i] = petalRitems[i].get();
		mRitemLayer.push_back(petalRitems[i].get());
		mAllRitems.push_back(move(petalRitems[i]));
	}
}

void PetalDancing::Update(const GameTimer& gt)
{
	for (UINT i = 0; i < PetalCount; ++i)
	{
		motionInfos[i].loc.x += motionInfos[i].speed.x * gt.DeltaTime();
		motionInfos[i].loc.y += motionInfos[i].speed.y * gt.DeltaTime();
		motionInfos[i].loc.z += motionInfos[i].speed.z * gt.DeltaTime();

		if (motionInfos[i].loc.y < -30.0f)
		{
			motionInfos[i].loc.y = MathHelper::RandF(20.0f, 70.0f);
		}

		XMMATRIX transMatrix = XMMatrixTranslation(motionInfos[i].loc.x, motionInfos[i].loc.y, motionInfos[i].loc.z);
		XMStoreFloat4x4(&ritems[i]->World, transMatrix);
		ritems[i]->NumFramesDirty = gNumFrameResources;
	}
}
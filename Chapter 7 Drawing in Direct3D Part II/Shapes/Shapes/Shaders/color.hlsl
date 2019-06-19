// 基于资源的更新频率对常量数据进行分组
// 在每次渲染过程中,只需将本次所需要的常量 cbPass 更新一次
// 当某个物体的世界矩阵发生改变时,只需更新该物体的相关常量 cbPerObject

// mRootSignature 包含两个cbvTable根参数,分别对应寄存器插槽 0,1
cbuffer cbPerObject: register(b0)
{
	float4x4 gWorld;
};

cbuffer cbPass: register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePow;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
};

// mInputLayout 指定插槽为 0
struct VertexIn
{
	float3 PosL	: POSITION;
	float4 Color: COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float4 Color: COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;


	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosH = mul(posW, gViewProj);


	vout.Color = vin.Color;

	return vout;
}

float4 PS(VertexOut pin): SV_Target
{
	return pin.Color;
}
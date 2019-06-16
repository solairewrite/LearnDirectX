//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	// 语义 "POSITION" 对应 D3D12_INPUT_ELEMENT_DESC 的 "POSITION"
	// D3D12_INPUT_ELEMENT_DESC 通过先后顺序对应 Vertex 结构体中的属性
	float3 PosL  : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	// SV: System Value, 它所修饰的顶点着色器输出元素存有齐次裁剪空间中的顶点位置信息
	// 必须为输出位置信息的参数附上 SV_POSITION 语义
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	// Just pass vertex color into the pixel shader.
	vout.Color = vin.Color;

	return vout;
}

// 在光栅化期间(为三角形计算像素颜色)对顶点着色器(或几何着色器)输出的顶点属性进行差值
// 随后,再将这些差值数据传至像素着色器中作为它的输入
// SV_Target: 返回值的类型应当与渲染目标格式相匹配(该输出值会被存于渲染目标之中)
float4 PS(VertexOut pin) : SV_Target
{
	return pin.Color;
}

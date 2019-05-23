// 第6章 彩色立方体
// 报错: error X3000: Illegal character in shader file
// 解决: 关闭UTF-8插件,貌似shader编码格式只能为ANSI?


// 将常量缓冲区资源绑定到常量缓冲区(cbuffer)寄存器槽0
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 PosL	: POSITION; // 参数语义:POSITION,将顶点结构体中的元素映射到顶点着色器的相应输入函数
	float4 Color: COLOR;	// 参数语义:COLOR
};

struct VertexOut
{
	float4 PosH	: SV_POSITION; // SV: System Value,必须为输出位置信息的参数附上SV_POSITION语义
	float4 Color: COLOR;
};
// 顶点着色器入口
VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 把顶点变换到齐次裁剪空间
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	// 直接将顶点的颜色信息传至像素着色器
	vout.Color = vin.Color;

	return vout;
}
// 像素着色器入口
float4 PS(VertexOut pin) : SV_Target // SV_Target语义表示返回值的类型与渲染目标格式(render target format)匹配
{
	return pin.Color;
}

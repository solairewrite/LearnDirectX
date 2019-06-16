// ******************** 渲染流水线 ********************

// 输入装配器阶段:	从显存中读取几何数据(顶点和索引),再将它们装配为几何图元
// 顶点着色器阶段:	图元被装配完毕后,其顶点送入VS.可以吧顶点着色器看做一种输入与输出数据皆为单个顶点的函数(对顶点的操作由GPU执行)
// 外壳着色器阶段:
// 曲面细分阶段  :
// 域着色器阶段  :
// 几何着色器阶段:
// 光栅化阶段    :	为投影主屏幕上的3D三角形计算出对应的颜色
// 像素着色器阶段:	GPU执行的程序,针对每一个像素片段(亦有译作片元)进行处理,并根据顶点的属性差值作为输入来计算出对应的颜色
// 输出合并阶段  :	通过像素着色器生成的像素片段会被移送至输出合并阶段
//					在此阶段中,一些像素可能会被丢弃(eg,未通过深度缓冲区测试/模板缓冲区测试),剩下的像素片段会被写入后台缓冲区中
//					混合操作在此阶段实现,此技术可令当前处理的像素与后台缓冲区中的对应像素相融合,而不仅是对后者进行完全的覆写

cbuffer cbPerObject : register(b0) // 通过根签名将常量缓冲区与寄存器槽绑定
{
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	// 语义 "POSITION" 对应 D3D12_INPUT_ELEMENT_DESC 的 "POSITION"
	// D3D12_INPUT_ELEMENT_DESC 通过先后顺序对应 Vertex 结构体中的属性
	float3 PosL		: POSITION;
	float4 Color	: COLOR;
};

struct VertexOut
{
	// SV: System Value, 它所修饰的顶点着色器输出元素存有齐次裁剪空间中的顶点位置信息
	// 必须为输出位置信息的参数附上 SV_POSITION 语义
	float4 PosH		: SV_POSITION;
	float4 Color	: COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 转换到齐次裁剪空间
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

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
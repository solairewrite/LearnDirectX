//=============================================================================
// Performs a separable Guassian blur with a blur radius up to 5 pixels.
//=============================================================================
// 计算模糊半径最多为5个像素的可分离高斯模糊
cbuffer cbSettings : register(b0)
{
	// We cannot have an array entry in a constant buffer that gets mapped onto
	// root constants, so list each element.  
	//不能把根常量映射到位于常量缓冲区中的数组元素,因此要将每一个元素都一一列出

    int gBlurRadius;

	// Support up to 11 blur weights.
	// 最多支持11个模糊权值
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

static const int gMaxBlurRadius = 5;


Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0); // 计算着色器的输出资源,RW: 读与写

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID, // 组内id
				int3 dispatchThreadID : SV_DispatchThreadID) // 全局id
{
	// Put in an array for each indexing.
	// 放在数组中,便于索引
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	// 通过填写本地线程存储区来减少带宽的负载,若要对N个像素进行模糊处理,根据模糊半径,我们需要加载 N+2*BlurRadius 个像素
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
	// 此线程组运行着N个线程,为了获取额外的 2*BlurRadius 个像素,就需要有 2*BlurRadius 个线程都多采集一个像素数据
    if (groupThreadID.x < gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
		// 对于图像左侧边界存在越界采样的情况进行钳位操作
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= N - gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
		// 对于图像右侧边界存在越界采样的情况进行钳位操作
        int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
    }

	// Clamp out of bound samples that occur at image borders.
	// 对于图像边界出得越界采样的情况进行钳位操作
    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

	// Wait for all threads to finish.
	// 等待所有的线程完成任务
    GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//
	// 对每个像素进行模糊处理

    float4 blurColor = float4(0, 0, 0, 0);
	
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + gBlurRadius + i;
		
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
	
    gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    if (groupThreadID.y < gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }
    if (groupThreadID.y >= N - gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
        gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
    }
	
	// Clamp out of bound samples that occur at image borders.
    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];


	// Wait for all threads to finish.
    GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

    float4 blurColor = float4(0, 0, 0, 0);
	
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.y + gBlurRadius + i;
		
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
	
    gOutput[dispatchThreadID.xy] = blurColor;
}

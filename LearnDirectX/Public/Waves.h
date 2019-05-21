// 模拟波浪的计算,客户需要将解决方案复制到顶点缓冲区
// 这个类只计算,不绘制






#ifndef WAVES_H
#define WAVES_H

#include <vector>
#include <DirectXMath.h>

class Waves
{
public:
	Waves(int m, int n, float dx, float dt, float speed, float damping);
	Waves(const Waves& rhs) = delete;
	Waves& operator=(const Waves& rhs) = delete;
	~Waves();

	int RowCount() const;
	int ColumnCount() const;
	int VertexCount() const;
	int TriangleCount() const;
	float Width() const;
	float Depth() const;

	// 返回第i个grid point的位置
	const DirectX::XMFLOAT3& Position(int i) const { return mCurrSolution[i]; }

	// 返回第i个grid point的法线
	const DirectX::XMFLOAT3& Normal(int i) const { return mNormals[i]; }

	// 返回第i个grid point的局部x轴的unit tangent vector
	const DirectX::XMFLOAT3& TangetX(int i) const { return mTangentX[i]; }

	void Update(float dt);
	void Disturb(int i, int j, float magnitude);

private:
	int mNumRows = 0; // 行数
	int mNumCols = 0; // 列数

	int mVertexCount = 0; // 顶点数
	int mTriangleCount = 0; // grid三角形数

	// 预先计算模拟常数
	float mK1 = 0.0f;
	float mK2 = 0.0f;
	float mK3 = 0.0f;

	float mTimeStep = 0.0f; // 更新时间频率
	float mSpatialStep = 0.0f; // grid的边长,Spatial:空间

	std::vector<DirectX::XMFLOAT3> mPrevSolution; // 上一帧的顶点位置数组
	std::vector<DirectX::XMFLOAT3> mCurrSolution; // 当前帧的顶点位置数组
	std::vector<DirectX::XMFLOAT3> mNormals; // 法线数组
	std::vector<DirectX::XMFLOAT3> mTangentX; // TabgetX数组
};

#endif // WAVES_H




#include "../Public/Waves.h"
#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace DirectX;
// m:行数, n:列数, dx:方格边长, dt:DeltaTine
Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
	mNumRows = m;
	mNumCols = n;

	mVertexCount = m * n;
	mTriangleCount = (m - 1)*(n - 1) * 2;

	mTimeStep = dt;
	mSpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed*speed)*(dt*dt) / (dx*dx);
	mK1 = (damping*dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f*e) / d;
	mK3 = (2.0f*e) / d;

	mPrevSolution.resize(m*n); // std::vector初始化元素个数
	mCurrSolution.resize(m*n);
	mNormals.resize(m*n);
	mTangentX.resize(m*n);

	// 在系统内存中生成grid顶点

	float halfWidth = (n - 1)*dx*0.5f;
	float halfDepth = (m - 1)*dx*0.5f;
	for (int i = 0; i < m; ++i) // i:行数
	{
		float z = halfDepth - i * dx;
		for (int j = 0; j < n; j++) // j:列数
		{
			float x = -halfWidth + j * dx;

			mPrevSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
			mCurrSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
			mNormals[i*n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mTangentX[i*n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}

Waves::~Waves()
{

}

int Waves::RowCount() const
{
	return mNumRows;
}

int Waves::ColumnCount()const
{
	return mNumCols;
}

int Waves::VertexCount()const
{
	return mVertexCount;
}

int Waves::TriangleCount()const
{
	return mTriangleCount;
}

float Waves::Width() const
{
	return mNumCols * mSpatialStep;
}

float Waves::Depth() const
{
	return mNumRows * mSpatialStep;
}

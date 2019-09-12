#ifndef CAMERA_H
#define CAMERA_H

#include "d3dUtil.h"

class Camera
{
public:

	Camera();
	~Camera();

	// 获取/设置相机的世界坐标
	DirectX::XMVECTOR GetPosition() const;
	DirectX::XMFLOAT3 GetPosition3f() const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);

	// 获取摄像机的基向量
	DirectX::XMVECTOR GetRight() const;
	DirectX::XMFLOAT3 GetRight3f() const;
	DirectX::XMVECTOR GetUp() const;
	DirectX::XMFLOAT3 GetUp3f() const;
	DirectX::XMVECTOR GetLook() const;
	DirectX::XMFLOAT3 GetLook3f() const;

	// 获取视锥体frustum的属性
	float GetNearZ() const;
	float GetFarZ() const;
	float GetAspect() const;
	float GetFovY() const;
	float GetFovX() const;

	// 获取用观察空间坐标表示的远近平面的大小
	float GetNearWindowWidth() const;
	float GetNearWindowHeight() const;
	float GetFarWindowWidth() const;
	float GetFarWindowHeight() const;

	// 设置视锥体frustum
	void SetLens(float fovY, float aspect, float zn, float zf);

	// 通过LookAt方法的参数来定义摄像机空间
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// 获取观察矩阵与投影矩阵
	DirectX::XMMATRIX GetView() const;
	DirectX::XMMATRIX GetProj() const;

	DirectX::XMFLOAT4X4 GetView4x4f() const;
	DirectX::XMFLOAT4X4 GetProj4x4f() const;

	// 将摄像机左右平移Strafe或前后移动
	void Strafe(float d);
	void Walk(float d);
	void UpDown(float d);

	// 将摄像机进行旋转
	void Pitch(float angle);
	void RotateY(float angle);

	// 修改摄像机的位置与朝向后,调用此函数来重新构建观察矩阵
	void UpdateViewMatrix();

private:

	// 摄像机坐标系相对于世界空间的坐标
	DirectX::XMFLOAT3 mPosition = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f,1.0f,0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f,0.0f,1.0f };

	// 缓存视锥体的属性
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	// 观察矩阵与投影矩阵
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};

#endif // CAMERA_H

// 按下'1'以线框模式显示,可以看到波纹





#include "../Public/d3dApp.h"
#include "../Public/MathHelper.h"
#include "../Public/UploadBuffer.h"
#include "../Public/GeometryGenerator.h"
#include "../Public/FrameResource.h"
#include "../Public/Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
// 引入另一个cpp文件的变量,使用extern关键字
extern const int gNumFrameResources;

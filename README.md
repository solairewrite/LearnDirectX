# LearnDirectX
学习书籍: DirectX 12 3D 游戏开发实战  
## 框架项目
核心文件: d3dApp.h
## 参考
[Github项目DXAppendixA: 为使用DX而创建Windows应用程序所需的最简代码](https://github.com/solairewrite/DXAppendixA)  
## Tips
VS2017安装插件后,可以Alt+Space调出命令行  
Ctrl+G: 跳转指定行  
#include \"\"文件结构是以当前文件夹开始的,比如\"../Public/xxx.h\"  
报错: unresolved external symbol _main referenced in function "int _cdecl invovke _main(void)"  
可能是因为创建了控制台程序,DX要创建窗口程序  
解决方法:配置项目属性  
1,C/C++ -> Preprocessor -> Processor Definitions: 删除_CONSOLE, 添加_WINDOWS  
2,Linker -> All Options -> Subsystem: Windows  

# LearnDirectX
学习书籍: DirectX 12 3D 游戏开发实战  
## 框架项目
核心文件: d3dApp.h
## 参考
[Github项目DXAppendixA: 为使用DX而创建Windows应用程序所需的最简代码](https://github.com/solairewrite/DXAppendixA)  
### cpp/hlsl 注释详细的项目
Chapter 9 Texturing: Crate -> CrateReciew  
Chapter 9 Texturing: TexColumns  
### CrateReciew 项目的总结
+ 我所理解的DX  
C++将资源传入GPU,GPU将资源传入register,hlsl通过VS,PS计算出rgb  
#### CrateReview 项目资源的传递  
| 资源 | 何时上传至GPU | 储存资源的变量 | 资源的索引 | 何时指定 register |
| - | - | - | - | - |
| 顶点/索引 | BuildShapeGeometry()<br/>CreateDefaultBuffer() | mGeometries[] | mGeometries[]->DrawArgs[]->StartIndexLocation | / |
| 贴图 | LoadTextures()<br/>CreateDDSTextureFromFile12() | mTextures<br/>与 mSrvDescriptorHeap 通过句柄关联 | 句柄偏移<br>boxRitem->Mat->DiffuseSrvHeapIndex | DrawRenderItems()<br/>SetGraphicsRootDescriptorTable() |
| ObjectCB | UpdateObjectCBs()<br/>CopyData() | mCurrFrameResource->ObjectCB | boxRitem->ObjCBIndex | DrawRenderItems()<br/>SetGraphicsRootConstantBufferView() |
 MatCB | UpdateMaterialCBs()<br/>CopyData() | mCurrFrameResource->MaterialCB | boxRitem->Mat->MatCBIndex | DrawRenderItems()<br/>SetGraphicsRootConstantBufferView() |
| PassCB | UpdateMainPassCB()<br/>CopyData() | mCurrFrameResource->PassCB | / | Draw()<br/>SetGraphicsRootConstantBufferView() |
## Tips
>VS2017命令行: 安装插件 Alt+Space  

>Ctrl+G: 跳转指定行  

>#include \"\"文件结构是以当前文件夹开始的,比如\"../Public/xxx.h\"  

>报错: unresolved external symbol _main referenced in function "int _cdecl invovke _main(void)"  
可能是因为创建了控制台程序,DX要创建窗口程序  
解决方法:配置项目属性  
1,C/C++ -> Preprocessor -> Processor Definitions: 删除_CONSOLE, 添加_WINDOWS  
2,Linker -> All Options -> Subsystem: Windows  

>报错: The build tools for v142 (Platform Toolset = 'v142') cannot be found  
解决方法: 配置项目属性  
General -> Platform Toolset -> 选择合适的  
Window SDK Version -> 选择合适的  

>VS C++ .自动变成->
>选项Text Editor -> C/C++ -> Advanced -> IntelliSense -> Enable Member List Dot-To-Arrow : false  
## 激励
学编程最重要的就是实践,就是写代码,看别人写的代码,再写代码.  
你看再多的书,不写代码,你还是不会编程.  
但是写代码也要讲究方法循序渐进,不能刚学了几天语法,就像写个操作系统什么的.  
树立这种不切实际的目标只会让你的自信受到严重打击迷失自我  
# Windows 安装SourceTree跳过注册
版本: SourceTreeSetup-3.1.3.exe  
第一次打开需要注册  
进入: C:\Users\jizhixin\AppData\Local\Atlassian\SourceTree  
新建: accounts.json  
```
[
    {
        "$id":"1",
        "$type":"SourceTree.Api.Host.Identity.Model.IdentityAccount, SourceTree.Api.Host.Identity",
        "Authenticate":true,
        "HostInstance":{
            "$id":"2",
            "$type":"SourceTree.Host.Atlassianaccount.AtlassianAccountInstance, SourceTree.Host.AtlassianAccount",
            "Host":{
                "$id":"3",
                "$type":"SourceTree.Host.Atlassianaccount.AtlassianAccountHost, SourceTree.Host.AtlassianAccount",
                "Id":"atlassian account"
            },
            "BaseUrl":"https://id.atlassian.com/"
        },
        "Credentials":{
            "$id":"4",
            "$type":"SourceTree.Model.BasicAuthCredentials, SourceTree.Api.Account",
            "Username":"",
            "Email":null
        },
        "IsDefault":false
    }
]
```
进入: C:\Users\jizhixin\AppData\Local\Atlassian\SourceTree.exe_Url_wxsixtweebey2e2x4fkohp3bqqmkyz3q\3.1.3.3158  
修改: user.config  
在 \<SourceTree.Properties.Settings> 下面加上:  
```
<setting name="AgreedToEULA"serializeAs="String">
    <value>True</value>
</setting>
<setting name="AgreedToEULAVersion"serializeAs="String">
    <value>20160201</value>
</setting>
```
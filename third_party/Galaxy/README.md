# Galaxy SDK

本目录保存 LearnOpenGLCN 相机适配器使用的大恒 Galaxy SDK 文件，使 Windows x64 构建不依赖本机 SDK 安装路径。

当前二进制版本：

- `GxIAPI`: 2.0.2508.8211
- `GxIAPICPPEx`: 2.0.2508.8211
- `DxImageProc`: 1.5.2506.8181
- 平台：Windows x64

`include/`、`libs/` 和 `bin/Win64/` 必须来自同一套 SDK。当前适配器只链接 `GxIAPICPPEx.lib`；`bin/Win64/` 保存 `GxIAPICPPEx.dll` 的最小递归运行依赖，共 13 个 DLL。不能只复制表层 SDK DLL，否则程序会因缺少 GenICam 传递依赖而在启动阶段返回 `0xC0000135`。

`licenses/galaxy_3rd_party_licenses.txt` 是安装包随附的第三方许可证清单；它不替代大恒 Galaxy SDK 自身的许可条款。提交或分发 SDK 文件前，维护者仍需确认大恒官方许可允许相应用途。

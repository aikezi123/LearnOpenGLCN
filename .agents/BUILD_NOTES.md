# Build Notes

本文件记录 Codex 在本项目中遇到的构建环境、构建命令和构建问题。正式规则仍以根目录 `AGENTS.md`、`.agents/AGENTS.md`、`.agents/CODEX_CONTEXT.md` 和 `CMakePresets.json` 为准。

## 当前已知构建入口

Windows PowerShell 推荐入口：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
.\msvc-cmake.ps1 -Config Release -NoPause
```

在已经初始化 MSVC x64 开发环境后，可以使用：

```powershell
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug

cmake --preset ninja-msvc-release
cmake --build --preset ninja-msvc-release
```

## 当前运行入口

Debug 构建后：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```

## 记录规则

- 构建命令必须优先从根目录 `CMakePresets.json` 和 `msvc-cmake.ps1` 确认。
- 不在这里记录本机绝对路径作为长期规则。
- 如果某次构建失败，记录第一个有意义的错误、触发场景和最终处理结论。

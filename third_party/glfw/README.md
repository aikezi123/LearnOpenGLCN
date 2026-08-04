# GLFW

本目录使用 GLFW 3.4 官方 Windows x64 预编译包，供 MSVC/Ninja 构建 LearnOpenGL 课程。

- 上游发布页：https://github.com/glfw/glfw/releases/tag/3.4
- 下载文件：`glfw-3.4.bin.WIN64.zip`
- 下载包 SHA-256：`54EFA829400F2A0537F742B2B3BDD74E437BB4F2F048E4B7D3C5557D11A611E6`
- `lib-vc2022/glfw3.lib` SHA-256：`ADD69A85D68A5304C49F306E99619A7169D182012E635D3B58143F511EC31E60`
- 许可证：zlib/libpng，见 `LICENSE.md`

仓库只保留当前 MSVC x64 构建需要的 VC2022 静态库，不保存发布包中的其他编译器版本或动态库。

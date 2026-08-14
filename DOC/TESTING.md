# 自动化测试

## 1. 当前状态

工程已经搭建 GoogleTest 与 CTest 测试框架，但当前还没有加入具体测试用例或测试可执行文件。

测试框架使用：

- GoogleTest 1.17.0，许可证为 BSD-3-Clause。
- CMake 标准 `BUILD_TESTING` 开关。
- CTest 统一发现和运行测试。
- `gtest_discover_tests()` 将后续每个 GoogleTest 用例注册为独立 CTest 测试。

GoogleTest 源码固定保存在 `third_party/googletest`，配置和构建过程不需要联网下载依赖。Windows 下 GoogleTest 使用父工程的动态 CRT 设置，避免测试 target 与生产 target 使用不同 CRT。

## 2. 目录和依赖边界

测试代码统一放在仓库根目录的 `tests/`，并按生产模块镜像组织：

```text
tests/
├── CMakeLists.txt
├── cmake/
│   └── LearnOpenGLAddTest.cmake
├── domain/
├── application/
└── infrastructure/
```

测试属于最外层消费者，依赖方向必须保持为：

```text
tests -> 被测生产 target -> 更内层生产 target
tests -> GoogleTest / GoogleMock
```

生产 target 不得依赖 `tests/`、GoogleTest 或 GoogleMock。测试不能通过额外的全局 include 路径访问生产模块私有实现；需要测试的行为应通过模块公开接口验证。

首批适合测试的模块是纯算法、application service 和不需要 GUI/OpenGL Context 的并发工具。Qt、OpenGL Context 和真实相机 SDK 路径应作为后续集成测试单独组织，不混入快速单元测试。

## 3. 新增测试 target

在对应测试目录中创建测试源码和 `CMakeLists.txt`，调用公共注册函数：

```cmake
learnopengl_add_gtest(learnopengl_domain_trajectory_tests
    SOURCES
        ArchimedeanSpiral2DGeneratorTest.cpp
    LIBRARIES
        learnopengl::domain
    LABELS
        unit
        domain
)
```

需要 GoogleMock 时增加 `USE_GMOCK`：

```cmake
learnopengl_add_gtest(learnopengl_application_camera_tests
    SOURCES
        CameraCaptureServiceTest.cpp
    LIBRARIES
        learnopengl::application
    LABELS
        unit
        application
    USE_GMOCK
)
```

然后在 `tests/CMakeLists.txt` 中加入对应子目录：

```cmake
add_subdirectory(domain/trajectory)
add_subdirectory(application/camera)
```

公共函数会统一完成：

- 创建测试 executable。
- 链接 `GTest::gtest_main` 或 `GTest::gmock_main`。
- 要求 C++17。
- 将测试 executable 输出到 `out/build/<preset>/bin/tests`。
- 使用 `gtest_discover_tests()` 注册 CTest 测试。

每个测试 target 只链接自己真正测试的生产 target。不要为了方便统一链接 `learnopengl_ui`、完整 `infrastructure` 或所有第三方库。

## 4. 配置、构建和运行

`include(CTest)` 创建的 `BUILD_TESTING` 默认值为 `ON`。正常 Debug 构建会构建后续加入的测试 target：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
ctest --test-dir out/build/ninja-msvc-debug --output-on-failure
```

只构建测试 target：

```powershell
cmake --build --preset ninja-msvc-debug --target learnopengl_domain_trajectory_tests
```

按名称或标签选择测试：

```powershell
ctest --test-dir out/build/ninja-msvc-debug -R trajectory --output-on-failure
ctest --test-dir out/build/ninja-msvc-debug -L unit --output-on-failure
```

不需要测试的生产构建可在配置时关闭：

```powershell
cmake --preset ninja-msvc-debug -DBUILD_TESTING=OFF
cmake --build --preset ninja-msvc-debug
```

当前框架阶段执行 CTest 会报告没有发现测试，这是预期结果；加入第一个测试 target 后，CTest 才会列出并运行具体用例。

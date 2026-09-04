# 自动化测试

## 1. 当前状态

工程已接入 GoogleTest 1.17.0 和 CTest，测试由标准 `BUILD_TESTING` 开关控制。GoogleTest 源码固定保存在 `third_party/googletest`，配置和构建过程不需要联网下载依赖；Windows 下沿用父工程的动态 CRT 设置。

当前测试目标如下：

| Target | 状态 | 被测模块 |
| --- | --- | --- |
| `engineeringlab_domain_trajectory_tests` | 已建立可编译占位文件，暂无用例 | `englab::domain` |
| `engineeringlab_application_camera_tests` | 已建立可编译占位文件和 `FakeCameraDevice`，暂无用例 | `englab::application` |
| `engineeringlab_infrastructure_concurrency_tests` | 已包含 20 个 `ThreadPool` 单元测试 | `englab::concurrency` |

`ThreadPool` 用例覆盖构造参数和类型所有权约束；`post()`执行、只执行一次、异常隔离和空任务拒绝；`submit()`返回值、多参数、`void`、移动专用 callable/参数、只执行一次和异常传播；空 `std::function`的延迟异常；关闭排空、析构排空、重复/外部并发关闭、关闭后拒绝；以及多生产者和多 worker 并发行为。

2026-09-04 在 EngineeringLab 命名统一和基础设施 target 拆分后，使用 `ninja-msvc-debug` 完成重新配置与全量构建，随后执行 `ctest --preset ninja-msvc-debug --output-on-failure`，实际发现并通过 20/20 个用例。该结果是当前 Debug 基线，不代表 Release、AddressSanitizer 或尚未编写的极端竞争场景已经验证。

## 2. 目录和依赖边界

测试代码放在仓库根目录的 `tests/`，按生产模块镜像组织：

```text
tests/
├── CMakeLists.txt
├── cmake/
│   └── LearnOpenGLAddTest.cmake
├── domain/
│   └── trajectory/
│       └── ArchimedeanSpiral2DGeneratorTest.cpp
├── application/
│   └── camera/
│       └── CameraCaptureServiceTest.cpp
├── infrastructure/
│   └── concurrency/
│       └── ThreadPoolTest.cpp
└── support/
    └── camera/
        └── FakeCameraDevice.h
```

测试是最外层消费者，依赖方向必须保持为：

```text
tests -> 被测生产 target -> 更内层生产 target
tests -> GoogleTest / GoogleMock
```

生产 target 不得依赖 `tests/`、GoogleTest 或 GoogleMock。测试只能通过模块公开接口验证行为，不能通过全局 include 路径访问生产模块私有实现。

线程池已从包含 OpenGL、GLFW 和 Galaxy SDK 的完整 `infrastructure` 中拆为独立的 `englab::concurrency` 静态库。线程池测试只链接该 target 和 GoogleTest，因而不需要 GUI、OpenGL Context 或真实相机 SDK。

线程池当前行为、八阶段进度和未覆盖边界统一记录在[线程池并发模块](../modules/THREAD_POOL.md)。新增并发能力时应同步更新该文档和本页的实际测试数量，不能只根据测试源文件存在就声称行为已验证。

## 3. 新增测试

在 `tests/CMakeLists.txt` 中使用公共函数注册测试 target：

```cmake
engineeringlab_add_gtest(engineeringlab_domain_trajectory_tests
    SOURCES
        domain/trajectory/ArchimedeanSpiral2DGeneratorTest.cpp
    LIBRARIES
        englab::domain
    LABELS
        unit
        domain
)
```

需要 GoogleMock 时增加 `USE_GMOCK`。公共函数会创建测试 executable、链接 `GTest::gtest_main` 或 `GTest::gmock_main`、启用 C++17，并通过 `gtest_discover_tests()` 把每个 GoogleTest 用例注册为独立 CTest 测试。

每个测试 target 只链接自己真正测试的生产 target。不要为了方便统一链接 `engineeringlab_ui`、完整 `infrastructure` 或所有第三方库。

测试源码使用 `TEST(TestSuiteName, TestName)` 定义普通用例；需要共享初始化与清理逻辑时，再定义继承 `::testing::Test` 的 fixture 并使用 `TEST_F`。断言优先使用 `EXPECT_*`；后续步骤依赖当前条件成立时使用 `ASSERT_*`。

## 4. 配置、构建和运行

`BUILD_TESTING` 默认值为 `ON`。仓库的 `CMakePresets.json` 为 Debug、Release 和 AddressSanitizer 分别定义了 configure、build 和 test preset；VS Code 工作区固定使用这些 presets，并由 CMake Tools 自动尝试加载 MSVC Developer Environment。

日常开发推荐在 VS Code 命令面板中依次选择：

```text
CMake: Select Configure Preset -> ninja-msvc-debug
CMake: Select Build Preset     -> ninja-msvc-debug
CMake: Select Test Preset      -> ninja-msvc-debug
CMake: Build Target            -> engineeringlab_infrastructure_concurrency_tests
CMake: Run Tests
```

也可以在 VS Code Testing 面板中运行或调试单个 GoogleTest 用例。

命令行或 CI 使用相同的 preset。使用 Ninja + MSVC 时，终端必须已经初始化 Visual Studio Developer Environment：

```powershell
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug
ctest --preset ninja-msvc-debug
```

普通 PowerShell 可通过项目脚本完成环境初始化、配置和构建，然后使用 test preset：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
ctest --preset ninja-msvc-debug
```

只构建和运行线程池测试：

```powershell
cmake --build --preset ninja-msvc-debug --target engineeringlab_infrastructure_concurrency_tests
ctest --preset ninja-msvc-debug -R ThreadPoolTest
```

也可以按标签筛选：

```powershell
ctest --preset ninja-msvc-debug -L concurrency
ctest --preset ninja-msvc-debug -L unit
```

不需要测试的生产构建可在配置时关闭：

```powershell
cmake --preset ninja-msvc-debug -DBUILD_TESTING=OFF
cmake --build --preset ninja-msvc-debug
```

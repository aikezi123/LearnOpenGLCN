# 指定 CMake 最低版本
# 低于 3.21 的 CMake 不允许配置这个项目
cmake_minimum_required(VERSION 3.21)

# 定义项目名称
# LANGUAGES C CXX 表示项目同时启用 C 和 C++ 两种语言
# C   用来编译 glad.c
# CXX 用来编译项目中的 C++ 源文件和入口文件
project(LearnOpenGL_Window LANGUAGES C CXX)

# 提供标准 BUILD_TESTING 开关并在启用时注册 CTest。
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
    LEARNOPENGL_ENABLE_ASAN
    "Enable AddressSanitizer for memory error diagnostics"
    OFF
)

if(LEARNOPENGL_ENABLE_ASAN)
    if(NOT MSVC)
        message(FATAL_ERROR "LEARNOPENGL_ENABLE_ASAN currently supports the MSVC toolchain only.")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "The current AddressSanitizer preset supports x64 builds only.")
    endif()

    add_compile_options(/fsanitize=address /Zi)
    add_link_options(/fsanitize=address /DEBUG /INCREMENTAL:NO)
endif()

# MSVC 使用 UTF-8，避免中文注释或字符串导致 C4819
if (MSVC)
    add_compile_options(/utf-8)
endif()

# 把 third_party/glad 子目录加入构建系统
# 这个子目录负责定义 glad 这个库
add_subdirectory(third_party/glad)

# 把 third_party/glfw 子目录加入构建系统
# 这个子目录负责导入已经下载好的 glfw3.lib
add_subdirectory(third_party/glfw)

# 把 third_party/stb 子目录加入构建系统
add_subdirectory(third_party/stb)

# 把 third_party/glm 子目录加入构建系统
add_subdirectory(third_party/glm)

# 把 third_party/Galaxy 子目录加入构建系统
# 这个子目录负责把大恒 Galaxy SDK 的头文件和 .lib 包装成 CMake target
add_subdirectory(third_party/Galaxy)

if(BUILD_TESTING)
    # GoogleTest 1.17.0 随仓库固定版本保存，避免配置阶段依赖网络。
    # Windows 下沿用父工程的动态 CRT 设置，避免测试与生产库的 CRT 不一致。
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
endif()

add_subdirectory(domain)
add_subdirectory(application)

add_subdirectory(infrastructure)

# LearnOpenGL示例代码
# lessons依赖infrastructure，因此要放在add_subdirectory(infrastructure)后面
add_subdirectory(lessons)

add_subdirectory(ui)

# 程序入口层
add_subdirectory(composition_root)

if(BUILD_TESTING)
    # 测试作为最外层消费者，必须在所有生产 target 定义完成后加载。
    add_subdirectory(tests)
endif()

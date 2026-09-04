# 指定 CMake 最低版本
# 低于 3.21 的 CMake 不允许配置这个项目
cmake_minimum_required(VERSION 3.21)

# 定义项目名称
# LANGUAGES C CXX 表示项目同时启用 C 和 C++ 两种语言
# C   用来编译 glad.c
# CXX 用来编译项目中的 C++ 源文件和入口文件
project(EngineeringLab LANGUAGES C CXX)

# 提供标准 BUILD_TESTING 开关并在启用时注册 CTest。
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
    ENGINEERINGLAB_ENABLE_ASAN
    "Enable AddressSanitizer for memory error diagnostics"
    OFF
)

if(ENGINEERINGLAB_ENABLE_ASAN)
    if(NOT MSVC)
        message(FATAL_ERROR "ENGINEERINGLAB_ENABLE_ASAN currently supports the MSVC toolchain only.")
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

# 开源依赖由 vcpkg manifest 提供，项目只消费标准 CMake target。
find_package(spdlog CONFIG REQUIRED)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
endif()

add_subdirectory(domain)
add_subdirectory(application)

add_subdirectory(infrastructure)

# OpenGL 课程代码依赖独立的 OpenGL 基础设施 target。
add_subdirectory(lessons)

add_subdirectory(ui)

# 程序入口层
add_subdirectory(composition_root)

if(BUILD_TESTING)
    # 测试作为最外层消费者，必须在所有生产 target 定义完成后加载。
    add_subdirectory(tests)
endif()

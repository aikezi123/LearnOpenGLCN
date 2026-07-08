# 指定 CMake 最低版本
# 低于 3.21 的 CMake 不允许配置这个项目
cmake_minimum_required(VERSION 3.21)

# 定义项目名称
# LANGUAGES C CXX 表示项目同时启用 C 和 C++ 两种语言
# C   用来编译 glad.c
# CXX 用来编译项目中的 C++ 源文件和入口文件
project(LearnOpenGL_Window LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_subdirectory(domain)
add_subdirectory(application)

add_subdirectory(infrastructure)

# LearnOpenGL示例代码
# lessons依赖infrastructure，因此要放在add_subdirectory(infrastructure)后面
add_subdirectory(lessons)

add_subdirectory(ui)

# 程序入口层
add_subdirectory(composition_root)

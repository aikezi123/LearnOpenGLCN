# 查找系统 OpenGL 库
#
# 在 Windows 上通常会找到 opengl32.lib
# CMake 会提供一个导入目标：OpenGL::GL
# 后面链接 OpenGL::GL 即可，不需要手动写 opengl32.lib
find_package(OpenGL REQUIRED)

# 自动递归收集当前 App 目录下的所有 .cpp 文件
#
# CONFIGURE_DEPENDS 的作用：
# 当你新增或删除 .cpp 文件时，CMake 会在下一次构建前尝试重新扫描文件列表
#
# 例如会自动收集：
#   App/main.cpp
#   App/getting_started/src/hello_window.cpp
#   App/getting_started/src/hello_triangle.cpp
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 生成最终可执行程序
#
# 目标名：LearnOpenGLCN
# 最终会生成 LearnOpenGLCN.exe
#
# APP_SOURCES 中包含 main.cpp 和各章节的 .cpp 文件
# 注意：整个项目只能有一个 main() 函数
add_executable(LearnOpenGLCN
    ${APP_SOURCES}
)

# 指定当前目标使用 C++17
#
# PRIVATE 表示这个要求只作用于 LearnOpenGLCN 自身
target_compile_features(LearnOpenGLCN PRIVATE
    cxx_std_17
)

# 配置头文件搜索路径
#
# 这样代码里可以直接写：
#   #include "hello_window.h"
#   #include "hello_triangle.h"
#
# 以及使用 common 目录中的公共头文件
target_include_directories(LearnOpenGLCN PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/getting_started/include
)

# 链接依赖库
#
# glad     ：OpenGL 函数加载器
# glfw3    ：窗口创建、OpenGL 上下文、键盘鼠标输入
# OpenGL::GL：系统 OpenGL 库，Windows 下通常对应 opengl32.lib
target_link_libraries(LearnOpenGLCN PRIVATE
    glad
    glfw3
    OpenGL::GL
)

# MSVC 编译器下强制按 UTF-8 处理源码
#
# 避免中文注释、中文字符串在 Windows 默认代码页下触发 C4819 等编码问题
if (MSVC)
    target_compile_options(LearnOpenGLCN PRIVATE /utf-8)
endif()

# 指定可执行文件输出目录
#
# 最终生成位置类似：
#   out/build/ninja-msvc-debug/bin/LearnOpenGLCN.exe
set_target_properties(LearnOpenGLCN PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
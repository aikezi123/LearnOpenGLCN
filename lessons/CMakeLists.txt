# 收集 lessons 下所有子目录中的 src/*.cpp 文件
# GLOB_RECURSE       : 递归遍历，进入所有子文件夹查找文件
# CONFIGURE_DEPENDS  : 当新增/删除匹配文件时，CMake 会尝试自动重新配置
# CMAKE_CURRENT_SOURCE_DIR : 当前 CMakeLists.txt 所在目录
file(GLOB_RECURSE LESSON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 收集 lessons 下所有子目录中的 include 目录
# LIST_DIRECTORIES true : 明确告诉 CMake 匹配结果中可以包含目录
file(GLOB_RECURSE LESSON_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

# 创建 lessons 静态库
add_library(lessons STATIC
    ${LESSON_SOURCES}
)

# lessons 对外暴露头文件目录
target_include_directories(lessons PUBLIC
    ${LESSON_INCLUDE_DIRS}
)

# 定义 assets 资源目录宏
# C++ 代码中可以通过 LEARNOPENGL_ASSET_DIR 获取 assets 目录路径
# target_compile_definitions() 的作用是：给 C++ 代码添加一个预处理宏。
# 等价于在编译 lessons 相关代码时，自动加上类似这样的宏定义：#define LEARNOPENGL_ASSET_DIR "你的项目根目录/assets"
# 所以你在 C++ 里就可以这样写：std::string shaderDir = std::string(LEARNOPENGL_ASSET_DIR) + "/shaders/getting_started/shader_exercises/";
target_compile_definitions(lessons PUBLIC
    LEARNOPENGL_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# lessons 依赖 infrastructure
# infrastructure 已经负责链接 glad / glfw / OpenGL
target_link_libraries(lessons PUBLIC
    infrastructure
)
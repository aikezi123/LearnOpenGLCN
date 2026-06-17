# 创建一个名为 glm 的 INTERFACE 库目标。
# INTERFACE 库不会编译源文件，也不会生成 .lib、.a 或 .dll。
# 它只用来保存并传播“使用 GLM 所需要的配置”，
# 很适合 GLM 这种只有头文件的 Header-Only 库。
add_library(glm INTERFACE)

# 给真实目标 glm 创建一个别名 glm::glm。
# glm::glm 和 glm 指向同一个 CMake target，不会创建第二个库。
# 使用 包名::目标名 的形式可读性更好，
# 也方便以后切换到 find_package(glm) 或包管理器提供的同名目标。
add_library(glm::glm ALIAS glm)

# 为 glm 目标声明头文件搜索目录。
#
# INTERFACE 表示：
# glm 自己没有源文件需要编译，
# 但所有链接 glm 或 glm::glm 的目标，
# 都会自动获得 third_party/glm/include 这个头文件搜索路径。
#
# 因此其他目标链接 glm::glm 后，就可以直接写：
#     #include <glm/glm.hpp>
target_include_directories(glm
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
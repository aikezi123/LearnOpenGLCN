# 递归收集各子模块 src 目录下的源文件
file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 递归收集各子模块 include 目录下的头文件
file(GLOB_RECURSE DOMAIN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)

# 收集各子模块的 include 目录，作为头文件搜索根目录
file(GLOB DOMAIN_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

# 创建 domain 静态库
add_library(learnopengl_domain STATIC
    ${DOMAIN_SOURCES}
    #${DOMAIN_HEADERS}不是必须加进去，主要编译的是源文件
)

# 为 domain 库创建命名空间别名
add_library(learnopengl::domain ALIAS learnopengl_domain)

# 将各模块 include 目录暴露给本库及依赖本库的 target
target_include_directories(learnopengl_domain
    PUBLIC
        ${DOMAIN_INCLUDE_DIRS}
)

# 要求本库及依赖本库的 target 至少使用 C++17
target_compile_features(learnopengl_domain
    PUBLIC
        cxx_std_17
)
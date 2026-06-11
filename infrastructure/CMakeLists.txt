find_package(OpenGL REQUIRED)

# 收集infrastructure下所有模块的src/*.cpp
file(GLOB_RECURSE INFRASTRUCTURE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 收集 infrastructure 下所有模块的头文件，主要用于 IDE 显示
file(GLOB_RECURSE INFRASTRUCTURE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)



# 收集所有模块的include目录
file(GLOB_RECURSE INFRASTRUCTURE_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)


# 创建动态库
add_library(infrastructure STATIC)

target_sources(infrastructure PRIVATE
    ${INFRASTRUCTURE_SOURCES}
    ${INFRASTRUCTURE_HEADERS}
)

# 对外暴露头文件
target_include_directories(infrastructure PUBLIC
    ${INFRASTRUCTURE_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(infrastructure PUBLIC
    glad
    glfw3
    OpenGL::GL
)


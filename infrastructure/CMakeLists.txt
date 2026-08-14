find_package(OpenGL REQUIRED)

# 并发工具不依赖 OpenGL、GLFW 或相机 SDK，单独形成可测试 target。
add_subdirectory(concurrency)

# 收集 infrastructure 下除 concurrency 外所有模块的 src/*.cpp。
file(GLOB_RECURSE INFRASTRUCTURE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/*.cpp"
)
list(FILTER INFRASTRUCTURE_SOURCES EXCLUDE REGEX "/concurrency/")

# 收集 infrastructure 下除 concurrency 外所有模块的头文件，主要用于 IDE 显示。
file(GLOB_RECURSE INFRASTRUCTURE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/include/*.hpp"
)
list(FILTER INFRASTRUCTURE_HEADERS EXCLUDE REGEX "/concurrency/")

# 收集除 concurrency 外所有模块的 include 目录。
file(GLOB_RECURSE INFRASTRUCTURE_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/include"
)
list(FILTER INFRASTRUCTURE_INCLUDE_DIRS EXCLUDE REGEX "/concurrency/")

add_library(infrastructure STATIC)
add_library(learnopengl::infrastructure ALIAS infrastructure)

target_sources(infrastructure
    PRIVATE
        ${INFRASTRUCTURE_SOURCES}
        ${INFRASTRUCTURE_HEADERS}
)

target_include_directories(infrastructure
    PUBLIC
        ${INFRASTRUCTURE_INCLUDE_DIRS}
)

target_link_libraries(infrastructure
    PUBLIC
        learnopengl::application
        learnopengl::concurrency
        glad
        glfw3
        OpenGL::GL
        glm::glm
    PRIVATE
        Galaxy::SDK
)

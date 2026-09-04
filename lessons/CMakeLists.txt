# 收集 lessons 下所有子目录中的 src/*.cpp 文件
# GLOB_RECURSE       : 递归遍历，进入所有子文件夹查找文件
# CONFIGURE_DEPENDS  : 当新增/删除匹配文件时，CMake 会尝试自动重新配置
file(GLOB_RECURSE LESSON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 收集 lessons 下所有子目录中的 include 目录
file(GLOB_RECURSE LESSON_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

add_library(engineeringlab_opengl_lessons STATIC
    ${LESSON_SOURCES}
)
add_library(englab::opengl_lessons ALIAS engineeringlab_opengl_lessons)

target_include_directories(engineeringlab_opengl_lessons
    PUBLIC
        ${LESSON_INCLUDE_DIRS}
)

# 课程资源仍保留在统一 assets 目录中。
target_compile_definitions(engineeringlab_opengl_lessons
    PUBLIC
        ENGINEERINGLAB_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# 课程只链接实际使用的 OpenGL 能力，不依赖相机 SDK 或并发实现。
find_package(OpenGL REQUIRED)
target_link_libraries(engineeringlab_opengl_lessons
    PUBLIC
        englab::graphics_opengl
        stb_image
        glad
        glfw3
        OpenGL::GL
        glm::glm
)

target_compile_features(engineeringlab_opengl_lessons
    PUBLIC
        cxx_std_17
)
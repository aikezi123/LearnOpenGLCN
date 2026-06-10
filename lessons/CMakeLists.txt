find_package(OpenGL REQUIRED)

# 收集 lessons 下所有子目录中的 src/*.cpp 文件
# GLOB_RECURSE : 递归遍历，进入所有子文件夹去查找文件
# CONFIGURE_DEPENDS : 配置依赖项检测，增加、删除、修改文件时，CMake会自动重新构建
# CMAKE_CURRENT_SOURCE_DIR : CMakeLists文件所在的文件夹
file(GLOB_RECURSE LESSON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 收集 lessons 下所有子目录中的 include 目录
# LIST_DIRECTORIES true : 作用是把匹配到的目录也放进结果列表里。某些情况下CMake可能只收集文件不收集目录，使用这个方式更明确用来收集目录
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

# lessons 依赖的库
target_link_libraries(lessons PUBLIC
    glad
    glfw3
    OpenGL::GL
)
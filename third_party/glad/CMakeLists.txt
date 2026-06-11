add_library(glad STATIC
    src/glad.c
)

# 对外暴露头文件搜索起点
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# 查找 C++ 标准线程库，供相机控制线程使用
find_package(Threads REQUIRED)

# 日志端口是独立、无第三方依赖的 application API。本阶段尚未被业务模块使用。
add_library(engineeringlab_diagnostics INTERFACE)
add_library(englab::diagnostics ALIAS engineeringlab_diagnostics)

target_include_directories(engineeringlab_diagnostics
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/include
)

target_compile_features(engineeringlab_diagnostics
    INTERFACE
        cxx_std_17
)

# 递归收集各子模块 src 目录下的源文件
file(GLOB_RECURSE APPLICATION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

# 递归收集各子模块 include 目录下的头文件
file(GLOB_RECURSE APPLICATION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)

# 收集各子模块的 include 目录，作为头文件搜索根目录
file(GLOB APPLICATION_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

# 创建 application 静态库，头文件同时加入 target 便于 IDE 展示
add_library(engineeringlab_application STATIC
    ${APPLICATION_SOURCES}
    ${APPLICATION_HEADERS}
)

# 为 application 库创建命名空间别名
add_library(englab::application ALIAS engineeringlab_application)

# 将各模块 include 目录暴露给本库及依赖本库的 target
target_include_directories(engineeringlab_application
    PUBLIC
        ${APPLICATION_INCLUDE_DIRS}
)

# domain 类型会出现在公共接口中；标准线程只属于 application 内部实现
target_link_libraries(engineeringlab_application
    PUBLIC
        englab::domain
    PRIVATE
        Threads::Threads
)

# 要求本库及依赖本库的 target 至少使用 C++17
target_compile_features(engineeringlab_application
    PUBLIC
        cxx_std_17
)

# promise/future 命令执行会捕获标准 C++ 异常，MSVC 需要启用栈展开语义。
if(MSVC)
    target_compile_options(engineeringlab_application
        PRIVATE
            /EHsc
    )
endif()

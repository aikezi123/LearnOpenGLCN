add_library(engineeringlab_logging STATIC
    include/logging/SpdlogLogger.h
    src/SpdlogLogger.cpp
)
add_library(englab::logging ALIAS engineeringlab_logging)

target_include_directories(engineeringlab_logging
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engineeringlab_logging
    PUBLIC
        englab::diagnostics
    PRIVATE
        spdlog::spdlog
)

target_compile_features(engineeringlab_logging
    PUBLIC
        cxx_std_17
)

# 日志初始化会报告配置异常，运行期边界会捕获 spdlog 异常；MSVC 需要完整栈展开语义。
if(MSVC)
    target_compile_options(engineeringlab_logging
        PRIVATE
            /EHsc
    )
endif()

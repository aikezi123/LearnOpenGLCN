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

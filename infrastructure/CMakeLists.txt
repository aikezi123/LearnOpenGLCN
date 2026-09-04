find_package(OpenGL REQUIRED)

# 各基础设施能力保持独立，避免课程或工具被迫链接无关的 SDK。
add_subdirectory(concurrency)
add_subdirectory(logging)
add_subdirectory(memory)

add_library(engineeringlab_graphics_opengl STATIC
    shader/include/shader/Shader.hpp
    shader/src/Shader.cpp
)
add_library(englab::graphics_opengl ALIAS engineeringlab_graphics_opengl)

target_include_directories(engineeringlab_graphics_opengl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/include
)

target_link_libraries(engineeringlab_graphics_opengl
    PRIVATE
        glad
        OpenGL::GL
)

target_compile_features(engineeringlab_graphics_opengl
    PUBLIC
        cxx_std_17
)

add_library(engineeringlab_camera_galaxy STATIC
    camera/galaxy/include/camera/galaxy/GalaxyCameraController.h
    camera/galaxy/src/GalaxyCameraController.cpp
)
add_library(englab::camera_galaxy ALIAS engineeringlab_camera_galaxy)

target_include_directories(engineeringlab_camera_galaxy
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/camera/galaxy/include
)

target_link_libraries(engineeringlab_camera_galaxy
    PUBLIC
        englab::application
    PRIVATE
        Galaxy::SDK
)

target_compile_features(engineeringlab_camera_galaxy
    PUBLIC
        cxx_std_17
)

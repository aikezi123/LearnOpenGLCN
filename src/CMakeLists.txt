find_package(OpenGL REQUIRED)

add_executable(HelloWindow
    main.cpp
)

target_compile_features(HelloWindow PRIVATE
    cxx_std_17
)

target_link_libraries(HelloWindow PRIVATE
    glad
    glfw3
    OpenGL::GL
)
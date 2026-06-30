# Qt/OpenGL 工程原型入口
add_executable(LearnOpenGLCN_Qt
    qt_main.cpp
)

target_link_libraries(LearnOpenGLCN_Qt PRIVATE
    learnopengl::ui
)

target_compile_features(LearnOpenGLCN_Qt PRIVATE
    cxx_std_17
)

if(COMMAND galaxy_copy_runtime_dlls)
    galaxy_copy_runtime_dlls(LearnOpenGLCN_Qt)
endif()

# LearnOpenGL 课程代码入口
add_executable(LearnOpenGLCN_Lessons
    lesson_main.cpp
)

target_link_libraries(LearnOpenGLCN_Lessons PRIVATE
    lessons
)

target_compile_features(LearnOpenGLCN_Lessons PRIVATE
    cxx_std_17
)

# 统一 exe 输出目录
set_target_properties(
    LearnOpenGLCN_Qt
    LearnOpenGLCN_Lessons
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

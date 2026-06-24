# 生成可执行程序
add_executable(LearnOpenGLCN
    main.cpp
)

# app 只负责程序入口
# 真正的 OpenGL示例放在lessons里
target_link_libraries(LearnOpenGLCN PRIVATE
    lessons
    learnopengl_ui
)

target_compile_features(LearnOpenGLCN PRIVATE
    cxx_std_17
)

# 统一exe输出目录
set_target_properties(LearnOpenGLCN PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
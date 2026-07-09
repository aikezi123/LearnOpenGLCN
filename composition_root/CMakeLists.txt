if(DEFINED ENV{QT_ROOT})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
endif()

find_package(Qt6 6.9 CONFIG REQUIRED COMPONENTS
    Widgets
)

# Qt/OpenGL 工程原型入口
add_executable(LearnOpenGLCN_Qt
    qt_main.cpp
)

target_link_libraries(LearnOpenGLCN_Qt PRIVATE
    learnopengl::ui
    learnopengl::infrastructure
)

target_compile_features(LearnOpenGLCN_Qt PRIVATE
    cxx_std_17
)

# LearnOpenGL 课程代码入口
add_executable(LearnOpenGLCN_Lessons
    lesson_main.cpp
    lesson_launcher/include/LessonLauncherWindow.h
    lesson_launcher/src/LessonLauncherWindow.cpp
)

target_include_directories(LearnOpenGLCN_Lessons PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lesson_launcher/include
)

target_link_libraries(LearnOpenGLCN_Lessons PRIVATE
    lessons
    Qt6::Widgets
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

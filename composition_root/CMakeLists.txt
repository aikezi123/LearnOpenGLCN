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

if(LEARNOPENGL_ENABLE_ASAN)
    get_filename_component(MSVC_COMPILER_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(
        MSVC_ASAN_RUNTIME_DLL
        "${MSVC_COMPILER_DIRECTORY}/clang_rt.asan_dbg_dynamic-x86_64.dll"
    )

    if(NOT EXISTS "${MSVC_ASAN_RUNTIME_DLL}")
        message(FATAL_ERROR "MSVC AddressSanitizer runtime was not found: ${MSVC_ASAN_RUNTIME_DLL}")
    endif()

    foreach(target_name IN ITEMS LearnOpenGLCN_Qt LearnOpenGLCN_Lessons)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${MSVC_ASAN_RUNTIME_DLL}"
                "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Deploying the MSVC AddressSanitizer runtime for ${target_name}"
            VERBATIM
        )
    endforeach()
endif()

if(DEFINED ENV{QT_ROOT})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
endif()

find_package(OpenGL REQUIRED)

find_package(Qt6 6.9 CONFIG REQUIRED COMPONENTS
    Concurrent
    Widgets
    OpenGL
    OpenGLWidgets
)

file(GLOB_RECURSE LEARNOPENGL_UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE LEARNOPENGL_UI_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

file(GLOB_RECURSE LEARNOPENGL_UI_FORMS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)

add_library(learnopengl_ui STATIC)

add_library(learnopengl::ui ALIAS learnopengl_ui)

target_sources(learnopengl_ui
    PRIVATE
        ${LEARNOPENGL_UI_SOURCES}
        ${LEARNOPENGL_UI_HEADERS}
        ${LEARNOPENGL_UI_FORMS}
)

target_include_directories(learnopengl_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(learnopengl_ui
    PUBLIC
        learnopengl::application
        learnopengl::domain
        Qt6::Widgets
        Qt6::OpenGLWidgets
    PRIVATE
        Qt6::Concurrent
        Qt6::OpenGL
        stb_image
        glad
        OpenGL::GL
)

target_compile_features(learnopengl_ui
    PUBLIC
        cxx_std_17
)

set_target_properties(learnopengl_ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC OFF
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

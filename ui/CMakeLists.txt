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

file(GLOB_RECURSE ENGINEERINGLAB_UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE ENGINEERINGLAB_UI_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

file(GLOB_RECURSE ENGINEERINGLAB_UI_FORMS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)


add_library(engineeringlab_ui STATIC)

add_library(englab::ui ALIAS engineeringlab_ui)

target_sources(engineeringlab_ui
    PRIVATE
        ${ENGINEERINGLAB_UI_SOURCES}
        ${ENGINEERINGLAB_UI_HEADERS}
        ${ENGINEERINGLAB_UI_FORMS}
)

target_include_directories(engineeringlab_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engineeringlab_ui
    PUBLIC
        englab::application
        englab::domain
        Qt6::Widgets
        Qt6::OpenGLWidgets
    PRIVATE
        Qt6::Concurrent
        Qt6::OpenGL
        stb_image
        glad
        OpenGL::GL
)

target_compile_features(engineeringlab_ui
    PUBLIC
        cxx_std_17
)

set_target_properties(engineeringlab_ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC OFF
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

add_library(learnopengl_application STATIC)

add_library(learnopengl::application ALIAS learnopengl_application)

file(GLOB_RECURSE APPLICATION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

file(GLOB_RECURSE APPLICATION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)

file(GLOB_RECURSE APPLICATION_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

target_sources(learnopengl_application
    PRIVATE
        ${APPLICATION_SOURCES}
        ${APPLICATION_HEADERS}
)

target_include_directories(learnopengl_application
    PUBLIC
        ${APPLICATION_INCLUDE_DIRS}
)

target_link_libraries(learnopengl_application
    PUBLIC
        learnopengl::domain
)

target_compile_features(learnopengl_application
    PUBLIC
        cxx_std_17
)

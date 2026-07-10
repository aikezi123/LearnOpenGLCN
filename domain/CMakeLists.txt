add_library(learnopengl_domain STATIC)

add_library(learnopengl::domain ALIAS learnopengl_domain)

file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp"
)

file(GLOB_RECURSE DOMAIN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include/*.hpp"
)

file(GLOB_RECURSE DOMAIN_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

target_sources(learnopengl_domain
    PRIVATE
        ${DOMAIN_SOURCES}
        ${DOMAIN_HEADERS}
)

target_include_directories(learnopengl_domain
    PUBLIC
        ${DOMAIN_INCLUDE_DIRS}
)

target_compile_features(learnopengl_domain
    PUBLIC
        cxx_std_17
)

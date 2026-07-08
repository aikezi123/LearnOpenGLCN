add_library(learnopengl_domain INTERFACE)

add_library(learnopengl::domain ALIAS learnopengl_domain)

file(GLOB_RECURSE DOMAIN_INCLUDE_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*/include"
)

target_include_directories(learnopengl_domain
    INTERFACE
        ${DOMAIN_INCLUDE_DIRS}
)

target_compile_features(learnopengl_domain
    INTERFACE
        cxx_std_17
)

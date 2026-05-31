add_library(glfw3 STATIC IMPORTED GLOBAL)

set_target_properties(glfw3 PROPERTIES
    IMPORTED_LOCATION
        "${CMAKE_CURRENT_SOURCE_DIR}/lib-vc2022/glfw3.lib"

    INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

    INTERFACE_LINK_LIBRARIES
        "user32;gdi32;shell32"
)
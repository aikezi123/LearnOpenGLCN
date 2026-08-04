set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib-vc2022/glfw3.lib")

if(NOT EXISTS "${GLFW_LIBRARY}")
    message(FATAL_ERROR "GLFW library is missing: ${GLFW_LIBRARY}")
endif()

add_library(glfw3 STATIC IMPORTED GLOBAL)

set_target_properties(glfw3 PROPERTIES
    IMPORTED_LOCATION
        "${GLFW_LIBRARY}"

    INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

    INTERFACE_LINK_LIBRARIES
        "user32;gdi32;shell32"
)

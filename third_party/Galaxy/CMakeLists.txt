set(GALAXY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(GALAXY_VC_INCLUDE_DIR "${GALAXY_ROOT}/include/VC_SDK")
set(GALAXY_CPP_INCLUDE_DIR "${GALAXY_ROOT}/include/C++_SDK")
set(GALAXY_VC_LIB_DIR "${GALAXY_ROOT}/libs/VC_Lib")
set(GALAXY_CPP_LIB_DIR "${GALAXY_ROOT}/libs/VC++_Lib")
set(GALAXY_RUNTIME_DIR "${GALAXY_ROOT}/bin/x64")

# 大恒 SDK 是预编译库。这里做两件事：
# 1. 把 SDK 头文件和 .lib 包装成一个 CMake target：Galaxy::SDK。
# 2. 上层模块只链接 Galaxy::SDK，不直接写 include 路径或 .lib 路径。
set(GALAXY_REQUIRED_FILES
    "${GALAXY_VC_INCLUDE_DIR}/GxIAPI.h"
    "${GALAXY_VC_INCLUDE_DIR}/DxImageProc.h"
    "${GALAXY_CPP_INCLUDE_DIR}/GalaxyIncludes.h"
    "${GALAXY_VC_LIB_DIR}/GxIAPI.lib"
    "${GALAXY_VC_LIB_DIR}/DxImageProc.lib"
    "${GALAXY_CPP_LIB_DIR}/GxIAPICPPEx.lib"
)

foreach(_galaxy_file IN LISTS GALAXY_REQUIRED_FILES)
    if(NOT EXISTS "${_galaxy_file}")
        message(FATAL_ERROR "Galaxy SDK file is missing: ${_galaxy_file}")
    endif()
endforeach()

add_library(galaxy_sdk INTERFACE)
add_library(Galaxy::SDK ALIAS galaxy_sdk)

target_include_directories(galaxy_sdk
    INTERFACE
        "${GALAXY_VC_INCLUDE_DIR}"
        "${GALAXY_CPP_INCLUDE_DIR}"
)

target_link_libraries(galaxy_sdk
    INTERFACE
        "${GALAXY_VC_LIB_DIR}/GxIAPI.lib"
        "${GALAXY_VC_LIB_DIR}/DxImageProc.lib"
        "${GALAXY_CPP_LIB_DIR}/GxIAPICPPEx.lib"
)

target_compile_definitions(galaxy_sdk
    INTERFACE
        LEARNOPENGL_GALAXY_HAS_CPP_SDK=1
)

# .lib 只负责链接，运行时仍需要 DLL。
# 若 DLL 放在 third_party/Galaxy/bin/x64，会在构建 LearnOpenGLCN_Qt 后复制到 exe 目录；
# 若 DLL 已经在系统 PATH 中，也可以不放到本仓库。
file(GLOB GALAXY_RUNTIME_DLLS CONFIGURE_DEPENDS
    "${GALAXY_RUNTIME_DIR}/*.dll"
)

function(galaxy_copy_runtime_dlls target_name)
    if(GALAXY_RUNTIME_DLLS)
        add_custom_command(TARGET "${target_name}" POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${GALAXY_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:${target_name}>
            COMMENT "Copy Galaxy runtime DLLs for ${target_name}"
        )
    endif()
endfunction()

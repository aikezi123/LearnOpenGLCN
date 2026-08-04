# 当前文件由顶层 CMakeLists.txt 通过 add_subdirectory(third_party/Galaxy) 引入。
# CMAKE_CURRENT_SOURCE_DIR 表示“当前这个 CMakeLists.txt 所在目录”，也就是 third_party/Galaxy。
# 这样 SDK 路径始终相对于仓库，不依赖某台电脑上的绝对安装路径。
set(GALAXY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# 大恒 Galaxy SDK 当前使用两套头文件和一套 C++ import library：
# - VC_SDK：C++ 头文件间接包含的基础类型与图像处理声明。
# - C++_SDK / VC++_Lib：当前适配器直接使用的 C++ 封装和 GxIAPICPPEx.lib。
# 这里先把路径集中成变量，下面检查文件、设置 include、设置 link 时复用，避免同一目录写很多遍。
set(GALAXY_VC_INCLUDE_DIR "${GALAXY_ROOT}/include/VC_SDK")
set(GALAXY_CPP_INCLUDE_DIR "${GALAXY_ROOT}/include/C++_SDK")
set(GALAXY_CPP_LIB_DIR "${GALAXY_ROOT}/libs/VC++_Lib")
set(GALAXY_RUNTIME_DIR "${GALAXY_ROOT}/bin/Win64")

set(GALAXY_RUNTIME_FILES
    "${GALAXY_RUNTIME_DIR}/GxIAPICPPEx.dll"
    "${GALAXY_RUNTIME_DIR}/DxImageProc.dll"
    "${GALAXY_RUNTIME_DIR}/GCBase_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/GenApi_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/Log_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/MathParser_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/NodeMapData_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/XmlParser_MD_VC120_v3_0.dll"
    "${GALAXY_RUNTIME_DIR}/msvcp100.dll"
    "${GALAXY_RUNTIME_DIR}/msvcr100.dll"
    "${GALAXY_RUNTIME_DIR}/vcomp100.dll"
    "${GALAXY_RUNTIME_DIR}/msvcp120.dll"
    "${GALAXY_RUNTIME_DIR}/msvcr120.dll"
)

# 这些是当前接入大恒 SDK 必须存在的文件。
# set(...) 在这里创建一个 CMake 列表变量 GALAXY_REQUIRED_FILES。
# 先显式列出来有两个好处：
# - 配错目录或漏提交 SDK 文件时，CMake 配置阶段立刻报错。
# - 后面真正 target_link_libraries(...) 时不用等到编译/链接阶段才发现 .lib 不存在。
set(GALAXY_REQUIRED_FILES
    "${GALAXY_VC_INCLUDE_DIR}/GxIAPI.h"
    "${GALAXY_VC_INCLUDE_DIR}/DxImageProc.h"
    "${GALAXY_CPP_INCLUDE_DIR}/GalaxyIncludes.h"
    "${GALAXY_CPP_LIB_DIR}/GxIAPICPPEx.lib"
    ${GALAXY_RUNTIME_FILES}
)

# foreach(_galaxy_file IN LISTS GALAXY_REQUIRED_FILES)
# 会逐个取出 GALAXY_REQUIRED_FILES 列表里的路径，临时变量名叫 _galaxy_file。
# if(NOT EXISTS "...") 用来判断这个头文件或 .lib 是否真的存在。
# message(FATAL_ERROR ...) 会让 CMake 配置直接失败，并打印缺失文件的完整路径。
# 这段不是链接必需语法，但对第三方 SDK 很有用：它把“目录放错/文件漏了”的问题提前暴露出来。
foreach(_galaxy_file IN LISTS GALAXY_REQUIRED_FILES)
    if(NOT EXISTS "${_galaxy_file}")
        message(FATAL_ERROR "Galaxy SDK file is missing: ${_galaxy_file}")
    endif()
endforeach()

# 大恒 SDK 是已经编译好的第三方库，我们不会在本项目里编译它的源码。
# add_library(galaxy_sdk INTERFACE) 创建的是“接口库”：
# - 它本身不生成 .lib/.dll/.obj。
# - 它只携带使用 SDK 所需的 include 路径、链接库路径、编译宏等“使用要求”。
# - 其他 target 只要 target_link_libraries(... Galaxy::SDK)，这些使用要求就会传过去。
#
# 和其他 add_library 关键字的区别：
# - STATIC：编译本项目源码并生成静态库 .lib。
# - SHARED：编译本项目源码并生成动态库 .dll/.lib。
# - MODULE：生成插件式动态库，通常不作为普通链接依赖使用。
# - OBJECT：只编译成 .obj，供其他 target 复用。
# - INTERFACE：不编译源码，只表达依赖和使用方式。预编译 SDK 包装最适合用它。
add_library(galaxy_sdk INTERFACE)

# ALIAS 给内部 target galaxy_sdk 起一个命名空间别名 Galaxy::SDK。
# 这样上层写 target_link_libraries(... Galaxy::SDK) 更像链接 Qt6::Widgets、OpenGL::GL 这类第三方 target。
# 也能避免上层误以为 galaxy_sdk 是本项目普通源码库。
add_library(Galaxy::SDK ALIAS galaxy_sdk)

# target_include_directories 给 galaxy_sdk 添加头文件搜索路径。
# INTERFACE 的意思是：这些 include 路径不是给 galaxy_sdk 自己用的，因为它自己没有源码；
# 它们会传递给所有链接 Galaxy::SDK 的 target。
#
# 常见作用域区别：
# - PRIVATE：只给当前 target 自己编译用，不传给依赖者。
# - PUBLIC：当前 target 自己用，也传给依赖者。
# - INTERFACE：当前 target 自己不用，只传给依赖者。
target_include_directories(galaxy_sdk
    INTERFACE
        "${GALAXY_VC_INCLUDE_DIR}"
        "${GALAXY_CPP_INCLUDE_DIR}"
)

# target_link_libraries 给 galaxy_sdk 添加实际需要链接的 .lib。
# 这里同样使用 INTERFACE，因为 galaxy_sdk 自己不参与链接；
# 真正需要这些 .lib 的是链接 Galaxy::SDK 的上层 target，例如 learnopengl_ui。
#
# 注意：.lib 只负责链接期解析符号。运行时仍然需要对应 .dll。
# 当前项目约定是把大恒运行时 .dll 直接放在 out/build/<preset>/bin，与 exe 同目录。
target_link_libraries(galaxy_sdk
    INTERFACE
        "${GALAXY_CPP_LIB_DIR}/GxIAPICPPEx.lib"
)

# Export the runtime files to the parent directory so executable targets can
# deploy the SDK beside the generated .exe without relying on a machine-wide PATH.
set(GALAXY_RUNTIME_FILES "${GALAXY_RUNTIME_FILES}" PARENT_SCOPE)

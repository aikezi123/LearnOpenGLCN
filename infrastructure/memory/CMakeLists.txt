# ObjectPool 是模板，采用 header-only target。
add_library(engineeringlab_object_pool INTERFACE)
add_library(englab::object_pool ALIAS engineeringlab_object_pool)

target_sources(engineeringlab_object_pool
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/object_pool/ObjectPool.h
)

target_include_directories(engineeringlab_object_pool
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(engineeringlab_object_pool
    INTERFACE
        cxx_std_17
)

# 等开始 MemoryPool 阶段后再加入：
#
# add_library(engineeringlab_memory_pool STATIC
#     include/memory_pool/MemoryPool.h
#     src/memory_pool/MemoryPool.cpp
# )
# add_library(englab::memory_pool ALIAS engineeringlab_memory_pool)
#
# target_include_directories(engineeringlab_memory_pool
#     PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
# )
#
# target_compile_features(engineeringlab_memory_pool
#     PUBLIC
#         cxx_std_17
# )
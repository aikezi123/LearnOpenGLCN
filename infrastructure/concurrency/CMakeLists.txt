find_package(Threads REQUIRED)

add_library(learnopengl_concurrency STATIC
    include/thread_pool/ThreadPool.h
    src/ThreadPool.cpp
)

add_library(learnopengl::concurrency ALIAS learnopengl_concurrency)

target_include_directories(learnopengl_concurrency
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(learnopengl_concurrency
    PRIVATE
        Threads::Threads
)

target_compile_features(learnopengl_concurrency
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_options(learnopengl_concurrency
        PRIVATE
            /EHsc
    )
endif()

find_package(Threads REQUIRED)

add_library(engineeringlab_concurrency STATIC
    include/thread_pool/ThreadPool.h
    src/ThreadPool.cpp
)

add_library(englab::concurrency ALIAS engineeringlab_concurrency)

target_include_directories(engineeringlab_concurrency
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engineeringlab_concurrency
    PRIVATE
        Threads::Threads
)

target_compile_features(engineeringlab_concurrency
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_options(engineeringlab_concurrency
        PRIVATE
            /EHsc
    )
endif()

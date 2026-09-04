# 测试目录只负责组织测试 target，不向生产模块传播任何依赖。

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineeringLabAddTest.cmake")

add_library(engineeringlab_test_support INTERFACE)
add_library(englab::test_support ALIAS engineeringlab_test_support)

target_include_directories(engineeringlab_test_support
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_link_libraries(engineeringlab_test_support
    INTERFACE
        englab::application
)

# 轨迹和相机目前只有可编译测试骨架，暂不注册具体 TEST 用例。
engineeringlab_add_gtest(engineeringlab_domain_trajectory_tests
    SOURCES
        domain/trajectory/ArchimedeanSpiral2DGeneratorTest.cpp
    LIBRARIES
        englab::domain
    LABELS
        unit
        domain
)

engineeringlab_add_gtest(engineeringlab_application_camera_tests
    SOURCES
        application/camera/CameraCaptureServiceTest.cpp
        support/camera/FakeCameraDevice.h
    LIBRARIES
        englab::application
        englab::test_support
    LABELS
        unit
        application
)

engineeringlab_add_gtest(engineeringlab_infrastructure_concurrency_tests
    SOURCES
        infrastructure/concurrency/ThreadPoolTest.cpp
    LIBRARIES
        englab::concurrency
    LABELS
        unit
        infrastructure
        concurrency
)

engineeringlab_add_gtest(engineeringlab_infrastructure_logging_tests
    SOURCES
        infrastructure/logging/SpdlogLoggerTest.cpp
    LIBRARIES
        englab::logging
    LABELS
        unit
        infrastructure
        logging
)

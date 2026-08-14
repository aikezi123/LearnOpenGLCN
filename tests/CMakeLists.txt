# 测试目录只负责组织测试 target，不向生产模块传播任何依赖。

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LearnOpenGLAddTest.cmake")

add_library(learnopengl_test_support INTERFACE)
add_library(learnopengl::test_support ALIAS learnopengl_test_support)

target_include_directories(learnopengl_test_support
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_link_libraries(learnopengl_test_support
    INTERFACE
        learnopengl::application
)

# 轨迹和相机目前只有可编译测试骨架，暂不注册具体 TEST 用例。
learnopengl_add_gtest(learnopengl_domain_trajectory_tests
    SOURCES
        domain/trajectory/ArchimedeanSpiral2DGeneratorTest.cpp
    LIBRARIES
        learnopengl::domain
    LABELS
        unit
        domain
)

learnopengl_add_gtest(learnopengl_application_camera_tests
    SOURCES
        application/camera/CameraCaptureServiceTest.cpp
        support/camera/FakeCameraDevice.h
    LIBRARIES
        learnopengl::application
        learnopengl::test_support
    LABELS
        unit
        application
)

learnopengl_add_gtest(learnopengl_infrastructure_concurrency_tests
    SOURCES
        infrastructure/concurrency/ThreadPoolTest.cpp
    LIBRARIES
        learnopengl::concurrency
    LABELS
        unit
        infrastructure
        concurrency
)

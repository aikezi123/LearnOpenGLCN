include_guard(GLOBAL)

include(CMakeParseArguments)
include(GoogleTest)

# 注册一个 GoogleTest 可执行文件，并让 CTest 自动发现其中的测试用例。
#
# 用法：
# engineeringlab_add_gtest(target_name
#     SOURCES
#         SomeModuleTest.cpp
#     LIBRARIES
#         englab::some_module
#     LABELS
#         unit
#         domain
#     USE_GMOCK
# )
function(engineeringlab_add_gtest target_name)
    set(options USE_GMOCK)
    set(one_value_arguments)
    set(multi_value_arguments SOURCES LIBRARIES LABELS)

    cmake_parse_arguments(
        ARG
        "${options}"
        "${one_value_arguments}"
        "${multi_value_arguments}"
        ${ARGN}
    )

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "engineeringlab_add_gtest(${target_name}) received unknown arguments: "
            "${ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR
            "engineeringlab_add_gtest(${target_name}) requires at least one source file"
        )
    endif()

    add_executable(${target_name})

    target_sources(${target_name}
        PRIVATE
            ${ARG_SOURCES}
    )

    if(ARG_USE_GMOCK)
        set(test_main_target GTest::gmock_main)
    else()
        set(test_main_target GTest::gtest_main)
    endif()

    target_link_libraries(${target_name}
        PRIVATE
            ${ARG_LIBRARIES}
            ${test_main_target}
    )

    target_compile_features(${target_name}
        PRIVATE
            cxx_std_17
    )

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        FOLDER "tests"
    )

    if(ARG_LABELS)
        # gtest_discover_tests() 的 PROPERTIES 参数按“属性/值”成对解析，
        # 因此标签列表中的分号必须转义为单个 LABELS 属性值。
        string(JOIN "\\;" test_labels ${ARG_LABELS})

        gtest_discover_tests(${target_name}
            TEST_PREFIX "${target_name}."
            DISCOVERY_MODE PRE_TEST
            PROPERTIES
                LABELS "${test_labels}"
        )
    else()
        gtest_discover_tests(${target_name}
            TEST_PREFIX "${target_name}."
            DISCOVERY_MODE PRE_TEST
        )
    endif()
endfunction()

include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.17.0
    OPTIONS
        "BUILD_GMOCK OFF"
        "INSTALL_GTEST OFF"
)

add_executable(native.tests
    bindings_generator_tests.cpp
    string_converter_tests.cpp
)

target_link_libraries(native.tests
    PRIVATE
        csbind23
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(native.tests)

add_subdirectory(e2e)

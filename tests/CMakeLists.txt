include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.17.0
    OPTIONS
        "BUILD_GMOCK OFF"
        "INSTALL_GTEST OFF"
)

enable_testing()

add_executable(csbind23_tests
    bindings_generator_tests.cpp
)

target_link_libraries(csbind23_tests
    PRIVATE
        csbind23::bindings_generator
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(csbind23_tests)

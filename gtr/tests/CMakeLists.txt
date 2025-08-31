cmake_minimum_required(VERSION 3.22)

project(gtr_tests CXX)


file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if (TEST_SOURCES STREQUAL "")
    message(FATAL_ERROR "No test sources found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()


add_executable(gtr_test_suite ${TEST_SOURCES})


target_include_directories(gtr_test_suite PRIVATE ${GTR_INCLUDE_DIRS})
target_compile_features(gtr_test_suite PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(gtr_test_suite PRIVATE /W4 /permissive-)
else()
    target_compile_options(gtr_test_suite PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link GoogleTest
if (TARGET GTest::gtest_main)
    target_link_libraries(gtr_test_suite PRIVATE GTest::gtest_main)
else()
    target_link_libraries(gtr_test_suite PRIVATE gtest gtest_main)
endif()

# pthread on non-Windows
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(gtr_test_suite PRIVATE Threads::Threads)
endif()

# ---------------------------------------------------------------
# CTest integration: discover individual GoogleTest cases
# ---------------------------------------------------------------
include(GoogleTest)
gtest_discover_tests(
        gtr_test_suite
        DISCOVERY_MODE PRE_TEST
        # Optional: set WORKING_DIRECTORY if your tests expect a specific cwd
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


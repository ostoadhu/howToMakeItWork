include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Every test_*.cpp in this folder is compiled into the test executable.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

if(NOT TEST_SOURCES)
    message(STATUS "No test_*.cpp files found in ${CMAKE_CURRENT_SOURCE_DIR}; skipping test target")
    return()
endif()

add_executable(the_app_AI_tests ${TEST_SOURCES})
target_link_libraries(the_app_AI_tests PRIVATE the_app_AI_core GTest::gtest_main)
target_compile_options(the_app_AI_tests PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(the_app_AI_tests)

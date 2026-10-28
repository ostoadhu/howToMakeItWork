cmake_minimum_required(VERSION 3.20)

project(the_app_AI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Everything except main.cpp lives in a library so it can be linked by the tests.
add_library(the_app_AI_core STATIC
    src/process.cpp
    src/browser.cpp
    src/screenshot.cpp
    src/session.cpp
    src/ocr.cpp
    src/input.cpp
)
target_include_directories(the_app_AI_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(the_app_AI_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(the_app_AI src/main.cpp)
target_link_libraries(the_app_AI PRIVATE the_app_AI_core)
target_compile_options(the_app_AI PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Warn at configure time if runtime tools are missing (they are invoked, not linked).
foreach(tool chromium xdotool import convert tesseract gimp)
    find_program(_tool_path ${tool})
    if(NOT _tool_path)
        message(WARNING "Runtime dependency '${tool}' not found in PATH")
    endif()
    unset(_tool_path CACHE)
endforeach()

install(TARGETS the_app_AI RUNTIME DESTINATION bin)

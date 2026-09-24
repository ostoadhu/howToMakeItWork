cmake_minimum_required(VERSION 3.20)

project(the_app_AI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(the_app_AI
    src/main.cpp
    src/process.cpp
    src/browser.cpp
    src/screenshot.cpp
    src/session.cpp
)

target_include_directories(the_app_AI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(the_app_AI PRIVATE -Wall -Wextra -Wpedantic)

# Warn at configure time if runtime tools are missing (they are invoked, not linked).
foreach(tool chromium xdotool import gimp)
    find_program(_tool_path ${tool})
    if(NOT _tool_path)
        message(WARNING "Runtime dependency '${tool}' not found in PATH")
    endif()
    unset(_tool_path CACHE)
endforeach()

install(TARGETS the_app_AI RUNTIME DESTINATION bin)

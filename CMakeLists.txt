cmake_minimum_required(VERSION 3.25)
project(file-manager VERSION 3.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IMGUI_UI_BUILD_SDL ON)
set(IMGUI_UI_BUILD_TESTS OFF)

if(NOT DEFINED OUTPUT_NAME OR OUTPUT_NAME STREQUAL "")
    set(OUTPUT_NAME "${PROJECT_NAME}")
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wdeprecated-copy-with-user-provided-copy)

find_package(OpenGL REQUIRED)
add_subdirectory(vendor/imgui-ui)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PUBLIC
    stdc++
    OpenGL::GL
    imgui-ui::sdl
)

target_compile_options(${PROJECT_NAME} PUBLIC
    -Wpedantic
    -Wunused-variable
    -Wunused-function
    -Wunused-lambda-capture

    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

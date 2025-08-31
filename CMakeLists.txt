cmake_minimum_required(VERSION 3.22)
project(chess)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/res"
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    COMMENT "Copying res/ → ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
)
include_directories(gtr)

# Optimizations
if (MSVC)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:/O2>  $<$<CONFIG:Release>:/O2>)
else ()
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-O3> $<$<CONFIG:Release>:-O3>)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-mpopcnt)
endif ()

add_subdirectory(imgui)

# Warnings
if (MSVC)
    add_compile_options(/W4 /EHsc)
else ()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif ()

add_subdirectory(gtr)
add_subdirectory(src)

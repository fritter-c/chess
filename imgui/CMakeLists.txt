add_subdirectory(glfw)

file(GLOB IMGUI_SOURCES
        "*.cpp"
        "*.h"
        "*.hpp"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
if (MSVC)
    target_compile_options(imgui PRIVATE $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:Release>:/O2>)
    target_compile_options(imgui PRIVATE /W4 /EHsc)
else ()
    target_compile_options(imgui PRIVATE $<$<CONFIG:RelWithDebInfo>:-O3>
            $<$<CONFIG:Release>:-O3>)
endif ()
target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

find_package(OpenGL REQUIRED)
target_link_libraries(imgui
        PUBLIC
        OpenGL::GL
)

target_link_libraries(imgui PUBLIC glfw)
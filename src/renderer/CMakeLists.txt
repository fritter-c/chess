file(GLOB RENDERER_SOURCES
    "*.cpp"
    "*.hpp"
    "*.h"
    "*.c"
)

add_library(renderer STATIC ${RENDERER_SOURCES})

if (MSVC)
    target_compile_options(renderer PRIVATE $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:MinSizeRel>:/O2>)
    target_compile_options(renderer PRIVATE /W4 /EHsc)
else ()
    target_compile_options(renderer PRIVATE $<$<CONFIG:RelWithDebInfo>:-O3>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:MinSizeRel>:-O3>)
endif ()

target_link_libraries(renderer PRIVATE raylib)
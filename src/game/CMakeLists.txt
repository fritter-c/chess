file(GLOB GAME_SOURCES
    "*.cpp"
    "*.hpp"
)
add_library(game STATIC ${GAME_SOURCES})

if (MSVC)
    target_compile_options(game PRIVATE $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:MinSizeRel>:/O2>)
    target_compile_options(game PRIVATE /W4 /EHsc)
else ()
    target_compile_options(game PRIVATE $<$<CONFIG:RelWithDebInfo>:-O3>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:MinSizeRel>:-O3>)
endif ()
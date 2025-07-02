add_subdirectory(third)
add_subdirectory(renderer)
add_subdirectory(game)
add_executable(chess main.cpp)
if (MSVC)
target_compile_options(chess PRIVATE $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:MinSizeRel>:/O2>)
    target_compile_options(chess PRIVATE /W4 /EHsc)
else ()
    target_compile_options(chess PRIVATE $<$<CONFIG:RelWithDebInfo>:-O3>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:MinSizeRel>:-O3>)
endif ()
target_link_libraries(chess PUBLIC renderer game)
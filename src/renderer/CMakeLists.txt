file(GLOB RENDERER_SOURCES
        "*.cpp"
        "*.h"
        "*.hpp"
)
add_library(renderer STATIC ${RENDERER_SOURCES})
target_link_libraries(renderer PUBLIC imgui miniaudio)


file(GLOB GAME_SOURCES
        "*.cpp"
        "*.hpp"
)
add_library(game STATIC ${GAME_SOURCES})
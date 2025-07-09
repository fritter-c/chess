#pragma once
#include "../game/game.hpp"
#include "visual_board.hpp"
#include "vector.hpp"
#include <string.hpp>
namespace renderer {
struct BoardPanel {
    VisualBoard chess_board{};
    game::Game chess_game{};
    bool debug_chess_board{false};
    void render();
    BoardPanel();
};
} // namespace renderer

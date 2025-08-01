#pragma once
#include "../game/game.hpp"
#include "visual_board.hpp"
namespace renderer {
struct BoardPanel {
    VisualBoard chess_board{};
    game::Game chess_game{};
    bool debug_chess_board{false};
    bool debug_plain_string{false};
    int32_t selected_magic_board{0};
    int32_t selected_square{0};
    void render();
    BoardPanel();
};
} // namespace renderer

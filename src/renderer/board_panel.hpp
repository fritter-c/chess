#pragma once
#include "../game/game.hpp"
#include "visual_board.hpp"
#include "vector.hpp"
#include <string.hpp>
namespace renderer {
struct BoardPanel {
    constexpr static int32_t ControlPanelWidth = 350;
    VisualBoard chess_board{};
    game::Game chess_game{};
    void render();
};
} // namespace renderer

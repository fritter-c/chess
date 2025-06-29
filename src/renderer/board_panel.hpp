#pragma once
#include "visual_board.hpp"
#include "../game/game.hpp"
namespace renderer {
    struct BoardPanel {
        constexpr static int32_t ControlPanelWidth = 350;
        VisualBoard chess_board{};
        game::Game chess_game{};
        BoardPanel();
        ~BoardPanel() = default;
        void render();
    };
}

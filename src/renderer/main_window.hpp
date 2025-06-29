#pragma once
#include "board_panel.hpp"
namespace renderer {

    struct MainWindow {
        BoardPanel board_panel;

        void render();
    };
}
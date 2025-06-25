#pragma once
#include "visual_board.hpp"
#include <cstdint>
#include <filesystem>
#include "../game/move.hpp"
#include "../utils/utils.hpp"
namespace renderer {
    struct MainPanel {
        constexpr static int32_t ControlPanelWidth = 350;
        int32_t width;
        int32_t height;
        int32_t control_panel_x;
        VisualBoard visual_board;
    };

    void
    main_panel_initialize(MainPanel *panel, const std::filesystem::path &path);

    void
    main_panel_resize(MainPanel* panel, int32_t w, int32_t h);

    void
    main_panel_draw(MainPanel *panel);
}
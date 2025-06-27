#pragma once
#include "visual_board.hpp"
#include <cstdint>
#include <filesystem>
#include "../game/move.hpp"
namespace renderer {
    struct MainPanel {
        constexpr static int32_t ControlPanelWidth = 350;
        int32_t width{};
        int32_t height{};
        int32_t control_panel_x{};
        VisualBoard visual_board{};
        int32_t list_scroll{};
        int32_t list_active_index{};
        utils::string_list moves_list{};
    };

    void
    main_panel_initialize(MainPanel *panel, const std::filesystem::path &path);

    void
    main_panel_resize(MainPanel* panel, int32_t w, int32_t h);

    void
    main_panel_draw(MainPanel *panel);

    void
    main_panel_push_move(MainPanel *panel, const game::AlgebraicMove &move);
}
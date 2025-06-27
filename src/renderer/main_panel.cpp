#include "main_panel.hpp"
#include "raygui.h"

namespace renderer {
    static void
    main_panel_draw_control_panel(MainPanel *panel) {
        const int32_t control_panel_x = panel->control_panel_x;
        const int32_t control_panel_width = panel->width - control_panel_x;
        const int32_t control_panel_height = panel->height;

        DrawRectangle(control_panel_x, 0, control_panel_width, control_panel_height, ColorAlpha(LIGHTGRAY, 0.8f));

        const Rectangle reset_button_rect = {static_cast<float>(control_panel_x + 14), 10, 100, 30};
        if (GuiButton(reset_button_rect, "Reset Board")) {
            visual_board_reset_pieces(&panel->visual_board);
            panel->moves_list.clear();
        }
        const Rectangle mode_button_rect = {static_cast<float>(control_panel_x + 124), 10, 100, 30};
        if (GuiButton(mode_button_rect, "Change Mode")) {
            // Placeholder for mode change logic
            // This could toggle between different game modes or settings
            TraceLog(LOG_INFO, "Change Mode button clicked");
        }
        const Rectangle flip_button_rect = {static_cast<float>(control_panel_x + 234), 10, 100, 30};
        if (GuiButton(flip_button_rect, "Flip Board")) {
            panel->visual_board.flipped = !panel->visual_board.flipped;
        }
    }

    static void
    main_panel_draw_moves_view(MainPanel *panel) {
        const Rectangle list_view_rect = {
            static_cast<float>(panel->control_panel_x + 10), 60,
            static_cast<float>(MainPanel::ControlPanelWidth - 20), static_cast<float>(panel->height - 200)
        };
        GuiListView(list_view_rect, panel->moves_list.c_str(), &panel->list_scroll, &panel->list_active_index);
    }

    void
    main_panel_initialize(MainPanel *panel, const std::filesystem::path &path) {
        visual_board_initialize(&panel->visual_board, panel);
        visual_board_load_resources(&panel->visual_board, path);
    }

    void
    main_panel_resize(MainPanel *panel, const int32_t w, const int32_t h) {
        panel->width = w;
        panel->height = h;
        panel->control_panel_x = w - MainPanel::ControlPanelWidth;
        visual_board_resize(&panel->visual_board, w - MainPanel::ControlPanelWidth, h);
    }

    void
    main_panel_draw(MainPanel *panel) {
        main_panel_draw_control_panel(panel);
        visual_board_draw(&panel->visual_board);
        main_panel_draw_moves_view(panel);
    }

    void
    main_panel_push_move(MainPanel *panel, const game::AlgebraicMove &move) {
        panel->moves_list.append(game::algebraic_move_to_string(move).c_str());
        if (panel->moves_list.items == 1) {
            panel->list_active_index = 0; // Set the first move as active
        } else {
            ++panel->list_active_index; // Increment the active index to the last move
        }
    }
} // namespace renderer

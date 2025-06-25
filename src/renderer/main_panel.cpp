#include "main_panel.hpp"
#include "../raygui.h"

namespace renderer {
    static void
    main_panel_draw_control_panel(MainPanel *panel) {
        const int32_t control_panel_x = panel->control_panel_x;
        const int32_t control_panel_width = panel->width - control_panel_x;
        const int32_t control_panel_height = panel->height;

        DrawRectangle(control_panel_x, 0, control_panel_width, control_panel_height, ColorAlpha(LIGHTGRAY, 0.8f));

        const Rectangle reset_button_rect = {static_cast<float>(control_panel_x + 10), 10, 100, 30};
        if (GuiButton(reset_button_rect, "Reset Board")) {
            visual_board_reset_pieces(&panel->visual_board);
        }
    }

    static void
    main_panel_draw_moves(MainPanel *panel) {
        for (auto  i = 0; i < panel->visual_board.moves.size; ++i) {
            const game::AlgebraicMove &move = panel->visual_board.moves.items[i];
            char move_str[32];
            game::algebraic_move_to_string(move, move_str, sizeof(move_str));
            const int32_t y_offset = 40 + i * 20;
            DrawText(move_str, panel->control_panel_x + 10, y_offset, 20, BLACK);
        }
    }

    void
    main_panel_initialize(MainPanel *panel, const std::filesystem::path &path) {
        std::memset(panel, 0, sizeof(MainPanel));
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
        main_panel_draw_moves(panel);
    }
} // namespace renderer

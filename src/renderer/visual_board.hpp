#pragma once
#include <cstdint>
#include "raylib.h"
#include "../game/board.hpp"
#include <filesystem>
#include "../utils/utils.hpp"
#include "../game/piece.hpp"

namespace renderer {
    struct MainPanel;
    struct VisualBoard {
        MainPanel *panel;
        int32_t offset_x;
        int32_t offset_y;
        int32_t window_size;
        int32_t size;
        int32_t cell_size;
        bool flipped;
        Texture2D piece_textures;
        Rectangle piece_original_rects[12];
        Font font_big;
        Font font_small;
        Sound move_sound;
        Sound check_sound;
        game::Board board;
        game::Piece dragging_piece;
        game::AvailableSquares dragging_piece_available_moves;
        uint8_t dragging_piece_row;
        uint8_t dragging_piece_col;
        int32_t last_moved_piece_dest_row;
        int32_t last_moved_piece_dest_col;
        int32_t last_moved_piece_org_row;
        int32_t last_moved_piece_org_col;
        bool has_moves;
    };

    void
    visual_board_initialize(VisualBoard* board, MainPanel *panel);

    void
    visual_board_resize(VisualBoard *board, int32_t w, int32_t h);

    void
    visual_board_draw(VisualBoard *board);

    void
    visual_board_load_resources(VisualBoard *board, const std::filesystem::path &path);

    void
    visual_board_reset_pieces(VisualBoard *board);
}

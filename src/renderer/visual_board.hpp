#pragma once
#include <cstdint>
#include "raylib.h"
#include "../game/board.hpp"
#include <filesystem>

namespace renderer {
    struct VisualBoard {
        int32_t offset_x;
        int32_t offset_y;
        int32_t window_size;
        int32_t size;
        int32_t cell_size;
        Texture2D piece_textures;
        Rectangle piece_original_rects[12];
        Font font_big;
        Font font_small;
        Sound capture_sound;
        game::Board board;
        game::ChessPiece dragging_piece;
        uint8_t dragging_piece_row;
        uint8_t dragging_piece_col;
        int32_t last_moved_piece_dest_row;
        int32_t last_moved_piece_dest_col;
        int32_t last_moved_piece_org_row;
        int32_t last_moved_piece_org_col;
        bool has_moves;
    };

    void
    visual_board_initialize(VisualBoard* board);

    void
    visual_board_resize(VisualBoard *board, int32_t w, int32_t h);

    void
    visual_board_draw(VisualBoard *board);

    void
    visual_board_load_resources(VisualBoard *board, const std::filesystem::path &path );
}

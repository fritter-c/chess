#pragma once
#include "board.hpp"
#include <cstdint>
namespace game
{
    bool
    analyzer_can_make_move(const Board *board, const int32_t from_row, const int32_t from_col,
                           const int32_t to_row, const int32_t to_col, bool &is_check,
                           bool &is_checkmate);

    BitBoard
    analyzer_get_available_moves_for_piece(const Board *board, const int32_t row, const int32_t col);

    bool
    analyzer_is_cell_under_attack_by_color(const Board *board, const int32_t row, const int32_t col,
                                           ChessPieceColor color);

    bool
    analyzer_is_color_in_check(const Board *board, ChessPieceColor color);
} // namespace game

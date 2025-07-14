#pragma once
#include <cstdint>
#include "board.hpp"

namespace game {
AvailableMoves analyzer_get_available_moves_for_piece(Board *board, int32_t row, int32_t col);

inline AvailableMoves analyzer_get_available_moves_for_piece(Board *board, const int32_t index) {
    return analyzer_get_available_moves_for_piece(board, Board::get_row(index), Board::get_col(index));
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, int32_t row, int32_t col, Color attacker);

bool analyzer_is_color_in_check(Board *board, Color color);

inline bool analyzer_can_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    return analyzer_get_available_moves_for_piece(board, Board::get_index(from_row, from_col)).get(Board::get_index(to_row, to_col));
}

bool analyzer_is_color_in_checkmate(Board *board, Color color);

Move analyzer_get_move_from_simple(Board *board, const SimpleMove &move, PromotionPieceType promotion_type = PROMOTION_QUEEN);

bool analyzer_move_puts_to_check(Board *board, const Move &move);

bool analyzer_move_puts_to_checkmate(Board *board, const Move &move);

bool analyzer_is_move_legal(Board *board, const Move &move);

int32_t analyzer_get_move_count(Board *board, Color color);

bool analyzer_get_is_stalemate(Board *board, Color friendly);

bool analyzer_is_insufficient_material(const Board *board);
} // namespace game

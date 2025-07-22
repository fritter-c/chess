#pragma once
#include <cstdint>
#include "board.hpp"
#include "move.hpp"
#include "bitboard.hpp"

namespace game {
AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(Board *board, int32_t row, int32_t col);

inline AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(Board *board, const int32_t index) {
    return analyzer_get_pseudo_legal_moves_for_piece(board, Board::get_row(index), Board::get_col(index));
}

AvailableMoves analyzer_filter_legal_moves(Board *board, AvailableMoves moves);

inline AvailableMoves analyzer_get_legal_moves_for_piece(Board *board, int32_t row, int32_t col) {
    return analyzer_filter_legal_moves(board, analyzer_get_pseudo_legal_moves_for_piece(board, row, col));
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, int32_t row, int32_t col, Color attacker);

bool analyzer_is_color_in_check(Board *board, Color color);

bool analyzer_is_color_in_checkmate(Board *board, Color color);

Move analyzer_get_move_from_simple(Board *board, const SimpleMove &move, PromotionPieceType promotion_type = PROMOTION_QUEEN);

bool analyzer_move_puts_to_check(Board *board, const Move &move);

bool analyzer_move_puts_to_checkmate(Board *board, const Move &move);

bool analyzer_is_move_legal(Board *board, const Move &move);

inline bool analyzer_is_move_legal(Board *board, const SimpleMove &move, PromotionPieceType promotion_type = PROMOTION_QUEEN) {
    return analyzer_is_move_legal(board, analyzer_get_move_from_simple(board, move, promotion_type));
}

int32_t analyzer_get_move_count(Board *board, Color color);

int32_t analyzer_get_legal_move_count(Board *board, Color color);

bool analyzer_get_is_stalemate(Board *board, Color friendly);

bool analyzer_is_insufficient_material(const Board *board);

void analyzer_init_magic_board();
} // namespace game

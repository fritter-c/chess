#pragma once
#include <cstdint>
#include "bitboard.hpp"
#include "board.hpp"
#include "move.hpp"

namespace game {
AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(const Board *board, int32_t row, int32_t col);

inline AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(const Board *board, const int32_t index) {
    return analyzer_get_pseudo_legal_moves_for_piece(board, Board::get_row(index), Board::get_col(index));
}

AvailableMoves analyzer_filter_legal_moves(Board *board, AvailableMoves moves);

inline AvailableMoves analyzer_get_legal_moves_for_piece(Board *board,const int32_t row,const int32_t col) {
    return analyzer_filter_legal_moves(board, analyzer_get_pseudo_legal_moves_for_piece(board, row, col));
}

inline AvailableMoves analyzer_get_legal_moves_for_piece(Board *board, const int32_t index) {
    return analyzer_get_legal_moves_for_piece(board, Board::get_row(index), Board::get_col(index));
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, int32_t row, int32_t col, Color attacker);

bool analyzer_is_color_in_check(Board *board, Color color);

bool analyzer_is_color_in_checkmate(Board *board, Color color);

Move analyzer_get_move_from_simple(const Board *board, const SimpleMove &move, PromotionPieceType promotion_type = PROMOTION_QUEEN);

bool analyzer_move_puts_to_check(Board *board, const Move &move);

bool analyzer_move_puts_to_checkmate(Board *board, const Move &move);

int32_t analyzer_get_legal_move_count(Board *board, Color color);

bool analyzer_get_is_stalemate(Board *board, Color friendly);

bool analyzer_is_insufficient_material(const Board *board);

bool analyzer_is_queen_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_pawn_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_knight_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_king_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_rook_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_bishop_attacking(const Board *board, SquareIndex index, Color attacker); /** any */

bool analyzer_is_queen_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

bool analyzer_is_pawn_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

bool analyzer_is_knight_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

bool analyzer_is_king_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

bool analyzer_is_rook_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

bool analyzer_is_bishop_attacking(const Board *board, SquareIndex index, Color attacker, SquareIndex origin); /** at origin */

} // namespace game

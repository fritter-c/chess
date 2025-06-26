#pragma once
#include "board.hpp"
#include <cstdint>

namespace game {
    AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board, int32_t row, int32_t col);

    inline AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board,const int32_t index) {
        return analyzer_get_available_moves_for_piece(board, board_get_row(index), board_get_col(index));
    }
    bool
    analyzer_is_cell_under_attack_by_color(const Board *board, int32_t row, int32_t col,
                                           ChessPieceColor attacker);

    bool
    analyzer_is_color_in_check(const Board *board, ChessPieceColor color);

    bool
    analyzer_is_color_in_checkmate(const Board *board, ChessPieceColor color);

    AlgebraicMove
    analyzer_get_algebraic_move(const Board *board, const SimpleMove &move);

    AlgebraicMove
    analyzer_algebraic_move_from_str(const Board *board, const char *str, ChessPieceColor turn);

    Square
    analyzer_where(const Board *board, ChessPieceType type, ChessPieceColor color, int32_t disambiguation_col = -1,
                   int32_t disambiguation_row = -1);
} // namespace game

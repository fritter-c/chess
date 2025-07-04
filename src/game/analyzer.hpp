#pragma once
#include "board.hpp"
#include <cstdint>

namespace game {
    AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board, int32_t row, int32_t col);

    inline AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board, const int32_t index) {
        return analyzer_get_available_moves_for_piece(board, Board::board_get_row(index), Board::board_get_col(index));
    }

    bool
    analyzer_is_cell_under_attack_by_color(const Board *board, int32_t row, int32_t col,
                                           Color attacker);

    bool
    analyzer_is_color_in_check(const Board *board, Color color);

    inline bool
    analyzer_can_move(const Board *board,const int32_t from_row,const int32_t from_col,const int32_t to_row,const int32_t to_col)
    {
        return analyzer_get_available_moves_for_piece(board, Board::board_get_index(from_row, from_col)).get(Board::board_get_index(to_row, to_col));
    }

    bool
    analyzer_is_color_in_checkmate(const Board *board, Color color);

    AlgebraicMove
    analyzer_get_algebraic_move(const Board *board, const SimpleMove &move);

    Move
    analyzer_get_move_from_simple(const Board *board, const SimpleMove &move,
                                  PromotionPieceType promotion_type = PROMOTION_QUEEN);

    Square
    analyzer_where(const Board *board, PieceType type, Color color, int32_t disambiguation_col = -1,
                   int32_t disambiguation_row = -1);

    int32_t
    analyzer_get_move_count(const Board *board, Color color);

    bool 
    analyzer_get_is_stalemate(const Board *board, Color friendly);

    bool 
    analyzer_is_insufficient_material(const Board* board);
} // namespace game

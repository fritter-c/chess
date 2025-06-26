#pragma once
#include <cstdint>
#include "piece.hpp"
namespace game
{
    struct SimpleMove
    {
        uint8_t from_row : 4;
        uint8_t from_col : 4;
        uint8_t to_row : 4;
        uint8_t to_col : 4;
    };

    struct AlgebraicMove
    {
        ChessPieceType piece_type : 3;
        uint8_t from_col : 3;
        uint8_t to_col : 3;
        uint8_t from_row : 3;
        uint8_t to_row : 3;
        uint8_t is_capture : 1;
        uint8_t is_check : 1;
        uint8_t is_checkmate : 1;
        uint8_t need_row_disambiguation : 1;
        uint8_t need_col_disambiguation : 1;
        uint8_t en_passant : 1;
        uint8_t kingside_castle : 1;
        uint8_t queen_side_castle : 1;
        uint8_t promotion : 1;
    };

    void
    algebraic_move_to_string(const AlgebraicMove &move, char *buffer, size_t buffer_size);
}
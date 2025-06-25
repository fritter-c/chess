#pragma once
#include <cstdint>
#include "piece.hpp"
#include <stddef.h>
namespace game
{
    struct SimpleMove
    {
        uint8_t from_row : 3;
        uint8_t from_col : 3;
        uint8_t to_row : 3;
        uint8_t to_col : 3;
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
    };

    void
    algebraic_move_to_string(const AlgebraicMove &move, char *buffer, size_t buffer_size);

    AlgebraicMove
    algebraic_move_from_string(char *str);
}
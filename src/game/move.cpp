#include "move.hpp"
#include <cstring>

namespace game {
    void
    algebraic_move_to_string(const AlgebraicMove &move, char *buffer, size_t buffer_size) {
        using enum ChessPieceType;

        size_t buffer_index{0};
        auto buffer_push = [&buffer_index, buffer_size](char *buf, char c) {
            if (buffer_index < buffer_size - 1) {
                buf[buffer_index++] = c;
            }
        };

        if (move.kingside_castle) {
            buffer_push(buffer, 'O');
            buffer_push(buffer, '-');
            buffer_push(buffer, 'O');
            goto end;
        }

        if (move.queen_side_castle) {
            buffer_push(buffer, 'O');
            buffer_push(buffer, '-');
            buffer_push(buffer, 'O');
            buffer_push(buffer, '-');
            buffer_push(buffer, 'O');
            goto end;
        }

        if (move.piece_type != PAWN) {
            buffer_push(buffer, chess_piece_to_algebraic_letter(move.piece_type));
            if (move.need_col_disambiguation) {
                buffer_push(buffer, static_cast<char>('a' + move.from_col));
            }
            if (move.need_row_disambiguation) {
                buffer_push(buffer, static_cast<char>('1' + move.from_row)); // '1' for row 0, ..., '8' for row 7
            }
        }

        if (move.is_capture) {
            if (move.piece_type == PAWN) {
                buffer_push(buffer, static_cast<char>('a' + move.from_col));
            }
            buffer_push(buffer, 'x');
        }

        buffer_push(buffer, static_cast<char>('a' + move.to_col));
        buffer_push(buffer, static_cast<char>('1' + move.to_row));

        if (move.is_checkmate)
            buffer_push(buffer, '#');
        else if (move.is_check)
            buffer_push(buffer, '+');

    end:
        buffer_push(buffer, '\0');
    }

}

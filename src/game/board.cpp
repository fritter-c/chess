#include "board.hpp"
#include <cstring>
#include <utility>

namespace game {
    void
    board_populate(Board *board) {
        std::memcpy(board->pieces, StartingPosition, sizeof(board->pieces));
    }

    static bool
    board_can_move_basic(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                         const int32_t to_col) {

        // Check if the move is to another cell
        if (from_row == to_row && from_col == to_col) {
            return false; // No move
        }

        // Check if the target cell is empty or occupied by an opponent's piece
        const int32_t from_index = board_get_index(from_row, from_col);
        const int32_t to_index = board_get_index(to_row, to_col);
        const ChessPiece from_piece = board->pieces[from_index];
        if (const ChessPiece to_piece = board->pieces[to_index]; to_piece.type != ChessPieceType::NONE &&
            to_piece.color == from_piece.color) {
            return false; // Cannot capture own piece
        }

        if (from_piece.type == ChessPieceType::NONE) {
            return false; // No piece to move
        }

        return true;
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
               const int32_t to_col) {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col)) {
            board->pieces[board_get_index(to_row, to_col)] = board->pieces[board_get_index(from_row, from_col)];
            board->pieces[board_get_index(from_row, from_col)] = None;
            return true; // Move successful
        }
        return false;
    }
}

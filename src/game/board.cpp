#include "board.hpp"
#include <cstring>
#include <utility>

namespace game
{
    void
    board_populate(Board *board)
    {
        std::memcpy(board->pieces, StartingPosition, sizeof(board->pieces));
    }

    static bool
    board_can_move_basic(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                         const int32_t to_col)
    {

        // Check if the move is to another cell
        if (from_row == to_row && from_col == to_col)
        {
            return false; // No move
        }

        // Check if the target cell is empty or occupied by an opponent's piece
        const int32_t from_index = board_get_index(from_row, from_col);
        const int32_t to_index = board_get_index(to_row, to_col);
        const ChessPiece from_piece = board->pieces[from_index];
        if (const ChessPiece to_piece = board->pieces[to_index]; to_piece.type != ChessPieceType::NONE &&
                                                                 to_piece.color == from_piece.color)
        {
            return false; // Cannot capture own piece
        }

        if (from_piece.type == ChessPieceType::NONE)
        {
            return false; // No piece to move
        }

        return true;
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
               const int32_t to_col)
    {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col))
        {
            board->pieces[board_get_index(from_row, from_col)].moved = 1;
            board->pieces[board_get_index(to_row, to_col)] = board->pieces[board_get_index(from_row, from_col)];
            board->pieces[board_get_index(from_row, from_col)] = None;
            return true;
        }
        return false;
    }

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col, AlgebraicMove &out_alg)
    {
        SimpleMove move;
        move.from_col = static_cast<uint8_t>(from_col);
        move.from_row = static_cast<uint8_t>(from_row);
        move.to_col = static_cast<uint8_t>(to_col);
        move.to_row = static_cast<uint8_t>(to_row);
        out_alg = board_get_algebraic_move(board, move);
        if (board_move(board, from_row, from_col, to_row, to_col))
        {
            return true;
        }
        return false;
    }

    AlgebraicMove
    board_get_algebraic_move(const Board *board, const SimpleMove &move)
    {
        AlgebraicMove alg{};
        int fromIdx = board_get_index(move.from_row, move.from_col);
        int toIdx = board_get_index(move.to_row, move.to_col);

        ChessPiece moving = board->pieces[fromIdx];
        ChessPiece target = board->pieces[toIdx];

        alg.piece_type = moving.type;

        alg.from_row = uint8_t(8 - move.from_row);

        alg.to_row = uint8_t(8 - move.to_row);

        alg.is_capture = (target.type != ChessPieceType::NONE);

        // 5) stub out checks/checkmates for now
        alg.is_check = false;
        alg.is_checkmate = false;

        return alg;
    }
}

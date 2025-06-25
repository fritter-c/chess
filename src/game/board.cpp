#include "board.hpp"
#include <cstring>
#include <immintrin.h>
#include <bit>
#include <cstdio>
#include "analyzer.hpp"
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

    static void
    board_reset_first_move(Board *board)
    {
        for (int32_t i = 0; i < 64; ++i)
        {
            board->pieces[i].first_move_was_last_turn = 0;
        }
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
               const int32_t to_col)
    {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col))
        {
            ChessPiece &from_piece = board->pieces[board_get_index(from_row, from_col)];
            board_reset_first_move(board);
            if (from_piece.moved == 0)
            {
                from_piece.first_move_was_last_turn = 1;
            }
            from_piece.moved = 1;
            board->pieces[board_get_index(to_row, to_col)] = from_piece;
            board->pieces[board_get_index(from_row, from_col)] = None;
            return true;
        }
        return false;
    }

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col, int32_t last_moved_index)
    {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col))
        {
            ChessPiece &from_piece = board->pieces[board_get_index(from_row, from_col)];
            board->pieces[last_moved_index].first_move_was_last_turn = 0;
            if (from_piece.moved == 0)
            {
                from_piece.first_move_was_last_turn = 1;
            }
            from_piece.moved = 1;
            board->pieces[board_get_index(to_row, to_col)] = from_piece;
            board->pieces[board_get_index(from_row, from_col)] = None;
            return true;
        }
        return false;
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col,
               AlgebraicMove &out_alg)
    {
        const SimpleMove move{
            static_cast<uint8_t>(from_row),
            static_cast<uint8_t>(from_col),
            static_cast<uint8_t>(to_row),
            static_cast<uint8_t>(to_col)};
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
        Board board_copy = *board;
        const int32_t fromIdx = board_get_index(move.from_row, move.from_col);
        const int32_t toIdx = board_get_index(move.to_row, move.to_col);

        const ChessPiece moving = board->pieces[fromIdx];
        const ChessPiece target = board->pieces[toIdx];

        alg.piece_type = moving.type;

        alg.from_row = static_cast<uint8_t>(8 - move.from_row);

        alg.to_row = static_cast<uint8_t>(8 - move.to_row);

        alg.from_col = static_cast<uint8_t>(move.from_col);
        alg.to_col = static_cast<uint8_t>(move.to_col);

        alg.is_capture = (target.type != ChessPieceType::NONE);

        board_move(&board_copy, move);

        // 5) stub out checks/checkmates for now
        alg.is_checkmate = analyzer_is_color_in_checkmate(&board_copy, target.color);
        alg.is_check = !alg.is_checkmate && analyzer_is_color_in_check(&board_copy, target.color);

        return alg;
    }
}

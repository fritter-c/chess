#include "board.hpp"
#include <cmath>
#include <cstring>
#include "types.hpp"

namespace game
{
    void
    Board::board_populate()
    {
        std::memcpy(pieces.data(), StartingPosition.data(), sizeof(pieces));
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
        const int32_t from_index = Board::board_get_index(from_row, from_col);
        const int32_t to_index = Board::board_get_index(to_row, to_col);
        const Piece from_piece = board->pieces[from_index];
        if (const Piece to_piece = board->pieces[to_index]; PIECE_TYPE(to_piece) != EMPTY &&
                                                            PIECE_COLOR(to_piece) == PIECE_COLOR(from_piece))
        {
            return false; // Cannot capture own piece
        }

        if (PIECE_TYPE(from_piece) == EMPTY)
        {
            return false; // No piece to move
        }

        return true;
    }

    static bool
    board_can_move_basic(const Board *board, uint8_t from_index, uint8_t to_index)
    {
        const int32_t from_row = Board::board_get_row(from_index);
        const int32_t from_col = Board::board_get_col(from_index);
        const int32_t to_row = Board::board_get_row(to_index);
        const int32_t to_col = Board::board_get_col(to_index);
        return board_can_move_basic(board, from_row, from_col, to_row, to_col);
    }

    static bool
    board_is_castle(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                    const int32_t to_col)
    {
        (void)to_row;
        if (const Piece from_piece = board->pieces[Board::board_get_index(from_row, from_col)];
            PIECE_TYPE(from_piece) == PieceType::KING && std::abs(from_col - to_col) > 1)
        {
            return true;
        }
        return false;
    }

    bool
    Board::board_is_en_passant(const int32_t from_row, const int32_t from_col, const int32_t to_row,
                        const int32_t to_col) const
    {
        (void)to_row;
        if (const Piece to_piece = pieces[Board::board_get_index(to_row, to_col)];
            PIECE_TYPE(to_piece) != PieceType::EMPTY)
        {
            return false;
        }
        if (const Piece from_piece = pieces[Board::board_get_index(from_row, from_col)];
            PIECE_TYPE(from_piece) == PieceType::PAWN && from_col != to_col)
        {
            return true;
        }
        return false;
    }

    static void
    board_do_castle(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                    const int32_t to_col)
    {
        Piece const &king = board->pieces[Board::board_get_index(from_row, from_col)];
        Piece &rook = board->pieces[Board::board_get_index(from_row, from_col - to_col > 0 ? 0 : 7)];

        board->pieces[Board::board_get_index(from_row, from_col - to_col > 0 ? 3 : 5)] = rook;
        rook = PIECE_NONE; // Remove the rook from the original position

        board->pieces[Board::board_get_index(to_row, to_col)] = king;
        board->pieces[Board::board_get_index(from_row, from_col)] = PIECE_NONE; // Remove the king from the original position
    }

    static void
    board_do_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                  const int32_t to_col)
    {
        board->en_passant_index = -1; // Reset en passant index
        Piece const &from_piece = board->pieces[Board::board_get_index(from_row, from_col)];
        if (PIECE_TYPE(from_piece) == PAWN && std::abs(from_row - to_row) == 2)
        {
            board->en_passant_index = static_cast<int8_t>(Board::board_get_index(to_row, to_col));
        }

        if (PIECE_TYPE(from_piece) == KING)
        {
            board->castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_ALL : ~CASTLE_WHITE_ALL;
        }
        // Needs to verify the row in case of extra rooks
        else if (PIECE_TYPE(from_piece) == ROOK)
        {
            if (from_col == 0)
            {
                board->castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_QUEENSIDE : ~CASTLE_WHITE_QUEENSIDE;
            }
            else if (from_col == 7)
            {
                board->castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_KINGSIDE : ~CASTLE_WHITE_KINGSIDE;
            }
        }

        if (board->board_is_en_passant(from_row, from_col, to_row, to_col))
        {
            // Remove the captured pawn
            const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? to_row - 1 : to_row + 1;
            const int32_t captured_col = to_col;
            board->pieces[Board::board_get_index(captured_row, captured_col)] = PIECE_NONE;
        }
        board->pieces[Board::board_get_index(to_row, to_col)] = from_piece;
        board->pieces[Board::board_get_index(from_row, from_col)] = PIECE_NONE;
    }


    void
    Board::board_move_no_check(const int32_t from_row, const int32_t from_col, const int32_t to_row,
                               const int32_t to_col)
    {
        if (board_is_castle(this, from_row, from_col, to_row, to_col))
        {
            board_do_castle(this, from_row, from_col, to_row, to_col);
        }
        else
        {
            board_do_move(this, from_row, from_col, to_row, to_col);
        }
    }


    bool
    Board::board_move(Move move)
    {
        if (board_can_move_basic(this, move.get_origin(), move.get_destination()))
        {
            const int32_t from_row = board_get_row(move.get_origin());
            const int32_t from_col = board_get_col(move.get_origin());
            const int32_t to_row = board_get_row(move.get_destination());
            const int32_t to_col = board_get_col(move.get_destination());
            if (move.get_special() == Move::MOVE_CASTLE)
            {
                board_do_castle(this, from_row, from_col, to_row, to_col);
            }
            else if (move.get_special() == Move::MOVE_PROMOTION)
            {
                if (move.get_special() == Move::MOVE_PROMOTION)
                {
                    pieces[board_get_index(to_row, to_col)] = chess_piece_make(
                        move.get_promotion_piece_type(), PIECE_COLOR(pieces[board_get_index(from_row, from_col)]));

                    // Remove the pawn from the original position
                    pieces[board_get_index(from_row, from_col)] = PIECE_NONE;
                }
            }
            else
            {
                board_do_move(this, from_row, from_col, to_row, to_col);
            }
            return true;
        }
        return false;
    }
}

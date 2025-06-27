#include "board.hpp"
#include <cmath>
#include <cstring>
#include "analyzer.hpp"
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
        if (const Piece to_piece = board->pieces[to_index]; PIECE_TYPE(to_piece) != PieceType::NONE &&
                                                            PIECE_COLOR(to_piece) == PIECE_COLOR(from_piece))
        {
            return false; // Cannot capture own piece
        }

        if (PIECE_TYPE(from_piece) == PieceType::NONE)
        {
            return false; // No piece to move
        }

        return true;
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

    static bool
    board_is_en_passant(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                        const int32_t to_col)
    {
        (void)to_row;
        if (const Piece to_piece = board->pieces[Board::board_get_index(to_row, to_col)];
            PIECE_TYPE(to_piece) != PieceType::NONE)
        {
            return false;
        }
        if (const Piece from_piece = board->pieces[Board::board_get_index(from_row, from_col)];
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
        if (PIECE_TYPE(from_piece) == PieceType::PAWN && std::abs(from_row - to_row) == 2)
        {
            board->en_passant_index = static_cast<int8_t>(Board::board_get_index(to_row, to_col));
        }

        if (PIECE_TYPE(from_piece) == PieceType::KING)
        {
            board->castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_ALL : ~CASTLE_WHITE_ALL;
        }
        // Needs to verify the row in case of extra rooks
        else if (PIECE_TYPE(from_piece) == PieceType::ROOK)
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

        if (board_is_en_passant(board, from_row, from_col, to_row, to_col))
        {
            // Remove the captured pawn
            const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? to_row - 1 : to_row + 1;
            const int32_t captured_col = to_col;
            board->pieces[Board::board_get_index(captured_row, captured_col)] = PIECE_NONE;
        }
        board->pieces[Board::board_get_index(to_row, to_col)] = from_piece;
        board->pieces[Board::board_get_index(from_row, from_col)] = PIECE_NONE;
    }

    bool
    Board::board_move(const int32_t from_row, const int32_t from_col, const int32_t to_row,
                      const int32_t to_col)
    {
        if (board_can_move_basic(this, from_row, from_col, to_row, to_col))
        {
            if (board_is_castle(this, from_row, from_col, to_row, to_col))
            {
                board_do_castle(this, from_row, from_col, to_row, to_col);
            }
            else
            {
                board_do_move(this, from_row, from_col, to_row, to_col);
            }
            return true;
        }
        return false;
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
    Board::board_move(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col,
               AlgebraicMove &out_alg)
    {
        const SimpleMove move{
            static_cast<uint8_t>(from_row),
            static_cast<uint8_t>(from_col),
            static_cast<uint8_t>(to_row),
            static_cast<uint8_t>(to_col)};
        out_alg = analyzer_get_algebraic_move(this, move);
        if (board_move(from_row, from_col, to_row, to_col))
        {
            return true;
        }
        return false;
    }

    bool
    Board::board_promote(const int32_t from_row, const int32_t from_col, const int32_t to_row,
                         const int32_t to_col,
                         const PieceType type)
    {
        if (board_can_move_basic(this, from_row, from_col, to_row, to_col))
        {
            pieces[Board::board_get_index(to_row, to_col)] = chess_piece_make(type, PIECE_COLOR(pieces[Board::board_get_index(from_row, from_col)]));
            pieces[Board::board_get_index(from_row, from_col)] = PIECE_NONE;
            return true;
        }
        return false;
    }

    bool
    Board::board_promote(const int32_t from_row, const int32_t from_col, const int32_t to_row,
                         const int32_t to_col,
                         AlgebraicMove &out_alg, const PieceType type)
    {
        const SimpleMove move{
            static_cast<uint8_t>(from_row),
            static_cast<uint8_t>(from_col),
            static_cast<uint8_t>(to_row),
            static_cast<uint8_t>(to_col)};
        out_alg = analyzer_get_algebraic_move(this, move);
        if (board_promote(from_row, from_col, to_row, to_col, type))
        {
            return true;
        }
        return false;
    }
}

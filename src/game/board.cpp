#include "board.hpp"

#include <cmath>
#include <cstring>
#include "analyzer.hpp"

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

    static void
    board_reset_first_move(Board *board) {
        for (auto &piece: board->pieces) {
            piece.first_move_was_last_turn = 0;
        }
    }

    bool
    board_is_castle(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                    const int32_t to_col) {
        (void) to_row;
        if (const ChessPiece from_piece = board->pieces[board_get_index(from_row, from_col)];
            from_piece.type == ChessPieceType::KING && std::abs(from_col - to_col) > 1) {
            return true;
        }
        return false;
    }

    static bool
    board_is_en_passant(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                        const int32_t to_col) {
        (void) to_row;
        if (const ChessPiece to_piece = board->pieces[board_get_index(to_row, to_col)];
            to_piece.type != ChessPieceType::NONE) {
            return false;
        }
        if (const ChessPiece from_piece = board->pieces[board_get_index(from_row, from_col)];
            from_piece.type == ChessPieceType::PAWN && from_col != to_col) {
            return true;
        }
        return false;
    }


    static void
    board_do_castle(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                    const int32_t to_col) {
        ChessPiece &king = board->pieces[board_get_index(from_row, from_col)];

        ChessPiece &rook = board->pieces[board_get_index(from_row, from_col - to_col > 0 ? 0 : 7)];
        rook.first_move_was_last_turn = 1;
        rook.moved = 1;
        board->pieces[board_get_index(from_row, from_col - to_col > 0 ? 3 : 5)] = rook;
        rook = None; // Remove the rook from the original position

        king.first_move_was_last_turn = 1;
        king.moved = 1;


        board->pieces[board_get_index(to_row, to_col)] = king;
        board->pieces[board_get_index(from_row, from_col)] = None;
    }

    static void
    board_do_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                  const int32_t to_col) {
        ChessPiece &from_piece = board->pieces[board_get_index(from_row, from_col)];

        if (from_piece.moved == 0) {
            from_piece.first_move_was_last_turn = 1;
        }
        from_piece.moved = 1;
        if (board_is_en_passant(board, from_row, from_col, to_row, to_col)) {
            // Remove the captured pawn
            const int32_t captured_row = from_piece.color == PIECE_WHITE ? to_row + 1 : to_row - 1;
            const int32_t captured_col = to_col;
            board->pieces[board_get_index(captured_row, captured_col)] = None;
        }
        board->pieces[board_get_index(to_row, to_col)] = from_piece;
        board->pieces[board_get_index(from_row, from_col)] = None;
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
               const int32_t to_col) {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col)) {
            board_reset_first_move(board);
            if (board_is_castle(board, from_row, from_col, to_row, to_col)) {
                board_do_castle(board, from_row, from_col, to_row, to_col);
            } else {
                board_do_move(board, from_row, from_col, to_row, to_col);
            }
            return true;
        }
        return false;
    }

    void
    board_move_no_check(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                        const int32_t to_col) {
        if (board_is_castle(board, from_row, from_col, to_row, to_col)) {
            board_do_castle(board, from_row, from_col, to_row, to_col);
        } else {
            board_do_move(board, from_row, from_col, to_row, to_col);
        }
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col,
               const int32_t last_moved_index) {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col)) {
            board->pieces[last_moved_index].first_move_was_last_turn = 0;
            if (board_is_castle(board, from_row, from_col, to_row, to_col)) {
                board_do_castle(board, from_row, from_col, to_row, to_col);
            } else {
                board_do_move(board, from_row, from_col, to_row, to_col);
            }
            return true;
        }
        return false;
    }

    bool
    board_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col,
               AlgebraicMove &out_alg) {
        const SimpleMove move{
            static_cast<uint8_t>(from_row),
            static_cast<uint8_t>(from_col),
            static_cast<uint8_t>(to_row),
            static_cast<uint8_t>(to_col)
        };
        out_alg = analyzer_get_algebraic_move(board, move);
        if (board_move(board, from_row, from_col, to_row, to_col)) {
            return true;
        }
        return false;
    }

    bool
    board_promote(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                  const int32_t to_col,
                  const ChessPieceType type) {
        if (board_can_move_basic(board, from_row, from_col, to_row, to_col)) {
            board_reset_first_move(board);
            board->pieces[board_get_index(to_row, to_col)] = ChessPiece{
                type, board->pieces[board_get_index(from_row, from_col)].color, 0, 0
            };
            board->pieces[board_get_index(from_row, from_col)] = None;
            return true;
        }
        return false;
    }

    bool
    board_promote(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                  const int32_t to_col,
                  AlgebraicMove &out_alg, const ChessPieceType type) {
        const SimpleMove move{
            static_cast<uint8_t>(from_row),
            static_cast<uint8_t>(from_col),
            static_cast<uint8_t>(to_row),
            static_cast<uint8_t>(to_col)
        };
        out_alg = analyzer_get_algebraic_move(board, move);
        if (board_promote(board, from_row, from_col, to_row, to_col, type)) {
            return true;
        }
        return false;
    }
}

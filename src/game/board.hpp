#pragma once
#include <bit>
#include "piece.hpp"
#include <cstring>
#include "move.hpp"
#include <cstdint>
#include "types.hpp"

namespace game {
    struct AvailableSquares {
        BitBoard bits;
        void set(const int32_t row, const int32_t col) {
            bits |= (1ULL << (row * 8 + col));
        }
        void clear(const int32_t row, const int32_t col) {
            bits &= ~(1ULL << (row * 8 + col));
        }
        bool get(const int32_t row, const int32_t col) const {
            return (bits & (1ULL << (row * 8 + col))) != 0;
        }
        bool get(const int32_t index) const {
            return  get(index / 8, index % 8);
        }
        void reset() {
            bits = 0;
        }
        int32_t move_count() const {
            return std::popcount(bits);
        }
    };

    struct Board {
        Piece pieces[64];
        int8_t en_passant_index;
        uint8_t castle_rights;

        Piece *begin() {
            return pieces;
        }

        Piece *end() {
            return pieces + 64;
        }

        void init() {
            std::memset(pieces, 0, sizeof(pieces));
            en_passant_index = -1;
            castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE |
                            CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
        }
    };

    inline int32_t
    board_get_piece_count(const Board *board, const ChessPieceColor color) {
        int32_t count = 0;
        for (const auto &piece: board->pieces) {
            if (PIECE_TYPE(piece) != ChessPieceType::NONE && PIECE_COLOR(piece) == color) {
                ++count;
            }
        }
        return count;
    }

    void
    board_populate(Board *board);

    inline void
    reset_board(Board *board) {
        std::memset(board, 0, sizeof(Board));
        board_populate(board);
    }

    inline int32_t
    board_get_index(const int32_t row, const int32_t col) {
        return row * 8 + col;
    }

    inline int32_t
    board_get_row(const int32_t index) {
        return index / 8;
    }

    inline int32_t
    board_get_col(const int32_t index) {
        return index % 8;
    }

    inline bool
    board_pawn_first_move(const Piece piece, const int32_t row) {
        return (PIECE_COLOR(piece) == PIECE_WHITE && row == 1) ||
               (PIECE_COLOR(piece) == PIECE_BLACK && row == 6);
    }

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

    void
    board_move_no_check(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
               AlgebraicMove &out_alg);

    inline bool
    board_move(Board *board, const SimpleMove &move) {
        return board_move(board, move.from_row, move.from_col, move.to_row, move.to_col);
    }

    inline bool
    board_move(Board *board, const AlgebraicMove &move) {
        return board_move(board, move.from_row, move.from_col, move.to_row, move.to_col);
    }

    bool
    board_promote(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
                  ChessPieceType type = ChessPieceType::QUEEN);

    bool
    board_promote(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
                  AlgebraicMove &out_alg, ChessPieceType type = ChessPieceType::QUEEN);

    inline bool
    board_valid_rol_col(const int32_t row, const int32_t col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    inline bool
    board_can_en_passant_this(const Board *board, const int32_t row, const int32_t col, const ChessPieceColor enemy) {
        return board_valid_rol_col(row, col) && PIECE_TYPE(board->pieces[board_get_index(row, col)]) == ChessPieceType::PAWN &&
               PIECE_COLOR(board->pieces[board_get_index(row, col)]) == enemy &&
               board_get_index(row, col) == board->en_passant_index;
    }

    inline bool 
    board_castle_rights_for(const Board* board, const ChessPieceColor color, const bool kingside) {
        if (color == PIECE_WHITE) {
            return kingside ? (board->castle_rights & CASTLE_WHITE_KINGSIDE) != 0
                            : (board->castle_rights & CASTLE_WHITE_QUEENSIDE) != 0;
        } else {
            return kingside ? (board->castle_rights & CASTLE_BLACK_KINGSIDE) != 0
                            : (board->castle_rights & CASTLE_BLACK_QUEENSIDE) != 0;
        }
    }
}

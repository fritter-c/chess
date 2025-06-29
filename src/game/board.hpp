#pragma once
#include <bit>
#include "piece.hpp"
#include <cstring>
#include "move.hpp"
#include <cstdint>
#include "types.hpp"
#include <array>

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
            return get(index / 8, index % 8);
        }

        void reset() {
            bits = 0;
        }

        int32_t move_count() const {
            return std::popcount(bits);
        }
    };

    struct Board {
        std::array<Piece, 64> pieces;
        int8_t en_passant_index;
        std::byte castle_rights;

        auto begin() {
            return pieces.begin();
        }

        auto end() {
            return pieces.end();
        }

        void init() {
            std::memset(pieces.data(), 0, sizeof(pieces));
            en_passant_index = -1;
            castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE |
                            CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
        }

        constexpr int32_t
        get_piece_count(const Color color) const {
            int32_t count = 0;
            for (const auto &piece: pieces) {
                if (PIECE_TYPE(piece) != NONE && PIECE_COLOR(piece) == color) {
                    ++count;
                }
            }
            return count;
        }

        void
        board_populate();

        void
        reset_board() {
            std::memset(this, 0, sizeof(Board));
            board_populate();
        }

        static int32_t
        board_get_index(const int32_t row, const int32_t col) {
            return row * 8 + col;
        }

        static int32_t
        board_get_row(const int32_t index) {
            return index / 8;
        }

        static int32_t
        board_get_col(const int32_t index) {
            return index % 8;
        }

        static bool
        board_pawn_first_move(const Piece piece, const int32_t row) {
            return (PIECE_COLOR(piece) == PIECE_WHITE && row == 1) ||
                   (PIECE_COLOR(piece) == PIECE_BLACK && row == 6);
        }

        bool
        board_move(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

        void
        board_move_no_check(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

        bool
        board_move(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
                   AlgebraicMove &out_alg);

        bool
        board_move(const SimpleMove &move) {
            return board_move(move.from_row, move.from_col, move.to_row, move.to_col);
        }

        bool
        board_move(const AlgebraicMove &move) {
            return board_move(move.from_row, move.from_col, move.to_row, move.to_col);
        }

        bool
        board_promote(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
                      PieceType type = QUEEN);

        bool
        board_promote(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
                      AlgebraicMove &out_alg, PieceType type = QUEEN);

        static bool
        board_valid_rol_col(const int32_t row, const int32_t col) {
            return row >= 0 && row < 8 && col >= 0 && col < 8;
        }

        bool
        board_can_en_passant_this(const int32_t row, const int32_t col, const Color enemy) const {
            return board_valid_rol_col(row, col) && PIECE_TYPE(pieces[board_get_index(row, col)]) == PAWN &&
                   PIECE_COLOR(pieces[board_get_index(row, col)]) == enemy &&
                   board_get_index(row, col) == en_passant_index;
        }

        bool
        board_castle_rights_for(const Color color, const bool kingside) const {
            if (color == PIECE_WHITE) {
                return kingside
                           ? std::to_integer<int>(castle_rights & CASTLE_WHITE_KINGSIDE) != 0
                           : std::to_integer<int>(castle_rights & CASTLE_WHITE_QUEENSIDE) != 0;
            }
            return kingside
                       ? std::to_integer<int>(castle_rights & CASTLE_BLACK_KINGSIDE) != 0
                       : std::to_integer<int>(castle_rights & CASTLE_BLACK_QUEENSIDE) != 0;
        }
    };
}

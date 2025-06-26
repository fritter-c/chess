#pragma once
#include <bit>

#include "piece.hpp"
#include <cstring>
#include "move.hpp"
#include <cstdint>

namespace game {
    constexpr ChessPiece None{ChessPieceType::NONE, PIECE_WHITE, 0};
    constexpr ChessPiece BPawn{ChessPieceType::PAWN, PIECE_BLACK, 0};
    constexpr ChessPiece WPawn{ChessPieceType::PAWN, PIECE_WHITE, 0};
    constexpr ChessPiece BRook{ChessPieceType::ROOK, PIECE_BLACK, 0};
    constexpr ChessPiece BKnight{ChessPieceType::KNIGHT, PIECE_BLACK, 0};
    constexpr ChessPiece BBishop{ChessPieceType::BISHOP, PIECE_BLACK, 0};
    constexpr ChessPiece BQueen{ChessPieceType::QUEEN, PIECE_BLACK, 0};
    constexpr ChessPiece BKing{ChessPieceType::KING, PIECE_BLACK, 0};
    constexpr ChessPiece WRook{ChessPieceType::ROOK, PIECE_WHITE, 0};
    constexpr ChessPiece WKnight{ChessPieceType::KNIGHT, PIECE_WHITE, 0};
    constexpr ChessPiece WBishop{ChessPieceType::BISHOP, PIECE_WHITE, 0};
    constexpr ChessPiece WQueen{ChessPieceType::QUEEN, PIECE_WHITE, 0};
    constexpr ChessPiece WKing{ChessPieceType::KING, PIECE_WHITE, 0};

    static constexpr ChessPiece StartingPosition[8][8] = {
        // rank 1
        {WRook, WKnight, WBishop, WQueen, WKing, WBishop, WKnight, WRook},
        // rank 2
        {WPawn, WPawn, WPawn, WPawn, WPawn, WPawn, WPawn, WPawn},
        // ranks 3–6 empty
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        // rank 7
        {BPawn, BPawn, BPawn, BPawn, BPawn, BPawn, BPawn, BPawn},
        // rank 8
        {BRook, BKnight, BBishop, BQueen, BKing, BBishop, BKnight, BRook},
    };

    static constexpr const char *CellNames[8][8] = {
        // rank 1
        {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"},
        // rank 2
        {"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2"},
        // ranks 3–6
        {"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3"},
        {"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4"},
        {"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5"},
        {"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6"},
        // rank 7
        {"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7"},
        // rank 8
        {"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"},
    };

    enum class File : uint8_t {
        FILE_A,
        FILE_B,
        FILE_C,
        FILE_D,
        FILE_E,
        FILE_F,
        FILE_G,
        FILE_H,
        FILE_NB
    };

    enum class Rank : uint8_t {
        RANK_1,
        RANK_2,
        RANK_3,
        RANK_4,
        RANK_5,
        RANK_6,
        RANK_7,
        RANK_8,
        RANK_NB
    };

    struct AvailableSquares {
        uint64_t bits;

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

    struct Square {
        uint8_t row: 4;
        uint8_t col: 4;
    };

    struct Board {
        ChessPiece pieces[64];

        ChessPiece *begin() {
            return pieces;
        }

        ChessPiece *end() {
            return pieces + 64;
        }
    };

    inline int32_t
    board_get_piece_count(const Board *board, const ChessPieceColor color) {
        int32_t count = 0;
        for (const auto &piece: board->pieces) {
            if (piece.type != ChessPieceType::NONE && piece.color == color) {
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
    board_pawn_first_move(const ChessPiece piece, const int32_t row) {
        return (piece.color == PIECE_WHITE && row == 1) ||
               (piece.color == PIECE_BLACK && row == 6);
    }

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

    void
    board_move_no_check(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
               int32_t last_moved_index);

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
        return board_valid_rol_col(row, col) && board->pieces[board_get_index(row, col)].type == ChessPieceType::PAWN &&
               board->pieces[board_get_index(row, col)].color == enemy &&
               board->pieces[board_get_index(row, col)].first_move_was_last_turn;
    }
}

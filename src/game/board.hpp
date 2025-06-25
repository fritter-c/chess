#pragma once
#include "piece.hpp"
#include <cstring>
#include "move.hpp"
#include <cstdint>
namespace game
{
    constexpr ChessPiece None{ChessPieceType::NONE, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece BPawn{ChessPieceType::PAWN, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece WPawn{ChessPieceType::PAWN, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece BRook{ChessPieceType::ROOK, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece BKnight{ChessPieceType::KNIGHT, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece BBishop{ChessPieceType::BISHOP, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece BQueen{ChessPieceType::QUEEN, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece BKing{ChessPieceType::KING, ChessPieceColor::PIECE_BLACK, 0};
    constexpr ChessPiece WRook{ChessPieceType::ROOK, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece WKnight{ChessPieceType::KNIGHT, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece WBishop{ChessPieceType::BISHOP, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece WQueen{ChessPieceType::QUEEN, ChessPieceColor::PIECE_WHITE, 0};
    constexpr ChessPiece WKing{ChessPieceType::KING, ChessPieceColor::PIECE_WHITE, 0};

    static constexpr ChessPiece StartingPosition[8][8] = {
        {BRook, BKnight, BBishop, BQueen, BKing, BBishop, BKnight, BRook},
        {BPawn, BPawn, BPawn, BPawn, BPawn, BPawn, BPawn, BPawn},
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        {None, None, None, None, None, None, None, None},
        {WPawn, WPawn, WPawn, WPawn, WPawn, WPawn, WPawn, WPawn},
        {WRook, WKnight, WBishop, WQueen, WKing, WBishop, WKnight, WRook},
    };

    static constexpr const char *CellNames[8][8]{
        {"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"},
        {"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7"},
        {"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6"},
        {"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5"},
        {"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4"},
        {"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3"},
        {"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2"},
        {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"}};

    enum class File : uint8_t
    {
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

    enum class Rank : uint8_t
    {
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

    struct BitBoard
    {
        uint64_t bits;

        void set(int32_t row, int32_t col)
        {
            bits |= (1ULL << (row * 8 + col));
        }

        void clear(int32_t row, int32_t col)
        {
            bits &= ~(1ULL << (row * 8 + col));
        }

        bool get(int32_t row, int32_t col) const
        {
            return (bits & (1ULL << (row * 8 + col))) != 0;
        }

        void reset()
        {
            bits = 0;
        }
    };
    
    struct Board
    {
        ChessPiece pieces[64];
    };

    void
    board_populate(Board *board);

    inline void
    reset_board(Board *board)
    {
        std::memset(board, 0, sizeof(Board));
        board_populate(board);
    }

    inline int32_t
    board_get_index(const int32_t row, const int32_t col)
    {
        return row * 8 + col;
    }

    inline int32_t
    board_get_row(const int32_t index)
    {
        return index / 8;
    }

    inline int32_t
    board_get_col(const int32_t index)
    {
        return index % 8;
    }

    inline bool
    board_pawn_first_move(const ChessPiece piece, const int32_t row)
    {
        return (piece.color == ChessPieceColor::PIECE_WHITE && row == 6) ||
               (piece.color == ChessPieceColor::PIECE_BLACK && row == 1);
    }

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

    bool
    board_move(Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col, AlgebraicMove &out_alg);

    inline bool
    board_move(Board *board, const SimpleMove &move)
    {
        return board_move(board, move.from_row, move.from_col, move.to_row, move.to_col);
    }

    inline bool
    board_move(Board *board, const AlgebraicMove &move)
    {
        return board_move(board, move.from_row, move.from_col, move.to_row, move.to_col);
    }

    AlgebraicMove
    board_get_algebraic_move(const Board *board, const SimpleMove &move);
}

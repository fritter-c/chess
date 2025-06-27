#pragma once
#include <cstdint>
#include <utility>
namespace game {
    enum PieceType : uint8_t {
        NONE,
        PAWN,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
        PIECE_COUNT
    };

    enum Piece : int8_t {
        PIECE_NONE,
        WHITE_PAWN,
        WHITE_KNIGHT,
        WHITE_BISHOP,
        WHITE_ROOK,
        WHITE_QUEEN,
        WHITE_KING,
        BLACK_PAWN = WHITE_PAWN + 8,
        BLACK_KNIGHT,
        BLACK_BISHOP,
        BLACK_ROOK,
        BLACK_QUEEN,
        BLACK_KING,
        PIECE_CB = 16
    };

    enum Color : int8_t {
        PIECE_WHITE,
        PIECE_BLACK
    };

    constexpr Color operator~(const Color color) {
        return static_cast<Color>(color ^ PIECE_BLACK);
    }

    inline Color
    chess_piece_other_color(const Color color) {
        return ~color;
    }


    enum PromotionPieceType : uint8_t {
        PROMOTION_QUEEN = 0,
        PROMOTION_ROOK,
        PROMOTION_BISHOP,
        PROMOTION_KNIGHT
    };

#define PIECE_TYPE(PIECE) static_cast<game::PieceType>(PIECE & 7)
#define PIECE_COLOR(PIECE)static_cast<game::Color>(PIECE >> 3)

    constexpr  Piece
    chess_piece_make(const PieceType type, const Color color) {
        return static_cast<Piece>((color << 3) + type);
    }

    inline char
    chess_piece_to_algebraic_letter(const PieceType type) {
        constexpr static char ALGEBRAIC_TABLE[] = {'N', 'B', 'R', 'Q', 'K'};
        if (type == PAWN || type == NONE) {
            return ' ';
        }
        return ALGEBRAIC_TABLE[std::to_underlying(type) - KNIGHT];
    }

    inline bool
    chess_piece_is_piece_from_char(const char c) {
        return c == 'K' || c == 'Q' || c == 'B' || c == 'N' || c == 'R';
    }
}

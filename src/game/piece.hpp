#pragma once
#include <cstdint>

namespace game
{
    enum class ChessPieceType : uint8_t
    {
        NONE,
        KING,
        QUEEN,
        BISHOP,
        KNIGHT,
        ROOK,
        PAWN
    };

    enum PromotionPieceType : uint8_t
    {
        PROMOTION_QUEEN = 0,
        PROMOTION_ROOK,
        PROMOTION_BISHOP,
        PROMOTION_KNIGHT
    };

    enum ChessPieceColor : uint8_t
    {
        PIECE_WHITE,
        PIECE_BLACK
    };

    inline ChessPieceColor
    chess_piece_other_color(const ChessPieceColor color)
    {
        return static_cast<ChessPieceColor>(!color);
    }

    enum Piece : int8_t
    {
        PIECE_NONE,
        WHITE_KING,
        WHITE_QUEEN,
        WHITE_BISHOP,
        WHITE_KNIGHT,
        WHITE_ROOK,
        WHITE_PAWN,
        BLACK_KING,
        BLACK_QUEEN,
        BLACK_BISHOP,
        BLACK_KNIGHT,
        BLACK_ROOK,
        BLACK_PAWN

    };

    static constexpr ChessPieceType PIECE_TYPES_TABLE[] = {
        ChessPieceType::NONE,
        ChessPieceType::KING,
        ChessPieceType::QUEEN,
        ChessPieceType::BISHOP,
        ChessPieceType::KNIGHT,
        ChessPieceType::ROOK,
        ChessPieceType::PAWN,
        ChessPieceType::KING,
        ChessPieceType::QUEEN,
        ChessPieceType::BISHOP,
        ChessPieceType::KNIGHT,
        ChessPieceType::ROOK,
        ChessPieceType::PAWN};

#define PIECE_TYPE(PIECE) game::PIECE_TYPES_TABLE[static_cast<int>(PIECE)]
#define PIECE_COLOR(PIECE) ((PIECE) < game::BLACK_KING ? game::PIECE_WHITE : game::PIECE_BLACK)

    inline Piece
    chess_piece_make(const ChessPieceType type, const ChessPieceColor color)
    {
        return static_cast<Piece>(static_cast<int>(type) + (color == ChessPieceColor::PIECE_BLACK ? 6 : 0));
    }

    inline char
    chess_piece_to_algebraic_letter(const ChessPieceType type)
    {
        constexpr static char ALGEBRAIC_TABLE[] = {'K', 'Q', 'B', 'N', 'R'};
        if (type == ChessPieceType::PAWN || type == ChessPieceType::NONE)
        {
            return ' ';
        }
        return ALGEBRAIC_TABLE[static_cast<int>(type) - 1];
    }

    inline bool
    chess_piece_is_piece_from_char(const char c)
    {
        return c == 'K' || c == 'Q' || c == 'B' || c == 'N' || c == 'R';
    }
}

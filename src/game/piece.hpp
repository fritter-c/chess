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

    enum class ChessPieceColor : uint8_t
    {
        PIECE_WHITE,
        PIECE_BLACK
    };

    struct ChessPiece
    {
        ChessPieceType type : 3;
        ChessPieceColor color : 1;
        uint8_t moved : 1;
        uint8_t first_move_was_last_turn : 1; // Used for pawns to track if the first move was the last move
        uint8_t reserved : 2;                 // Reserved for future use, currently unused
    };

    inline bool
    chess_piece_equal(ChessPiece left, ChessPiece right)
    {
        return left.type == right.type && left.color == right.color;
    }

    constexpr static char AlgebraicPieceType[] = {'K', 'Q', 'B', 'N', 'R'};

    inline char
    chess_piece_to_algebraic_letter(const ChessPieceType type)
    {
        if (type == ChessPieceType::PAWN)
        {
            return '\0';
        }
        return AlgebraicPieceType[static_cast<int>(type) - 1];
    }
}
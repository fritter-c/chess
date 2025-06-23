#pragma once
#include <cstdint>
namespace game {
    enum class ChessPieceType : uint8_t {
        NONE,
        KING,
        QUEEN,
        BISHOP,
        KNIGHT,
        ROOK,
        PAWN
    };

    enum class ChessPieceColor : uint8_t {
        PIECE_WHITE,
        PIECE_BLACK
    };

    struct ChessPiece {
        ChessPieceType type : 3 ;
        ChessPieceColor color : 1;
    };

    constexpr static char AlgebraicPieceType[] = {'K', 'Q', 'B', 'N', 'R'};

}
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

    enum ChessPieceColor : uint8_t {
        PIECE_WHITE,
        PIECE_BLACK
    };

    inline ChessPieceColor
    chess_piece_other_color(const ChessPieceColor color) {
        return static_cast<ChessPieceColor>(!color);
    }

    struct ChessPiece {
        ChessPieceType type: 3;
        ChessPieceColor color: 1;
        uint8_t moved: 1;
        uint8_t first_move_was_last_turn: 1; // Used for pawns to track if the first move was the last move in the game
        uint8_t reserved: 2; // Reserved for future use, currently unused
    };

    inline bool
    chess_piece_equal(const ChessPiece left, const ChessPiece right) {
        return left.type == right.type && left.color == right.color;
    }

    constexpr static char AlgebraicPieceType[] = {'K', 'Q', 'B', 'N', 'R'};

    inline char
    chess_piece_to_algebraic_letter(const ChessPieceType type) {
        if (type == ChessPieceType::PAWN || type == ChessPieceType::NONE) {
            return ' ';
        }
        return AlgebraicPieceType[static_cast<int>(type) - 1];
    }

    inline bool
    chess_piece_is_piece_from_char(const char c) {
        return c == 'K' || c == 'Q' || c == 'B' || c == 'N' || c == 'R';
    }
}

#pragma once
#include <cstdint>
#include <utility>

namespace game {
enum PieceType : uint8_t { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING = 6, PIECE_COUNT = 6, ANY, PIECE_COUNT_PLUS_ANY };

enum Color : int8_t { PIECE_WHITE, PIECE_BLACK, COLOR_COUNT };

constexpr Color operator~(const Color color) { return static_cast<Color>(color ^ PIECE_BLACK); }

inline Color chess_piece_other_color(const Color color) { return ~color; }

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

constexpr Piece chess_piece_make(const PieceType type, const Color color) { return static_cast<Piece>((color << 3) + type); }

inline char chess_piece_to_algebraic_letter(const PieceType type) {
    constexpr static char ALGEBRAIC_TABLE[] = {'N', 'B', 'R', 'Q', 'K'};
    if (type == PAWN || type == EMPTY) {
        return ' ';
    }
    return ALGEBRAIC_TABLE[std::to_underlying(type) - KNIGHT];
}

inline bool chess_piece_is_piece_from_char(const char c) { return c == 'K' || c == 'Q' || c == 'B' || c == 'N' || c == 'R'; }

inline const char *piece_to_string(Piece p) {
    switch (p) {
    case WHITE_PAWN  : return "White Pawn";
    case WHITE_KNIGHT: return "White Knight";
    case WHITE_BISHOP: return "White Bishop";
    case WHITE_ROOK  : return "White Rook";
    case WHITE_QUEEN : return "White Queen";
    case WHITE_KING  : return "White King";
    case BLACK_PAWN  : return "Black Pawn";
    case BLACK_KNIGHT: return "Black Knight";
    case BLACK_BISHOP: return "Black Bishop";
    case BLACK_ROOK  : return "Black Rook";
    case BLACK_QUEEN : return "Black Queen";
    case BLACK_KING  : return "Black King";
    default          : return "Empty Square";
    }
}

#define PIECE_TYPE(PIECE) static_cast<game::PieceType>(PIECE & 7)
#define PIECE_COLOR(PIECE) static_cast<game::Color>(PIECE >> 3)
#define IS_WHITE(PIECE) PIECE_COLOR(PIECE) == game::PIECE_WHITE
#define IS_BLACK(PIECE) PIECE_COLOR(PIECE) == game::PIECE_BLACK
#define IS_PAWN(PIECE) PIECE_TYPE(PIECE) == game::PAWN

inline char piece_to_string_short(Piece p) {

    static constexpr char piece_font_table[2][7] = {{'z', 'O', 'M', 'V', 'T', 'W', 'L'}, {'z', 'P', 'N', 'B', 'R', 'Q', 'K'}};
    return piece_font_table[IS_BLACK(p)][PIECE_TYPE(p)];
}

enum PromotionPieceType : uint8_t { PROMOTION_QUEEN = 0, PROMOTION_ROOK, PROMOTION_BISHOP, PROMOTION_KNIGHT };

constexpr Piece promotion_piece_type_to_piece(const PromotionPieceType type, const Color color) {
    return chess_piece_make(static_cast<PieceType>(std::to_underlying(QUEEN) - std::to_underlying(type)), color);
}


} // namespace game

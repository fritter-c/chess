#pragma once
#include <array>
#include <cstdint>
#include <utility>
namespace game {
enum PieceType : uint8_t { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING = 6, PIECE_COUNT = 6, ANY, PIECE_COUNT_PLUS_ANY = 8 };

inline const char *piece_type_to_string(const PieceType type) {
    static constexpr std::array piece_names = {"Empty", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King", "Any"};
    return piece_names[type];
}

enum Color : int8_t { PIECE_WHITE, PIECE_BLACK, COLOR_COUNT };

inline const char *color_to_string(const Color color) {
    static constexpr std::array color_names = {"White", "Black"};
    return color_names[color];
}

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

constexpr bool chess_piece_is_piece_from_char(const char c) { return c == 'K' || c == 'Q' || c == 'B' || c == 'N' || c == 'R'; }

inline const char *piece_to_string(const Piece p) {
    static constexpr std::array piece_names = {"Empty Square", "White Pawn", "White Knight", "White Bishop", "White Rook", "White Queen", "White King", "Empty Square",
                                               "Empty Square", "Black Pawn", "Black Knight", "Black Bishop", "Black Rook", "Black Queen", "Black King"};
    return piece_names[p];
}

#define PIECE_TYPE(PIECE) static_cast<game::PieceType>(PIECE & 7)
#define PIECE_COLOR(PIECE) static_cast<game::Color>(PIECE >> 3)
#define IS_WHITE(PIECE) PIECE_COLOR(PIECE) == game::PIECE_WHITE
#define IS_BLACK(PIECE) PIECE_COLOR(PIECE) == game::PIECE_BLACK
#define IS_PAWN(PIECE) PIECE_TYPE(PIECE) == game::PAWN

inline char piece_to_string_short(Piece p) {
    static constexpr std::array piece_font_table = {std::array{'z', 'O', 'M', 'V', 'T', 'W', 'L'}, std::array{'z', 'P', 'N', 'B', 'R', 'Q', 'K'}};
    return piece_font_table[IS_BLACK(p)][PIECE_TYPE(p)];
}

enum PromotionPieceType : uint8_t { PROMOTION_QUEEN = 0, PROMOTION_ROOK, PROMOTION_BISHOP, PROMOTION_KNIGHT };

constexpr Piece promotion_piece_type_to_piece(const PromotionPieceType type, const Color color) {
    return chess_piece_make(static_cast<PieceType>(std::to_underlying(QUEEN) - std::to_underlying(type)), color);
}

} // namespace game

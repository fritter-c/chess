#pragma once
#include <cstdint>
#include "piece.hpp"
#include "string.hpp"
#include "types.hpp"
#include "utils.hpp"
namespace game {
struct SimpleMove {
    int32_t from_row{0};
    int32_t from_col{0};
    int32_t to_row{0};
    int32_t to_col{0};

    constexpr SimpleMove() noexcept = default;
    constexpr SimpleMove(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col)
        : from_row(from_row), from_col(from_col), to_row(to_row), to_col(to_col) {}

    constexpr SimpleMove(const int32_t origin, const int32_t destination) : from_row(origin / 8), from_col(origin % 8), to_row(destination / 8), to_col(destination % 8) {}
};

enum class DisambiguationType { NONE, FILE, RANK, FILE_RANK };

// First 6-bits origin square, next 6-bits destination square
// Them 2-bits for promotion piece type
// Them 2-bits for flags (en passant, castle, promotion)
struct Move {
    using storage_type = uint16_t;
    enum MoveSpecialType : uint8_t { MOVE_NONE = 0, MOVE_CASTLE, MOVE_PROMOTION, MOVE_EN_PASSANT };

    storage_type move;

    static constexpr int ORIGIN_SHIFT = 0;
    static constexpr storage_type ORIGIN_MASK = (1u << 6) - 1; // 0x003F

    static constexpr int DEST_SHIFT = 6;
    static constexpr storage_type DEST_MASK = (1u << 6) - 1; // 0x003F

    static constexpr int PROMO_SHIFT = 12;
    static constexpr storage_type PROMO_MASK = (1u << 2) - 1; // 0x0003

    static constexpr int FLAGS_SHIFT = 14;
    static constexpr storage_type FLAGS_MASK = (1u << 2) - 1; // 0x0003

    void set_origin(const uint8_t sq) noexcept {
        constexpr storage_type ORIGIN{static_cast<storage_type>(~(ORIGIN_MASK << ORIGIN_SHIFT))};
        move = move & ORIGIN | static_cast<storage_type>(static_cast<storage_type>(sq) << ORIGIN_SHIFT);
    }

    uint8_t get_origin() const { return move >> ORIGIN_SHIFT & ORIGIN_MASK; }

    void set_destination(const uint8_t d) {
        constexpr storage_type DEST{static_cast<storage_type>(~(DEST_MASK << DEST_SHIFT))};
        move = move & DEST | static_cast<storage_type>((static_cast<storage_type>(d) & DEST_MASK) << DEST_SHIFT);
    }

    uint8_t get_destination() const { return move >> DEST_SHIFT & DEST_MASK; }

    void set_promotion_piece(const PromotionPieceType p) {
        constexpr storage_type PROMO{static_cast<storage_type>(~(PROMO_MASK << PROMO_SHIFT))};
        move = move & PROMO | static_cast<storage_type>((std::to_underlying(p) & PROMO_MASK) << PROMO_SHIFT);
    }

    PromotionPieceType get_promotion_piece() const { return static_cast<PromotionPieceType>(move >> PROMO_SHIFT & PROMO_MASK); }

    void set_special(const MoveSpecialType s) {
        constexpr storage_type FLAGS{static_cast<storage_type>(~(FLAGS_MASK << FLAGS_SHIFT))};
        move = move & FLAGS | static_cast<storage_type>(std::to_underlying(s) << FLAGS_SHIFT);
    }

    MoveSpecialType get_special() const { return static_cast<MoveSpecialType>(move >> FLAGS_SHIFT & FLAGS_MASK); }

    bool is_en_passant() const { return get_special() == MOVE_EN_PASSANT; }

    bool is_castle() const { return get_special() == MOVE_CASTLE; }

    bool is_promotion() const { return get_special() == MOVE_PROMOTION; }

    PieceType get_promotion_piece_type() const { return static_cast<PieceType>(std::to_underlying(QUEEN) - std::to_underlying(get_promotion_piece())); }

    int32_t from_row() const { return get_origin() / 8; }

    int32_t from_col() const { return get_origin() % 8; }

    int32_t to_row() const { return get_destination() / 8; }

    int32_t to_col() const { return get_destination() % 8; }

    SquareIndex get_origin_index() const { return static_cast<SquareIndex>(get_origin()); }

    SquareIndex get_destination_index() const { return static_cast<SquareIndex>(get_destination()); }

    bool going_right() const { return (static_cast<int8_t>(get_destination()) - static_cast<int8_t>(get_origin())) % 8 > 0; }

    bool going_left() const { return (static_cast<int8_t>(get_destination()) - static_cast<int8_t>(get_origin())) % 8 < 0; }

    bool operator==(const Move &b) const = default;

    bool king_side_castle() const {
        Assert(is_castle(), "Move is not a castle move");
        return to_col() == 6;
    }

    bool queen_side_castle() const {
        Assert(is_castle(), "Move is not a castle move");
        return to_col() == 2;
    }
};

using AlgebraicMove = gtr::char_string<32>;
constexpr auto MIN_ALGEBRAIC_MOVE_LENGTH = 2; // Minimum length for a move (e.g., "e4")
struct Board;
AlgebraicMove move_to_algebraic(Board &board, Move move);
enum class MoveParserConversionError {
    NONE,
    DISAMBIGUATION_NEEDED,
    INVALID_ORIGIN,
    FILE_DISAMBIGUATION_NEEDED,
    RANK_DISAMBIGUATION_NEEDED,
    INVALID_FILE_DISAMBIGUATION,
    INVALID_RANK_DISAMBIGUATION,
    INVALID_DISAMBIGUATION,
    COULD_NOT_PARSE_DESTINATION,
    INVALID_PIECE_TYPE,
    TOO_LITTLE_INFORMATION,
    PAWN_MOVE_TO_PROMOTION_RANK_WITHOUT_PROMOTION,
    NO_PIECE_FOUND_FOR_ORIGIN,
    NO_PIECE_FOUND_AT_CAPTURE_DESTINATION,
    INVALID_NOTATION,
};

gtr::string conversion_error_to_string(MoveParserConversionError e) noexcept;
MoveParserConversionError algebraic_to_move(Color turn, const Board &board, const AlgebraicMove &move, Move &result);
inline bool algebraic_is_castle(const AlgebraicMove &move) { return move[0] == 'O'; }
inline bool algebraic_is_capture(const AlgebraicMove &move) { return move.find('x') != gtr::string::npos; }
inline bool algebraic_is_promotion(const AlgebraicMove &move) { return move.find('=') != gtr::string::npos; }
DisambiguationType algebraic_has_disambiguation(const AlgebraicMove &move, int32_t &disambiguation_end);
inline PieceType algebraic_get_piece_type(const AlgebraicMove &move) {
    switch (move[0]) {
    case 'N': return KNIGHT;
    case 'B': return BISHOP;
    case 'R': return ROOK;
    case 'Q': return QUEEN;
    case 'O':
    case 'K': return KING;
    default : return PAWN; // Pawn is not represented in algebraic notation
    }
}

inline SquareIndex algebraic_get_index(const AlgebraicMove &move, const int8_t index) {
    if (index < 0 || index + 1 >= move.size()) {
        return SQUARE_COUNT; // Invalid index
    }
    const char column = move[index];
    const char row = move[index + 1];
    if (column < 'a' || column > 'h' || row < '1' || row > '8') {
        return SQUARE_COUNT;
    }
    return static_cast<SquareIndex>((row - '1') * 8 + (column - 'a'));
}

} // namespace game

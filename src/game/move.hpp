#pragma once
#include <cstdint>
#include "piece.hpp"
#include "string.hpp"
#include "types.hpp"

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

    uint8_t get_origin() const { return (move >> ORIGIN_SHIFT) & ORIGIN_MASK; }

    void set_destination(const uint8_t d) {
        constexpr storage_type DEST{static_cast<storage_type>(~(DEST_MASK << DEST_SHIFT))};
        move = move & DEST | static_cast<storage_type>((static_cast<storage_type>(d) & DEST_MASK) << DEST_SHIFT);
    }

    uint8_t get_destination() const { return (move >> DEST_SHIFT) & DEST_MASK; }

    void set_promotion_piece(const PromotionPieceType p) {
        constexpr storage_type PROMO{static_cast<storage_type>(~(PROMO_MASK << PROMO_SHIFT))};
        move = move & PROMO | static_cast<storage_type>((std::to_underlying(p) & PROMO_MASK) << PROMO_SHIFT);
    }

    PromotionPieceType get_promotion_piece() const { return static_cast<PromotionPieceType>((move >> PROMO_SHIFT) & PROMO_MASK); }

    void set_special(const MoveSpecialType s) {
        constexpr storage_type FLAGS{static_cast<storage_type>(~(FLAGS_MASK << FLAGS_SHIFT))};
        move = move & FLAGS | static_cast<storage_type>(std::to_underlying(s) << FLAGS_SHIFT);
    }

    MoveSpecialType get_special() const { return static_cast<MoveSpecialType>((move >> FLAGS_SHIFT) & FLAGS_MASK); }

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

    SquareIndex get_castle_rook_destination_index() const {
        if (is_castle()) {
            switch (get_destination_index()) {
            case G1: return F1;
            case C1: return D1;
            case G8: return F8;
            case C8: return D8;
            default: return SQUARE_COUNT;
            }
        }
        return SQUARE_COUNT;
    }

    SquareIndex get_castle_rook_origin_index() const {
        if (is_castle()) {
            switch (get_destination_index()) {
            case G1: return H1;
            case C1: return A1;
            case G8: return H8;
            case C8: return A8;
            default: return SQUARE_COUNT;
            }
        }
        return SQUARE_COUNT;
    }
    bool operator==(const Move &b) const = default;
};

using AlgebraicMove = gtr::string;
struct Board;
// Consider that board still haven't move yet
AlgebraicMove move_to_algebraic(Board &board, Move move);
Move algebraic_to_move(Color turn, const Board &board, const AlgebraicMove &move);
} // namespace game

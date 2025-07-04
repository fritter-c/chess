#pragma once
#include <cstdint>
#include "piece.hpp"
#include <string>

namespace game
{
    struct SimpleMove
    {
        uint8_t from_row : 4;
        uint8_t from_col : 4;
        uint8_t to_row : 4;
        uint8_t to_col : 4;

        constexpr SimpleMove() noexcept
            : from_row(0), from_col(0), to_row(0), to_col(0) {}

        constexpr SimpleMove(uint8_t from_row, uint8_t from_col, uint8_t to_row, uint8_t to_col)
            : from_row(from_row), from_col(from_col), to_row(to_row), to_col(to_col) {}
            
        constexpr SimpleMove(int32_t origin, int32_t destination)
            : from_row(static_cast<uint8_t>(origin / 8)), from_col(static_cast<uint8_t>(origin % 8)),
              to_row(static_cast<uint8_t>(destination / 8)), to_col(static_cast<uint8_t>(destination % 8)) {}
    };

    struct AlgebraicMove
    {
        PieceType piece_type : 3;
        uint8_t from_col : 3;
        uint8_t to_col : 3;
        uint8_t from_row : 3;
        uint8_t to_row : 3;
        uint8_t is_capture : 1;
        uint8_t is_check : 1;
        uint8_t is_checkmate : 1;
        uint8_t need_row_disambiguation : 1;
        uint8_t need_col_disambiguation : 1;
        uint8_t en_passant : 1;
        uint8_t kingside_castle : 1;
        uint8_t queen_side_castle : 1;
        uint8_t promotion : 1;
    };

    // First 6 bits origin square, next 6 bits destination square
    // Them 2 bits for promotion piece type
    // Them 2 bits for flags (en passant, castle, promotion)
    struct Move
    {
        using storage_type = uint16_t;
        enum MoveSpecialType : uint8_t
        {
            MOVE_NONE = 0,
            MOVE_CASTLE,
            MOVE_PROMOTION,
            MOVE_EN_PASSANT
        };

        storage_type move;

        static constexpr int ORIGIN_SHIFT = 0;
        static constexpr storage_type ORIGIN_MASK = (1u << 6) - 1; // 0x003F

        static constexpr int DEST_SHIFT = 6;
        static constexpr storage_type DEST_MASK = (1u << 6) - 1; // 0x003F

        static constexpr int PROMO_SHIFT = 12;
        static constexpr storage_type PROMO_MASK = (1u << 2) - 1; // 0x0003

        static constexpr int FLAGS_SHIFT = 14;
        static constexpr storage_type FLAGS_MASK = (1u << 2) - 1; // 0x0003

        void set_origin(const uint8_t sq) noexcept
        {
            constexpr storage_type ORIGIN{static_cast<storage_type>(~(ORIGIN_MASK << ORIGIN_SHIFT))};
            move = move & ORIGIN | static_cast<storage_type>(static_cast<storage_type>(sq) << ORIGIN_SHIFT);
        }

        uint8_t get_origin() const
        {
            return (move >> ORIGIN_SHIFT) & ORIGIN_MASK;
        }

        void set_destination(const uint8_t d)
        {
            constexpr storage_type DEST{static_cast<storage_type>(~(DEST_MASK << DEST_SHIFT))};
            move = move & DEST | static_cast<storage_type>((static_cast<storage_type>(d) & DEST_MASK) << DEST_SHIFT);
        }

        uint8_t get_destination() const
        {
            return (move >> DEST_SHIFT) & DEST_MASK;
        }

        void set_promotion_piece(const PromotionPieceType p)
        {
            constexpr storage_type PROMO{static_cast<storage_type>(~(PROMO_MASK << PROMO_SHIFT))};
            move = move & PROMO | static_cast<storage_type>((std::to_underlying(p) & PROMO_MASK) << PROMO_SHIFT);
        }

        PromotionPieceType get_promotion_piece() const
        {
            return static_cast<PromotionPieceType>((move >> PROMO_SHIFT) & PROMO_MASK);
        }

        void set_special(const MoveSpecialType s)
        {
            constexpr storage_type FLAGS{static_cast<storage_type>(~(FLAGS_MASK << FLAGS_SHIFT))};
            move = move & FLAGS | static_cast<storage_type>(std::to_underlying(s) << FLAGS_SHIFT);
        }

        MoveSpecialType get_special() const
        {
            return static_cast<MoveSpecialType>((move >> FLAGS_SHIFT) & FLAGS_MASK);
        }

        PieceType get_promotion_piece_type() const
        {
            return static_cast<PieceType>(std::to_underlying(QUEEN) - std::to_underlying(get_promotion_piece()));
        }

        int32_t from_row() const
        {
            return get_origin() / 8;
        }

        int32_t from_col() const
        {
            return get_origin() % 8;
        }

        int32_t to_row() const
        {
            return get_destination() / 8;
        }

        int32_t to_col() const
        {
            return get_destination() % 8;
        }
    };

    std::string
    algebraic_move_to_string(const AlgebraicMove &move);
}

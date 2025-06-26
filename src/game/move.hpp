#pragma once
#include <cstdint>
#include "piece.hpp"
#include <cstddef>
namespace game
{
    struct SimpleMove
    {
        uint8_t from_row : 4;
        uint8_t from_col : 4;
        uint8_t to_row : 4;
        uint8_t to_col : 4;
    };

    struct AlgebraicMove
    {
        ChessPieceType piece_type : 3;
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

    // Firt 6 bits origin square, next 6 bits destination square
    // Them 2 bits for promotion piece type
    // Them 2 bits for flags (en passant, castle, promotion)
    struct Move
    {
        enum MoveSpecialType : int8_t
        {
            MOVE_NONE = 0,
            MOVE_CASTLE,
            MOVE_PROMOTION,
            MOVE_EN_PASSANT
        };

        int16_t move;

        static constexpr uint16_t ORIGIN_MASK = (1u << 6) - 1;
        static constexpr uint16_t DEST_MASK = ORIGIN_MASK;
        static constexpr uint16_t PROMO_MASK = (1u << 2) - 1;
        static constexpr uint16_t SPECIAL_MASK = PROMO_MASK;
        static constexpr int ORIGIN_SHIFT = 0;
        static constexpr int DEST_SHIFT = 6;
        static constexpr int PROMO_SHIFT = 12;
        static constexpr int SPECIAL_SHIFT = 14;

        void set_origin(uint8_t o)
        {
            move = (move & ~(ORIGIN_MASK << ORIGIN_SHIFT)) | ((o & ORIGIN_MASK) << ORIGIN_SHIFT);
        }
        uint8_t get_origin() const
        {
            return (move >> ORIGIN_SHIFT) & ORIGIN_MASK;
        }
        void set_destination(uint8_t d)
        {
            move = (move & ~(DEST_MASK << DEST_SHIFT)) | ((d & DEST_MASK) << DEST_SHIFT);
        }
        uint8_t get_destination() const
        {
            return (move >> DEST_SHIFT) & DEST_MASK;
        }
        void set_promotion_piece(PromotionPieceType p)
        {
            move = (move & ~(PROMO_MASK << PROMO_SHIFT)) | ((static_cast<uint16_t>(p) & PROMO_MASK) << PROMO_SHIFT);
        }
        PromotionPieceType get_promotion_piece() const
        {
            return static_cast<PromotionPieceType>((move >> PROMO_SHIFT) & PROMO_MASK);
        }
        void set_special(MoveSpecialType s)
        {
            move = (move & ~(SPECIAL_MASK << SPECIAL_SHIFT)) | ((static_cast<uint16_t>(s) & SPECIAL_MASK) << SPECIAL_SHIFT);
        }
        MoveSpecialType get_special() const
        {
            return static_cast<MoveSpecialType>((move >> SPECIAL_SHIFT) & SPECIAL_MASK);
        }
    };

    void
    algebraic_move_to_string(const AlgebraicMove &move, char *buffer, size_t buffer_size);
}
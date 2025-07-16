#pragma once
#include "types.hpp"

namespace game {
template <typename Offsets> constexpr BitBoard make_attack_mask(int square, const Offsets &offsets) noexcept {
    const int r = square / 8;
    const int c = square % 8;
    BitBoard bb = 0;
    for (auto [dr, dc] : offsets) {
        int rr = r + dr, cc = c + dc;
        if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8)
            bb |= (BitBoard{1} << (rr * 8 + cc));
    }
    return bb;
}

consteval MagicBoards init_magic_boards() noexcept {
    // pawn attacks
    MagicBoards mb;
    for (int color = 0; color < COLOR_COUNT; ++color) {
        // for white, dr=+1; for black, dr=–1
        int pawn_dr = (color == 0 ? +1 : -1);
        for (int sq = 0; sq < SQUARE_COUNT; ++sq) { mb.pawn_attacks[color][sq] = make_attack_mask(sq, std::array{std::pair{pawn_dr, -1}, std::pair{pawn_dr, +1}}); }
    }
    for (int sq = 0; sq < SQUARE_COUNT; ++sq) {
        mb.knight_attacks[sq] = make_attack_mask(
            sq, std::array{std::pair{+2, +1}, std::pair{+1, +2}, std::pair{-1, +2}, std::pair{-2, +1}, std::pair{-2, -1}, std::pair{-1, -2}, std::pair{+1, -2}, std::pair{+2, -1}});

        mb.king_attacks[sq] = make_attack_mask(
            sq, std::array{std::pair{+1, 0}, std::pair{-1, 0}, std::pair{0, +1}, std::pair{0, -1}, std::pair{+1, +1}, std::pair{+1, -1}, std::pair{-1, +1}, std::pair{-1, -1}});

        mb.rook_attacks[sq] =
            make_attack_mask(sq, std::array{std::pair{+1, 0}, std::pair{0, +1}, std::pair{-1, 0}, std::pair{0, -1}, std::pair{+2, 0}, std::pair{0, +2}, std::pair{-2, 0},
                                            std::pair{0, -2}, std::pair{+3, 0}, std::pair{0, +3}, std::pair{-3, 0}, std::pair{0, -3}, std::pair{+4, 0}, std::pair{0, +4},
                                            std::pair{-4, 0}, std::pair{0, -4}, std::pair{+5, 0}, std::pair{0, +5}, std::pair{-5, 0}, std::pair{0, -5}, std::pair{+6, 0},
                                            std::pair{0, +6}, std::pair{-6, 0}, std::pair{0, -6}, std::pair{+7, 0}, std::pair{0, +7}, std::pair{-7, 0}, std::pair{0, -7}});
        mb.bishop_attacks[sq] =
            make_attack_mask(sq, std::array{std::pair{+1, +1}, std::pair{-1, -1}, std::pair{+1, -1}, std::pair{-1, +1}, std::pair{+2, +2}, std::pair{-2, -2}, std::pair{+2, -2},
                                            std::pair{-2, +2}, std::pair{+3, +3}, std::pair{-3, -3}, std::pair{+3, -3}, std::pair{-3, +3}, std::pair{+4, +4}, std::pair{-4, -4},
                                            std::pair{+4, -4}, std::pair{-4, +4}, std::pair{+5, +5}, std::pair{-5, -5}, std::pair{+5, -5}, std::pair{-5, +5}, std::pair{+6, +6},
                                            std::pair{-6, -6}, std::pair{+6, -6}, std::pair{-6, +6}, std::pair{+7, +7}, std::pair{-7, -7}, std::pair{+7, -7}, std::pair{-7, +7}});
    }
    return mb;
}
}
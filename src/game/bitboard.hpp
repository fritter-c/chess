#pragma once
#include "types.hpp"

namespace game {
template <typename Offsets> constexpr BitBoard make_attack_mask(int32_t square, const Offsets &offsets) noexcept {
    const int32_t r = square / 8;
    const int32_t c = square % 8;
    BitBoard bb = 0;
    for (auto [dr, dc] : offsets) {
        const int32_t rr = r + dr;
        const int32_t cc = c + dc;
        if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8)
            bb |= (BitBoard{1} << (rr * 8 + cc));
    }
    return bb;
}

consteval MagicBoards init_magic_boards() noexcept {
    // pawn attacks
    MagicBoards mb{};
    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        // white dr = +1; black dr = -1
        int32_t pawn_dr = (color == 0 ? +1 : -1);
        for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) { mb.pawn_attacks[color][sq] = make_attack_mask(sq, std::array{std::pair{pawn_dr, -1}, std::pair{pawn_dr, +1}}); }
    }

    // — pawn moves (single + double pushes only) —
    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        // white dr = +1; black dr = -1
        const int32_t dr = (color == 0 ? +1 : -1);
        for (int sq = 0; sq < SQUARE_COUNT; ++sq) {
            const int r = sq / 8, c = sq % 8;
            BitBoard push_bb = 0;

            const int32_t r1 = r + dr;
            // single‐step
            if (r1 >= 0 && r1 < 8) {
                const int32_t sq1 = r1 * 8 + c;
                push_bb |= BitBoard{1} << sq1;

                // double‐step from start rank (rank 2 for white, 7 for black)
                if (color == 0 ? r == 1 : r == 6) {
                    const int32_t r2 = r + 2 * dr;
                    const int32_t sq2 = r2 * 8 + c;
                    push_bb |= BitBoard{1} << sq2;
                }
            }

            mb.pawn_moves[color][sq] = push_bb;
        }
    }
    for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) {
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

        const BitBoard from_bb = BitBoard{1} << sq;

        // pawn_attackers (for both colors)
        for (int32_t color = 0; color < 2; ++color) {
            const BitBoard fw = mb.pawn_attacks[color][sq];
            for (int32_t t = 0; t < 64; ++t) {
                if (fw & (BitBoard{1} << t))
                    mb.pawn_attackers[color][t] |= from_bb;
            }
        }

        // knight_attackers
        {
            const BitBoard fw = mb.knight_attacks[sq];
            for (int32_t t = 0; t < 64; ++t) {
                if (fw & (BitBoard{1} << t))
                    mb.knight_attackers[t] |= from_bb;
            }
        }

        // king_attackers
        {
            const BitBoard fw = mb.king_attacks[sq];
            for (int32_t t = 0; t < 64; ++t) {
                if (fw & (BitBoard{1} << t))
                    mb.king_attackers[t] |= from_bb;
            }
        }
    }

    return mb;
}

constexpr BitBoard FileA = 0x0101010101010101ULL;
constexpr BitBoard FileB = 0x0202020202020202ULL;
constexpr BitBoard FileC = 0x0404040404040404ULL;
constexpr BitBoard FileD = 0x0808080808080808ULL;
constexpr BitBoard FileE = 0x1010101010101010ULL;
constexpr BitBoard FileF = 0x2020202020202020ULL;
constexpr BitBoard FileG = 0x4040404040404040ULL;
constexpr BitBoard FileH = 0x8080808080808080ULL;

constexpr BitBoard Rank1 = 0x00000000000000FFULL;
constexpr BitBoard Rank2 = 0x000000000000FF00ULL;
constexpr BitBoard Rank3 = 0x0000000000FF0000ULL;
constexpr BitBoard Rank4 = 0x00000000FF000000ULL;
constexpr BitBoard Rank5 = 0x000000FF00000000ULL;
constexpr BitBoard Rank6 = 0x0000FF0000000000ULL;
constexpr BitBoard Rank7 = 0x00FF000000000000ULL;
constexpr BitBoard Rank8 = 0xFF00000000000000ULL;

constexpr BitBoard NotFileA = ~FileA;
constexpr BitBoard NotFileB = ~FileB;
constexpr BitBoard NotFileC = ~FileC;
constexpr BitBoard NotFileD = ~FileD;
constexpr BitBoard NotFileE = ~FileE;
constexpr BitBoard NotFileF = ~FileF;
constexpr BitBoard NotFileG = ~FileG;
constexpr BitBoard NotFileH = ~FileH;

constexpr BitBoard NotRank1 = ~Rank1;
constexpr BitBoard NotRank2 = ~Rank2;
constexpr BitBoard NotRank3 = ~Rank3;
constexpr BitBoard NotRank4 = ~Rank4;
constexpr BitBoard NotRank5 = ~Rank5;
constexpr BitBoard NotRank6 = ~Rank6;
constexpr BitBoard NotRank7 = ~Rank7;
constexpr BitBoard NotRank8 = ~Rank8;

static constexpr std::array BITBOARD_FILES = {FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH};
static constexpr std::array BITBOARD_RANKS = {Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8};

} // namespace game
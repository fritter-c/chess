#include "bitboard.hpp"
#include "types.hpp"
namespace game {
inline int popcount(uint64_t x) { return __builtin_popcountll(x); }

// build the i-th subset of bits under `mask`
inline uint64_t build_occupancy(uint64_t mask, int subset_index) {
    uint64_t occ = 0;
    // iterate bits of mask
    uint64_t m = mask;
    while (m) {
        int32_t bit = std::countr_zero(m);
        m &= m - 1;
        if (subset_index & 1)
            occ |= (1ULL << bit);
        subset_index >>= 1;
    }
    return occ;
}

static constexpr std::array<std::pair<int, int>, 4> rook_dirs{{
    {+1, 0},
    {-1, 0},
    {0, +1},
    {0, -1},
}};

// diagonal directions for bishops
static constexpr std::array<std::pair<int, int>, 4> bishop_dirs{{
    {+1, +1},
    {+1, -1},
    {-1, +1},
    {-1, -1},
}};

constexpr BitBoard sliding_attacks_rook(SquareIndex sq, BitBoard occ) noexcept {
    BitBoard attacks = 0;
    int r = int(sq) / 8;
    int c = int(sq) % 8;

    for (auto [dr, dc] : rook_dirs) {
        int rr = r + dr, cc = c + dc;
        while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
            int nsq = rr * 8 + cc;
            BitBoard bit = BitBoard{1} << nsq;
            attacks |= bit;
            if (occ & bit) // stop at first blocker
                break;
            rr += dr;
            cc += dc;
        }
    }

    return attacks;
}

constexpr BitBoard sliding_attacks_bishop(SquareIndex sq, BitBoard occ) noexcept {
    BitBoard attacks = 0;
    int r = int(sq) / 8;
    int c = int(sq) % 8;

    for (auto [dr, dc] : bishop_dirs) {
        int rr = r + dr, cc = c + dc;
        while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
            int nsq = rr * 8 + cc;
            BitBoard bit = BitBoard{1} << nsq;
            attacks |= bit;
            if (occ & bit) // stop at first blocker
                break;
            rr += dr;
            cc += dc;
        }
    }

    return attacks;
}

constexpr BitBoard sliding_attacks(SquareIndex sq, BitBoard occ, bool is_rook) noexcept { return is_rook ? sliding_attacks_rook(sq, occ) : sliding_attacks_bishop(sq, occ); }
inline uint64_t random_magic_candidate(std::mt19937_64 &rng) { return rng() & rng() & rng(); }

inline void find_magics_for_slider(const std::array<BitBoard, 64> &precomputed_full_attacks, const std::array<BitBoard, 64> &mask_array, std::array<uint64_t, 64> &out_magic,
                                   std::array<int, 64> &out_shift, bool is_rook) {
    std::mt19937_64 rng(0xDEADBEEF);

    for (int sq = 0; sq < 64; ++sq) {
        uint64_t mask = mask_array[sq];
        int bits = popcount(mask);
        int table_size = 1 << bits;

        // 1) generate all possible occupancies & true attacks
        gtr::vector<uint64_t> occs(table_size);
        gtr::vector<BitBoard> trues(table_size);
        for (int i = 0; i < table_size; ++i) {
            occs[i] = build_occupancy(mask, i);
            trues[i] = sliding_attacks(SquareIndex(sq), occs[i], is_rook);
        }

        int shift = 64 - bits;
        while (true) {
            uint64_t magic = random_magic_candidate(rng);
            gtr::vector<BitBoard> table(table_size, 0ULL);
            bool collision = false;

            for (int i = 0; i < table_size; ++i) {
                uint64_t idx = (occs[i] * magic) >> shift;
                if (table[idx] == 0ULL) {
                    table[idx] = trues[i];
                } else if (table[idx] != trues[i]) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                out_magic[sq] = magic;
                out_shift[sq] = shift;
                break;
            }
            // else try another magic candidate
        }
    }
}

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

MagicBoards init_magic_boards() noexcept {
    // pawn attacks
    MagicBoards mb{};
    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        int32_t pawn_dr = (color == 0 ? +1 : -1);
        for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) { mb.pawn_attacks[color][sq] = make_attack_mask(sq, std::array{std::pair{pawn_dr, -1}, std::pair{pawn_dr, +1}}); }
    }

    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        const int32_t dr = (color == 0 ? +1 : -1);
        for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) {
            const int32_t r = sq / 8, c = sq % 8;
            BitBoard push_bb = 0;

            const int32_t r1 = r + dr;
            if (r1 >= 0 && r1 < 8) {
                const int32_t sq1 = r1 * 8 + c;
                push_bb |= BitBoard{1} << sq1;
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

        const int32_t r0 = sq / 8;
        const int32_t c0 = sq % 8;

        // rook mask: all squares along rank/file *excluding* the board edges
        BitBoard rm = 0;
        for (auto [dr, dc] : rook_dirs) {
            int32_t r = r0 + dr, c = c0 + dc;
            while (r > 0 && r < 7 && c > 0 && c < 7) {
                rm |= BitBoard{1} << (r * 8 + c);
                r += dr;
                c += dc;
            }
        }
        mb.rook_mask[sq] = rm;

        // bishop mask: same idea on diagonals
        BitBoard bm = 0;
        for (auto [dr, dc] : bishop_dirs) {
            int32_t r = r0 + dr, c = c0 + dc;
            while (r > 0 && r < 7 && c > 0 && c < 7) {
                bm |= BitBoard{1} << (r * 8 + c);
                r += dr;
                c += dc;
            }
        }
        mb.bishop_mask[sq] = bm;
    }

    return mb;
}

} // namespace game
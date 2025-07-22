#include "bitboard.hpp"
#include <iostream>
#include "board.hpp"
#include "types.hpp"
namespace game {

static constexpr std::array<std::pair<int, int>, 4> rook_dirs{{
    {+1, 0},
    {-1, 0},
    {0, +1},
    {0, -1},
}};

static constexpr std::array<std::pair<int, int>, 4> bishop_dirs{{
    {+1, +1},
    {+1, -1},
    {-1, +1},
    {-1, -1},
}};

constexpr BitBoard sliding_attacks_rook(const SquareIndex sq, const BitBoard occ) noexcept {
    BitBoard attacks = 0;
    const int32_t r = std::to_underlying(sq) / 8;
    const int32_t c = std::to_underlying(sq) % 8;

    for (auto [dr, dc] : rook_dirs) {
        int32_t rr = r + dr;
        int32_t cc = c + dc;
        while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
            const int32_t nsq = rr * 8 + cc;
            const BitBoard bit = BitBoard{1} << nsq;
            attacks |= bit;
            if (occ & bit) // stop at first blocker
                break;
            rr += dr;
            cc += dc;
        }
    }
    return attacks;
}

constexpr BitBoard sliding_attacks_bishop(const SquareIndex sq, const BitBoard occ) noexcept {
    BitBoard attacks = 0;
    const int32_t r = std::to_underlying(sq) / 8;
    const int32_t c = std::to_underlying(sq) % 8;

    for (auto [dr, dc] : bishop_dirs) {
        int32_t rr = r + dr;
        int32_t cc = c + dc;
        while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
            const int32_t nsq = rr * 8 + cc;
            const BitBoard bit = BitBoard{1} << nsq;
            attacks |= bit;
            if (occ & bit) // stop at first blocker
                break;
            rr += dr;
            cc += dc;
        }
    }

    return attacks;
}

template <typename Offsets> constexpr BitBoard make_attack_mask(const int32_t square, const Offsets &offsets) noexcept {
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
void fill_sliders_magic(MagicBoards &mb) {
    static constexpr int MAGIC_SEEDS[8] = {728, 10316, 55013, 32803, 12281, 15100, 16645, 255};
    {
        std::array<BitBoard, 4096> occupancy{};
        std::array<BitBoard, 4096> reference{};
        std::mt19937_64 rng; // default seed is fine
        int32_t table_size = 0;
        int32_t rook_offset = 0;

        for (int32_t sq = A1; sq < SQUARE_COUNT; ++sq) {
            const auto mask = mb.rook_mask[sq];
            const int32_t bits = std::popcount(mask);

            // record shift
            mb.rook_shift[sq] = 64 - bits;
            mb.rook_offset[sq] = rook_offset;
            const auto attacks = mb.rook_attack_table.data() + rook_offset;

            BitBoard b = 0;
            table_size = 0;
            do {
                occupancy[table_size] = b;
                reference[table_size] = sliding_attacks_rook(static_cast<SquareIndex>(sq), b);
                ++table_size;
                b = b - mask & mask;
            } while (b);
            
            uint64_t &best_magic{mb.rook_magic[sq]};
            rng.seed(MAGIC_SEEDS[rank_of(static_cast<SquareIndex>(sq))]);

            while (true) {
                for (best_magic = 0; std::popcount((best_magic * mask) >> 56) < 6;) best_magic = rng() & rng() & rng();
                bool collision = false;
                for (int32_t i = 0; i < table_size; ++i) {
                    const auto idx = static_cast<uint32_t>(((occupancy[i] & mask) * best_magic) >> mb.rook_shift[sq]);
                    if (attacks[idx] != 0 && attacks[idx] != reference[i]) {
                        collision = true;
                        break;
                    }
                    attacks[idx] = reference[i];
                }
                if (!collision)
                    break;
                std::memset(attacks, 0, sizeof(BitBoard) * table_size); // reset attacks
            }
            rook_offset += table_size;
        }
    }
    {
        std::array<BitBoard, 4096> occupancy{};
        std::array<BitBoard, 4096> reference{};
        std::mt19937_64 rng;
        int32_t table_size = 0;
        int32_t bishop_offset = 0;
        for (int sq = A1; sq < SQUARE_COUNT; ++sq) {
            rng.seed(MAGIC_SEEDS[rank_of(static_cast<SquareIndex>(sq))]);
            const auto mask = mb.bishop_mask[sq];
            const int32_t bits = std::popcount(mask);

            mb.bishop_shift[sq] = 64 - bits;
            mb.bishop_offset[sq] = bishop_offset;
            const auto attacks = mb.bishop_attack_table.data() + bishop_offset;

            BitBoard b = 0;
            table_size = 0;
            do {
                occupancy[table_size] = b;
                reference[table_size] = sliding_attacks_bishop(static_cast<SquareIndex>(sq), b);
                ++table_size;
                b = b - mask & mask;
            } while (b);

            uint64_t &best_magic{mb.bishop_magic[sq]};
            rng.seed(MAGIC_SEEDS[rank_of(static_cast<SquareIndex>(sq))]);
            while (true) {
                for (best_magic = 0; std::popcount((best_magic * mask) >> 56) < 6;) best_magic = rng() & rng() & rng();
                bool collision = false;
                for (int32_t i = 0; i < table_size; ++i) {
                    const auto idx = static_cast<uint32_t>(((occupancy[i] & mask) * best_magic) >> mb.bishop_shift[sq]);
                    if (attacks[idx] != 0 && attacks[idx] != reference[i]) {
                        collision = true;
                        break;
                    }
                    attacks[idx] = reference[i];
                }
                if (!collision)
                    break;
                std::memset(attacks, 0, sizeof(BitBoard) * table_size); // reset attacks
            }
            bishop_offset += table_size;
        }
    }
}
void init_magic_boards(MagicBoards &mb) {
    // pawn attacks
    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        int32_t pawn_dr = (color == 0 ? +1 : -1);
        for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) { mb.pawn_attacks[color][sq] = make_attack_mask(sq, std::array{std::pair{pawn_dr, -1}, std::pair{pawn_dr, +1}}); }
    }

    for (int32_t color = 0; color < COLOR_COUNT; ++color) {
        const int32_t dr = (color == 0 ? +1 : -1);
        for (int32_t sq = 0; sq < SQUARE_COUNT; ++sq) {
            const int32_t r = sq / 8;
            const int32_t c = sq % 8;
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

        const BitBoard edges = ((Rank1 | Rank8) & ~bitboard_get_rank(static_cast<SquareIndex>(sq)) | (FileA | FileH) & ~bitboard_get_file(static_cast<SquareIndex>(sq)));
        mb.rook_mask[sq] = sliding_attacks_rook(static_cast<SquareIndex>(sq), 0) & ~edges;
        mb.bishop_mask[sq] = sliding_attacks_bishop(static_cast<SquareIndex>(sq), 0) & ~edges;
    }

    fill_sliders_magic(mb);
}
gtr::large_string print_bitboard(const BitBoard board) {
    gtr::large_string board_str;
    for (int32_t row = 7; row >= 0; --row) {
        board_str += gtr::format("%d. ", row + 1);
        for (int32_t col = 0; col < 8; ++col) {
            if (bitboard_get(board, row, col)) {
                board_str += "1 ";
            } else {
                board_str += "0 ";
            }
        }
        board_str += "\n";
    }
    static constexpr char files[] = "abcdefgh";
    board_str += "   ";
    for (int32_t col = 0; col < 8; ++col) {
        board_str += files[col];
        board_str += " ";
    }
    return board_str;
}
} // namespace game
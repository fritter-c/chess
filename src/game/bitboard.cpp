#include "bitboard.hpp"
#include "board.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "unordered_map"
namespace game {

static BitBoard sliding_attacks_rook(const SquareIndex sq, const BitBoard occ) noexcept {
    BitBoard attacks = 0;
    const int32_t r = std::to_underlying(sq) / 8;
    const int32_t c = std::to_underlying(sq) % 8;
    static constexpr std::array rook_dirs{
        std::pair{+1, 0},
        std::pair{-1, 0},
        std::pair{0, +1},
        std::pair{0, -1},
    };
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

static BitBoard sliding_attacks_bishop(const SquareIndex sq, const BitBoard occ) noexcept {
    BitBoard attacks = 0;
    const int32_t r = std::to_underlying(sq) / 8;
    const int32_t c = std::to_underlying(sq) % 8;

    static constexpr std::array bishop_dirs{
        std::pair{+1, +1},
        std::pair{+1, -1},
        std::pair{-1, +1},
        std::pair{-1, -1},
    };

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

static void fill_sliders_magic(MagicBoards &mb) {
    static constexpr std::array MAGIC_SEEDS = {728, 10316, 55013, 32803, 12281, 15100, 16645, 255};
    gtr::vector<BitBoard> rook_attack_table(0x19000, 0);
    gtr::vector<BitBoard> bishop_attack_table(0x1480, 0);
    {
        std::array<BitBoard, 4096> occupancy{};
        std::array<BitBoard, 4096> reference{};
        std::mt19937_64 rng(MAGIC_SEEDS[0]);
        int32_t table_size = 0;
        int32_t rook_offset = 0;
        // Precomputed rook magic
        mb.rook_magic = {180165019697168385ULL,  1351084561672441858ULL, 144124259317350914ULL,  4647719282254284801ULL, 4647717048846385280ULL,  7061661816492983304ULL,
                         324259722943267328ULL,  180145084633989248ULL,  577727391848742912ULL,  6936739763525263360ULL, 13835761811462291588ULL, 4644371476512896ULL,
                         576601524159906816ULL,  1226245821941547136ULL, 18577357053363201ULL,   730849916615676160ULL,  2341907540370014208ULL,  143486804312064ULL,
                         4504149920059524ULL,    4648279964964098057ULL, 4618582705179723776ULL, 6922315201891270658ULL, 145137699652112ULL,      4756084880876798979ULL,
                         176551073300488ULL,     4573969447391233ULL,    141020957376544ULL,     1152994149684346912ULL, 1689150508507184ULL,     9403518223128724480ULL,
                         3864369959572471812ULL, 144115471543959713ULL,  4611756456436301952ULL, 40532534089482320ULL,   9223407223382741002ULL,  9043483289456640ULL,
                         299207969080346624ULL,  108088592236020736ULL,  378304917829322768ULL,  9853947454318182548ULL, 612489824737181696ULL,   4612055454871240706ULL,
                         4922434547469000704ULL, 20266748347449356ULL,   2251851361779720ULL,    9259436293191368976ULL, 12970368036271030312ULL, 72216203430592532ULL,
                         72093053396976512ULL,   144173466387072ULL,     72092778695246080ULL,   148628683576320256ULL,  2308235563696128128ULL,  1130435664939009ULL,
                         18016634308461568ULL,   4683806301816440320ULL, 39547235369754881ULL,   1729663818331865089ULL, 2305851942746734849ULL,  153405103620493313ULL,
                         576742238286512133ULL,  563135979736066ULL,     4613955412614514692ULL, 9224639225096570914ULL};
        for (int32_t sq = A1; sq < SQUARE_COUNT; ++sq) {
            const auto mask = mb.rook_mask[sq];
            const int64_t bits = popcnt(mask);

            // record shift
            mb.rook_shift[sq] = 64 - static_cast<int32_t>(bits);
            mb.rook_offset[sq] = rook_offset;
            const auto attacks = rook_attack_table.data + rook_offset;

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
                while (popcnt((best_magic * mask) >> 56) < 6) best_magic = rng() & rng() & rng();
                for (int32_t i = 0; i < table_size; ++i) {
                    const auto idx = static_cast<uint32_t>(((occupancy[i] & mask) * best_magic) >> mb.rook_shift[sq]);
                    if (attacks[idx] != 0 && attacks[idx] != reference[i]) {
                        goto retry_rook;
                    }
                    attacks[idx] = reference[i];
                }
                break;
            retry_rook:
                std::memset(attacks, 0, sizeof(BitBoard) * table_size); // reset attacks
                best_magic = 0;
            }
            rook_offset += table_size;
        }
    }

    {
        std::array<BitBoard, 4096> occupancy{};
        std::array<BitBoard, 4096> reference{};
        std::mt19937_64 rng(MAGIC_SEEDS[0]);
        int32_t table_size = 0;
        int32_t bishop_offset = 0;
        for (int sq = A1; sq < SQUARE_COUNT; ++sq) {
            rng.seed(MAGIC_SEEDS[rank_of(static_cast<SquareIndex>(sq))]);
            const auto mask = mb.bishop_mask[sq];
            const int64_t bits = popcnt(mask);

            mb.bishop_shift[sq] = 64 - static_cast<int32_t>(bits);
            mb.bishop_offset[sq] = bishop_offset;
            // Precomputed bishop magic
            mb.bishop_magic = {866951793761330944ULL,  4661933617877632ULL,     581035272595047432ULL,  289431044996761600ULL,   12390533972426754ULL,   113154109003530752ULL,
                               50667729279264768ULL,   140879241158658ULL,      52845697106976ULL,      52845697106976ULL,       5649325136815104ULL,    1302991156166658ULL,
                               216458999808131074ULL,  845593832589872ULL,      2305914632358987784ULL, 72902577322369026ULL,    9241386581426970880ULL, 1129232936210496ULL,
                               4758053634489714976ULL, 5225301571876751360ULL,  2883429670283640848ULL, 4620834577949179948ULL,  4612566188357591616ULL, 288511870464688448ULL,
                               2959448796930775040ULL, 1730033305266685952ULL,  2260613288042531ULL,    290271136858257ULL,      6926681371029020672ULL, 300172210897024ULL,
                               1126177076413442ULL,    198317280248070272ULL,   18656790335980160ULL,   9223658047992367106ULL,  9368613683185336832ULL, 153124588501860480ULL,
                               3242664316654190736ULL, 581529509497751584ULL,   1315407384571612416ULL, 11817623551694356778ULL, 5260784917781520448ULL, 721781024186467392ULL,
                               4654752240710455304ULL, 13835339813860934784ULL, 37453192768512ULL,      9295713450955113489ULL,  9224594796868289824ULL, 1173207545683148840ULL,
                               167160161763488ULL,     281823138030088ULL,      9878570945347592ULL,    299205709136162ULL,      37177856496902150ULL,   61581377013768ULL,
                               74311627637326100ULL,   146386865335836672ULL,   4215721405385818112ULL, 26680405295616ULL,       577586652781301760ULL,  3035500922223723524ULL,
                               1729452900800922120ULL, 5489178154500616ULL,     9951275604095566880ULL, 614742792321237124ULL};
            const auto attacks = bishop_attack_table.data + bishop_offset;

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
                while (popcnt((best_magic * mask) >> 56) < 6) best_magic = rng() & rng() & rng();
                for (int32_t i = 0; i < table_size; ++i) {
                    const auto idx = static_cast<uint32_t>(((occupancy[i] & mask) * best_magic) >> mb.bishop_shift[sq]);
                    if (attacks[idx] != 0 && attacks[idx] != reference[i]) {
                        goto retry_bishop;
                    }
                    attacks[idx] = reference[i];
                }
                break;
            retry_bishop:
                std::memset(attacks, 0, sizeof(BitBoard) * table_size); // reset attacks
                best_magic = 0;
            }
            bishop_offset += table_size;
        }
    }
    // Compress rook unique indexes
    {
        std::unordered_map<BitBoard, gtr::vector<int32_t>> unique_rook_masks;
        for (int32_t i = 0; i < rook_attack_table.size(); ++i) {
            const auto &mbit = rook_attack_table[i];
            if (!unique_rook_masks.contains(mbit)) {
                unique_rook_masks[mbit] = gtr::vector<int32_t>();
            }
            unique_rook_masks[mbit].push_back(i);
        }
        int16_t major_index = 0;
        for (const auto &[fst, snd] : unique_rook_masks) {
            mb.rook_unique_table[major_index] = fst;
            for (const auto &index : snd) { mb.rook_unique_indexes[index] = major_index; }
            major_index++;
        }
    }

    {
        // Compress bishop unique indexes
        std::unordered_map<BitBoard, gtr::vector<int32_t>> unique_bishop_masks;
        for (int32_t i = 0; i < bishop_attack_table.size(); ++i) {
            const auto &mbit = bishop_attack_table[i];
            if (!unique_bishop_masks.contains(mbit)) {
                unique_bishop_masks[mbit] = gtr::vector<int32_t>();
            }
            unique_bishop_masks[mbit].push_back(i);
        }
        int16_t major_index = 0;
        for (const auto &[fst, snd] : unique_bishop_masks) {
            mb.bishop_unique_table[major_index] = fst;
            for (const auto &index : snd) { mb.bishop_unique_indexes[index] = major_index; }
            major_index++;
        }
    }
}
MagicBoards detail::init_magic_boards() {
    // pawn attacks
    MagicBoards mb{};
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
    return mb;
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
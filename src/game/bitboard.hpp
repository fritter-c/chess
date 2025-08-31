#pragma once
#include <os.hpp>
#include <random>
#include <string.hpp>
#include "array.hpp"
#include "types.hpp"

#if defined(_MSC_VER)
#include <immintrin.h>
#endif
namespace game {
using BitBoard = uint64_t;

inline int32_t popcnt(BitBoard bb) {
#if defined(__has_builtin)
#if __has_builtin(__builtin_popcountll)
    return __builtin_popcountll(bb);
#endif
#elif defined(_MSC_VER)
    return static_cast<int32_t>(_mm_popcnt_u64(bb));
#else
    return std::popcount(bb);
#endif
}

inline uint32_t lsb(BitBoard bb) {
#if defined(__has_builtin)
#if __has_builtin(__builtin_ctzll)
    return __builtin_ctzll(bb);
#endif
#elif defined(_MSC_VER)
    unsigned long index{0};
    _BitScanForward64(&index, bb);
    return index;
#else
    return std::countr_zero(bb);
#endif
}

static constexpr BitBoard RANK_MASK = 0xFFULL;

static constexpr BitBoard BITBOARD_FULL = 0xFFFFFFFFFFFFFFFFULL;

constexpr void bitboard_set(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit |= static_cast<BitBoard>(1) << (r * 8 + f); }

constexpr void bitboard_set(BitBoard &bit, const uint32_t sq) noexcept { bit |= static_cast<BitBoard>(1) << sq; }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit &= ~(static_cast<BitBoard>(1) << (r * 8 + f)); }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t sq) noexcept { bit &= ~(static_cast<BitBoard>(1) << sq); }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { return (bit & static_cast<BitBoard>(1) << (r * 8 + f)) != 0; }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t sq) noexcept { return (bit & static_cast<BitBoard>(1) << sq) != 0; }

inline int32_t bitboard_count(const BitBoard &bit) noexcept { return popcnt(bit); }

constexpr void bitboard_move_bit(BitBoard &b, const uint32_t from_square, const uint32_t to_square) noexcept {
    b = (b & ~(static_cast<BitBoard>(1) << from_square)) | static_cast<BitBoard>(1) << to_square;
}

template <uint32_t... Squares> constexpr BitBoard bitboard_from_squares() noexcept {
    BitBoard bit = 0;
    ((bit |= static_cast<BitBoard>(1) << Squares), ...);
    return bit;
}

template <int32_t = 0, typename... Squares> constexpr BitBoard bitboard_from_squares(Squares... s) noexcept {
    BitBoard bit = 0;
    ((bit |= static_cast<BitBoard>(1) << static_cast<uint32_t>(s)), ...);
    return bit;
}

inline uint8_t bitboard_extract_rank(const BitBoard bb, const int32_t r) noexcept { return static_cast<uint8_t>(bb >> (r * 8) & RANK_MASK); }

inline int32_t bitboard_index(const BitBoard bb) noexcept {
    Assert(bb != 0 && std::has_single_bit(bb), "bitboard_index: expected exactly one bit set");
    return lsb(bb);
}

inline BitBoard bitboard_set_bit_if_set(BitBoard bb, const SquareIndex sq, const SquareIndex dest) noexcept {
    Assert(sq < SQUARE_COUNT, "bitboard_set_bit_if_set: square index out of bounds");
    bb |= ((bb >> sq) << 1ULL) << dest;
    return bb;
}

struct BitBoardIterator {
    BitBoard bits{0};
    SquareIndex index{A1};

    explicit BitBoardIterator(const BitBoard b) : bits(b) {
        // go to the first set bit
        if (bits == 0) {
            index = SQUARE_COUNT; // end of iteration
        } else {
            index = static_cast<SquareIndex>(lsb(bits));
            bits &= bits - 1; // clear the lowest set bit
        }
    }

    SquareIndex operator*() const { return index; }

    BitBoardIterator &operator++() {
        if (bits == 0) {
            index = SQUARE_COUNT; // end of iteration
        } else {
            index = static_cast<SquareIndex>(lsb(bits));
            bits &= bits - 1; // clear the lowest set bit
        }
        return *this;
    }

    bool operator==(const BitBoardIterator &other) const = default;

    BitBoardIterator begin() const { return *this; }

    static BitBoardIterator end() { return BitBoardIterator{0}; }
};

enum BitBoardDirection : int8_t {
    BLACK_DIRECTION = 8,
    WHITE_DIRECTION = -BLACK_DIRECTION,
    RIGHT_DIRECTION = 1,
    LEFT_DIRECTION = -RIGHT_DIRECTION,
    BLACK_RIGHT_DIRECTION = BLACK_DIRECTION + RIGHT_DIRECTION,
    BLACK_LEFT_DIRECTION = BLACK_DIRECTION + LEFT_DIRECTION,
    WHITE_RIGHT_DIRECTION = WHITE_DIRECTION + RIGHT_DIRECTION,
    WHITE_LEFT_DIRECTION = WHITE_DIRECTION + LEFT_DIRECTION
};

inline int32_t RowIncrement(const Color c) {
    static constexpr gtr::array row_increments = {1, -1};
    return row_increments[std::to_underlying(c)];
}

constexpr int8_t EN_PASSANT_INVALID_INDEX{-1};

struct AvailableMoves {
    BitBoard bits{0};
    int32_t origin_index{-1};
    AvailableMoves() = default;
    explicit AvailableMoves(const int32_t origin) : origin_index(origin) {}
    void set(const int32_t row, const int32_t col) { bitboard_set(bits, row, col); }
    void clear(const int32_t row, const int32_t col) { bitboard_clear(bits, row, col); }
    bool get(const int32_t row, const int32_t col) const { return bitboard_get(bits, row, col); }
    bool get(const int32_t index) const { return get(index / 8, index % 8); }
    void reset() { bits = static_cast<BitBoard>(0); }
    int32_t move_count() const { return bitboard_count(bits); }
};

gtr::large_string print_bitboard(BitBoard board);

struct MagicBoards {
    gtr::array<gtr::array<BitBoard, SQUARE_COUNT + 1>, COLOR_COUNT> en_passant_conversion_table;
    gtr::array<gtr::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_attacks;
    gtr::array<gtr::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_moves;
    gtr::array<BitBoard, SQUARE_COUNT> knight_attacks;

    gtr::array<BitBoard, SQUARE_COUNT> king_attacks;
    gtr::array<BitBoard, 2> castle_king_empty = {{bitboard_from_squares<F1, G1>(), bitboard_from_squares<F8, G8>()}};
    gtr::array<gtr::array<SquareIndex, 2>, 2> castle_king_squares = {{{F1, G1}, {F8, G8}}};
    gtr::array<BitBoard, 2> castle_queen_empty = {{bitboard_from_squares<D1, C1, B1>(), bitboard_from_squares<D8, C8, B8>()}};
    gtr::array<gtr::array<SquareIndex, 3>, 2> castle_queen_squares = {{{D1, C1, B1}, {D8, C8, B8}}};
    gtr::array<int32_t, 2> king_row = {{0, 7}};
    gtr::array<BitBoard, 2> castle_king_dest = {{bitboard_from_squares<G1>(), bitboard_from_squares<G8>()}};
    gtr::array<BitBoard, 2> castle_queen_dest = {{bitboard_from_squares<C1>(), bitboard_from_squares<C8>()}};

    gtr::array<gtr::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_attackers;
    gtr::array<BitBoard, SQUARE_COUNT> knight_attackers;
    gtr::array<BitBoard, SQUARE_COUNT> king_attackers;

    BitBoard queen_mask(const SquareIndex sq) const noexcept { return bishop_mask[sq] | rook_mask[sq]; }

    gtr::array<BitBoard, SQUARE_COUNT> bishop_mask;
    gtr::array<uint64_t, SQUARE_COUNT> bishop_magic;
    gtr::array<uint32_t, SQUARE_COUNT> bishop_shift;
    gtr::array<uint32_t, SQUARE_COUNT> bishop_offset;
    gtr::array<uint16_t, 0x1480> bishop_unique_indexes;
    gtr::array<BitBoard, 1426> bishop_unique_table; // Why I am getting 1426 and not 1428 like in the wiki?

    gtr::array<BitBoard, SQUARE_COUNT> rook_mask;
    gtr::array<uint64_t, SQUARE_COUNT> rook_magic;
    gtr::array<uint32_t, SQUARE_COUNT> rook_shift;
    gtr::array<uint32_t, SQUARE_COUNT> rook_offset;
    gtr::array<uint16_t, 0x19000> rook_unique_indexes;
    gtr::array<BitBoard, 4900> rook_unique_table;

    template <PieceType T> BitBoard slider_attacks(BitBoard occ, const SquareIndex sq) const noexcept {
        if constexpr (T == ROOK) {
            occ &= rook_mask[sq];   // Apply the mask to the occupancy
            occ *= rook_magic[sq];  // Multiply by the magic number
            occ >>= rook_shift[sq]; // Shift to get the index
            return rook_unique_table[rook_unique_indexes[rook_offset[sq] + occ]];
        } else if constexpr (T == BISHOP) {
            occ &= bishop_mask[sq];   // Apply the mask to the occupancy
            occ *= bishop_magic[sq];  // Multiply by the magic number
            occ >>= bishop_shift[sq]; // Shift to get the index
            return bishop_unique_table[bishop_unique_indexes[bishop_offset[sq] + occ]];
        } else if constexpr (T == QUEEN) {
            return slider_attacks<ROOK>(occ, sq) | slider_attacks<BISHOP>(occ, sq);
        } else {
            Unreachable("Invalid piece type for slider attacks");
        }
    }

    template <PieceType T> BitBoard slider_attacks(const BitBoard occ, BitBoard bb) const noexcept {
        BitBoard attacks = 0;
        while (bb) {
            unsigned idx = lsb(bb);
            const BitBoard lsb = BitBoard{1} << idx;
            attacks |= slider_attacks<T>(occ, static_cast<SquareIndex>(idx));
            bb ^= lsb;
        }
        return attacks;
    }
};
namespace detail {
MagicBoards init_magic_boards();
} // namespace detail
alignas(64) extern const MagicBoards MAGIC_BOARD;

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

constexpr BitBoard bitboard_get_rank(const SquareIndex s) noexcept { return Rank1 << (8 * (s >> 3)); }

constexpr BitBoard bitboard_get_file(const SquareIndex s) noexcept { return FileA << (s & 7); }

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

static constexpr gtr::array BITBOARD_FILES = {FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH};
static constexpr gtr::array BITBOARD_RANKS = {Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8};
} // namespace game
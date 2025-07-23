#pragma once
#include <os.hpp>
#include <random>
#include <string.hpp>
#include "types.hpp"
namespace game {
using BitBoard = uint64_t;

inline int64_t popcnt(BitBoard bb) {
#if defined(_MSC_VER)
    return _mm_popcnt_u64(bb);
#elif defined(__has_builtin)
#if __has_builtin(__builtin_popcountll)
    return __builtin_popcountll(bb);
#endif
#else
    return std::popcount(bb);
#endif
}

static constexpr BitBoard RANK_MASK = 0xFFULL;

constexpr void bitboard_set(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit |= static_cast<BitBoard>(1) << (r * 8 + f); }

constexpr void bitboard_set(BitBoard &bit, const uint32_t sq) noexcept { bit |= static_cast<BitBoard>(1) << sq; }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit &= ~(static_cast<BitBoard>(1) << (r * 8 + f)); }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t sq) noexcept { bit &= ~(static_cast<BitBoard>(1) << sq); }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { return (bit & static_cast<BitBoard>(1) << (r * 8 + f)) != 0; }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t sq) noexcept { return (bit & static_cast<BitBoard>(1) << sq) != 0; }

inline int64_t bitboard_count(const BitBoard &bit) noexcept { return popcnt(bit); }

constexpr void bitboard_move_bit(BitBoard &b, const uint32_t from_square, const uint32_t to_square) noexcept {
    b = b & ~(static_cast<BitBoard>(1) << from_square) | static_cast<BitBoard>(1) << to_square;
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

inline uint8_t bitboard_extract_file(const BitBoard bb, const int32_t f) noexcept { return static_cast<uint8_t>((bb >> f) & 0x0101010101010101ULL); }

inline int32_t bitboard_index(const BitBoard bb) noexcept {
    Assert(bb != 0 && std::has_single_bit(bb), "bitboard_index: expected exactly one bit set");
    return std::countr_zero(bb);
}

inline BitBoard bitboard_set_bit_if_set(BitBoard bb,const SquareIndex sq,const SquareIndex dest) noexcept {
    Assert(sq < SQUARE_COUNT, "bitboard_set_bit_if_set: square index out of bounds");
    bb |= ((bb >> sq) << 1ULL) << dest;
    return bb;
}

struct BitBoardIterator {
    BitBoard bits;
    SquareIndex index{A1};

    explicit BitBoardIterator(BitBoard b) : bits(b) {
        // go to the first set bit
        if (bits == 0) {
            index = SQUARE_COUNT; // end of iteration
        } else {
            index = static_cast<SquareIndex>(std::countr_zero(bits));
            bits &= bits - 1; // clear the lowest set bit
        }
    }

    SquareIndex operator*() const { return index; }

    BitBoardIterator &operator++() {
        if (bits == 0) {
            index = SQUARE_COUNT; // end of iteration
        } else {
            index = static_cast<SquareIndex>(std::countr_zero(bits));
            bits &= bits - 1; // clear the lowest set bit
        }
        return *this;
    }

    bool operator==(const BitBoardIterator &other) const = default;

    static BitBoardIterator begin(BitBoard b) { return BitBoardIterator{b}; }

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
    static constexpr std::array row_increments = {1, -1};
    return row_increments[std::to_underlying(c)];
}

static constexpr std::array<BitBoard, 2> CASTLE_KING_EMPTY = {{bitboard_from_squares<F1, G1>(), bitboard_from_squares<F8, G8>()}};
static constexpr std::array<std::array<SquareIndex, 2>, 2> CASTLE_KING_SQUARES = {{{F1, G1}, {F8, G8}}};
static constexpr std::array<BitBoard, 2> CASTLE_QUEEN_EMPTY = {{bitboard_from_squares<D1, C1, B1>(), bitboard_from_squares<D8, C8, B8>()}};
static constexpr std::array<std::array<SquareIndex, 3>, 2> CASTLE_QUEEN_SQUARES = {{{D1, C1, B1}, {D8, C8, B8}}};
static constexpr std::array<int32_t, 2> KING_ROW = {{0, 7}};
static constexpr std::array<BitBoard, 2> CASTLE_KING_DEST = {{bitboard_from_squares<G1>(), bitboard_from_squares<G8>()}};
static constexpr std::array<BitBoard, 2> CASTLE_QUEEN_DEST = {{bitboard_from_squares<C1>(), bitboard_from_squares<C8>()}};

consteval std::array<std::array<BitBoard, SQUARE_COUNT + 1>, COLOR_COUNT> generate_en_passant_conversion_table() {
    std::array<std::array<BitBoard, SQUARE_COUNT + 1>, COLOR_COUNT> result{};
    for (int32_t i = 1; i <= SQUARE_COUNT; ++i) {
        // These are the index of the squares where the en passant can be applied for white pieces and black to capture
        if (i >= 17 && i <= 23) {
            result[PIECE_WHITE][i] = bitboard_from_squares(i - 1);
        }
    }
    for (int32_t i = 1; i <= SQUARE_COUNT; ++i) {
        // These are the index of the squares where the en passant can be applied for black pieces and white to capture
        if (i >= 41 && i <= 48) {
            result[PIECE_BLACK][i] = bitboard_from_squares(i - 1);
        }
    }
    return result;
}
static constexpr std::array<std::array<BitBoard, SQUARE_COUNT + 1>, COLOR_COUNT> EN_PASSANT_CONVERSION_TABLE = generate_en_passant_conversion_table();

struct AvailableMoves {
    BitBoard bits;
    int32_t origin_index;
    void set(const int32_t row, const int32_t col) { bitboard_set(bits, row, col); }
    void clear(const int32_t row, const int32_t col) { bitboard_clear(bits, row, col); }
    bool get(const int32_t row, const int32_t col) const { return bitboard_get(bits, row, col); }
    bool get(const int32_t index) const { return get(index / 8, index % 8); }
    void reset() { bits = static_cast<BitBoard>(0); }
    int64_t move_count() const { return bitboard_count(bits); }
};

gtr::large_string print_bitboard(BitBoard board);

constexpr std::byte CASTLE_NONE{0};
constexpr std::byte CASTLE_WHITE_KINGSIDE{1 << 0};
constexpr std::byte CASTLE_WHITE_QUEENSIDE{1 << 1};
constexpr std::byte CASTLE_BLACK_KINGSIDE{1 << 2};
constexpr std::byte CASTLE_BLACK_QUEENSIDE{1 << 3};
constexpr std::byte CASTLE_BLACK_ALL{CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE};
constexpr std::byte CASTLE_WHITE_ALL{CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE};
constexpr std::byte CASTLE_RIGHTS_ALL{CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE};
constexpr int8_t CASTLE_RIGHTS_COUNT{16};
struct MagicBoards {
    std::array<std::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_attacks;
    std::array<std::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_moves;
    std::array<BitBoard, SQUARE_COUNT> knight_attacks;
    std::array<BitBoard, SQUARE_COUNT> king_attacks;

    // reverse
    std::array<std::array<BitBoard, SQUARE_COUNT>, COLOR_COUNT> pawn_attackers;
    std::array<BitBoard, SQUARE_COUNT> knight_attackers;
    std::array<BitBoard, SQUARE_COUNT> king_attackers;

    BitBoard queen_mask(const SquareIndex sq) const noexcept { return bishop_mask[sq] | rook_mask[sq]; }

    std::array<BitBoard, SQUARE_COUNT> rook_mask;
    std::array<uint64_t, SQUARE_COUNT> rook_magic;
    std::array<uint32_t, SQUARE_COUNT> rook_shift;
    std::array<uint32_t, SQUARE_COUNT> rook_offset;
    std::array<uint16_t, 0x19000> rook_unique_indexes;
    std::array<BitBoard, 4900> rook_unique_table;

    std::array<BitBoard, SQUARE_COUNT> bishop_mask;
    std::array<uint64_t, SQUARE_COUNT> bishop_magic;
    std::array<uint32_t, SQUARE_COUNT> bishop_shift;
    std::array<uint32_t, SQUARE_COUNT> bishop_offset;
    std::array<uint16_t, 0x1480> bishop_unique_indexes;
    std::array<BitBoard, 1426> bishop_unique_table; // Why im getting 1426 and not 1428 like in the wiki?

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

    template<PieceType T>
    BitBoard slider_attacks(const BitBoard occ, BitBoard bb) const noexcept {
        BitBoard attacks = 0;
        while (bb) {
            unsigned idx = std::countr_zero(bb);
            const BitBoard lsb = BitBoard{1} << idx;
            attacks |= slider_attacks<T>(occ, static_cast<SquareIndex>(idx));
            bb ^= lsb;
        }
        return attacks;
    }
};

MagicBoards init_magic_boards();

extern const MagicBoards MAGIC_BOARD;



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

constexpr BitBoard bitboard_get_rank(SquareIndex s) noexcept { return Rank1 << (8 * (s >> 3)); }

constexpr BitBoard bitboard_get_file(SquareIndex s) noexcept { return FileA << (s & 7); }

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
#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include "../utils/utils.hpp"
#include "piece.hpp"

namespace game {
// clang-format off
enum SquareIndex : uint8_t {
    A1,B1,C1,D1,E1,F1,G1,H1,
    A2,B2,C2,D2,E2,F2,G2,H2,
    A3,B3,C3,D3,E3,F3,G3,H3,
    A4,B4,C4,D4,E4,F4,G4,H4,
    A5,B5,C5,D5,E5,F5,G5,H5,
    A6,B6,C6,D6,E6,F6,G6,H6,
    A7,B7,C7,D7,E7,F7,G7,H7,
    A8,B8,C8,D8,E8,F8,G8,H8,
    SQUARE_COUNT
};
// clang-format on

constexpr SquareIndex square_index(const int32_t row, const int32_t col) noexcept { return static_cast<SquareIndex>(row * 8 + col); }

enum File : uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_COUNT };

enum Rank : uint8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_COUNT };

using RankArray = std::array<Piece, static_cast<std::size_t>(std::to_underlying(FILE_COUNT))>;
using BoardArray = std::array<RankArray, static_cast<std::size_t>(std::to_underlying(RANK_COUNT))>;
// clang-format off
static constexpr BoardArray StartingPosition{{RankArray{{WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK}},
                                                 RankArray{{WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN}},
                                                 RankArray{{PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE}},
                                                 RankArray{{PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE}},
                                                 RankArray{{PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE}},
                                                 RankArray{{PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE, PIECE_NONE}},
                                                 RankArray{{BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN}},
                                                 RankArray{{BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK}}}};

using NameRank = std::array<const char *, static_cast<std::size_t>(std::to_underlying(FILE_COUNT))>;
using NameArray = std::array<NameRank, static_cast<std::size_t>(std::to_underlying(RANK_COUNT))>;

static constexpr NameArray CellNames{{NameRank{{"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"}},
                                         NameRank{{"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2"}},
                                         NameRank{{"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3"}},
                                         NameRank{{"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4"}},
                                         NameRank{{"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5"}},
                                         NameRank{{"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6"}},
                                         NameRank{{"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7"}},
                                         NameRank{{"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"}}}};

static constexpr std::array CellNamesC {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
                                        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
                                        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
                                        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
                                        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
                                        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
                                        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
                                        "a8", "b8", "c8", "d8", "e8", "f8", "g8"};
// clang-format on

constexpr int32_t square_file(const SquareIndex sq) noexcept { return static_cast<int32_t>(std::to_underlying(sq)) & 7; }

constexpr int32_t square_rank(const SquareIndex sq) noexcept { return static_cast<int32_t>(std::to_underlying(sq)) >> 3; }

inline bool squares_same_diagonal(const SquareIndex a, const SquareIndex b) noexcept {
    return utils::abs(square_file(a) - square_file(b)) == utils::abs(square_rank(a) - square_rank(b));
}

constexpr bool squares_same_main_diagonal(const SquareIndex a, const SquareIndex b) noexcept { return square_file(a) - square_rank(a) == square_file(b) - square_rank(b); }

constexpr bool squares_same_anti_diagonal(const SquareIndex a, const SquareIndex b) noexcept { return square_file(a) + square_rank(a) == square_file(b) + square_rank(b); }

using BitBoard = uint64_t;

static constexpr BitBoard RANK_MASK = 0xFFULL;

constexpr void bitboard_set(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit |= static_cast<BitBoard>(1) << (r * 8 + f); }

constexpr void bitboard_set(BitBoard &bit, const uint32_t sq) noexcept { bit |= static_cast<BitBoard>(1) << sq; }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { bit &= ~(static_cast<BitBoard>(1) << (r * 8 + f)); }

constexpr void bitboard_clear(BitBoard &bit, const uint32_t sq) noexcept { bit &= ~(static_cast<BitBoard>(1) << sq); }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t r, const uint32_t f) noexcept { return (bit & static_cast<BitBoard>(1) << (r * 8 + f)) != 0; }

constexpr bool bitboard_get(const BitBoard &bit, const uint32_t sq) noexcept { return (bit & static_cast<BitBoard>(1) << sq) != 0; }

constexpr int32_t bitboard_count(const BitBoard &bit) noexcept { return std::popcount(bit); }

constexpr void bitboard_move_bit(BitBoard &b, const uint32_t from_square, const uint32_t to_square) noexcept {
    b = b & ~(static_cast<BitBoard>(1) << from_square) | static_cast<BitBoard>(1) << to_square;
}

template <uint32_t... Squares> constexpr BitBoard bitboard_from_squares() noexcept {
    BitBoard bit = 0;
    ((bit |= static_cast<BitBoard>(1) << Squares), ...);
    return bit;
}

template <int = 0, typename... Squares> constexpr BitBoard bitboard_from_squares(Squares... s) noexcept {
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
static constexpr std::array<BitBoard, 2> CASTLE_QUEEN_EMPTY = {{bitboard_from_squares<D1, C1, B1>(), bitboard_from_squares<D8, C8, B8>()}};
static constexpr std::array<int32_t, 2> KING_ROW = {{0, 7}};
static constexpr std::array<BitBoard, 2> CASTLE_KING_DEST = {{bitboard_from_squares<G1>(), bitboard_from_squares<G8>()}};
static constexpr std::array<BitBoard, 2> CASTLE_QUEEN_DEST = {{bitboard_from_squares<C1>(), bitboard_from_squares<C8>()}};
struct AvailableMoves {
    BitBoard bits;
    int32_t origin_index;
    void set(const int32_t row, const int32_t col) { bitboard_set(bits, row, col); }
    void clear(const int32_t row, const int32_t col) { bitboard_clear(bits, row, col); }
    bool get(const int32_t row, const int32_t col) const { return bitboard_get(bits, row, col); }
    bool get(const int32_t index) const { return get(index / 8, index % 8); }
    void reset() { bits = static_cast<BitBoard>(0); }
    int32_t move_count() const { return bitboard_count(bits); }
};

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

    BitBoard queen_attacks(const SquareIndex sq) const noexcept { return bishop_attacks[sq] | rook_attacks[sq]; }

    std::array<BitBoard, SQUARE_COUNT> rook_attacks;
    std::array<BitBoard, SQUARE_COUNT> bishop_attacks;
};

} // namespace game

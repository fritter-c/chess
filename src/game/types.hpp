#pragma once
#include <array>
#include "math.hpp"
#include <cstdint>
#include <utility>
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
    SQUARE_COUNT, OUT_OF_BOUNDS = SQUARE_COUNT,
};
// clang-format on

constexpr SquareIndex square_index(const int32_t row, const int32_t col) noexcept { return static_cast<SquareIndex>(row * 8 + col); }

enum File : uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_COUNT };

enum Rank : uint8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_COUNT };


constexpr Rank rank_of (const SquareIndex sq) noexcept {
    return static_cast<Rank>(std::to_underlying(sq) >> 3);
}

constexpr File file_of (const SquareIndex sq) noexcept {
    return static_cast<File>(std::to_underlying(sq) & 7);
}

constexpr File file_of (const char f) noexcept {
    return static_cast<File>(f - 'a');
}

constexpr Rank rank_of (const char r) noexcept {
    return static_cast<Rank>(r - '1');
}

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
    return gtr::abs(square_file(a) - square_file(b)) == gtr::abs(square_rank(a) - square_rank(b));
}

constexpr bool squares_same_main_diagonal(const SquareIndex a, const SquareIndex b) noexcept { return square_file(a) - square_rank(a) == square_file(b) - square_rank(b); }

constexpr bool squares_same_anti_diagonal(const SquareIndex a, const SquareIndex b) noexcept { return square_file(a) + square_rank(a) == square_file(b) + square_rank(b); }


constexpr std::byte CASTLE_NONE{0};
constexpr std::byte CASTLE_WHITE_KINGSIDE{1 << 0};
constexpr std::byte CASTLE_WHITE_QUEENSIDE{1 << 1};
constexpr std::byte CASTLE_BLACK_KINGSIDE{1 << 2};
constexpr std::byte CASTLE_BLACK_QUEENSIDE{1 << 3};
constexpr std::byte CASTLE_BLACK_ALL{CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE};
constexpr std::byte CASTLE_WHITE_ALL{CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE};
constexpr std::byte CASTLE_RIGHTS_ALL{CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE};
constexpr int8_t CASTLE_RIGHTS_COUNT{16};

} // namespace gam

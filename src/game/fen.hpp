#ifndef FEN_HPP
#define FEN_HPP
#include <array>
#include <cstdint>
#include <string.hpp>
#include "piece.hpp"
#include "types.hpp"
namespace game {
struct Fen : gtr::char_string<128> {
    static constexpr auto FEN_START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::array<uint8_t, 5> fields_index{44, 46, 51, 53, 55}; ///< Size of each field in FEN string

    Color turn() const { return at(fields_index[0]) == 'w' ? PIECE_WHITE : PIECE_BLACK; }
    std::byte castle_rights() const;
    int32_t halfmove_clock() const { return substr(fields_index[3], fields_index[4] - 1).to_int(); }
    int32_t fullmove_number() const { return substr(fields_index[4]).to_int(); }
    SquareIndex en_passant_square() const;
    Piece piece_at(SquareIndex s) const;

    void reset() { set_fen(FEN_START); }
    bool set_fen(const char *fen);
    static Fen build(const std::array<Piece, SQUARE_COUNT>& pieces, Color t, std::byte rights, int8_t index, int32_t halfmove, int32_t fullmove);
};

} // namespace game
#endif
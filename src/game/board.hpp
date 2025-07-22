#pragma once
#include <array>
#include <cstdint>
#include "history.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "types.hpp"
#include "bitboard.hpp"
namespace game {
struct BoardState {
    std::byte castle_rights;
    int8_t en_passant_index;
    Piece captured_piece;
    Piece moved_piece;
    Move last_move;
    BitBoard castle_rights_bit;
};

struct Board {
    std::array<Piece, SQUARE_COUNT> pieces{};
    std::array<BitBoard, PIECE_COUNT_PLUS_ANY> pieces_by_type{};
    std::array<BitBoard, COLOR_COUNT> pieces_by_color{};
    history<BoardState> state_history;
    BoardState* current_state{nullptr};

    Board() { init(); }

    Piece &operator[](const int32_t index) { return pieces[index]; }

    const Piece &operator[](const int32_t index) const { return pieces[index]; }

    auto begin() { return pieces.begin(); }

    auto end() { return pieces.end(); }

    void init();

    constexpr int32_t get_piece_count(const Color color) const { return bitboard_count(pieces_by_color[color]); }

    void populate();

    void populate_bitboards();
    

    static constexpr int32_t get_index(const int32_t row, const int32_t col) { return row * 8 + col; }

    static constexpr SquareIndex square_index(const int32_t row, const int32_t col) { return static_cast<SquareIndex>(get_index(row, col)); }

    static constexpr int32_t get_row(const int32_t index) { return index / 8; }

    static constexpr int32_t get_col(const int32_t index) { return index % 8; }

    void move(Move m);
    void move(Move m, AlgebraicMove &out_alg);
    void move_stateless(Move m, BoardState &state);

    bool undo();
    bool undo_stateless(const BoardState &state);

    bool redo();

    static constexpr bool valid_rol_col(const int32_t row, const int32_t col) { return row >= RANK_1 && row <= RANK_7 && col >= FILE_A && col <= FILE_H; }

    bool pawn_is_being_promoted(const SimpleMove move) const {
        static constexpr std::array<BitBoard, 15> pawn_promotion_bitboards = {0, bitboard_from_squares<A8>(), 0, 0, 0, 0, 0, 0, 0, bitboard_from_squares<A1>(), 0, 0, 0, 0, 0};
        return bitboard_get(pawn_promotion_bitboards[pieces[get_index(move.from_row, move.from_col)]], square_index(move.to_row, 0));
    }

    Color get_color(const int32_t row, const int32_t col) const { return PIECE_COLOR(pieces[Board::get_index(row, col)]); }

    Color get_color(const int32_t index) const { return PIECE_COLOR(pieces[index]); }

    bool is_en_passant(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col) const;

    gtr::large_string board_to_string() const;

    constexpr void move_piece(const SquareIndex origin, const SquareIndex destination) {
        const Piece p = pieces[origin];
        pieces[destination] = pieces[origin];
        pieces[origin] = PIECE_NONE;
        bitboard_move_bit(pieces_by_type[PIECE_TYPE(p)], origin, destination);
        bitboard_move_bit(pieces_by_color[PIECE_COLOR(p)], origin, destination);
        bitboard_move_bit(pieces_by_type[ANY], origin, destination);
        bitboard_move_bit(pieces_by_type[EMPTY], destination, origin);
    }

    constexpr void move_piece(const int32_t row, const int32_t col, const int32_t to_row, const int32_t to_col) {
        move_piece(static_cast<SquareIndex>(get_index(row, col)), static_cast<SquareIndex>(get_index(to_row, to_col)));
    }

    constexpr void remove_piece(const SquareIndex index) {
        const Piece piece = pieces[index];
        Assert(PIECE_TYPE(piece) != EMPTY, "Attempting to remove empty square");
        pieces[index] = PIECE_NONE;
        bitboard_clear(pieces_by_type[PIECE_TYPE(piece)], index);
        bitboard_clear(pieces_by_color[PIECE_COLOR(piece)], index);
        bitboard_clear(pieces_by_type[ANY], index);
        bitboard_set(pieces_by_type[EMPTY], index);
    }

    constexpr void remove_piece(const int32_t row, const int32_t col) { remove_piece(static_cast<SquareIndex>(get_index(row, col))); }

    constexpr void put_piece(const Piece p, const SquareIndex s) {
        Assert(PIECE_TYPE(p) != EMPTY, "Use remove piece");
        pieces[s] = p;
        bitboard_set(pieces_by_type[PIECE_TYPE(p)], s);
        bitboard_set(pieces_by_color[PIECE_COLOR(p)], s);
        bitboard_set(pieces_by_type[ANY], s);
        bitboard_clear(pieces_by_type[EMPTY], s);
    }
    static gtr::large_string print_bitboard(BitBoard board);

    template <PieceType T, Color C> 
    constexpr BitBoard get_piece_bitboard() const {
        return pieces_by_type[T] & pieces_by_color[C];
    }

    constexpr BitBoard get_piece_bitboard(const PieceType t, const Color c) const { return pieces_by_type[t] & pieces_by_color[c]; }
};
} // namespace game

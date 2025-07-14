#pragma once
#include <array>
#include <cstdint>
#include "history.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "types.hpp"

namespace game {
struct BoardState {
    std::byte castle_rights;
    int8_t en_passant_index;
    Piece captured_piece;
    Piece moved_piece;
    Move last_move;
};

struct Board {
    std::array<Piece, SQUARE_COUNT> pieces{};
    std::array<BitBoard, PIECE_COUNT_PLUS_ANY> pieces_by_type{};
    std::array<BitBoard, COLOR_COUNT> pieces_by_color{};
    history<BoardState> state_history;

    Board() { init(); }
    BoardState &current_state() { return *state_history.current(); }

    const BoardState &current_state() const { return *state_history.current(); }

    Piece &operator[](const int32_t index) { return pieces[index]; }

    const Piece &operator[](const int32_t index) const { return pieces[index]; }

    auto begin() { return pieces.begin(); }

    auto end() { return pieces.end(); }

    void init();

    constexpr int32_t get_piece_count(const Color color) const { return bitboard_count(pieces_by_color[color]); }

    void populate();

    void populate_bitboards();

    BitBoard pieces_type(const PieceType t) const { return pieces_by_type[t]; }

    BitBoard pieces_color(const Color c) const { return pieces_by_color[c]; }

    static constexpr int32_t get_index(const int32_t row, const int32_t col) { return row * 8 + col; }

    static constexpr int32_t get_row(const int32_t index) { return index / 8; }

    static constexpr int32_t get_col(const int32_t index) { return index % 8; }

    static bool pawn_first_move(const Piece piece, const int32_t row) {
        static constexpr std::array pawn_starting_pos_col_0 = {bitboard_from_squares<A2>(), bitboard_from_squares<A7>()};
        return bitboard_get(pawn_starting_pos_col_0[PIECE_COLOR(piece)], row, 0);
    }

    void move(Move m);
    void move(Move m, AlgebraicMove &out_alg);
    void move_stateless(Move m, BoardState &state);

    bool undo();
    bool undo_stateless(const BoardState &state);

    bool redo();

    static constexpr bool valid_rol_col(const int32_t row, const int32_t col) { return row >= RANK_1 && row <= RANK_7 && col >= FILE_A && col <= FILE_H; }

    bool can_en_passant_this(const int32_t row, const int32_t col, const Color enemy) const {
        return valid_rol_col(row, col) && PIECE_TYPE(pieces[get_index(row, col)]) == PAWN && PIECE_COLOR(pieces[get_index(row, col)]) == enemy &&
               get_index(row, col) == current_state().en_passant_index;
    }

    bool castle_rights_for(const Color color, const bool kingside) const {
        if (color == PIECE_WHITE) {
            return kingside ? std::to_integer<int>(current_state().castle_rights & CASTLE_WHITE_KINGSIDE) != 0
                            : std::to_integer<int>(current_state().castle_rights & CASTLE_WHITE_QUEENSIDE) != 0;
        }
        return kingside ? std::to_integer<int>(current_state().castle_rights & CASTLE_BLACK_KINGSIDE) != 0
                        : std::to_integer<int>(current_state().castle_rights & CASTLE_BLACK_QUEENSIDE) != 0;
    }

    constexpr bool pawn_is_being_promoted(const SimpleMove move) const {
        static constexpr std::array<BitBoard, 15> pawn_promotion_bitboards = {0, bitboard_from_squares<A8>(), 0, 0, 0, 0, 0, 0, 0, bitboard_from_squares<A1>(), 0, 0, 0, 0, 0};
        return bitboard_get(pawn_promotion_bitboards[pieces[get_index(move.from_row, move.from_col)]], square_index(move.to_row, 0));
    }

    Color get_color(const int32_t row, const int32_t col) const { return PIECE_COLOR(pieces[Board::get_index(row, col)]); }

    Color get_color(const int32_t index) const { return PIECE_COLOR(pieces[index]); }

    bool is_en_passant(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col) const;

    gtr::char_string<128> board_to_string() const;

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
    static gtr::char_string<128> print_bitboard(BitBoard board);
};
} // namespace game

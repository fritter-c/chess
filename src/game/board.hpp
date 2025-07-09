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
    std::array<BitBoard, PIECE_COUNT_PLUS_ANY> pieces_by_type{};//not used yet
    std::array<BitBoard, COLOR_COUNT> pieces_by_color{}; // not used yet;
    history<BoardState> state_history;

    Board() { init(); }

    BoardState &current_state() { return *state_history.current(); }

    const BoardState &current_state() const { return *state_history.current(); }

    Piece &operator[](const int32_t index) { return pieces[index]; }

    const Piece &operator[](const int32_t index) const { return pieces[index]; }

    auto begin() { return pieces.begin(); }

    auto end() { return pieces.end(); }

    void init() {
        state_history.push({});
        current_state().castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
        current_state().en_passant_index = -1;
    }

    void reset() {
        populate();
        state_history.clear();
        state_history.push({});
        current_state().castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
        current_state().en_passant_index = -1;
    }

    constexpr int32_t get_piece_count(const Color color) const {
        int32_t count = 0;
        for (const auto &piece : pieces) {
            if (PIECE_TYPE(piece) != EMPTY && PIECE_COLOR(piece) == color) {
                ++count;
            }
        }
        return count;
    }

    void populate();

    static constexpr int32_t get_index(const int32_t row, const int32_t col) { return row * 8 + col; }

    static constexpr int32_t get_row(const int32_t index) { return index / 8; }

    static constexpr int32_t get_col(const int32_t index) { return index % 8; }

    static bool pawn_first_move(const Piece piece, const int32_t row) {
        return (PIECE_COLOR(piece) == PIECE_WHITE && row == RANK_2) || (PIECE_COLOR(piece) == PIECE_BLACK && row == RANK_7);
    }

    bool move(Move m);

    bool move(Move m, AlgebraicMove& out_alg);

    bool undo_move(Move move);

    bool undo_last_move() {
        if (current_state().last_move != Move()) {
            return undo_move(current_state().last_move);
        }
        return false;
    }

    void move_no_check(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);

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
        const auto piece = pieces[get_index(move.from_row, move.from_col)];
        if (IS_PAWN(piece)) {
            if (move.from_row == 6 && move.to_row == 7) {
                return IS_WHITE(piece);
            }
            if (move.from_row == 1 && move.to_row == 0) {
                return IS_BLACK(piece);
            }
        }
        return false;
    }

    Color get_color(const int32_t row, const int32_t col) const { return PIECE_COLOR(pieces[Board::get_index(row, col)]); }

    Color get_color(const int32_t index) const { return PIECE_COLOR(pieces[index]); }

    int32_t piece_count(const Color c) const {
        int32_t result{0};
        for (const auto p : pieces) {
            if (PIECE_COLOR(p) == c) {
                ++result;
            }
        }
        return result;
    }

    bool is_en_passant(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col) const;

    gtr::char_string<256> board_to_string() const;
};
} // namespace game

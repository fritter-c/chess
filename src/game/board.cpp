#include "board.hpp"
#include <cmath>
#include "types.hpp"

namespace game {
void Board::populate() { std::memcpy(pieces.data(), StartingPosition.data(), sizeof(pieces)); }

static bool board_can_move_basic(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    // Check if the move is to another cell
    if (from_row == to_row && from_col == to_col) {
        return false; // No move
    }

    // Check if the target cell is empty or occupied by an opponent's piece
    const int32_t from_index = Board::get_index(from_row, from_col);
    const int32_t to_index = Board::get_index(to_row, to_col);
    const Piece from_piece = board->pieces[from_index];
    if (const Piece to_piece = board->pieces[to_index]; PIECE_TYPE(to_piece) != EMPTY && PIECE_COLOR(to_piece) == PIECE_COLOR(from_piece)) {
        return false; // Cannot capture own piece
    }

    if (PIECE_TYPE(from_piece) == EMPTY) {
        return false; // No piece to move
    }

    return true;
}

static bool board_can_move_basic(const Board *board, uint8_t from_index, uint8_t to_index) {
    const int32_t from_row = Board::get_row(from_index);
    const int32_t from_col = Board::get_col(from_index);
    const int32_t to_row = Board::get_row(to_index);
    const int32_t to_col = Board::get_col(to_index);
    return board_can_move_basic(board, from_row, from_col, to_row, to_col);
}

static bool board_is_castle(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    (void)to_row;
    if (const Piece from_piece = board->pieces[Board::get_index(from_row, from_col)]; PIECE_TYPE(from_piece) == KING && std::abs(from_col - to_col) > 1) {
        return true;
    }
    return false;
}

bool Board::is_en_passant(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) const {
    (void)to_row;
    if (const Piece to_piece = pieces[get_index(to_row, to_col)]; PIECE_TYPE(to_piece) != EMPTY) {
        return false;
    }
    if (const Piece from_piece = pieces[get_index(from_row, from_col)]; PIECE_TYPE(from_piece) == PAWN && from_col != to_col) {
        return true;
    }
    return false;
}

static void board_do_castle(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    Piece const &king = board->pieces[Board::get_index(from_row, from_col)];
    Piece &rook = board->pieces[Board::get_index(from_row, from_col - to_col > 0 ? 0 : 7)];

    board->pieces[Board::get_index(from_row, from_col - to_col > 0 ? 3 : 5)] = rook;
    rook = PIECE_NONE; // Remove the rook from the original position

    board->pieces[Board::get_index(to_row, to_col)] = king;
    board->pieces[Board::get_index(from_row, from_col)] = PIECE_NONE; // Remove the king from the original position
}

static bool on_corner(const int32_t row, const int32_t col) {
    static constexpr BitBoard corner_bitboard = bitboard_from_squares<A1, H1, A8, H8>();
    return bitboard_get(corner_bitboard, row, col);
}

static void board_undo_castle(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    const int queen_side = static_cast<int>(from_col - to_col > 0);
    static constexpr std::array rook_orig_col = {7, 0};
    static constexpr std::array rook_castled_col = {5, 3};
    Piece const &king = board->pieces[Board::get_index(to_row, to_col)];
    Piece const &rook = board->pieces[Board::get_index(from_row, rook_castled_col[queen_side])];
    board->pieces[Board::get_index(from_row, from_col)] = king;
    board->pieces[Board::get_index(to_row, to_col)] = PIECE_NONE;
    board->pieces[Board::get_index(from_row, rook_orig_col[queen_side])] = rook;
    board->pieces[Board::get_index(from_row, rook_castled_col[queen_side])] = PIECE_NONE;
}

static void update_rights(BoardState &state, Piece piece, const int32_t piece_row, const int32_t piece_col) {
    static constexpr std::array all_rights{~CASTLE_WHITE_ALL, ~CASTLE_BLACK_ALL};
    static constexpr std::byte side_rights[2][2]{{~CASTLE_WHITE_KINGSIDE, ~CASTLE_BLACK_KINGSIDE}, {~CASTLE_WHITE_QUEENSIDE, ~CASTLE_BLACK_QUEENSIDE}};
    switch (PIECE_TYPE(piece)) {
    case KING: state.castle_rights &= all_rights[PIECE_COLOR(piece)]; break;
    case ROOK:
        if (on_corner(piece_row, piece_col)) {
            state.castle_rights &= side_rights[piece_col == 0][PIECE_COLOR(piece)];
        }
        break;
    default: break;
    }
}

static void board_do_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col, BoardState &state) {
    Piece const &from_piece = board->pieces[Board::get_index(from_row, from_col)];
    state.en_passant_index = -1; // Reset en passant index
    state.moved_piece = from_piece;

    if (PIECE_TYPE(from_piece) == PAWN && utils::abs(from_row - to_row) == 2) {
        state.en_passant_index = static_cast<int8_t>(Board::get_index(to_row, to_col));
    }

    update_rights(state, from_piece, from_row, from_col);

    if (board->is_en_passant(from_row, from_col, to_row, to_col)) {
        // Remove the captured pawn
        const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? to_row - 1 : to_row + 1;
        const int32_t captured_col = to_col;
        state.captured_piece = board->pieces[Board::get_index(captured_row, captured_col)];
        board->pieces[Board::get_index(captured_row, captured_col)] = PIECE_NONE;
    } else {
        state.captured_piece = board->pieces[Board::get_index(to_row, to_col)];
    }

    board->pieces[Board::get_index(to_row, to_col)] = from_piece;
    board->pieces[Board::get_index(from_row, from_col)] = PIECE_NONE;
}

static void apply_move(Board &board, const Move move, BoardState &state) {
    if (board_can_move_basic(&board, move.get_origin(), move.get_destination())) {
        const int32_t from_row = Board::get_row(move.get_origin());
        const int32_t from_col = Board::get_col(move.get_origin());
        const int32_t to_row = Board::get_row(move.get_destination());
        const int32_t to_col = Board::get_col(move.get_destination());
        if (move.get_special() == Move::MOVE_CASTLE) {
            board_do_castle(&board, from_row, from_col, to_row, to_col);
        } else if (move.get_special() == Move::MOVE_PROMOTION) {
            if (move.get_special() == Move::MOVE_PROMOTION) {
                state.moved_piece = board.pieces[Board::get_index(from_row, from_col)];
                state.captured_piece = board.pieces[Board::get_index(to_row, to_col)];

                board.pieces[Board::get_index(to_row, to_col)] = chess_piece_make(move.get_promotion_piece_type(), PIECE_COLOR(board.pieces[Board::get_index(from_row, from_col)]));

                // Remove the pawn from the original position
                board.pieces[Board::get_index(from_row, from_col)] = PIECE_NONE;
            }
        } else {
            board_do_move(&board, from_row, from_col, to_row, to_col, state);
        }
    }
}

bool Board::move_stateless(Move m, BoardState &state) {
    if (board_can_move_basic(this, m.get_origin(), m.get_destination())) {
        state.last_move = m;
        apply_move(*this, m, state);
        return true;
    }
    return false;
}

bool Board::move(const Move move) {
    if (board_can_move_basic(this, move.get_origin(), move.get_destination())) {
        state_history.push(current_state());
        current_state().last_move = move;
        apply_move(*this, move, current_state());
        return true;
    }
    return false;
}

bool Board::move(const Move m, AlgebraicMove &out_alg) {
    out_alg = move_to_algebraic(*this, m);
    return move(m);
}

bool Board::redo() {
    if (state_history.empty()) {
        return false; // No moves to redo
    }
    if (state_history.read_index + 1 >= state_history.data.size()) {
        return false; // No more moves to redo
    }

    Move &move = state_history.data[state_history.read_index + 1].last_move;
    if (board_can_move_basic(this, move.get_origin(), move.get_destination())) {
        BoardState dummy{};
        apply_move(*this, move, dummy);
        return state_history.redo();
    }

    return false; // Cannot redo the move, as it is not valid
}

static bool do_undo(Board &board, Move &move, BoardState &state) {
    if (move != Move()) {
        const int32_t from_row = Board::get_row(move.get_origin());
        const int32_t from_col = Board::get_col(move.get_origin());
        const int32_t to_row = Board::get_row(move.get_destination());
        const int32_t to_col = Board::get_col(move.get_destination());
        if (move.get_special() == Move::MOVE_CASTLE) {
            board_undo_castle(&board, from_row, from_col, to_row, to_col);
        } else {
            board.pieces[Board::get_index(from_row, from_col)] = state.moved_piece;
            if (move.get_special() == Move::MOVE_EN_PASSANT) {
                board.pieces[Board::get_index(to_row + ((PIECE_COLOR(state.moved_piece) == PIECE_WHITE) ? -1 : +1), to_col)] = state.captured_piece;
                board.pieces[Board::get_index(to_row, to_col)] = PIECE_NONE;
            } else {
                board.pieces[Board::get_index(to_row, to_col)] = state.captured_piece;
            }
        }
        return true;
    }
    return false;
}

bool Board::undo() {
    Move &move = current_state().last_move;
    if (do_undo(*this, move, current_state())) {
        state_history.undo();
        return true;
    }
    return false;
}

bool Board::undo_stateless(BoardState &state) {
    Move move = state.last_move;
    return do_undo(*this, move, state);
}

gtr::char_string<128> Board::board_to_string() const {
    gtr::char_string<128> board_str;
    for (int32_t row = 7; row >= 0; --row) {
        for (int32_t col = 0; col < 8; ++col) { board_str += piece_to_string_short(pieces[get_index(row, col)]); }
        board_str += "\n";
    }
    return board_str;
}
} // namespace game

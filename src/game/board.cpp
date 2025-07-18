#include "board.hpp"
#include <cmath>
#include "types.hpp"

namespace game {
void Board::populate() {
    static_assert(sizeof(pieces) == sizeof(StartingPosition));
    std::memcpy(pieces.data(), StartingPosition.data(), pieces.size());
}

void Board::populate_bitboards() {
    std::memset(&pieces_by_type, 0, sizeof(pieces_by_type));
    std::memset(&pieces_by_color, 0, sizeof(pieces_by_color));
    for (int32_t i = 0; i < SQUARE_COUNT; ++i) {
        if (PIECE_TYPE(pieces[i]) != EMPTY) {
            bitboard_set(pieces_by_type[ANY], i);
            bitboard_set(pieces_by_type[PIECE_TYPE(pieces[i])], i);
            bitboard_set(pieces_by_color[PIECE_COLOR(pieces[i])], i);
        } else {
            bitboard_set(pieces_by_type[EMPTY], i);
        }
    }
}

void Board::init() {
    populate();
    populate_bitboards();
    state_history.clear();
    state_history.push({});
    current_state().castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
    current_state().en_passant_index = -1;
    current_state().castle_rights_bit = bitboard_from_squares<G1, G8, C1, C8>();
}

static bool board_can_move_basic(const Board *board, const uint8_t from_index, const uint8_t to_index) {
    if (from_index == to_index) {
        return false;
    }

    const Piece from_piece = board->pieces[from_index];
    if (const Piece to_piece = board->pieces[to_index]; PIECE_TYPE(to_piece) != EMPTY && PIECE_COLOR(to_piece) == PIECE_COLOR(from_piece)) {
        return false;
    }

    if (PIECE_TYPE(from_piece) == EMPTY) {
        return false;
    }

    return true;
}

bool Board::is_en_passant(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) const {
    return PIECE_TYPE(pieces[get_index(from_row, from_col)]) == PAWN && PIECE_TYPE(pieces[get_index(to_row, to_col)]) == EMPTY && to_col != from_col;
}

static void board_do_castle(Board *board, const Move move) {
    const auto queen_side = static_cast<int>(move.from_col() - move.to_col() > 0);
    static constexpr std::array rook_orig_col = {7, 0};
    static constexpr std::array rook_castled_col = {5, 3};
    board->move_piece(move.from_row(), rook_orig_col[queen_side], move.from_row(), rook_castled_col[queen_side]);
    board->move_piece(move.get_origin_index(), move.get_destination_index());
}

static bool on_corner(const int32_t row, const int32_t col) {
    static constexpr BitBoard corner_bitboard = bitboard_from_squares<A1, H1, A8, H8>();
    return bitboard_get(corner_bitboard, row, col);
}

static void board_undo_castle(Board *board, const Move move) {
    const auto queen_side = static_cast<int>(move.from_col() - move.to_col() > 0);
    static constexpr std::array rook_orig_col = {7, 0};
    static constexpr std::array rook_castled_col = {5, 3};
    board->move_piece(move.get_destination_index(), move.get_origin_index());
    board->move_piece(move.from_row(), rook_castled_col[queen_side], move.from_row(), rook_orig_col[queen_side]);
}

static void update_rights(BoardState &state, Piece piece, const int32_t piece_row, const int32_t piece_col) {
    static constexpr std::array all_rights{~CASTLE_WHITE_ALL, ~CASTLE_BLACK_ALL};
    static constexpr std::array all_rights_bit{~bitboard_from_squares<G1, C1>(), ~bitboard_from_squares<G8, C8>()};
    static constexpr std::array side_rights{std::array{~CASTLE_WHITE_KINGSIDE, ~CASTLE_BLACK_KINGSIDE}, std::array{~CASTLE_WHITE_QUEENSIDE, ~CASTLE_BLACK_QUEENSIDE}};
    static constexpr std::array<std::array<BitBoard, 2>, COLOR_COUNT> side_rights_bit{
        std::array{~bitboard_from_squares<G1>(), ~bitboard_from_squares<C1>()}, // White
        std::array{~bitboard_from_squares<G8>(), ~bitboard_from_squares<C8>()}  // Black
    };

    switch (PIECE_TYPE(piece)) {
    case KING:
        state.castle_rights &= all_rights[PIECE_COLOR(piece)];
        state.castle_rights_bit &= all_rights_bit[PIECE_COLOR(piece)];
        break;
    case ROOK:
        if (on_corner(piece_row, piece_col)) {
            state.castle_rights &= side_rights[piece_col == 0][PIECE_COLOR(piece)];
            state.castle_rights_bit &= side_rights_bit[piece_col == 0][PIECE_COLOR(piece)];
        }
        break;
    default: break;
    }
}

static void board_do_move(Board *board, const Move move, BoardState &state) {
    Piece const &from_piece = board->pieces[move.get_origin()];
    state.en_passant_index = -1; // Reset en passant index
    state.moved_piece = from_piece;

    if (PIECE_TYPE(from_piece) == PAWN && utils::abs(move.from_row() - move.to_row()) == 2) {
        state.en_passant_index = static_cast<int8_t>(static_cast<int8_t>(move.get_destination()) + (IS_WHITE(from_piece) ? WHITE_DIRECTION : BLACK_DIRECTION));
    }

    update_rights(state, from_piece, move.from_row(), move.from_col());

    if (move.is_en_passant()) {
        // Remove the captured pawn
        const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? move.to_row() - 1 : move.to_row() + 1;
        const int32_t captured_col = move.to_col();
        state.captured_piece = board->pieces[Board::get_index(captured_row, captured_col)];
        board->remove_piece(captured_row, captured_col);
    } else {
        state.captured_piece = board->pieces[Board::get_index(move.to_row(), move.to_col())];
        if (PIECE_TYPE(state.captured_piece) != EMPTY) {
            board->remove_piece(move.to_row(), move.to_col());
        }
    }

    board->move_piece(move.get_origin_index(), move.get_destination_index());
}

static void apply_move(Board &board, const Move move, BoardState &state) {
    Assert(board_can_move_basic(&board, move.get_origin(), move.get_destination()), "Invalid move");

    if (move.get_special() == Move::MOVE_CASTLE) {
        board_do_castle(&board, move);
    } else if (move.get_special() == Move::MOVE_PROMOTION) {
        if (move.get_special() == Move::MOVE_PROMOTION) {
            state.moved_piece = board.pieces[move.get_origin()];
            state.captured_piece = board.pieces[move.get_destination()];
            board.put_piece(chess_piece_make(move.get_promotion_piece_type(), PIECE_COLOR(board.pieces[move.get_origin()])), move.get_destination_index());
            board.remove_piece(move.get_origin_index());
        }
    } else {
        board_do_move(&board, move, state);
    }
}

void Board::move_stateless(Move m, BoardState &state) {
    Assert(board_can_move_basic(this, m.get_origin(), m.get_destination()), "Invalid move");
    state.last_move = m;
    apply_move(*this, m, state);
}

void Board::move(const Move move) {
    Assert(board_can_move_basic(this, move.get_origin(), move.get_destination()), "Invalid move");
    state_history.push(current_state());
    current_state().last_move = move;
    apply_move(*this, move, current_state());
}

void Board::move(const Move m, AlgebraicMove &out_alg) {
    out_alg = move_to_algebraic(*this, m);
    move(m);
}

bool Board::redo() {
    if (state_history.empty()) {
        return false; // No moves to redo
    }
    if (state_history.read_index + 1 >= state_history.data.size()) {
        return false; // No more moves to redo
    }

    const Move &move = state_history.data[state_history.read_index + 1].last_move;

    Assert(board_can_move_basic(this, move.get_origin(), move.get_destination()), "Invalid move");
    BoardState dummy{};
    apply_move(*this, move, dummy);
    return state_history.redo();
}

static bool do_undo(Board &board, const Move &move, const BoardState &state) {
    if (move != Move()) {
        if (move.get_special() == Move::MOVE_CASTLE) {
            board_undo_castle(&board, move);
        } else {
            board.remove_piece(move.get_destination_index());
            board.put_piece(state.moved_piece, move.get_origin_index());
            if (move.get_special() == Move::MOVE_EN_PASSANT) {
                board.put_piece(state.captured_piece, square_index(move.to_row() + ((PIECE_COLOR(state.moved_piece) == PIECE_WHITE) ? -1 : +1), move.to_col()));
            } else if (PIECE_TYPE(state.captured_piece) != EMPTY) {
                board.put_piece(state.captured_piece, move.get_destination_index());
            }
        }
        return true;
    }
    return false;
}

bool Board::undo() {
    const Move &move = current_state().last_move;
    if (do_undo(*this, move, current_state())) {
        state_history.undo();
        return true;
    }
    return false;
}

bool Board::undo_stateless(const BoardState &state) {
    Move move = state.last_move;
    return do_undo(*this, move, state);
}

gtr::large_string Board::board_to_string() const {
    gtr::large_string board_str;
    for (int32_t row = 7; row >= 0; --row) {
        for (int32_t col = 0; col < 8; ++col) { board_str += piece_to_string_short(pieces[get_index(row, col)]); }
        board_str += "\n";
    }
    return board_str;
}

gtr::large_string Board::print_bitboard(const BitBoard board) {
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

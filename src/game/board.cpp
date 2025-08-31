#include "board.hpp"
#include <cmath>
#include "analyzer.hpp"
#include "math.hpp"
#include "profiler.hpp"
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
    current_state = state_history.current();
    current_state->castle_rights = CASTLE_WHITE_KINGSIDE | CASTLE_WHITE_QUEENSIDE | CASTLE_BLACK_KINGSIDE | CASTLE_BLACK_QUEENSIDE;
    current_state->castle_rights_bit = bitboard_from_squares<G1, G8, C1, C8>();
    current_state->en_passant_index = EN_PASSANT_INVALID_INDEX;
    side_to_move = PIECE_WHITE;
    move_count = 0;
}

[[maybe_unused]] static bool board_can_move_basic(const Board *board, const uint8_t from_index, const uint8_t to_index) {
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

static bool on_corner(const int32_t row, const int32_t col) {
    static constexpr BitBoard corner_bitboard = bitboard_from_squares<A1, H1, A8, H8>();
    return bitboard_get(corner_bitboard, row, col);
}

static void board_undo_castle(Board *board, const Move move) {
    const auto queen_side = static_cast<int>(move.from_col() - move.to_col() > 0);
    static constexpr gtr::array rook_orig_col = {7, 0};
    static constexpr gtr::array rook_castled_col = {5, 3};
    board->move_piece(move.get_destination_index(), move.get_origin_index());
    board->move_piece(move.from_row(), rook_castled_col[queen_side], move.from_row(), rook_orig_col[queen_side]);
}

static void update_rights(BoardState &state, Piece piece, const int32_t piece_row, const int32_t piece_col) {
    static constexpr gtr::array rights_king_on_corner_0{gtr::array{~CASTLE_WHITE_ALL, ~CASTLE_BLACK_ALL}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_king_on_corner_7{gtr::array{~CASTLE_WHITE_ALL, ~CASTLE_BLACK_ALL}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_king = {rights_king_on_corner_0, rights_king_on_corner_7};

    static constexpr gtr::array rights_rook_on_corner_0{gtr::array{~CASTLE_WHITE_KINGSIDE, ~CASTLE_BLACK_KINGSIDE}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_rook_on_corner_7{gtr::array{~CASTLE_WHITE_QUEENSIDE, ~CASTLE_BLACK_QUEENSIDE}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_rook = {rights_rook_on_corner_0, rights_rook_on_corner_7};

    static constexpr gtr::array rights_non_relevant_on_corner_0{gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_non_relevant_on_corner_7{gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}, gtr::array{CASTLE_RIGHTS_ALL, CASTLE_RIGHTS_ALL}};
    static constexpr gtr::array rights_non_relevant = {rights_non_relevant_on_corner_0, rights_non_relevant_on_corner_7};

    static constexpr gtr::array rights_king_bit_on_corner_0{gtr::array{bitboard_from_squares<G8, C8>(), bitboard_from_squares<G1, C1>()}, gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_king_bit_on_corner_7{gtr::array{bitboard_from_squares<G8, C8>(), bitboard_from_squares<G1, C1>()}, gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_king_bit = {rights_king_bit_on_corner_0, rights_king_bit_on_corner_7};

    static constexpr gtr::array rights_rook_bit_on_corner_0{gtr::array{bitboard_from_squares<G8, C1, C8>(), bitboard_from_squares<G1, C1, C8>()},
                                                            gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_rook_bit_on_corner_7{gtr::array{bitboard_from_squares<G1, G8, C8>(), bitboard_from_squares<G1, G8, C1>()},
                                                            gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_rook_bit = {rights_rook_bit_on_corner_0, rights_rook_bit_on_corner_7};

    static constexpr gtr::array rights_non_relevant_bit_on_corner_0{gtr::array{BITBOARD_FULL, BITBOARD_FULL}, gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_non_relevant_bit_on_corner_7{gtr::array{BITBOARD_FULL, BITBOARD_FULL}, gtr::array{BITBOARD_FULL, BITBOARD_FULL}};
    static constexpr gtr::array rights_non_relevant_bit = {rights_non_relevant_bit_on_corner_0, rights_non_relevant_bit_on_corner_7};

    static constexpr gtr::array rights{rights_non_relevant, rights_non_relevant, rights_non_relevant, rights_non_relevant, rights_rook, rights_non_relevant, rights_king};

    static constexpr gtr::array rights_bit{rights_non_relevant_bit, rights_non_relevant_bit, rights_non_relevant_bit, rights_non_relevant_bit,
                                           rights_rook_bit,         rights_non_relevant_bit, rights_king_bit};

    state.castle_rights &= rights[PIECE_TYPE(piece)][piece_col == 0][on_corner(piece_row, piece_col)][PIECE_COLOR(piece)];
    state.castle_rights_bit &= rights_bit[PIECE_TYPE(piece)][piece_col == 0][on_corner(piece_row, piece_col)][PIECE_COLOR(piece)];
}

static void apply_move(Board &board, const Move move, BoardState &state) {
    TimeFunction;
    const Piece from_piece = board.pieces[move.get_origin()];
    state.en_passant_index = EN_PASSANT_INVALID_INDEX; // Reset en passant index
    state.moved_piece = from_piece;
    state.captured_piece = board.pieces[Board::get_index(move.to_row(), move.to_col())];
    if (PIECE_TYPE(from_piece) == PAWN && gtr::abs(move.from_row() - move.to_row()) == 2) {
        state.en_passant_index = static_cast<int8_t>(static_cast<int8_t>(move.get_destination()) + (IS_WHITE(from_piece) ? WHITE_DIRECTION : BLACK_DIRECTION));
    }
    for (int i = 0; i < 80000; ++i) update_rights(state, from_piece, move.from_row(), move.from_col());
    switch (move.get_special()) {
    case Move::MOVE_EN_PASSANT: {
        const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? move.to_row() - 1 : move.to_row() + 1;
        const int32_t captured_col = move.to_col();
        state.captured_piece = board.pieces[Board::get_index(captured_row, captured_col)];
        board.remove_piece(captured_row, captured_col);
        board.remove_piece(move.get_origin_index());
        board.put_piece(from_piece, move.get_destination_index());
        break;
    }
    case Move::MOVE_CASTLE: {
        const auto queen_side = static_cast<int>(move.from_col() - move.to_col() > 0);
        static constexpr gtr::array rook_orig_col = {7, 0};
        static constexpr gtr::array rook_castled_col = {5, 3};
        board.move_piece(move.from_row(), rook_orig_col[queen_side], move.from_row(), rook_castled_col[queen_side]);
        board.move_piece(move.get_origin_index(), move.get_destination_index());
    } break;
    case Move::MOVE_PROMOTION:
        if (PIECE_TYPE(state.captured_piece) != EMPTY) {
            board.remove_piece(move.get_destination_index());
        }
        board.put_piece(chess_piece_make(move.get_promotion_piece_type(), PIECE_COLOR(board.pieces[move.get_origin()])), move.get_destination_index());
        board.remove_piece(move.get_origin_index());
        break;
    case Move::MOVE_NONE:
    default:
        if (PIECE_TYPE(state.captured_piece) != EMPTY) {
            board.remove_piece(move.to_row(), move.to_col());
        }
        board.remove_piece(move.get_origin_index());
        board.put_piece(from_piece, move.get_destination_index());
        break;
    }
}

void Board::move_stateless(const Move m, BoardState &state) {
    Assert(board_can_move_basic(this, m.get_origin(), m.get_destination()), "Invalid move");
    state.last_move = m;
    apply_move(*this, m, state);
    side_to_move = ~side_to_move; // Switch sides
    move_count++;
}

void Board::move(const Move m) {
    Assert(board_can_move_basic(this, m.get_origin(), m.get_destination()), "Invalid move");
    const BoardState current_state_copy = *current_state;
    state_history.push(current_state_copy);
    current_state = state_history.current();
    current_state->last_move = m;
    apply_move(*this, m, *current_state);
    side_to_move = ~side_to_move; // Switch sides
    move_count++;
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
    state_history.redo();
    side_to_move = ~side_to_move; // Switch sides
    current_state = state_history.current();
    move_count++;
    return true;
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
    const Move &move = current_state->last_move;
    if (do_undo(*this, move, *current_state)) {
        state_history.undo();
        current_state = state_history.current();
        side_to_move = ~side_to_move; // Switch sides
        move_count--;
        return true;
    }
    return false;
}

bool Board::undo_stateless(const BoardState &state) {
    const Move move = state.last_move;
    if (do_undo(*this, move, state)) {
        side_to_move = ~side_to_move; // Switch sides
        move_count--;
        return true;
    }
    return false;
}

gtr::large_string Board::board_to_string() const {
    gtr::large_string board_str;
    for (int32_t row = 7; row >= 0; --row) {
        for (int32_t col = 0; col < 8; ++col) { board_str += piece_to_string_font(pieces[get_index(row, col)]); }
        board_str += "\n";
    }
    return board_str;
}

void Board::set_position(const Fen &fen) {
    state_history.clear();
    state_history.push({});
    std::memset(&pieces_by_type, 0, sizeof(pieces_by_type));
    std::memset(&pieces_by_color, 0, sizeof(pieces_by_color));
    std::memset(&pieces, 0, sizeof(pieces));
    side_to_move = fen.turn();
    move_count = fen.fullmove_number() - 1; // FEN fullmove_number starts at 1, we start at 0
    current_state->castle_rights = fen.castle_rights();
    current_state->castle_rights_bit |= (current_state->castle_rights & CASTLE_WHITE_KINGSIDE) != std::byte{0} ? bitboard_from_squares<G1>() : 0;
    current_state->castle_rights_bit |= (current_state->castle_rights & CASTLE_WHITE_QUEENSIDE) != std::byte{0} ? bitboard_from_squares<C1>() : 0;
    current_state->castle_rights_bit |= (current_state->castle_rights & CASTLE_BLACK_KINGSIDE) != std::byte{0} ? bitboard_from_squares<G8>() : 0;
    current_state->castle_rights_bit |= (current_state->castle_rights & CASTLE_BLACK_QUEENSIDE) != std::byte{0} ? bitboard_from_squares<C8>() : 0;

    const auto en_passant_index = fen.en_passant_square();
    current_state->en_passant_index = en_passant_index == OUT_OF_BOUNDS ? EN_PASSANT_INVALID_INDEX : std::to_underlying(en_passant_index);
    for (int32_t sq = A1; sq <= H8; ++sq) {
        if (const auto piece = fen.piece_at(static_cast<SquareIndex>(sq)); piece == PIECE_NONE) {
            pieces_by_type[EMPTY] |= bitboard_from_squares(sq);
        } else {
            put_piece(piece, static_cast<SquareIndex>(sq));
        }
    }
}

Fen Board::get_fen() const { return Fen::build(pieces, side_to_move, current_state->castle_rights, static_cast<SquareIndex>(current_state->en_passant_index), 0, move_count + 1); }

} // namespace game

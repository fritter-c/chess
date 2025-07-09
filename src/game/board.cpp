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

static void board_undo_castle(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    const bool queen_side = (from_col - to_col > 0);
    const int rook_orig_col = queen_side ? 0 : 7;
    const int rook_castled_col = queen_side ? 3 : 5;
    Piece const &king = board->pieces[Board::get_index(to_row, to_col)];
    Piece const &rook = board->pieces[Board::get_index(from_row, rook_castled_col)];
    board->pieces[Board::get_index(from_row, from_col)] = king;
    board->pieces[Board::get_index(to_row, to_col)] = PIECE_NONE;
    board->pieces[Board::get_index(from_row, rook_orig_col)] = rook;
    board->pieces[Board::get_index(from_row, rook_castled_col)] = PIECE_NONE;
}

static void board_do_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    board->current_state().en_passant_index = -1; // Reset en passant index
    Piece const &from_piece = board->pieces[Board::get_index(from_row, from_col)];
    board->current_state().moved_piece = from_piece;

    if (PIECE_TYPE(from_piece) == PAWN && std::abs(from_row - to_row) == 2) {
        board->current_state().en_passant_index = static_cast<int8_t>(Board::get_index(to_row, to_col));
    }

    if (PIECE_TYPE(from_piece) == KING) {
        board->current_state().castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_ALL : ~CASTLE_WHITE_ALL;
    }
    // Needs to verify the row in case of extra rooks
    else if (PIECE_TYPE(from_piece) == ROOK) {
        if (from_col == 0) {
            board->current_state().castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_QUEENSIDE : ~CASTLE_WHITE_QUEENSIDE;
        } else if (from_col == 7) {
            board->current_state().castle_rights &= PIECE_COLOR(from_piece) ? ~CASTLE_BLACK_KINGSIDE : ~CASTLE_WHITE_KINGSIDE;
        }
    }

    if (board->is_en_passant(from_row, from_col, to_row, to_col)) {
        // Remove the captured pawn
        const int32_t captured_row = PIECE_COLOR(from_piece) == PIECE_WHITE ? to_row - 1 : to_row + 1;
        const int32_t captured_col = to_col;
        board->current_state().captured_piece = board->pieces[Board::get_index(captured_row, captured_col)];
        board->pieces[Board::get_index(captured_row, captured_col)] = PIECE_NONE;
    } else {
        board->current_state().captured_piece = board->pieces[Board::get_index(to_row, to_col)];
    }

    board->pieces[Board::get_index(to_row, to_col)] = from_piece;
    board->pieces[Board::get_index(from_row, from_col)] = PIECE_NONE;
}

void Board::move_no_check(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col) {
    if (board_is_castle(this, from_row, from_col, to_row, to_col)) {
        board_do_castle(this, from_row, from_col, to_row, to_col);
    } else {
        board_do_move(this, from_row, from_col, to_row, to_col);
    }
}

bool Board::move(const Move move) {
    if (board_can_move_basic(this, move.get_origin(), move.get_destination())) {
        const int32_t from_row = get_row(move.get_origin());
        const int32_t from_col = get_col(move.get_origin());
        const int32_t to_row = get_row(move.get_destination());
        const int32_t to_col = get_col(move.get_destination());
        state_history.push(current_state());
        current_state().last_move = move;
        if (move.get_special() == Move::MOVE_CASTLE) {
            board_do_castle(this, from_row, from_col, to_row, to_col);
        } else if (move.get_special() == Move::MOVE_PROMOTION) {
            if (move.get_special() == Move::MOVE_PROMOTION) {
                current_state().moved_piece = pieces[get_index(from_row, from_col)];
                current_state().captured_piece = pieces[get_index(to_row, to_col)];

                pieces[get_index(to_row, to_col)] = chess_piece_make(move.get_promotion_piece_type(), PIECE_COLOR(pieces[get_index(from_row, from_col)]));

                // Remove the pawn from the original position
                pieces[get_index(from_row, from_col)] = PIECE_NONE;
            }
        } else {
            board_do_move(this, from_row, from_col, to_row, to_col);
        }
        return true;
    }
    return false;
}

bool Board::move(const Move m, AlgebraicMove& out_alg) {
    out_alg = move_to_algebraic(*this, m);
    return move(m);
}

bool Board::undo_move(const Move move) {
    if (move == current_state().last_move) {
        const int32_t from_row = get_row(move.get_origin());
        const int32_t from_col = get_col(move.get_origin());
        const int32_t to_row = get_row(move.get_destination());
        const int32_t to_col = get_col(move.get_destination());
        if (move.get_special() == Move::MOVE_CASTLE) {
            board_undo_castle(this, from_row, from_col, to_row, to_col);
        } else {
            pieces[get_index(from_row, from_col)] = current_state().moved_piece;
            if (move.get_special() == Move::MOVE_EN_PASSANT) {
                pieces[get_index(to_row + ((PIECE_COLOR(current_state().moved_piece) == PIECE_WHITE) ? -1 : +1), to_col)] = current_state().captured_piece;
                pieces[get_index(to_row, to_col)] = PIECE_NONE;
            }else {
                pieces[get_index(to_row, to_col)] = current_state().captured_piece;
            }

        }
        state_history.undo();

        return true;
    }
    return false;
}

gtr::char_string<256> Board::board_to_string() const {
    gtr::char_string<256> board_str;
    for (int32_t row = 7; row >= 0; --row) {
        board_str += gtr::format("%d. ", row + 1);
        for (int32_t col = 0; col < 8; ++col) {
            board_str += piece_to_string_short(pieces[get_index(row, col)]);
            board_str += " ";
        }
        board_str += "\n";
    }
    board_str += "   ";
    for (int32_t col = 0; col < 8; ++col) { board_str += gtr::format("%c  ", static_cast<char>('a' + col )); }
    return board_str;
}
} // namespace game

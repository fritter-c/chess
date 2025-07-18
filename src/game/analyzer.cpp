#include "analyzer.hpp"
#include <array>
#include <cstdint>
#include "board.hpp"
#include "move.hpp"
#define BITBOARD_VERSION 1
namespace game {

// Offsets for knight jumps:
static constexpr std::array<std::pair<int32_t, int32_t>, 8> KNIGHT_DELTAS = {{{+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}, {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2}}};
// Offsets for king moves (also used for pawn attack deltas and sliders' increment):
static constexpr std::array<std::pair<int32_t, int32_t>, 8> KING_DELTAS = {{{+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}}};

static bool analyzer_is_pawn_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.pawn_attackers[attacker][index] & board->pieces_by_type[PAWN] & board->pieces_by_color[attacker];
}

static bool analyzer_is_knight_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.knight_attackers[index] & board->pieces_by_type[KNIGHT] & board->pieces_by_color[attacker];
}

static bool analyzer_is_king_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.king_attackers[index] & board->pieces_by_type[KING] & board->pieces_by_color[attacker];
}

static bool analyzer_is_slider_attacking(const Board *board, const int32_t row, const int32_t col, const Color attacker) {
    using enum PieceType;
    for (auto [dr, dc] : KING_DELTAS) {
        int32_t r = row + dr;
        int32_t c = col + dc;
        int32_t dist = 1;
        while (r >= RANK_1 && r < RANK_COUNT && c >= FILE_A && c < FILE_COUNT) {
            const auto p = board->pieces[Board::get_index(r, c)];
            if (PIECE_TYPE(p) == EMPTY) {
                ++dist;
                r += dr;
                c += dc;
                continue;
            }

            if (const bool is_diag = (dr != 0 && dc != 0);
                PIECE_COLOR(p) != attacker || !(PIECE_TYPE(p) == QUEEN || (PIECE_TYPE(p) == ROOK && !is_diag) || (PIECE_TYPE(p) == BISHOP && is_diag))) {
                break;
            }
            return true;
        }
    }
    return false;
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, const int32_t row, const int32_t col, const Color attacker) {
    const SquareIndex cell = Board::square_index(row, col);
    return analyzer_is_knight_attacking(board, cell, attacker) || analyzer_is_king_attacking(board, cell, attacker) || analyzer_is_pawn_attacking(board, cell, attacker) ||
           analyzer_is_slider_attacking(board, row, col, attacker);
}

static bool analyzer_add_move(Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col, AvailableMoves &moves, const Color enemy) {
    const auto friendly = chess_piece_other_color(enemy);
    if (to_row < RANK_1 || to_row >= RANK_COUNT || to_col < FILE_A || to_col >= FILE_COUNT)
        return false;
    const auto target = board->pieces[Board::get_index(to_row, to_col)];
    BoardState state{};
    if (PIECE_TYPE(target) == EMPTY) {
        board->move_stateless(
            analyzer_get_move_from_simple(board, {static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col), static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col)}),
            state);
        if (!analyzer_is_color_in_check(board, friendly)) {
            moves.set(to_row, to_col);
        }
        board->undo_stateless(state);
        return true; // Is not blocked can continue to try to add new moves
    }
    if (PIECE_COLOR(target) != enemy) {
        return false;
    }

    board->move_stateless(
        analyzer_get_move_from_simple(board, {static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col), static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col)}), state);
    if (!analyzer_is_color_in_check(board, friendly)) {
        moves.set(to_row, to_col);
        board->undo_stateless(state);
        return true;
    }
    board->undo_stateless(state);

    return false;
}

static void analyzer_get_pawn_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard empty = board->pieces_by_type[EMPTY];
    const BitBoard enemy_pieces = board->pieces_by_color[enemy];
    const int32_t inc = RowIncrement(~enemy);
    const int32_t one_sq = (row + inc) * 8 + col;
    const BitBoard one_bb = BitBoard(1) << one_sq;
    const BitBoard first = one_bb & empty;
    const BitBoard guard = BitBoard(0) - BitBoard((first >> one_sq) & 1ULL);
    const BitBoard two_bb = PAWN_DOUBLE_STEP_POSITION[~enemy][row][col];
    const BitBoard pawn_attacks = MAGIC_BOARD.pawn_attacks[~enemy][Board::square_index(row, col)];
    const BitBoard en_passant_rank = bitboard_from_squares(board->current_state().en_passant_index);

    moves.bits |= first;
    moves.bits |= two_bb & empty & guard;
    moves.bits |= pawn_attacks & enemy_pieces;
    moves.bits |= pawn_attacks & en_passant_rank;
}

static void analyzer_get_king_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard king_attacks = MAGIC_BOARD.king_attacks[Board::square_index(row, col)];
    const Color side = ~enemy;
    const BitBoard empty = board->pieces_by_type[EMPTY];
    const BitBoard ks_between = CASTLE_KING_EMPTY[std::to_underlying(side)];
    const BitBoard ks_dest = CASTLE_KING_DEST[std::to_underlying(side)];
    const BitBoard qs_between = CASTLE_QUEEN_EMPTY[std::to_underlying(side)];
    const BitBoard qs_dest = CASTLE_QUEEN_DEST[std::to_underlying(side)];

    moves.bits |= ((empty & qs_between) == qs_between) * qs_dest & board->current_state().castle_rights_bit;
    moves.bits |= ((empty & ks_between) == ks_between) * ks_dest & board->current_state().castle_rights_bit;
    moves.bits |= king_attacks & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]);
}

static void analyzer_get_knight_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard knight_attacks = MAGIC_BOARD.knight_attackers[Board::square_index(row, col)];
    moves.bits |= knight_attacks & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]);
}

static void analyzer_get_bishop_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard bishop_attacks = MAGIC_BOARD.bishop_attacks[Board::square_index(row, col)];
    moves.bits |= bishop_attacks;
}

static void analyzer_get_rook_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard rook_attacks = MAGIC_BOARD.rook_attacks[Board::square_index(row, col)];
    moves.bits |= rook_attacks;
}

static void analyzer_get_queen_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard queen_attacks = MAGIC_BOARD.queen_attacks(Board::square_index(row, col));
    moves.bits |= queen_attacks;
}

AvailableMoves analyzer_get_available_moves_for_piece(Board *board, const int32_t row, const int32_t col) {
    using enum PieceType;
    AvailableMoves moves{};
    moves.origin_index = Board::get_index(row, col);
    if (const auto piece = board->pieces[Board::get_index(row, col)]; PIECE_TYPE(piece) != EMPTY) {
        const auto enemy_color = chess_piece_other_color(PIECE_COLOR(piece));
        switch (PIECE_TYPE(piece)) {
        case PAWN  : analyzer_get_pawn_moves(board, piece, row, col, enemy_color, moves); break;
        case KNIGHT: analyzer_get_knight_moves(board, piece, row, col, enemy_color, moves); break;
        case BISHOP: analyzer_get_bishop_moves(board, piece, row, col, enemy_color, moves); break;
        case ROOK  : analyzer_get_rook_moves(board, piece, row, col, enemy_color, moves); break;
        case QUEEN : analyzer_get_queen_moves(board, piece, row, col, enemy_color, moves); break;
        case KING  : analyzer_get_king_moves(board, piece, row, col, enemy_color, moves); break;
        default    : break;
        }
    }
    return moves;
}

bool analyzer_is_color_in_check(Board *board, Color color) {
    const int32_t index = bitboard_index(board->pieces_by_type[KING] & board->pieces_by_color[color]);
    const int32_t kr = Board::get_row(index);
    const int32_t kc = Board::get_col(index);
    return analyzer_is_cell_under_attack_by_color(board, kr, kc, chess_piece_other_color(color));
}

bool analyzer_is_color_in_checkmate(Board *board, Color color) {
    if (!analyzer_is_color_in_check(board, color)) {
        return false;
    }
    for (uint8_t i = 0; i < SQUARE_COUNT; ++i) {
        if (PIECE_COLOR(board->pieces[i]) == color) {
            const auto [bits, origin] = analyzer_get_available_moves_for_piece(board, Board::get_row(i), Board::get_col(i));
            if (bits != 0) {
                return false;
            }
        }
    }
    return true;
}

Move analyzer_get_move_from_simple(Board *board, const SimpleMove &move, PromotionPieceType promotion_type) {
    Move result{};
    result.set_origin(static_cast<uint8_t>(Board::get_index(move.from_row, move.from_col)));
    result.set_destination(static_cast<uint8_t>(Board::get_index(move.to_row, move.to_col)));
    if (board->is_en_passant(move.from_row, move.from_col, move.to_row, move.to_col)) {
        result.set_special(Move::MOVE_EN_PASSANT);
    } else if (PIECE_TYPE(board->pieces[result.get_origin()]) == KING &&
               ((move.from_col == FILE_E && move.to_col == FILE_G) || (move.from_col == FILE_E && move.to_col == FILE_C))) {
        result.set_special(Move::MOVE_CASTLE);
    } else if (board->pawn_is_being_promoted(move)) {
        result.set_special(Move::MOVE_PROMOTION);
        result.set_promotion_piece(promotion_type);
    } else {
        result.set_special(Move::MOVE_NONE);
    }
    return result;
}

int32_t analyzer_get_move_count(Board *board, Color color) {
    int32_t count = 0;
    for (uint8_t i = 0; i < SQUARE_COUNT; ++i) {
        if (PIECE_COLOR(board->pieces[i]) == color) {
            const auto moves = analyzer_get_available_moves_for_piece(board, Board::get_row(i), Board::get_col(i));
            count += moves.move_count();
        }
    }
    return count;
}

bool analyzer_get_is_stalemate(Board *board, Color friendly) { return analyzer_get_move_count(board, friendly) == 0 && !analyzer_is_color_in_checkmate(board, friendly); }

bool analyzer_is_insufficient_material(const Board *board) {
    using std::popcount;

    // 1) any pawn, rook or queen (of either color) → sufficient material
    if (board->pieces_by_type[PAWN] != 0ULL)
        return false;
    if (board->pieces_by_type[ROOK] != 0ULL)
        return false;
    if (board->pieces_by_type[QUEEN] != 0ULL)
        return false;

    // 2) get the bitboards of bishops and knights
    const BitBoard bishop_bb = board->pieces_by_type[BISHOP];
    const BitBoard knight_bb = board->pieces_by_type[KNIGHT];

    // 3) split them by color
    const BitBoard white_bb = board->pieces_by_color[PIECE_WHITE];
    const BitBoard black_bb = board->pieces_by_color[PIECE_BLACK];

    // count each
    const int32_t white_bishops = popcount(bishop_bb & white_bb);
    const int32_t black_bishops = popcount(bishop_bb & black_bb);
    const int32_t white_knights = popcount(knight_bb & white_bb);
    const int32_t black_knights = popcount(knight_bb & black_bb);

    const int32_t white_minors = white_bishops + white_knights;
    const int32_t black_minors = black_bishops + black_knights;
    const int32_t total_minors = white_minors + black_minors;

    // 4) the same three insufficient‐material cases:
    //    K vs. K
    if (total_minors == 0)
        return true;

    //    K + one minor vs K
    if (total_minors == 1)
        return true;

    //    K+B vs. K+B or K+N vs. K+N (one minor each side)
    if (total_minors == 2 && white_minors == 1 && black_minors == 1) {
        return true;
    }

    // 5) otherwise, there is mating material
    return false;
}

bool analyzer_move_puts_to_check(Board *board, const Move &move) {
    const auto friendly = PIECE_COLOR(board->pieces[move.get_origin()]);
    bool result = false;
    if (analyzer_is_move_legal(board, move)) {
        BoardState state{};
        board->move_stateless(move, state);
        if (analyzer_is_color_in_check(board, ~friendly)) {
            result = true;
        }
        board->undo_stateless(state);
    }
    return result;
}

bool analyzer_move_puts_to_checkmate(Board *board, const Move &move) {
    const auto friendly = PIECE_COLOR(board->pieces[move.get_origin()]);
    bool result = false;
    BoardState state{};
    if (analyzer_is_move_legal(board, move)) {
        board->move_stateless(move, state);
        if (analyzer_is_color_in_checkmate(board, ~friendly)) {
            result = true;
        }
        board->undo_stateless(state);
    }
    return result;
}

bool analyzer_is_move_legal(Board *board, const Move &move) { return analyzer_can_move(board, move.from_row(), move.from_col(), move.to_row(), move.to_col()); }
} // namespace game

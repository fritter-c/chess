#include "analyzer.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include "bitboard.hpp"
#include "board.hpp"
#include "move.hpp"
namespace game {
const MagicBoards MAGIC_BOARD = detail::init_magic_boards();
static bool analyzer_is_pawn_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.pawn_attackers[attacker][index] & board->pieces_by_type[PAWN] & board->pieces_by_color[attacker];
}

static bool analyzer_is_knight_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.knight_attackers[index] & board->pieces_by_type[KNIGHT] & board->pieces_by_color[attacker];
}

static bool analyzer_is_king_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.king_attackers[index] & board->pieces_by_type[KING] & board->pieces_by_color[attacker];
}

static bool analyzer_is_rook_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard rooks = board->pieces_by_type[ROOK] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<ROOK>(occ, rooks), index);
}

static bool analyzer_is_bishop_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard bishops = board->pieces_by_type[BISHOP] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<BISHOP>(occ, bishops), index);
}

static bool analyzer_is_queen_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard queens = board->pieces_by_type[QUEEN] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<QUEEN>(occ, queens), index);
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, const int32_t row, const int32_t col, const Color attacker) {
    const SquareIndex cell = Board::square_index(row, col);
    return analyzer_is_knight_attacking(board, cell, attacker) || analyzer_is_king_attacking(board, cell, attacker) || analyzer_is_pawn_attacking(board, cell, attacker) ||
           analyzer_is_rook_attacking(board, cell, attacker) || analyzer_is_bishop_attacking(board, cell, attacker) || analyzer_is_queen_attacking(board, cell, attacker);
}

// Pawn moves needs further check in case they are pinners
static void analyzer_get_pawn_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard empty = board->pieces_by_type[EMPTY];
    const BitBoard enemy_pieces = board->pieces_by_color[enemy];
    const BitBoard en_passant_rank = EN_PASSANT_CONVERSION_TABLE[enemy][board->current_state->en_passant_index + 1];
    const BitBoard pawn_attacks = MAGIC_BOARD.pawn_attacks[~enemy][Board::square_index(row, col)];
    const BitBoard pawn_moves = MAGIC_BOARD.pawn_moves[~enemy][Board::square_index(row, col)];
    const int32_t inc = RowIncrement(~enemy);
    const int32_t one_sq = (row + inc) * 8 + col;
    const BitBoard one_bb = BitBoard{1} << one_sq;
    const BitBoard first = one_bb & empty;
    const BitBoard guard = BitBoard{0} - BitBoard{first >> one_sq & 1ULL};
    moves.bits |= first;
    moves.bits |= pawn_moves & empty & guard;
    moves.bits |= pawn_attacks & enemy_pieces;
    moves.bits |= pawn_attacks & en_passant_rank;
}

// Castles need to further check for attacks on the king's path and current check status.
static void analyzer_get_king_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard king_attacks = MAGIC_BOARD.king_attacks[Board::square_index(row, col)]; // King attacks are in a table (All neighbor cells)
    const Color side = ~enemy;                                                             // Friendly color
    const BitBoard empty = board->pieces_by_type[EMPTY];                                   // Where the squares are empty.
    const BitBoard ks_between = CASTLE_KING_EMPTY[std::to_underlying(side)];               // The king side squares that need to be empty
    const BitBoard ks_dest = CASTLE_KING_DEST[std::to_underlying(side)];                   // The king side destination squares
    const BitBoard qs_between = CASTLE_QUEEN_EMPTY[std::to_underlying(side)];              // The queen side squares that need to be empty
    const BitBoard qs_dest = CASTLE_QUEEN_DEST[std::to_underlying(side)];                  // The queen side destination squares

    moves.bits |= static_cast<int32_t>((empty & qs_between) == qs_between) * qs_dest & board->current_state->castle_rights_bit; // Are the squares empty and have the castle-rights
    moves.bits |= static_cast<int32_t>((empty & ks_between) == ks_between) * ks_dest & board->current_state->castle_rights_bit; // Are the squares empty and have the castle-rights
    moves.bits |= king_attacks & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]); // Just the king attacks. The move is available if the dest. cell is enemy or empty
}

// Knight moves need further check in case they are pinners
static void analyzer_get_knight_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    moves.bits |= MAGIC_BOARD.knight_attackers[Board::square_index(row, col)] & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]); // Knight moves are just attacks
}

// Bishop, Rook and Queen moves need further check in case they are pinners
static void analyzer_get_bishop_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard occ = board->pieces_by_type[ANY];
    BitBoard bishop_attacks = MAGIC_BOARD.slider_attacks<BISHOP>(occ, Board::square_index(row, col));
    bishop_attacks &= ~board->pieces_by_color[~enemy]; // Remove the friendly pieces from the attacks
    moves.bits |= bishop_attacks;
}

static void analyzer_get_rook_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    const BitBoard occ = board->pieces_by_type[ANY];
    BitBoard rook_attacks = MAGIC_BOARD.slider_attacks<ROOK>(occ, Board::square_index(row, col));
    rook_attacks &= ~board->pieces_by_color[~enemy]; // Remove the friendly pieces from the attacks
    moves.bits |= rook_attacks;
}

static void analyzer_get_queen_moves(const Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    analyzer_get_bishop_moves(board, piece, row, col, enemy, moves);
    analyzer_get_rook_moves(board, piece, row, col, enemy, moves);
}

AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(const Board *board, const int32_t row, const int32_t col) {
    using enum PieceType;
    AvailableMoves moves{};
    moves.origin_index = Board::get_index(row, col);
    const auto piece = board->pieces[Board::get_index(row, col)];
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
    return moves;
}

AvailableMoves analyzer_filter_legal_moves(Board *board, const AvailableMoves moves) {
    AvailableMoves legal{};
    SimpleMove move{};
    move.from_row = Board::get_row(moves.origin_index);
    move.from_col = Board::get_col(moves.origin_index);
    legal.origin_index = moves.origin_index;
    for (auto it = BitBoardIterator::begin(moves.bits); it != BitBoardIterator::end(); ++it) {
        move.to_row = Board::get_row(*it);
        move.to_col = Board::get_col(*it);
        if (analyzer_is_move_legal(board, move)) {
            legal.set(move.to_row, move.to_col);
        }
    }
    return legal;
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
    return analyzer_get_legal_move_count(board, color) == 0;
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

int64_t analyzer_get_legal_move_count(Board *board, Color color) {
    int64_t count = 0;
    for (auto it = BitBoardIterator::begin(board->pieces_by_color[color]); it != BitBoardIterator::end(); ++it) {
        const auto moves = analyzer_get_pseudo_legal_moves_for_piece(board, *it);
        const auto legal_moves = analyzer_filter_legal_moves(board, moves);
        count += legal_moves.move_count();
    }
    return count;
}

bool analyzer_get_is_stalemate(Board *board, Color friendly) { return analyzer_get_legal_move_count(board, friendly) == 0 && !analyzer_is_color_in_checkmate(board, friendly); }

bool analyzer_is_insufficient_material(const Board *board) {
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
    const int64_t white_bishops = popcnt(bishop_bb & white_bb);
    const int64_t black_bishops = popcnt(bishop_bb & black_bb);
    const int64_t white_knights = popcnt(knight_bb & white_bb);
    const int64_t black_knights = popcnt(knight_bb & black_bb);

    const int64_t white_minors = white_bishops + white_knights;
    const int64_t black_minors = black_bishops + black_knights;
    const int64_t total_minors = white_minors + black_minors;

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

bool analyzer_is_move_legal(Board *board, const Move &move) {
    const auto friendly = PIECE_COLOR(board->pieces[move.get_origin()]);

    if (move.is_castle()) {
        if (analyzer_is_color_in_check(board, friendly)) {
            return false;
        }

        if (move.king_side_castle()) {
            for (auto sq : CASTLE_KING_SQUARES[std::to_underlying(friendly)]) {
                if (analyzer_is_cell_under_attack_by_color(board, Board::get_row(sq), Board::get_col(sq), ~friendly)) {
                    return false;
                }
            }
        } else {
            for (auto sq : CASTLE_QUEEN_SQUARES[std::to_underlying(friendly)]) {
                if (analyzer_is_cell_under_attack_by_color(board, Board::get_row(sq), Board::get_col(sq), ~friendly)) {
                    return false;
                }
            }
        }
    } else {
        BoardState state{};
        board->move_stateless(move, state);
        if (analyzer_is_color_in_check(board, friendly)) {
            board->undo_stateless(state);
            return false;
        }
        board->undo_stateless(state);
    }
    return true;
}
} // namespace game

#include "analyzer.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include "bitboard.hpp"
#include "board.hpp"
#include "move.hpp"
#include "profiler.hpp"

#ifndef TimeFunction
#define TimeFunction
#endif

namespace game {
alignas(64) const MagicBoards MAGIC_BOARD = detail::init_magic_boards();
bool analyzer_is_pawn_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.pawn_attackers[attacker][index] & board->pieces_by_type[PAWN] & board->pieces_by_color[attacker];
}

bool analyzer_is_knight_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.knight_attackers[index] & board->pieces_by_type[KNIGHT] & board->pieces_by_color[attacker];
}

bool analyzer_is_king_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    return MAGIC_BOARD.king_attackers[index] & board->pieces_by_type[KING] & board->pieces_by_color[attacker];
}

bool analyzer_is_rook_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard rooks = board->pieces_by_type[ROOK] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<ROOK>(occ, rooks), index);
}

bool analyzer_is_bishop_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard bishops = board->pieces_by_type[BISHOP] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<BISHOP>(occ, bishops), index);
}

bool analyzer_is_queen_attacking(const Board *board, const SquareIndex index, const Color attacker) {
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard queens = board->pieces_by_type[QUEEN] & board->pieces_by_color[attacker];
    return bitboard_get(MAGIC_BOARD.slider_attacks<QUEEN>(occ, queens), index);
}

bool analyzer_is_queen_attacking([[maybe_unused]] const Board *board, const SquareIndex index, Color, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == QUEEN, "Origin square is not a queen");
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard attacks = MAGIC_BOARD.slider_attacks<QUEEN>(occ, origin);
    return bitboard_get(attacks, index);
}

bool analyzer_is_pawn_attacking([[maybe_unused]] const Board *board, const SquareIndex index, const Color attacker, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == PAWN, "Origin square is not a pawn");
    const BitBoard pawn_attackers = MAGIC_BOARD.pawn_attackers[attacker][index];
    return bitboard_get(pawn_attackers, origin);
}

bool analyzer_is_knight_attacking([[maybe_unused]] const Board *board, const SquareIndex index, Color, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == KNIGHT, "Origin square is not a knight");
    const BitBoard knight_attacks = MAGIC_BOARD.knight_attackers[index];
    return bitboard_get(knight_attacks, origin);
}

bool analyzer_is_king_attacking([[maybe_unused]] const Board *board, const SquareIndex index, Color, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == KING, "Origin square is not a king");
    const BitBoard king_attacks = MAGIC_BOARD.king_attackers[index];
    return bitboard_get(king_attacks, origin);
}

bool analyzer_is_rook_attacking(const Board *board, const SquareIndex index, const Color, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == ROOK, "Origin square is not a rook");
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard attacks = MAGIC_BOARD.slider_attacks<ROOK>(occ, origin);
    return bitboard_get(attacks, index);
}

bool analyzer_is_bishop_attacking(const Board *board, const SquareIndex index, Color, const SquareIndex origin) {
    Assert(PIECE_TYPE(board->pieces[origin]) == BISHOP, "Origin square is not a bishop");
    const BitBoard occ = board->pieces_by_type[ANY];
    const BitBoard attacks = MAGIC_BOARD.slider_attacks<BISHOP>(occ, origin);
    return bitboard_get(attacks, index);
}

bool analyzer_is_cell_under_attack_by_color(const Board *board, const int32_t row, const int32_t col, const Color attacker) {
    TimeFunction;
    const SquareIndex cell = Board::square_index(row, col);
    return analyzer_is_knight_attacking(board, cell, attacker) || analyzer_is_king_attacking(board, cell, attacker) || analyzer_is_pawn_attacking(board, cell, attacker) ||
           analyzer_is_rook_attacking(board, cell, attacker) || analyzer_is_bishop_attacking(board, cell, attacker) || analyzer_is_queen_attacking(board, cell, attacker);
}

// Pawn moves needs further check in case they are pinners
static void analyzer_get_pawn_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    TimeFunction;
    const BitBoard empty = board->pieces_by_type[EMPTY];
    const BitBoard enemy_pieces = board->pieces_by_color[enemy];
    const BitBoard en_passant_rank = MAGIC_BOARD.en_passant_conversion_table[enemy][board->current_state->en_passant_index + 1];
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
    TimeFunction;
    const BitBoard king_attacks = MAGIC_BOARD.king_attacks[Board::square_index(row, col)]; // King attacks are in a table (All neighbor cells)
    const Color side = ~enemy;                                                             // Friendly color
    const BitBoard empty = board->pieces_by_type[EMPTY];                                   // Where the squares are empty.
    const BitBoard ks_between = MAGIC_BOARD.castle_king_empty[std::to_underlying(side)];   // The king side squares that need to be empty
    const BitBoard ks_dest = MAGIC_BOARD.castle_king_dest[std::to_underlying(side)];       // The king side destination squares
    const BitBoard qs_between = MAGIC_BOARD.castle_queen_empty[std::to_underlying(side)];  // The queen side squares that need to be empty
    const BitBoard qs_dest = MAGIC_BOARD.castle_queen_dest[std::to_underlying(side)];      // The queen side destination squares

    moves.bits |= static_cast<int32_t>((empty & qs_between) == qs_between) * qs_dest & board->current_state->castle_rights_bit; // Are the squares empty and have the castle-rights?
    moves.bits |= static_cast<int32_t>((empty & ks_between) == ks_between) * ks_dest & board->current_state->castle_rights_bit; // Are the squares empty and have the castle-rights?
    moves.bits |= king_attacks & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]); // Just the king attacks. The move is available if the dest. Cell is enemy or empty
}

// Knight moves need further check in case they are pinners
static void analyzer_get_knight_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    TimeFunction;
    moves.bits |= MAGIC_BOARD.knight_attackers[Board::square_index(row, col)] & (board->pieces_by_color[enemy] | board->pieces_by_type[EMPTY]); // Knight moves are just attacks
}

/*
 Bishop, Rook and Queen moves need further check in case they are pinners.
 The tables mark available squares until the first occupied square (including it) as occ does not discriminate color of pieces on it, so
 we need to remove the friendly pieces from the attacks after getting the attacks
*/
static void analyzer_get_bishop_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    TimeFunction;
    const BitBoard occ = board->pieces_by_type[ANY];
    BitBoard bishop_attacks = MAGIC_BOARD.slider_attacks<BISHOP>(occ, Board::square_index(row, col));
    bishop_attacks &= ~board->pieces_by_color[~enemy]; // Remove the friendly pieces from the attacks
    moves.bits |= bishop_attacks;
}

/*
 Bishop, Rook and Queen moves need further check in case they are pinners.
 The tables mark available squares until the first occupied square (including it) as occ does not discriminate color of pieces on it, so
 we need to remove the friendly pieces from the attacks after getting the attacks
*/
static void analyzer_get_rook_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    TimeFunction;
    const BitBoard occ = board->pieces_by_type[ANY];
    BitBoard rook_attacks = MAGIC_BOARD.slider_attacks<ROOK>(occ, Board::square_index(row, col));
    rook_attacks &= ~board->pieces_by_color[~enemy]; // Remove the friendly pieces from the attacks
    moves.bits |= rook_attacks;
}

/*
 Bishop, Rook and Queen moves need further check in case they are pinners.
 The tables mark available squares until the first occupied square (including it) as occ does not discriminate color of pieces on it, so
 we need to remove the friendly pieces from the attacks after getting the attacks
*/
static void analyzer_get_queen_moves(const Board *board, const Piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    TimeFunction;
    const BitBoard occ = board->pieces_by_type[ANY];
    BitBoard queen_attacks = MAGIC_BOARD.slider_attacks<QUEEN>(occ, Board::square_index(row, col));
    queen_attacks &= ~board->pieces_by_color[~enemy]; // Remove the friendly pieces from the attacks
    moves.bits |= queen_attacks;
}

AvailableMoves analyzer_get_pseudo_legal_moves_for_piece(const Board *board, const int32_t row, const int32_t col) {
    TimeFunction;
    using enum PieceType;
    AvailableMoves moves(Board::get_index(row, col));
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

static bool analyzer_is_move_legal(Board *board, const Move &move) {
    TimeFunction;
    const auto friendly = PIECE_COLOR(board->pieces[move.get_origin()]);

    if (move.is_castle()) {
        if (analyzer_is_color_in_check(board, friendly)) {
            return false;
        }
        if (move.king_side_castle()) {
            for (const auto sq : MAGIC_BOARD.castle_king_squares[std::to_underlying(friendly)]) {
                if (analyzer_is_cell_under_attack_by_color(board, Board::get_row(sq), Board::get_col(sq), ~friendly)) {
                    return false;
                }
            }
        } else {
            // B1 and B1 can be under attack
            for (int i = 0; i < 2; i++) {
                const auto sq = MAGIC_BOARD.castle_queen_squares[std::to_underlying(friendly)][i];
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

static bool analyzer_is_move_legal(Board *board, const SimpleMove &move, PromotionPieceType promotion_type = PROMOTION_QUEEN) {
    return analyzer_is_move_legal(board, analyzer_get_move_from_simple(board, move, promotion_type));
}

AvailableMoves analyzer_filter_legal_moves(Board *board, const AvailableMoves moves) {
    TimeFunction;
    AvailableMoves legal(moves.origin_index);
    SimpleMove move{};
    move.from_row = Board::get_row(moves.origin_index);
    move.from_col = Board::get_col(moves.origin_index);
    for (const auto it : BitBoardIterator(moves.bits)) {
        move.to_row = Board::get_row(it);
        move.to_col = Board::get_col(it);
        if (analyzer_is_move_legal(board, move)) {
            legal.set(move.to_row, move.to_col);
        }
    }
    return legal;
}

bool analyzer_is_color_in_check(Board *board, const Color color) {
    TimeFunction;
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

constexpr bool pawn_in_promotion(const Color c, const SquareIndex s) noexcept {
    static std::array promotion_ranks = {bitboard_from_squares<A7, B7, C7, D7, E7, F7, G7, H7>(), bitboard_from_squares<A2, B2, C2, D2, E2, F2, G2, H2>()};
    return bitboard_get(promotion_ranks[c], s);
}

static Move::MoveSpecialType analyzer_get_special_type(const Board *board, const SquareIndex o, const SquareIndex d) {
    auto select_mask = [](const int32_t oldv, const int32_t newv, const uint32_t cond) {
        const auto m = -static_cast<int32_t>(cond);
        return (oldv & ~m) | (newv & m);
    };
    const auto src = board->pieces[o];
    const auto dst = board->pieces[d];

    const int from_f = file_of(o);
    const int to_f = file_of(d);
    const int df = to_f - from_f;
    const auto adf = gtr::abs(df);

    const unsigned is_pawn = PIECE_TYPE(src) == PAWN;
    const unsigned is_king = PIECE_TYPE(src) == KING;
    const unsigned dest_empty = PIECE_TYPE(dst) == EMPTY;

    const unsigned col_is_1 = adf == 1;
    const unsigned col_is_2 = adf == 2;

    const unsigned promote = is_pawn & static_cast<unsigned>(pawn_in_promotion(PIECE_COLOR(src), o));
    const unsigned enpassant = is_pawn & dest_empty & col_is_1;
    const unsigned castle = is_king & col_is_2;

    int sp = Move::MOVE_NONE;
    sp = select_mask(sp, Move::MOVE_CASTLE, castle);
    sp = select_mask(sp, Move::MOVE_EN_PASSANT, enpassant);
    sp = select_mask(sp, Move::MOVE_PROMOTION, promote);
    return static_cast<Move::MoveSpecialType>(sp);
}

Move analyzer_get_move_from_simple(const Board *board, const SimpleMove &move, const PromotionPieceType promotion_type) {
    TimeFunction;
    Move result{};
    const auto o = static_cast<SquareIndex>(Board::get_index(move.from_row, move.from_col));
    const auto d = static_cast<SquareIndex>(Board::get_index(move.to_row, move.to_col));
    result.set_origin(o);
    result.set_destination(d);
    result.set_promotion_piece(promotion_type);
    result.set_special(analyzer_get_special_type(board, o, d));
    return result;
}

int32_t analyzer_get_legal_move_count(Board *board, const Color color) {
    TimeFunction;
    int32_t count = 0;
    for (const auto it : BitBoardIterator(board->pieces_by_color[color])) {
        const auto moves = analyzer_get_pseudo_legal_moves_for_piece(board, it);
        const auto legal_moves = analyzer_filter_legal_moves(board, moves);
        count += legal_moves.move_count();
    }
    return count;
}

bool analyzer_get_is_stalemate(Board *board, const Color friendly) {
    return analyzer_get_legal_move_count(board, friendly) == 0 && !analyzer_is_color_in_checkmate(board, friendly);
}

bool analyzer_is_insufficient_material(const Board *board) {
    if (board->pieces_by_type[PAWN] | board->pieces_by_type[ROOK] | board->pieces_by_type[QUEEN])
        return false;

    const BitBoard bishop_bb = board->pieces_by_type[BISHOP];
    const BitBoard knight_bb = board->pieces_by_type[KNIGHT];
    const BitBoard white_bb = board->pieces_by_color[PIECE_WHITE];
    const BitBoard black_bb = board->pieces_by_color[PIECE_BLACK];

    const int64_t white_bishops = popcnt(bishop_bb & white_bb);
    const int64_t black_bishops = popcnt(bishop_bb & black_bb);
    const int64_t white_knights = popcnt(knight_bb & white_bb);
    const int64_t black_knights = popcnt(knight_bb & black_bb);

    const int64_t white_minors = white_bishops + white_knights;
    const int64_t black_minors = black_bishops + black_knights;
    const int64_t total_minors = white_minors + black_minors;

    if (total_minors < 2)
        return true;

    if (total_minors == 2 && white_minors == 1) {
        return true;
    }

    if (total_minors == 2 && (white_knights == 2 || black_knights == 2)) {
        return true;
    }

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
    if (analyzer_is_move_legal(board, move)) {
        BoardState state{};
        board->move_stateless(move, state);
        if (analyzer_is_color_in_checkmate(board, ~friendly)) {
            result = true;
        }
        board->undo_stateless(state);
    }
    return result;
}

} // namespace game

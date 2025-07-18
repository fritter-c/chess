#include "analyzer.hpp"
#include <array>
#include <cstdint>
#include "board.hpp"
#include "move.hpp"
#define BITBOARD_VERSION 0
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
#if BITBOARD_VERSION
    (void)piece;
    const auto friendly = chess_piece_other_color(enemy);
    const BitBoard emp = board->pieces_by_type[EMPTY];
    const BitBoard enemy_bb = board->pieces_by_color[enemy];
    const BitBoard origin_bb = static_cast<BitBoard>(1) << Board::get_index(row, col);
    constexpr BitBoard not_file_a = 0xfefefefefefefefeULL;
    constexpr BitBoard not_file_h = 0x7f7f7f7f7f7f7f7fULL;
    auto record = [&](const BitBoard b) {
        if (b) {
            const int32_t t = std::countr_zero(b);
            std::ignore = analyzer_add_move(board, row, col, t / 8, t % 8, moves, enemy);
        }
    };

    if (friendly == PIECE_WHITE) {
        record(origin_bb << 8 & emp);
        if (row == RANK_2)
            record((origin_bb << 8 & emp) << 8 & emp);

        // captures
        record(origin_bb << 7 & enemy_bb & not_file_h);
        record(origin_bb << 9 & enemy_bb & not_file_a);

        // en passant
        if (const int8_t ep = board->current_state().en_passant_index; ep >= 0) {
            const BitBoard epb = static_cast<BitBoard>(1) << static_cast<uint8_t>(ep);
            record(origin_bb << 7 & epb & not_file_h);
            record(origin_bb << 9 & epb & not_file_a);
        }
    } else {
        // 1‐step
        record((origin_bb >> 8) & emp);
        // 2‐step
        if (row == RANK_7)
            record((origin_bb >> 8 & emp) >> 8 & emp);

        // captures
        record(origin_bb >> 9 & enemy_bb & not_file_h);
        record(origin_bb >> 7 & enemy_bb & not_file_a);

        // en passant
        if (const int8_t ep = board->current_state().en_passant_index; ep >= 0) {
            const BitBoard epb = static_cast<BitBoard>(1) << static_cast<uint8_t>(ep);
            record(origin_bb >> 9 & epb & not_file_h);
            record(origin_bb >> 7 & epb & not_file_a);
        }
    }
#else
    const auto friendly = PIECE_COLOR(piece);
    const int32_t dir = friendly == PIECE_WHITE ? 1 : -1;
    // one-step
    const int32_t r1 = row + dir;
    if (const int32_t c1 = col; r1 >= RANK_1 && r1 < RANK_COUNT && PIECE_TYPE(board->pieces[Board::get_index(r1, c1)]) == EMPTY) {
        std::ignore = analyzer_add_move(board, row, col, r1, c1, moves, enemy);
        // two-step from home rank
        if (const int32_t r2 = row + 2 * dir; Board::pawn_first_move(piece, row) && PIECE_TYPE(board->pieces[Board::get_index(r2, c1)]) == EMPTY) {
            std::ignore = analyzer_add_move(board, row, col, r2, c1, moves, enemy);
        }
    }
    // captures
    for (const int32_t dc : {-1, +1}) {
        const int32_t c2 = col + dc;
        const int32_t r2 = row + dir;
        if (r2 < RANK_1 || r2 >= RANK_COUNT || c2 < FILE_A || c2 >= FILE_COUNT) {
            continue;
        }
        if (const auto p = board->pieces[Board::get_index(r2, c2)]; PIECE_TYPE(p) != EMPTY && PIECE_COLOR(p) == enemy) {
            std::ignore = analyzer_add_move(board, row, col, r2, c2, moves, enemy);
        }
    }

    // en passant
    // No need to check if the target square is empty because en passant captures are only valid
    // if the pawn has just moved two squares forward, which means the target square must be
    // empty at the moment of the en passant capture.
    if (PIECE_COLOR(piece) == PIECE_WHITE && row == RANK_5) {
        // Check for en passant capture to the left
        if (board->can_en_passant_this(row, col - 1, enemy)) {
            std::ignore = analyzer_add_move(board, row, col, row + 1, col - 1, moves, enemy); // Capture to the left
        }
        // Check for en passant capture to the right
        if (board->can_en_passant_this(row, col + 1, enemy)) {
            std::ignore = analyzer_add_move(board, row, col, row + 1, col + 1, moves, enemy);
        }
    } else if (PIECE_COLOR(piece) == PIECE_BLACK && row == RANK_4) {
        // Check for en passant capture to the left
        if (board->can_en_passant_this(row, col - 1, enemy)) {
            std::ignore = analyzer_add_move(board, row, col, row - 1, col - 1, moves, enemy); // Capture to the left
        }
        // Check for en passant capture to the right
        if (board->can_en_passant_this(row, col + 1, enemy)) {
            std::ignore = analyzer_add_move(board, row, col, row - 1, col + 1, moves, enemy);
        }
    }
#endif
}

static void analyzer_get_king_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    using enum PieceType;
    const auto friendly = PIECE_COLOR(piece);
    for (auto [dr, dc] : KING_DELTAS) {
        const int32_t r = row + dr;
        const int32_t c = col + dc;
        if (r < RANK_1 || r >= RANK_COUNT || c < FILE_A || c >= FILE_COUNT)
            continue;
        std::ignore = analyzer_add_move(board, row, col, r, c, moves, enemy);
    }

    // Kingside castling
    if (!analyzer_is_color_in_check(board, friendly)) {
        const uint8_t king_row = friendly ? 7 : 0;
        if (PIECE_TYPE(board->pieces[Board::get_index(king_row, 5)]) == EMPTY && PIECE_TYPE(board->pieces[Board::get_index(king_row, 6)]) == EMPTY &&
            PIECE_TYPE(board->pieces[Board::get_index(king_row, 7)]) == ROOK && PIECE_COLOR(board->pieces[Board::get_index(king_row, 7)]) == friendly &&
            board->castle_rights_for(friendly, true) && !analyzer_is_cell_under_attack_by_color(board, king_row, 5, chess_piece_other_color(friendly)) &&
            !analyzer_is_cell_under_attack_by_color(board, king_row, 6, chess_piece_other_color(friendly))) {
            moves.set(king_row, 6); // Castling move to g1
        }

        // Queenside castling
        if (PIECE_TYPE(board->pieces[Board::get_index(king_row, 3)]) == EMPTY && PIECE_TYPE(board->pieces[Board::get_index(king_row, 2)]) == EMPTY &&
            PIECE_TYPE(board->pieces[Board::get_index(king_row, 1)]) == EMPTY && PIECE_TYPE(board->pieces[Board::get_index(king_row, 0)]) == ROOK &&
            PIECE_COLOR(board->pieces[Board::get_index(king_row, 0)]) == friendly && board->castle_rights_for(friendly, false) &&
            !analyzer_is_cell_under_attack_by_color(board, king_row, 3, chess_piece_other_color(friendly)) &&
            !analyzer_is_cell_under_attack_by_color(board, king_row, 2, chess_piece_other_color(friendly)) &&
            !analyzer_is_cell_under_attack_by_color(board, king_row, 1, chess_piece_other_color(friendly))) {
            moves.set(king_row, 2); // Castling move to c1
        }
    }
}

static void analyzer_get_sliders_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    for (auto [dr, dc] : KING_DELTAS) {
        // skip impossible increment for ROOK and BISHOP
        if (PIECE_TYPE(piece) == ROOK && dr != 0 && dc != 0)
            continue;
        if (PIECE_TYPE(piece) == BISHOP && (dr == 0 || dc == 0))
            continue;

        int32_t r = row + dr;
        int32_t c = col + dc;
        auto is_enemy_next = [&r, &c, &board, &enemy]() {
            return PIECE_TYPE(board->pieces[Board::get_index(r, c)]) != EMPTY && PIECE_COLOR(board->pieces[Board::get_index(r, c)]) == enemy;
        };
        // add until a move is blocked or encounters an enemy piece
        while (analyzer_add_move(board, row, col, r, c, moves, enemy) && !is_enemy_next()) {
            r += dr;
            c += dc;
        }
    }
}

static void analyzer_get_knight_moves(Board *board, const Piece piece, const int32_t row, const int32_t col, const Color enemy, AvailableMoves &moves) {
    (void)piece;
    for (auto [dr, dc] : KNIGHT_DELTAS) {
        const int32_t r = row + dr;
        const int32_t c = col + dc;
        if (r < RANK_1 || r >= RANK_COUNT || c < FILE_A || c >= FILE_COUNT)
            continue;
        const auto p = board->pieces[Board::get_index(r, c)];
        if (PIECE_TYPE(p) == EMPTY || PIECE_COLOR(p) == enemy)
            analyzer_add_move(board, row, col, r, c, moves, enemy);
    }
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
        case BISHOP:
        case ROOK  :
        case QUEEN : analyzer_get_sliders_moves(board, piece, row, col, enemy_color, moves); break;
        case KING  : analyzer_get_king_moves(board, piece, row, col, enemy_color, moves); break;
        default    : break;
        }
    }
    return moves;
}

bool analyzer_is_color_in_check(Board *board, Color color) {
    uint8_t kr = 0xFF;
    uint8_t kc = 0xFF;
    for (uint8_t r = 0; r < RANK_COUNT; ++r)
        for (uint8_t c = 0; c < FILE_COUNT; ++c) {
            const auto p = board->pieces[Board::get_index(r, c)];
            if (PIECE_TYPE(p) == KING && PIECE_COLOR(p) == color) {
                kr = r;
                kc = c;
                break;
            }
        }

    if (kr == 0xFF || kc == 0xFF)
        return false; // No king found, cannot be in check

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

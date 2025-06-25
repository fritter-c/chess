// analyzer.cpp
#include "board.hpp" // for Board, board_get_index, etc.
#include "move.hpp"  // for AlgebraicMove
#include <array>
#include <cstdint>
#include <algorithm>
#include "analyzer.hpp" // for analyzer_is_cell_under_attack_by_color, etc.

namespace game
{

    // Offsets for knight jumps:
    static constexpr std::array<std::pair<int8_t, int8_t>, 8> KNIGHT_DELTAS = {{{+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}, {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2}}};

    // Offsets for king moves (also used for pawn attack deltas below):
    static constexpr std::array<std::pair<int8_t, int8_t>, 8> KING_DELTAS = {{{+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}}};

    bool analyzer_is_cell_under_attack_by_color(
        const Board *board, int32_t row, int32_t col, ChessPieceColor attacker)
    {
        // 1) Pawn attacks
        int pawn_dir = (attacker == ChessPieceColor::PIECE_WHITE ? -1 : +1);
        for (int dc : {-1, +1})
        {
            int r = row + pawn_dir, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::PAWN && p.color == attacker)
                    return true;
            }
        }

        // 2) Knight attacks
        for (auto [dr, dc] : KNIGHT_DELTAS)
        {
            int r = row + dr, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KNIGHT && p.color == attacker)
                    return true;
            }
        }

        // 3) Sliding attacks (rook, bishop, queen)
        //    directions: orthogonal for rook, diagonal for bishop, both for queen
        const std::array<std::pair<int8_t, int8_t>, 8> DIRS = KING_DELTAS;
        for (auto [dr, dc] : DIRS)
        {
            int r = row + dr, c = col + dc;
            int dist = 1;
            while (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type != ChessPieceType::NONE)
                {
                    if (p.color == attacker)
                    {
                        bool is_diag = (dr != 0 && dc != 0);
                        if (p.type == ChessPieceType::QUEEN || (p.type == ChessPieceType::ROOK && !is_diag) || (p.type == ChessPieceType::BISHOP && is_diag))
                        {
                            return true;
                        }
                    }
                    break; // blocked by any piece
                }
                ++dist;
                r += dr;
                c += dc;
            }
        }

        // 4) King adjacency (to detect illegal "touching kings")
        for (auto [dr, dc] : KING_DELTAS)
        {
            int r = row + dr, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KING && p.color == attacker)
                    return true;
            }
        }

        return false;
    }

    BitBoard analyzer_get_available_moves_for_piece(
        const Board *board, int32_t row, int32_t col)
    {
        BitBoard moves{};
        moves.bits = 0;
        Board aux_board = *board;

        auto restore_board = [&aux_board, &board]()
        {
            aux_board = *board;
        };

        auto me = board->pieces[board_get_index(row, col)];
        if (me.type == ChessPieceType::NONE)
            return moves;

        auto friendly = me.color;
        auto enemy = (friendly == ChessPieceColor::PIECE_WHITE
                          ? ChessPieceColor::PIECE_BLACK
                          : ChessPieceColor::PIECE_WHITE);

        auto add_move = [&](int r, int c)
        {
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                return false;
            auto target = board->pieces[board_get_index(r, c)];

            if (target.type == ChessPieceType::NONE)
            {
                if (board_move(&aux_board, row, col, r, c))
                {
                    if (!analyzer_is_color_in_check(&aux_board, me.color))
                    {
                        moves.set(r, c);
                        restore_board();
                        return true;
                    }
                    restore_board();
                    return false; // cannot move here, would be in check
                }
                return false; // can continue sliding
            }
            else if (target.color == enemy)
            {
                if (board_move(&aux_board, row, col, r, c))
                {
                    if (!analyzer_is_color_in_check(&aux_board, me.color))
                    {
                        moves.set(r, c);
                        restore_board();
                        return true;
                    }
                    restore_board();
                    return false;
                }
                return false; // stop sliding
            };
            return false;
        };

        switch (me.type)
        {
        case ChessPieceType::PAWN:
        {
            int dir = (friendly == ChessPieceColor::PIECE_WHITE ? -1 : +1);
            // one-step
            int r1 = row + dir, c1 = col;
            if (r1 >= 0 && r1 < 8 && board->pieces[board_get_index(r1, c1)].type == ChessPieceType::NONE)
            {
                add_move(r1, c1);
                // two-step from home rank
                if (board_pawn_first_move(me, row))
                {
                    int r2 = row + 2 * dir;
                    if (board->pieces[board_get_index(r2, c1)].type == ChessPieceType::NONE)
                        add_move(r2, c1);
                }
            }
            // captures
            for (int dc : {-1, +1})
            {
                int c2 = col + dc, r2 = row + dir;
                if (r2 >= 0 && r2 < 8 && c2 >= 0 && c2 < 8)
                {
                    auto p = board->pieces[board_get_index(r2, c2)];
                    if (p.type != ChessPieceType::NONE && p.color == enemy)
                        add_move(r2, c2);
                }
            }

            // en passant
            // No need to check if the target square is empty because en passant captures are only valid
            // if the pawn has just moved two squares forward, which means the target square must be
            // empty at the moment of the en passant capture.
            if (me.color == ChessPieceColor::PIECE_WHITE && row == 3)
            {
                // Check for en passant capture to the left
                if (col > 0 && board->pieces[board_get_index(row, col - 1)].type == ChessPieceType::PAWN &&
                    board->pieces[board_get_index(row, col - 1)].color == ChessPieceColor::PIECE_BLACK &&
                    board->pieces[board_get_index(row, col - 1)].first_move_was_last_turn)
                {
                    add_move(row - 1, col - 1); // Capture to the left
                }
                // Check for en passant capture to the right
                if (col < 7 && board->pieces[board_get_index(row, col + 1)].type == ChessPieceType::PAWN &&
                    board->pieces[board_get_index(row, col + 1)].color == ChessPieceColor::PIECE_BLACK &&
                    board->pieces[board_get_index(row, col + 1)].first_move_was_last_turn)
                {
                    add_move(row - 1, col + 1); // Capture to the right
                }
            }
            else if (me.color == ChessPieceColor::PIECE_BLACK && row == 4)
            {
                // Check for en passant capture to the left
                if (col > 0 && board->pieces[board_get_index(row, col - 1)].type == ChessPieceType::PAWN &&
                    board->pieces[board_get_index(row, col - 1)].color == ChessPieceColor::PIECE_WHITE &&
                    board->pieces[board_get_index(row, col - 1)].first_move_was_last_turn)
                {
                    add_move(row + 1, col - 1); // Capture to the left
                }
                // Check for en passant capture to the right
                if (col < 7 && board->pieces[board_get_index(row, col + 1)].type == ChessPieceType::PAWN &&
                    board->pieces[board_get_index(row, col + 1)].color == ChessPieceColor::PIECE_WHITE &&
                    board->pieces[board_get_index(row, col + 1)].first_move_was_last_turn)
                {
                    add_move(row + 1, col + 1); // Capture to the right
                }
            }

            break;
        }

        case ChessPieceType::KNIGHT:
            for (auto [dr, dc] : KNIGHT_DELTAS)
            {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= 8 || c < 0 || c >= 8)
                    continue;
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::NONE || p.color == enemy)
                    add_move(r, c);
            }
            break;

        case ChessPieceType::BISHOP:
        case ChessPieceType::ROOK:
        case ChessPieceType::QUEEN:
        {
            for (auto [dr, dc] : KING_DELTAS)
            {
                bool slide = true;
                if (me.type == ChessPieceType::ROOK && dr != 0 && dc != 0)
                    slide = false;
                if (me.type == ChessPieceType::BISHOP && (dr == 0 || dc == 0))
                    slide = false;
                if (!slide && me.type == ChessPieceType::QUEEN)
                    slide = true;
                if (!slide)
                    continue;

                int r = row + dr, c = col + dc;
                while (add_move(r, c))
                {
                    r += dr;
                    c += dc;
                }
            }
            break;
        }

        case ChessPieceType::KING:
            for (auto [dr, dc] : KING_DELTAS)
            {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= 8 || c < 0 || c >= 8)
                    continue;
                auto p = board->pieces[board_get_index(r, c)];
                if ((p.type == ChessPieceType::NONE || p.color == enemy) && !analyzer_is_cell_under_attack_by_color(board, r, c, enemy))
                    moves.set(r, c);
            }

            if (me.moved == 0 && friendly == ChessPieceColor::PIECE_WHITE)
            {
                // Kingside castling
                if (board->pieces[board_get_index(7, 5)].type == ChessPieceType::NONE &&
                    board->pieces[board_get_index(7, 6)].type == ChessPieceType::NONE &&
                    board->pieces[board_get_index(7, 7)].type == ChessPieceType::ROOK &&
                    board->pieces[board_get_index(7, 7)].color == ChessPieceColor::PIECE_WHITE &&
                    board->pieces[board_get_index(7, 7)].moved == 0 &&
                    !analyzer_is_cell_under_attack_by_color(board, 7, 5, ChessPieceColor::PIECE_BLACK) &&
                    !analyzer_is_cell_under_attack_by_color(board, 7, 6, ChessPieceColor::PIECE_BLACK))
                {
                    moves.set(7, 6); // Castling move to g1
                }

                // Queenside castling
                if (board->pieces[board_get_index(7, 3)].type == ChessPieceType::NONE &&
                    board->pieces[board_get_index(7, 2)].type == ChessPieceType::NONE &&
                    board->pieces[board_get_index(7, 1)].type == ChessPieceType::NONE &&
                    board->pieces[board_get_index(7, 0)].type == ChessPieceType::ROOK &&
                    board->pieces[board_get_index(7, 0)].color == ChessPieceColor::PIECE_WHITE &&
                    board->pieces[board_get_index(7, 0)].moved == 0 &&
                    !analyzer_is_cell_under_attack_by_color(board, 7, 3, ChessPieceColor::PIECE_BLACK) &&
                    !analyzer_is_cell_under_attack_by_color(board, 7, 2, ChessPieceColor::PIECE_BLACK) &&
                    !analyzer_is_cell_under_attack_by_color(board, 7, 1, ChessPieceColor::PIECE_BLACK))
                {
                    moves.set(7, 2); // Castling move to c1
                }
            }
            break;
        default:
            break;
        }

        return moves;
    }

    bool
    analyzer_can_make_move(const Board *board, int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col, bool &is_check, bool &is_checkmate)
    {
        is_check = is_checkmate = false;

        // 1) Basic legality: is target in move bitboard?
        BitBoard avail = analyzer_get_available_moves_for_piece(board, from_row, from_col);
        if (!avail.get(to_row, to_col))
            return false;

        // 2) Simulate the move on a copy
        Board copy = *board;
        auto &cp = copy.pieces[board_get_index(to_row, to_col)];
        copy.pieces[board_get_index(to_row, to_col)] = copy.pieces[board_get_index(from_row, from_col)];
        copy.pieces[board_get_index(from_row, from_col)] = None;

        // 3) Find our king’s square
        ChessPieceColor me = board->pieces[board_get_index(from_row, from_col)].color;
        int kr = -1, kc = -1;
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c)
            {
                auto p = copy.pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KING && p.color == me)
                {
                    kr = r;
                    kc = c;
                }
            }

        // 4) Are we in check?
        if (analyzer_is_cell_under_attack_by_color(&copy, kr, kc,
                                                   me == ChessPieceColor::PIECE_WHITE ? ChessPieceColor::PIECE_BLACK : ChessPieceColor::PIECE_WHITE))
        {
            is_check = true;

            // 5) Check if any legal reply exists → not checkmate
            bool hasReply = false;
            for (int r1 = 0; r1 < 8 && !hasReply; ++r1)
                for (int c1 = 0; c1 < 8 && !hasReply; ++c1)
                {
                    if (copy.pieces[board_get_index(r1, c1)].color != me)
                        continue;
                    BitBoard m2 = analyzer_get_available_moves_for_piece(&copy, r1, c1);
                    for (uint64_t bb = m2.bits; bb && !hasReply; bb &= bb - 1)
                    {
                        int idx = __builtin_ctzll(bb);
                        int r2 = idx / 8, c2 = idx % 8;
                        bool dummy1, dummy2;
                        if (analyzer_can_make_move(&copy, r1, c1, r2, c2, dummy1, dummy2))
                            hasReply = true;
                    }
                }
            is_checkmate = !hasReply;
        }

        return !is_check;
    }

    bool
    analyzer_is_color_in_check(const Board *board, ChessPieceColor color)
    {
        int8_t kr = -1, kc = -1;
        for (int8_t r = 0; r < 8; ++r)
            for (int8_t c = 0; c < 8; ++c)
            {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KING && p.color == color)
                {
                    kr = r;
                    kc = c;
                }
            }

        if (kr == -1 || kc == -1)
            return false; // No king found, cannot be in check

        return analyzer_is_cell_under_attack_by_color(board, kr, kc,
                                                      color == ChessPieceColor::PIECE_WHITE ? ChessPieceColor::PIECE_BLACK : ChessPieceColor::PIECE_WHITE);
    }

} // namespace game

#include "board.hpp"
#include "move.hpp"
#include <array>
#include <cstdint>
#include <algorithm>
#include <cstddef>
#include "analyzer.hpp"

namespace game
{
    // Offsets for knight jumps:
    static constexpr std::array<std::pair<int32_t, int32_t>, 8> KNIGHT_DELTAS = {
        {{+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}, {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2}}};

    // Offsets for king moves (also used for pawn attack deltas and sliders increment):
    static constexpr std::array<std::pair<int32_t, int32_t>, 8> KING_DELTAS = {
        {{+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}}};

    static bool
    analyzer_is_pawn_attacking(const Board *board, const int32_t row, const int32_t col,
                               const Color attacker)
    {
        const int32_t pawn_dir = (attacker == PIECE_WHITE ? -1 : +1);
        for (const int32_t dc : {-1, +1})
        {
            const int32_t r = row + pawn_dir;
            if (const int32_t c = col + dc;
                r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[Board::board_get_index(r, c)];
                if (PIECE_TYPE(p) == PAWN && PIECE_COLOR(p) == attacker)
                    return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_knight_attacking(const Board *board, const int32_t row, const int32_t col,
                                 const Color attacker)
    {
        for (auto [dr, dc] : KNIGHT_DELTAS)
        {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[Board::board_get_index(r, c)];
                if (PIECE_TYPE(p) == KNIGHT && PIECE_COLOR(p) == attacker)
                    return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_slider_attacking(const Board *board, const int32_t row, const int32_t col,
                                 const Color attacker)
    {
        using enum PieceType;
        for (auto [dr, dc] : KING_DELTAS)
        {
            int32_t r = row + dr;
            int32_t c = col + dc;
            int32_t dist = 1;
            while (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                const auto p = board->pieces[Board::board_get_index(r, c)];
                if (PIECE_TYPE(p) == NONE)
                {
                    ++dist;
                    r += dr;
                    c += dc;
                    continue;
                }

                if (const bool is_diag = (dr != 0 && dc != 0);
                    PIECE_COLOR(p) != attacker || !(
                                                      PIECE_TYPE(p) == QUEEN || (PIECE_TYPE(p) == ROOK && !is_diag) || (PIECE_TYPE(p) == BISHOP && is_diag)))
                {
                    break;
                }
                return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_king_attacking(const Board *board, const int32_t row, const int32_t col,
                               const Color attacker)
    {
        for (auto [dr, dc] : KING_DELTAS)
        {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
            {
                auto p = board->pieces[Board::board_get_index(r, c)];
                if (PIECE_TYPE(p) == KING && PIECE_COLOR(p) == attacker)
                    return true;
            }
        }
        return false;
    }

    bool analyzer_is_cell_under_attack_by_color(
        const Board *board, const int32_t row, const int32_t col, const Color attacker)
    {
        if (analyzer_is_pawn_attacking(board, row, col, attacker))
        {
            return true;
        }

        if (analyzer_is_knight_attacking(board, row, col, attacker))
        {
            return true;
        }

        if (analyzer_is_slider_attacking(board, row, col, attacker))
        {
            return true;
        }

        return analyzer_is_king_attacking(board, row, col, attacker);
    }

    static bool
    analyzer_add_move(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                      const int32_t to_col, AvailableSquares &moves, const Color enemy)
    {
        const auto friendly = chess_piece_other_color(enemy);
        if (to_row < 0 || to_row >= 8 || to_col < 0 || to_col >= 8)
            return false;
        const auto target = board->pieces[Board::board_get_index(to_row, to_col)];
        Board aux_board = *board;
        if (PIECE_TYPE(target) == NONE)
        {
            aux_board.board_move_no_check(from_row, from_col, to_row, to_col);
            if (!analyzer_is_color_in_check(&aux_board, friendly))
            {
                moves.set(to_row, to_col);
            }
            return true; // Is not blocked can continue to try to add new moves
        }
        if (PIECE_COLOR(target) != enemy)
        {
            return false;
        }
        aux_board.board_move_no_check(from_row, from_col, to_row, to_col);
        if (!analyzer_is_color_in_check(&aux_board, friendly))
        {
            moves.set(to_row, to_col);
            return true;
        }
        return false;
    }

    static void
    analyzer_get_pawn_moves(const Board *board, const Piece piece, const int32_t row, const int32_t col,
                            const Color enemy, AvailableSquares &moves)
    {
        const auto friendly = PIECE_COLOR(piece);
        const int32_t dir = (friendly == PIECE_WHITE ? 1 : -1);
        // one-step
        const int32_t r1 = row + dir;
        if (const int32_t c1 = col; r1 >= 0 && r1 < 8 && PIECE_TYPE(board->pieces[Board::board_get_index(r1, c1)]) == NONE)
        {
            std::ignore = analyzer_add_move(board, row, col, r1, c1, moves, enemy);
            // two-step from home rank
            if (const int32_t r2 = row + 2 * dir;
                Board::board_pawn_first_move(piece, row) && PIECE_TYPE(board->pieces[Board::board_get_index(r2, c1)]) ==
                                                                NONE)
            {
                std::ignore = analyzer_add_move(board, row, col, r2, c1, moves, enemy);
            }
        }
        // captures
        for (const int32_t dc : {-1, +1})
        {
            const int32_t c2 = col + dc;
            const int32_t r2 = row + dir;
            if (r2 < 0 || r2 >= 8 || c2 < 0 || c2 >= 8)
            {
                continue;
            }
            if (const auto p = board->pieces[Board::board_get_index(r2, c2)];
                PIECE_TYPE(p) != NONE && PIECE_COLOR(p) == enemy)
            {
                std::ignore = analyzer_add_move(board, row, col, r2, c2, moves, enemy);
            }
        }

        // en passant
        // No need to check if the target square is empty because en passant captures are only valid
        // if the pawn has just moved two squares forward, which means the target square must be
        // empty at the moment of the en passant capture.
        if (PIECE_COLOR(piece) == PIECE_WHITE && row == 4)
        {
            // Check for en passant capture to the left
            if (board->board_can_en_passant_this(row, col - 1, enemy))
            {
                std::ignore = analyzer_add_move(board, row, col, row + 1, col - 1, moves, enemy); // Capture to the left
            }
            // Check for en passant capture to the right
            if (board->board_can_en_passant_this(row, col + 1, enemy))
            {
                std::ignore = analyzer_add_move(board, row, col, row + 1, col + 1, moves, enemy);
            }
        }
        else if (PIECE_COLOR(piece) == PIECE_BLACK && row == 3)
        {
            // Check for en passant capture to the left
            if (board->board_can_en_passant_this(row, col - 1, enemy))
            {
                std::ignore = analyzer_add_move(board, row, col, row - 1, col - 1, moves, enemy); // Capture to the left
            }
            // Check for en passant capture to the right
            if (board->board_can_en_passant_this(row, col + 1, enemy))
            {
                std::ignore = analyzer_add_move(board, row, col, row - 1, col + 1, moves, enemy);
            }
        }
    }

    static void
    analyzer_get_king_moves(const Board *board, const Piece piece, const int32_t row, const int32_t col,
                            const Color enemy, AvailableSquares &moves)
    {
        using enum PieceType;
        const auto friendly = PIECE_COLOR(piece);
        for (auto [dr, dc] : KING_DELTAS)
        {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                continue;
            std::ignore = analyzer_add_move(board, row, col, r, c, moves, enemy);
        }

        // Kingside castling
        if (!analyzer_is_color_in_check(board, friendly))
        {
            const uint8_t king_row = friendly ? 7 : 0;
            if (PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 5)]) == NONE &&
                PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 6)]) == NONE &&
                PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 7)]) == ROOK &&
                PIECE_COLOR(board->pieces[Board::board_get_index(king_row, 7)]) == friendly &&
                board->board_castle_rights_for(friendly, true) &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 5, chess_piece_other_color(friendly)) &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 6,
                                                        chess_piece_other_color(friendly)))
            {
                moves.set(king_row, 6); // Castling move to g1
            }

            // Queenside castling
            if (PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 3)]) == NONE &&
                PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 2)]) == NONE &&
                PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 1)]) == NONE &&
                PIECE_TYPE(board->pieces[Board::board_get_index(king_row, 0)]) == ROOK &&
                PIECE_COLOR(board->pieces[Board::board_get_index(king_row, 0)]) == friendly &&
                board->board_castle_rights_for(friendly, false) &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 3, chess_piece_other_color(friendly)) &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 2, chess_piece_other_color(friendly)) &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 1,
                                                        chess_piece_other_color(friendly)))
            {
                moves.set(king_row, 2); // Castling move to c1
            }
        }
    }

    static void
    analyzer_get_sliders_moves(const Board *board, const Piece piece, const int32_t row, const int32_t col,
                               const Color enemy, AvailableSquares &moves)
    {
        for (auto [dr, dc] : KING_DELTAS)
        {
            // skip impossible increment for ROOK and BISHOP
            if (PIECE_TYPE(piece) == ROOK && dr != 0 && dc != 0)
                continue;
            if (PIECE_TYPE(piece) == BISHOP && (dr == 0 || dc == 0))
                continue;

            int32_t r = row + dr;
            int32_t c = col + dc;
            auto is_enemy_next = [&r, &c, &board, &enemy]()
            {
                return PIECE_TYPE(board->pieces[Board::board_get_index(r, c)]) != NONE && PIECE_COLOR(
                                                                                              board->pieces[Board::board_get_index(r, c)]) == enemy;
            };
            // add until a move is blocked or encounters an enemy piece
            while (analyzer_add_move(board, row, col, r, c, moves, enemy) && !is_enemy_next())
            {
                r += dr;
                c += dc;
            }
        }
    }

    static void
    analyzer_get_poney_moves(const Board *board, const Piece piece, const int32_t row, const int32_t col,
                             const Color enemy, AvailableSquares &moves)
    {
        (void)piece;
        for (auto [dr, dc] : KNIGHT_DELTAS)
        {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                continue;
            auto p = board->pieces[Board::board_get_index(r, c)];
            if (PIECE_TYPE(p) == NONE || PIECE_COLOR(p) == enemy)
                analyzer_add_move(board, row, col, r, c, moves, enemy);
        }
    }

    AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board, const int32_t row, const int32_t col)
    {
        using enum PieceType;
        AvailableSquares moves{};
        moves.origin_index = Board::board_get_index(row, col);
        if (const auto piece = board->pieces[Board::board_get_index(row, col)]; PIECE_TYPE(piece) != NONE)
        {
            const auto enemy_color = chess_piece_other_color(PIECE_COLOR(piece));
            switch (PIECE_TYPE(piece))
            {
            case PAWN:
                analyzer_get_pawn_moves(board, piece, row, col, enemy_color, moves);
                break;
            case KNIGHT:
                analyzer_get_poney_moves(board, piece, row, col, enemy_color, moves);
                break;
            case BISHOP:
            case ROOK:
            case QUEEN:
                analyzer_get_sliders_moves(board, piece, row, col, enemy_color, moves);
                break;
            case KING:
                analyzer_get_king_moves(board, piece, row, col, enemy_color, moves);
                break;
            default:
                break;
            }
        }
        return moves;
    }

    bool
    analyzer_is_color_in_check(const Board *board, Color color)
    {
        int8_t kr = -1;
        int8_t kc = -1;
        for (int8_t r = 0; r < 8; ++r)
            for (int8_t c = 0; c < 8; ++c)
            {
                const auto p = board->pieces[Board::board_get_index(r, c)];
                if (PIECE_TYPE(p) == KING && PIECE_COLOR(p) == color)
                {
                    kr = r;
                    kc = c;
                    break;
                }
            }

        if (kr == -1 || kc == -1)
            return false; // No king found, cannot be in check

        return analyzer_is_cell_under_attack_by_color(board, kr, kc,
                                                      chess_piece_other_color(color));
    }

    bool
    analyzer_is_color_in_checkmate(const Board *board, Color color)
    {
        if (!analyzer_is_color_in_check(board, color))
        {
            return false;
        }
        for (uint8_t i = 0; i < 64; ++i)
        {
            if (PIECE_COLOR(board->pieces[i]) == color)
            {
                const auto [bits, origin] = analyzer_get_available_moves_for_piece(
                    board, Board::board_get_row(i), Board::board_get_col(i));
                if (bits != 0)
                {
                    return false;
                }
            }
        }
        return true;
    }

    AlgebraicMove
    analyzer_get_algebraic_move(const Board *board, const SimpleMove &move)
    {
        using enum PieceType;
        AlgebraicMove alg{};
        Board board_copy = *board;
        const int32_t fromIdx = Board::board_get_index(move.from_row, move.from_col);
        const int32_t toIdx = Board::board_get_index(move.to_row, move.to_col);

        const Piece moving = board->pieces[fromIdx];
        const Piece target = board->pieces[toIdx];

        alg.piece_type = PIECE_TYPE(moving);

        alg.from_row = move.from_row;

        alg.to_row = move.to_row;

        alg.from_col = move.from_col;
        alg.to_col = move.to_col;

        alg.en_passant = PIECE_TYPE(moving) == PAWN && move.from_col != move.to_col;
        alg.is_capture = (PIECE_TYPE(target) != NONE) || alg.en_passant;

        if (PIECE_TYPE(moving) == KING)
        {
            if (move.from_col == 4 && move.to_col == 6)
            {
                alg.kingside_castle = 1; // e1 to g1
            }
            else if (move.from_col == 4 && move.to_col == 2)
            {
                alg.queen_side_castle = 1; // e1 to c1
            }
        }

        board_copy.board_move_no_check(move.from_row, move.from_col, move.to_row, move.to_col);

        alg.is_checkmate = analyzer_is_color_in_checkmate(&board_copy, chess_piece_other_color(PIECE_COLOR(moving)));
        alg.is_check = !alg.is_checkmate && analyzer_is_color_in_check(&board_copy,
                                                                       chess_piece_other_color(PIECE_COLOR(moving)));

        for (int32_t i = 0; i < 64; ++i)
        {
            if (i == fromIdx || moving != board->pieces[i])
            {
                continue; // Skip the moved piece and the target square
            }

            const int32_t row = Board::board_get_row(i);
            const int32_t col = Board::board_get_col(i);
            const AvailableSquares moves = analyzer_get_available_moves_for_piece(board, row, col);
            if (moves.get(move.to_row, move.to_col))
            {
                if (col != move.from_col)
                {
                    alg.need_col_disambiguation = 1;
                }
                else if (row != move.from_row)
                {
                    alg.need_row_disambiguation = 1;
                }
            }
        }

        return alg;
    }

    Move
    analyzer_get_move_from_simple(const Board *board, const SimpleMove &move,
                                  PromotionPieceType promotion_type)
    {
        Move result{};
        result.set_origin(Board::board_get_index(move.from_row, move.from_col));
        result.set_destination(Board::board_get_index(move.to_row, move.to_col));
        if (board->board_is_en_passant(move.from_row, move.from_col, move.to_row, move.to_col))
        {
            result.set_origin(Board::board_get_index(move.to_row, move.to_col));
            result.set_destination(Board::board_get_index(move.from_row, move.to_col));
            result.set_special(Move::MOVE_EN_PASSANT);
        }
        else if (PIECE_TYPE(board->pieces[result.get_origin()]) == KING &&
                 ((move.from_col == 4 && move.to_col == 6) || (move.from_col == 4 && move.to_col == 2)))
        {
            result.set_special(Move::MOVE_CASTLE);
        }
        else if (board->board_pawn_is_being_promoted(move))
        {
            result.set_special(Move::MOVE_PROMOTION);
            result.set_promotion_piece(promotion_type);
        }
        else
        {
            result.set_special(Move::MOVE_NONE);
        }
        return result;
    }

    Square
    analyzer_where(const Board *board, const PieceType type, const Color color,
                   const int32_t disambiguation_col,
                   const int32_t disambiguation_row)
    {
        Square square{};
        for (uint8_t i = 0; i < 64; ++i)
        {
            const auto row = Board::board_get_row(i);
            const auto col = Board::board_get_col(i);
            if (disambiguation_col != -1 && disambiguation_col != col)
            {
                continue;
            }
            if (disambiguation_row != -1 && disambiguation_row != row)
            {
                continue;
            }

            if (const auto piece = board->pieces[i]; PIECE_TYPE(piece) == type && PIECE_COLOR(piece) == color)
            {
                square.row = static_cast<uint8_t>(row);
                square.col = static_cast<uint8_t>(col);
                return square; // Return the first found piece of the specified type and color
            }
        }
        return square; // Return an empty square if not found
    }

    int32_t
    analyzer_get_move_count(const Board *board, Color color)
    {
        int32_t count = 0;
        for (uint8_t i = 0; i < 64; ++i)
        {
            if (PIECE_COLOR(board->pieces[i]) == color)
            {
                const auto moves = analyzer_get_available_moves_for_piece(
                    board, Board::board_get_row(i), Board::board_get_col(i));
                count += moves.move_count();
            }
        }
        return count;
    }

    bool
    analyzer_get_is_stalemate(const Board *board, Color friendly)
    {
        return analyzer_get_move_count(board, friendly) == 0 && !analyzer_is_color_in_checkmate(board, friendly);
    }

    static bool
    square_is_light(int32_t sq)
    {
        // a1 = dark (0,0)->0, (row+col)&1 == 1 => light
        return ((Board::board_get_row(sq) + Board::board_get_col(sq)) & 1) != 0;
    }

    bool
    analyzer_is_insufficient_material(const Board *board)
    {
        using PT = PieceType;
        int32_t white_minors = 0, black_minors = 0;
        bool white_bsq[2] = {false, false}, // light/dark bishop present?
            black_bsq[2] = {false, false};

        for (int32_t sq = 0; sq < 64; ++sq)
        {
            auto p = board->pieces[sq];
            switch (PIECE_TYPE(p))
            {
            case PT::KING:
                break;
            case PT::PAWN:
            case PT::ROOK:
            case PT::QUEEN:
                // any pawn, rook or queen → sufficient
                return false;
            case PT::BISHOP:
            {
                bool isLight = square_is_light(sq);
                if (PIECE_COLOR(p) == PIECE_WHITE)
                {
                    ++white_minors;
                    white_bsq[isLight] = true;
                }
                else
                {
                    ++black_minors;
                    black_bsq[isLight] = true;
                }
                break;
            }
            case PT::KNIGHT:
                if (PIECE_COLOR(p) == PIECE_WHITE)
                    ++white_minors;
                else
                    ++black_minors;
                break;
            default:
                break;
            }
        }

        int32_t total_minors = white_minors + black_minors;

        // K vs K
        if (total_minors == 0)
            return true;

        // K+N or K+B vs K
        if (total_minors == 1)
            return true;

        // K+B vs K+B  (we treat any two-bishop endgame as insufficient)
        if (total_minors == 2 && white_minors == 1 && black_minors == 1)
            return true;

        // everything else (e.g. two knights, knight+bishop vs king, >2 minors…) is “sufficient”
        return false;
    }

} // namespace game

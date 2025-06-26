// analyzer.cpp
#include "board.hpp" // for Board, board_get_index, etc.
#include "move.hpp"  // for AlgebraicMove
#include <array>
#include <cstdint>
#include <algorithm>
#include "analyzer.hpp" // for analyzer_is_cell_under_attack_by_color, etc.

namespace game {
    // Offsets for knight jumps:
    static constexpr std::array<std::pair<int32_t, int32_t>, 8> KNIGHT_DELTAS = {
        {{+2, +1}, {+2, -1}, {-2, +1}, {-2, -1}, {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2}}
    };

    // Offsets for king moves (also used for pawn attack deltas and sliders increment):
    static constexpr std::array<std::pair<int32_t, int32_t>, 8> KING_DELTAS = {
        {{+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}}
    };

    static bool
    analyzer_is_pawn_attacking(const Board *board, const int32_t row, const int32_t col,
                               const ChessPieceColor attacker) {
        const int32_t pawn_dir = (attacker == PIECE_WHITE ? 1 : -1);
        for (const int32_t dc: {-1, +1}) {
            const int32_t r = row + pawn_dir;
            if (const int32_t c = col + dc;
                r >= 0 && r < 8 && c >= 0 && c < 8) {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::PAWN && p.color == attacker)
                    return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_knight_attacking(const Board *board, const int32_t row, const int32_t col,
                                 const ChessPieceColor attacker) {
        for (auto [dr, dc]: KNIGHT_DELTAS) {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KNIGHT && p.color == attacker)
                    return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_slider_attacking(const Board *board, const int32_t row, const int32_t col,
                                 const ChessPieceColor attacker) {
        using enum ChessPieceType;
        for (auto [dr, dc]: KING_DELTAS) {
            int32_t r = row + dr;
            int32_t c = col + dc;
            int32_t dist = 1;
            while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                const auto p = board->pieces[board_get_index(r, c)];
                if (p.type == NONE) {
                    ++dist;
                    r += dr;
                    c += dc;
                    continue;
                }

                if (const bool is_diag = (dr != 0 && dc != 0);
                    p.color != attacker || !(p.type == QUEEN || (p.type == ROOK && !is_diag) || (
                                                 p.type == BISHOP && is_diag))) {
                    break;
                }
                return true;
            }
        }
        return false;
    }

    static bool
    analyzer_is_king_attacking(const Board *board, const int32_t row, const int32_t col,
                               const ChessPieceColor attacker) {
        for (auto [dr, dc]: KING_DELTAS) {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KING && p.color == attacker)
                    return true;
            }
        }
        return false;
    }

    bool analyzer_is_cell_under_attack_by_color(
        const Board *board, const int32_t row, const int32_t col, const ChessPieceColor attacker) {
        if (analyzer_is_pawn_attacking(board, row, col, attacker)) {
            return true;
        }

        if (analyzer_is_knight_attacking(board, row, col, attacker)) {
            return true;
        }

        if (analyzer_is_slider_attacking(board, row, col, attacker)) {
            return true;
        }

        return analyzer_is_king_attacking(board, row, col, attacker);
    }

    static bool
    analyzer_add_move(const Board *board, const int32_t from_row, const int32_t from_col, const int32_t to_row,
                      const int32_t to_col, AvailableSquares &moves, ChessPieceColor enemy
    ) {
        const auto friendly = chess_piece_other_color(enemy);
        if (to_row < 0 || to_row >= 8 || to_col < 0 || to_col >= 8)
            return false;
        const auto target = board->pieces[board_get_index(to_row, to_col)];
        Board aux_board = *board;
        if (target.type == ChessPieceType::NONE) {
            board_move_no_check(&aux_board, from_row, from_col, to_row, to_col);
            if (!analyzer_is_color_in_check(&aux_board, friendly)) {
                moves.set(to_row, to_col);
                return true;
            }
            return false; // cannot move here, would be in check
        }
        if (target.color != enemy) {
            return false;
        }
        board_move_no_check(&aux_board, from_row, from_col, to_row, to_col);
        if (!analyzer_is_color_in_check(&aux_board, friendly)) {
            moves.set(to_row, to_col);
            return true;
        }
        return false;
    }

    static void
    analyzer_get_pawn_moves(const Board *board, const ChessPiece piece, const int32_t row, const int32_t col,
                            const ChessPieceColor enemy, AvailableSquares &moves) {
        const auto friendly = piece.color;
        const int32_t dir = (friendly == PIECE_WHITE ? 1 : -1);
        // one-step
        const int32_t r1 = row + dir;
        if (const int32_t c1 = col; r1 >= 0 && r1 < 8 && board->pieces[board_get_index(r1, c1)].type ==
                                    ChessPieceType::NONE) {
            std::ignore = analyzer_add_move(board, row, col, r1, c1, moves, enemy);
            // two-step from home rank
            if (const int32_t r2 = row + 2 * dir;
                board_pawn_first_move(piece, row) && board->pieces[board_get_index(r2, c1)].type ==
                ChessPieceType::NONE) {
                std::ignore = analyzer_add_move(board, row, col, r2, c1, moves, enemy);
            }
        }
        // captures
        for (const int32_t dc: {-1, +1}) {
            const int32_t c2 = col + dc;
            const int32_t r2 = row + dir;
            if (r2 >= 0 && r2 < 8 && c2 >= 0 && c2 < 8) {
                continue;
            }
            if (const auto p = board->pieces[board_get_index(r2, c2)];
                p.type != ChessPieceType::NONE && p.color == enemy) {
                std::ignore = analyzer_add_move(board, row, col, r2, c2, moves, enemy);
            }
        }

        // en passant
        // No need to check if the target square is empty because en passant captures are only valid
        // if the pawn has just moved two squares forward, which means the target square must be
        // empty at the moment of the en passant capture.
        if (piece.color == PIECE_WHITE && row == 4) {
            // Check for en passant capture to the left
            if (board_can_en_passant_this(board, row, col - 1, enemy)) {
                std::ignore = analyzer_add_move(board, row, col, row + 1, col - 1, moves, enemy); // Capture to the left
            }
            // Check for en passant capture to the right
            if (board_can_en_passant_this(board, row, col + 1, enemy)) {
                std::ignore = analyzer_add_move(board, row, col, row + 1, col + 1, moves, enemy);
            }
        } else if (piece.color == PIECE_BLACK && row == 3) {
            // Check for en passant capture to the left
            if (board_can_en_passant_this(board, row, col - 1, enemy)) {
                std::ignore = analyzer_add_move(board, row, col, row - 1, col - 1, moves, enemy); // Capture to the left
            }
            // Check for en passant capture to the right
            if (board_can_en_passant_this(board, row, col + 1, enemy)) {
                std::ignore = analyzer_add_move(board, row, col, row - 1, col + 1, moves, enemy);
            }
        }
    }

    static void
    analyzer_get_king_moves(const Board *board, const ChessPiece piece, const int32_t row, const int32_t col,
                            const ChessPieceColor enemy, AvailableSquares &moves) {
        using enum ChessPieceType;
        const auto friendly = piece.color;
        for (auto [dr, dc]: KING_DELTAS) {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                continue;
            auto p = board->pieces[board_get_index(r, c)];
            if ((p.type == NONE || p.color == enemy) && !analyzer_is_cell_under_attack_by_color(
                    board, r, c, enemy))
                moves.set(r, c);
        }

        if (piece.moved == 0) {
            // Kingside castling
            const uint8_t king_row = friendly ? 7 : 0;
            if (board->pieces[board_get_index(king_row, 5)].type == NONE &&
                board->pieces[board_get_index(king_row, 6)].type == NONE &&
                board->pieces[board_get_index(king_row, 7)].type == ROOK &&
                board->pieces[board_get_index(king_row, 7)].color == friendly &&
                board->pieces[board_get_index(king_row, 7)].moved == 0 &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 5, chess_piece_other_color(friendly))
                &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 6,
                                                        chess_piece_other_color(friendly))) {
                moves.set(king_row, 6); // Castling move to g1
            }

            // Queenside castling
            if (board->pieces[board_get_index(king_row, 3)].type == NONE &&
                board->pieces[board_get_index(king_row, 2)].type == NONE &&
                board->pieces[board_get_index(king_row, 1)].type == NONE &&
                board->pieces[board_get_index(king_row, 0)].type == ROOK &&
                board->pieces[board_get_index(king_row, 0)].color == friendly &&
                board->pieces[board_get_index(king_row, 0)].moved == 0 &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 3, chess_piece_other_color(friendly))
                &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 2, chess_piece_other_color(friendly))
                &&
                !analyzer_is_cell_under_attack_by_color(board, king_row, 1,
                                                        chess_piece_other_color(friendly))) {
                moves.set(king_row, 2); // Castling move to c1
            }
        }
    }

    static void
    analyzer_get_sliders_moves(const Board *board, const ChessPiece piece, const int32_t row, const int32_t col,
                               const ChessPieceColor enemy, AvailableSquares &moves) {
        for (auto [dr, dc]: KING_DELTAS) {
            // skip impossible increment for ROOK and BISHOP
            if (piece.type == ChessPieceType::ROOK && dr != 0 && dc != 0)
                continue;
            if (piece.type == ChessPieceType::BISHOP && (dr == 0 || dc == 0))
                continue;

            int32_t r = row + dr;
            int32_t c = col + dc;
            auto is_enemy_next = [&r,&c, &board, &enemy]() {
                return board->pieces[board_get_index(r, c)].type != ChessPieceType::NONE && board->pieces[
                           board_get_index(r, c)].color == enemy;
            };
            // add until a move is blocked encounters a piece, or is not valid
            while (analyzer_add_move(board, row, col, r, c, moves, enemy) && !is_enemy_next()) {
                r += dr;
                c += dc;
            }
        }
    }

    static void
    analyzer_get_poney_moves(const Board *board, const ChessPiece piece, const int32_t row, const int32_t col,
                             const ChessPieceColor enemy, AvailableSquares &moves) {
        (void) piece;
        for (auto [dr, dc]: KNIGHT_DELTAS) {
            const int32_t r = row + dr;
            const int32_t c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                continue;
            auto p = board->pieces[board_get_index(r, c)];
            if (p.type == ChessPieceType::NONE || p.color == enemy)
                analyzer_add_move(board, row, col, r, c, moves, enemy);
        }
    }

    AvailableSquares
    analyzer_get_available_moves_for_piece(const Board *board, const int32_t row, const int32_t col) {
        using enum ChessPieceType;
        AvailableSquares moves{};
        if (const auto piece = board->pieces[board_get_index(row, col)]; piece.type != NONE) {
            const auto enemy_color = chess_piece_other_color(piece.color);
            switch (piece.type) {
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
    analyzer_is_color_in_check(const Board *board, ChessPieceColor color) {
        int8_t kr = -1;
        int8_t kc = -1;
        for (int8_t r = 0; r < 8; ++r)
            for (int8_t c = 0; c < 8; ++c) {
                const auto p = board->pieces[board_get_index(r, c)];
                if (p.type == ChessPieceType::KING && p.color == color) {
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
    analyzer_is_color_in_checkmate(const Board *board, ChessPieceColor color) {
        if (!analyzer_is_color_in_check(board, color)) {
            return false;
        }
        for (uint8_t i = 0; i < 64; ++i) {
            if (board->pieces[i].color == color) {
                const auto [bits] = analyzer_get_available_moves_for_piece(board, board_get_row(i), board_get_col(i));
                if (bits != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    AlgebraicMove
    analyzer_get_algebraic_move(const Board *board, const SimpleMove &move) {
        using enum ChessPieceType;
        AlgebraicMove alg{};
        Board board_copy = *board;
        const int32_t fromIdx = board_get_index(move.from_row, move.from_col);
        const int32_t toIdx = board_get_index(move.to_row, move.to_col);

        const ChessPiece moving = board->pieces[fromIdx];
        const ChessPiece target = board->pieces[toIdx];

        alg.piece_type = moving.type;

        alg.from_row = move.from_row;

        alg.to_row = move.to_row;

        alg.from_col = move.from_col;
        alg.to_col = move.to_col;

        alg.en_passant = moving.type == PAWN && move.from_col != move.to_col;
        alg.is_capture = (target.type != NONE) || alg.en_passant;

        if (moving.type == KING) {
            if (move.from_col == 4 && move.to_col == 6) {
                alg.kingside_castle = 1; // e1 to g1
            } else if (move.from_col == 4 && move.to_col == 2) {
                alg.queen_side_castle = 1; // e1 to c1
            }
        }

        board_move_no_check(&board_copy, move.from_row, move.from_col, move.to_row, move.to_col);

        alg.is_checkmate = analyzer_is_color_in_checkmate(&board_copy, chess_piece_other_color(moving.color));
        alg.is_check = !alg.is_checkmate && analyzer_is_color_in_check(&board_copy,
                                                                       chess_piece_other_color(moving.color));

        for (int32_t i = 0; i < 64; ++i) {
            if (i == fromIdx || !chess_piece_equal(moving, board->pieces[i])) {
                continue; // Skip the moved piece and the target square
            }

            const int32_t row = board_get_row(i);
            const int32_t col = board_get_col(i);
            const AvailableSquares moves = analyzer_get_available_moves_for_piece(board, row, col);
            if (moves.get(move.to_row, move.to_col)) {
                if (col != move.from_col) {
                    alg.need_col_disambiguation = 1;
                } else if (row != move.from_row) {
                    alg.need_row_disambiguation = 1;
                }
            }
        }

        return alg;
    }

    Square
    analyzer_where(const Board *board, const ChessPieceType type, const ChessPieceColor color,
                   const int32_t disambiguation_col,
                   const int32_t disambiguation_row) {
        Square square{};
        for (uint8_t i = 0; i < 64; ++i) {
            const auto row = board_get_row(i);
            const auto col = board_get_col(i);
            if (disambiguation_col != -1 && disambiguation_col != col) {
                continue;
            }
            if (disambiguation_row != -1 && disambiguation_row != row) {
                continue;
            }

            if (const auto piece = board->pieces[i]; piece.type == type && piece.color == color) {
                square.row = static_cast<uint8_t>(row);
                square.col = static_cast<uint8_t>(col);
                return square; // Return the first found piece of the specified type and color
            }
        }
        return square; // Return an empty square if not found
    }

    AlgebraicMove
    analyzer_algebraic_move_from_str(const Board *board, const char *str, const ChessPieceColor turn) {
        using enum ChessPieceType;
        AlgebraicMove move{};
        size_t str_pointer = std::strlen(str);

        auto count_char = [](const char *s, char c) {
            size_t count = 0;
            while (*s) {
                if (*s == c) {
                    ++count;
                }
                ++s;
            }
            return count;
        };
        if (str_pointer <= 0) {
            return move;
        }

        if (str[str_pointer - 1] == '#') {
            move.is_checkmate = true;
            --str_pointer;
        } else if (str[str_pointer - 1] == '+') {
            move.is_check = true;
            --str_pointer;
        }

        if (str[str_pointer - 1] == '0') {
            move.piece_type = KING;
            const auto king_square = analyzer_where(board, KING, turn);
            move.from_col = king_square.col;
            move.from_row = king_square.row;
            if (count_char(str, '0') == 3 && count_char(str, '-') == 2) {
                move.kingside_castle = true;
                const auto rook_square = analyzer_where(board, ROOK, turn, 7);
                move.to_col = rook_square.col;
                move.to_row = rook_square.row;
            } else if (count_char(str, '0') == 4 && count_char(str, '-') == 3) {
                move.queen_side_castle = true;
                const auto rook_square = analyzer_where(board, ROOK, turn, 0);
                move.to_col = rook_square.col;
                move.to_row = rook_square.row;
            }
            return move;
        }

        // promotion
        if (chess_piece_is_piece_from_char(str[str_pointer - 1])) {
            --str_pointer;
            move.piece_type = PAWN;
            if (turn == PIECE_WHITE) {
                move.to_row = 7; // Promotion to row 7
                move.from_row = 6; // Where the pawn came
            } else {
                move.to_row = 0;
                move.from_row = 1;
            }
        }

        // get destination
        move.to_row = static_cast<uint8_t>(str[str_pointer - 1] - '0') - 1; // '1' for row 0, ..., '8' for row 7
        --str_pointer;
        move.to_col = static_cast<uint8_t>(str[str_pointer - 1] - 'a'); // 'a' for col 0, ..., 'h' for col 7
        --str_pointer;

        // pawn move - no capture
        if (str_pointer == 0) {
            for (auto i = move.to_col - 1; i >= 0; --i) {
            }
        }
    }
} // namespace game

#include "move.hpp"
#include <array>
#include "analyzer.hpp"
#include "board.hpp"
namespace game {
static constexpr char files[] = "abcdefgh";
static constexpr char ranks[] = "12345678";

static void disambiguate(Board &board, const Move move, AlgebraicMove &out) {
    const auto origin_idx = Board::get_index(move.from_row(), move.from_col());
    const Piece mover = board.pieces[origin_idx];
    if (PIECE_TYPE(mover) == PAWN) {
        return;
    }
    for (int32_t i = 0; i < SQUARE_COUNT; ++i) {
        if (i != origin_idx && board.pieces[i] == mover && analyzer_get_legal_moves_for_piece(&board, i).get(move.to_row(), move.to_col())) {
            if (Board::get_col(i) != move.from_col()) {
                out.push_back(files[move.from_col()]);
                continue;
            }
            if (Board::get_row(i) != move.from_row()) {
                out.push_back(ranks[move.from_row()]);
            }
        }
    }
}
AlgebraicMove move_to_algebraic(Board &board, const Move move) {

    AlgebraicMove result;
    if (move.is_castle()) {
        result = (move.get_destination() > move.get_origin()) ? "O-O" : "O-O-O";
        return result;
    }

    const auto origin_idx = Board::get_index(move.from_row(), move.from_col());
    const auto target_idx = Board::get_index(move.to_row(), move.to_col());
    const Piece mover = board.pieces[origin_idx];
    const Piece target = board.pieces[target_idx];

    if (PIECE_TYPE(mover) != PAWN) {
        static constexpr char piece_map[] = {'?', '?', 'N', 'B', 'R', 'Q', 'K'};
        result.push_back(piece_map[PIECE_TYPE(mover)]);
    }

    if (PIECE_TYPE(mover) == PAWN && PIECE_TYPE(target) != EMPTY) {
        result.push_back(files[move.from_col()]);
    }

    if (PIECE_TYPE(mover) == PAWN && move.is_en_passant()) {
        result.push_back(files[move.from_col()]);
        result.push_back('x');
    }

    disambiguate(board, move, result);

    if (PIECE_TYPE(target) != EMPTY) {
        result.push_back('x');
    }

    result.push_back(files[move.to_col()]);
    result.push_back(ranks[move.to_row()]);

    if (move.is_promotion()) {
        static constexpr char promo_map[] = {'Q', 'R', 'B', 'N'};
        const auto pidx = std::to_underlying(move.get_promotion_piece());
        result.push_back('=');
        result.push_back(promo_map[pidx]);
    }

    if (analyzer_move_puts_to_checkmate(&board, move)) {
        result.push_back('#');
    } else if (analyzer_move_puts_to_check(&board, move)) {
        result.push_back('+');
    }

    return result;
}

static MoveParserConversionError algebraic_pawn_to_move(const Color turn, const Board &board, const AlgebraicMove &move, Move &result) {
    using enum MoveParserConversionError;
    const bool capture = algebraic_is_capture(move);
    if (capture) {
        const auto capture_index = move.find('x');
        if (capture_index == 0) {
            result = Move{};
            return INVALID_NOTATION;
        }
        const auto file_index = capture_index - 1;
        const auto dest_index = file_index + 2;
        const auto destination = algebraic_get_index(move, static_cast<int8_t>(dest_index));
        if (destination == SQUARE_COUNT) {
            result = Move{};
            return COULD_NOT_PARSE_DESTINATION;
        }
        result.set_destination(destination);
        const bool is_en_passant = bitboard_get(board.pieces_by_type[EMPTY], destination);
        if (is_en_passant) {
            result.set_special(Move::MOVE_EN_PASSANT);
        }

        const auto file = file_of(move[file_index]);
        const auto rank = static_cast<Rank>(std::to_underlying(rank_of(destination)) - RowIncrement(turn));
        if (file < FILE_A || file > FILE_H || rank < RANK_1 || rank > RANK_8) {
            result = Move{};
            return INVALID_ORIGIN;
        }
        const auto origin = Board::get_index(rank, file);
        result.set_origin(static_cast<uint8_t>(origin));
    } else {
        const auto destination = algebraic_get_index(move, 0);
        result.set_destination(destination);
        const auto increment = (turn == PIECE_WHITE) ? WHITE_DIRECTION : BLACK_DIRECTION;
        auto origin = result.get_destination() + increment;
        if (origin < 0 || origin >= SQUARE_COUNT) {
            result = Move{};
            return INVALID_ORIGIN;
        }
        if (!bitboard_get(board.pieces_by_color[turn] & board.pieces_by_type[PAWN], origin)) {
            origin += increment;
        }

        result.set_origin(static_cast<uint8_t>(origin));
    }

    if (algebraic_is_promotion(move)) {
        result.set_special(Move::MOVE_PROMOTION);
        switch (move.last()) {
        case 'Q': result.set_promotion_piece(PROMOTION_QUEEN); break;
        case 'R': result.set_promotion_piece(PROMOTION_ROOK); break;
        case 'B': result.set_promotion_piece(PROMOTION_BISHOP); break;
        case 'N': result.set_promotion_piece(PROMOTION_KNIGHT); break;
        default : {
            result = Move{};
            return INVALID_PROMOTION_PIECE;
        }
        }
    } else {
        if (rank_of(result.get_destination()) == (turn == PIECE_WHITE ? RANK_8 : RANK_1)) {
            result = Move{};
            return PAWN_MOVE_TO_PROMOTION_RANK_WITHOUT_PROMOTION;
        }
    }

    if (capture && !bitboard_get(board.pieces_by_color[~turn], result.get_destination())) {
        result = Move{};
        return NO_PIECE_FOUND_AT_CAPTURE_DESTINATION;
    }

    return NONE;
}

static MoveParserConversionError algebraic_castle_to_move(const Color turn, const Board &, const AlgebraicMove &move, Move &result) {
    using enum MoveParserConversionError;

    const bool king_side = std::ranges::count_if(move, [](const char c) { return c == 'O'; }) == 2;
    result.set_special(Move::MOVE_CASTLE);
    if (king_side) {
        if (turn == PIECE_WHITE) {
            result.set_origin(E1);
            result.set_destination(G1);
        } else {
            result.set_origin(E8);
            result.set_destination(G8);
        }
    } else {
        if (turn == PIECE_WHITE) {
            result.set_origin(E1);
            result.set_destination(C1);
        } else {
            result.set_origin(E8);
            result.set_destination(C8);
        }
    }

    if (move != "O-O" && move != "O-O-O") {
        return INVALID_NOTATION;
    }

    return NONE;
}

static MoveParserConversionError algebraic_king_to_move(const Color turn, const Board &board, const AlgebraicMove &move, Move &result) {
    using enum MoveParserConversionError;
    const bool capture = algebraic_is_capture(move);
    const auto destination = algebraic_get_index(move, capture ? 2 : 1);
    if (destination == SQUARE_COUNT) {
        result = Move{};
        return COULD_NOT_PARSE_DESTINATION;
    }
    result.set_destination(destination);
    const auto origin = static_cast<uint8_t>(bitboard_index(board.pieces_by_color[turn] & board.pieces_by_type[KING]));
    result.set_origin(origin);
    if (origin == SQUARE_COUNT) {
        result = Move{};
        return INVALID_PIECE_TYPE;
    }
    if (capture && !bitboard_get(board.pieces_by_color[~turn], result.get_destination())) {
        result = Move{};
        return NO_PIECE_FOUND_AT_CAPTURE_DESTINATION;
    }
    return NONE;
}

static MoveParserConversionError algebraic_complex_to_move(const Color turn, const Board &board, const AlgebraicMove &move, Move &result) {
    using enum MoveParserConversionError;
    const PieceType type = algebraic_get_piece_type(move);
    const bool capture = algebraic_is_capture(move);
    const auto is_attacking = [type](const Board *b, const SquareIndex index, const Color attacker, const SquareIndex origin) {
        switch (type) {
        case KNIGHT: return analyzer_is_knight_attacking(b, index, attacker, origin);
        case BISHOP: return analyzer_is_bishop_attacking(b, index, attacker, origin);
        case ROOK  : return analyzer_is_rook_attacking(b, index, attacker, origin);
        case QUEEN : return analyzer_is_queen_attacking(b, index, attacker, origin);
        default    : return false;
        }
    };
    int32_t disambiguation_end = 0;
    SquareIndex destination;
    const auto disambiguation = algebraic_has_disambiguation(move, disambiguation_end);
    disambiguation_end += (capture ? 1 : 0); // If there is a capture, the disambiguation ends one character later
    const BitBoard bb = board.pieces_by_color[turn] & board.pieces_by_type[type];
    if (disambiguation != DisambiguationType::NONE) {
        destination = algebraic_get_index(move, static_cast<int8_t>(disambiguation_end));
        if (destination == SQUARE_COUNT) {
            result = Move{};
            return COULD_NOT_PARSE_DESTINATION;
        }
        result.set_destination(destination);
        bool has_origin = false;
        if (disambiguation == DisambiguationType::FILE) {
            const File f = file_of(move[1]); // If there is a capture, the file is at index 2
            if (f < FILE_A || f > FILE_H) {
                result = Move{};
                return INVALID_FILE_DISAMBIGUATION;
            }
            for (const auto it : BitBoardIterator(bb)) {
                if (file_of(it) == f && is_attacking(&board, destination, turn, it) && !has_origin) {
                    result.set_origin(it);
                    has_origin = true;
                } else if (file_of(it) == f && has_origin && is_attacking(&board, destination, turn, it)) {
                    result = Move{};
                    return RANK_DISAMBIGUATION_NEEDED;
                }
            }

            if (!has_origin) {
                result = Move{};
                return INVALID_FILE_DISAMBIGUATION;
            }

        } else if (disambiguation == DisambiguationType::RANK) {
            Rank r = rank_of(move[1]);
            if (r < RANK_1 || r > RANK_8) {
                result = Move{};
                return INVALID_RANK_DISAMBIGUATION;
            }
            for (const auto it : BitBoardIterator(bb)) {
                if (rank_of(it) == r && is_attacking(&board, destination, turn, it) && !has_origin) {
                    result.set_origin(it);
                    has_origin = true;
                } else if (rank_of(it) == r && has_origin && is_attacking(&board, destination, turn, it)) {
                    result = Move{};
                    return FILE_DISAMBIGUATION_NEEDED;
                }
            }

            if (!has_origin) {
                result = Move{};
                return INVALID_RANK_DISAMBIGUATION;
            }

        } else {
            const File f = file_of(move[1]);
            const Rank r = rank_of(move[2]);
            if (f < FILE_A || f > FILE_H || r < RANK_1 || r > RANK_8) {
                result = Move{};
                return INVALID_DISAMBIGUATION;
            }
            result.set_origin(static_cast<uint8_t>(Board::get_index(r, f)));
        }
    } else {
        destination = algebraic_get_index(move, 1 + (capture ? 1 : 0));
        if (destination == SQUARE_COUNT) {
            result = Move{};
            return COULD_NOT_PARSE_DESTINATION;
        }
        result.set_destination(destination);
        bool has_origin = false;
        for (const auto it : BitBoardIterator(bb)) {
            if (is_attacking(&board, destination, turn, it) && !has_origin) {
                result.set_origin(it);
                has_origin = true;
            } else if (is_attacking(&board, destination, turn, it) && has_origin) {
                result = Move{};
                return DISAMBIGUATION_NEEDED;
            }
        }
        if (!has_origin) {
            result = Move{};
            return NO_PIECE_FOUND_FOR_ORIGIN;
        }
    }
    if (capture && !bitboard_get(board.pieces_by_color[~turn], result.get_destination())) {
        result = Move{};
        return NO_PIECE_FOUND_AT_CAPTURE_DESTINATION;
    }

    return NONE;
}
MoveParserConversionError algebraic_to_move(const Color turn, const Board &board, const AlgebraicMove &move, Move &result) {
    using enum MoveParserConversionError;
    result = Move{};
    if (move.size() < MIN_ALGEBRAIC_MOVE_LENGTH) {
        return TOO_LITTLE_INFORMATION;
    }

    const bool capture = algebraic_is_capture(move);
    if (algebraic_is_castle(move)) {
        return algebraic_castle_to_move(turn, board, move, result);
    }
    const PieceType type = algebraic_get_piece_type(move);
    if (type == PAWN) {
        return algebraic_pawn_to_move(turn, board, move, result);
    }

    if ((capture && move.size() < 4) || move.size() < 3) {
        result = Move{};
        return TOO_LITTLE_INFORMATION;
    }

    if (type == KING) {
        return algebraic_king_to_move(turn, board, move, result);
    }

    return algebraic_complex_to_move(turn, board, move, result);
}

const char* conversion_error_to_string(const MoveParserConversionError e) noexcept {
    using enum MoveParserConversionError;
    switch (e) {
    case NONE                                         : return "Move parsed successfully.";
    case DISAMBIGUATION_NEEDED                        : return "Ambiguous move: Please disambiguate.";
    case FILE_DISAMBIGUATION_NEEDED                   : return "Ambiguous move: Need file disambiguation.";
    case RANK_DISAMBIGUATION_NEEDED                   : return "Ambiguous move: Need rank disambiguation.";
    case INVALID_FILE_DISAMBIGUATION                  : return "Invalid file in disambiguation: must be ‘a’ through ‘h’.";
    case INVALID_RANK_DISAMBIGUATION                  : return "Invalid rank in disambiguation: must be ‘1’ through ‘8’.";
    case INVALID_DISAMBIGUATION                       : return "Invalid disambiguation: please use a valid file or rank.";
    case PAWN_MOVE_TO_PROMOTION_RANK_WITHOUT_PROMOTION: return "Invalid promotion: Specify promotion piece type.";
    case INVALID_PROMOTION_PIECE                      : return "Invalid promotion piece type: use Q, R, B or N.";
    case COULD_NOT_PARSE_DESTINATION                  : return "Could not parse destination square: file (a-h) and rank(1-8).";
    case INVALID_PIECE_TYPE                           : return "Invalid piece type: use K, Q, R, B, N or omit for pawn.";
    case TOO_LITTLE_INFORMATION                       : return "Insufficient information: move notation too short.";
    case INVALID_NOTATION                             : return "Invalid algebraic notation: please check syntax.";
    case NO_PIECE_FOUND_FOR_ORIGIN                    : return "Invalid origin square: no compatible piece found.";
    case INVALID_ORIGIN                               : return "Invalid origin square: must be a valid square on the board.";
    case NO_PIECE_FOUND_AT_CAPTURE_DESTINATION        : return "Invalid capture: Destination is empty";
    }
    return "Unknown error.";
}

DisambiguationType algebraic_has_disambiguation(const AlgebraicMove &move, int32_t &disambiguation_end) {
    using enum DisambiguationType;
    Assert(!move.empty() && (move[0] == 'Q' || move[0] == 'R' || move[0] == 'B' || move[0] == 'N'), "Invalid piece type in disambiguation notation");
    disambiguation_end = 0;
    // If the move is too short, it cannot have disambiguation it requires the piece type + destination (3 characters) + extra for disambiguation
    if (move.size() <= 3) {
        return NONE;
    }

    // Disambiguation happens before any capture notation
    if (move[1] == 'x') {
        return NONE;
    }

    if (move[0] == 'Q' || move[0] == 'R' || move[0] == 'B' || move[0] == 'N') {
        // If the second character is a rank it is a disambiguation
        if (gtr::is_numeric(move[1])) {
            disambiguation_end = 2;
            return RANK;
        }
        // If the second character is a file and the third character is also a file, it is a disambiguation
        if (gtr::is_alpha(move[1]) && gtr::is_alpha(move[2])) {
            disambiguation_end = 2;
            return FILE;
        }
        // Full disambiguation
        if (gtr::is_alpha(move[1]) && gtr::is_numeric(move[2]) && gtr::is_alpha(move[3])) {
            disambiguation_end = 3;
            return FILE_RANK;
        }
    }
    return NONE;
}
} // namespace game

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
        if (i != origin_idx && board.pieces[i] == mover && analyzer_is_move_legal(&board, {Board::get_row(i), Board::get_col(i), move.to_row(), move.to_col()})) {
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

    AlgebraicMove result{};
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

    if (PIECE_TYPE(target) != EMPTY) {
        result.push_back('x');
    }

    disambiguate(board, move, result);

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

Move algebraic_to_move(const Color turn, const Board& board, const AlgebraicMove& move) {
    Move result{};


    return result;
}

} // namespace game

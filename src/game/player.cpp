#include "player.hpp"
#include "analyzer.hpp"
#include <random>

namespace game {
Move Human::get_move(Board &) { return {}; }

std::random_device DrunkMan::rd;
std::mt19937 DrunkMan::gen(DrunkMan::rd());

Move DrunkMan::get_move(Board &b) {
    static constexpr std::array promotions_pieces = {PROMOTION_QUEEN, PROMOTION_ROOK, PROMOTION_BISHOP, PROMOTION_KNIGHT};

    std::array<AvailableMoves, 16> moves_per_piece{};
    int32_t piece_index = 0;
    for (int32_t i = 0; i < 64; i++) {
        if (PIECE_COLOR(b[i]) == player.color) {
            auto moves = analyzer_get_legal_moves_for_piece(&b, i);
            if (moves.move_count()) {
                moves_per_piece[piece_index] = moves;
                piece_index++;
            }
        }
    }

    if (piece_index < 1) {
        // No available moves 
        return {};
    }
    std::uniform_int_distribution piece_dist(0, piece_index - 1);
    const int32_t piece_selected = piece_dist(gen);
    const AvailableMoves &moves = moves_per_piece[piece_selected];
    const auto count = moves.move_count();

    std::uniform_int_distribution move_dist(0, static_cast<int32_t>(count - 1));
    int64_t move_selected = move_dist(gen);

    for (int32_t i = 0; i < 64; i++) {
        if (moves.get(i)) {
            if (move_selected != 0) {
                move_selected--;
                continue;
            }
            const SimpleMove move{
                static_cast<uint8_t>(Board::get_row(moves.origin_index)),
                static_cast<uint8_t>(Board::get_col(moves.origin_index)),
                static_cast<uint8_t>(Board::get_row(i)),
                static_cast<uint8_t>(Board::get_col(i)),
            };
            PromotionPieceType promotion_type = PROMOTION_QUEEN;
            if (b.pawn_is_being_promoted(move)) {
                std::uniform_int_distribution promotion_dist(0, 3);
                promotion_type = promotions_pieces[promotion_dist(gen)];
            }
            return analyzer_get_move_from_simple(&b, move, promotion_type);
        }
    }
    return {};
}
} // namespace game

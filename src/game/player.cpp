#include "player.hpp"
#include "analyzer.hpp"
namespace game
{

    Move
    Human::get_move(const Board &b)
    {
        return {};
    }

    std::random_device DrunkMan::rd;
    std::mt19937 DrunkMan::gen(DrunkMan::rd());

    Move
    DrunkMan::get_move(const Board &b)
    {
        static constexpr std::array<PromotionPieceType, 4> promotions_pieces = {
            PROMOTION_QUEEN, PROMOTION_ROOK, PROMOTION_BISHOP, PROMOTION_KNIGHT};

        AvailableSquares moves_per_piece[16];
        int32_t piece_index = 0;
        for (int32_t i = 0; i < 64; i++)
        {
            if (PIECE_COLOR(b[i]) == player.color)
            {

                auto moves = analyzer_get_available_moves_for_piece(&b, i);
                if (moves.move_count())
                {
                    moves_per_piece[piece_index] = moves;
                    piece_index++;
                }
            }
        }

        std::uniform_int_distribution<int32_t> piece_dist(0, piece_index - 1);
        int32_t piece_selected = piece_dist(gen);
        AvailableSquares &moves = moves_per_piece[piece_selected];
        auto count = moves.move_count();

        std::uniform_int_distribution<int32_t> move_dist(0, count - 1);
        int32_t move_selected = move_dist(gen);

        for (int32_t i = 0; i < 64; i++)
        {
            if (moves.get(i))
            {
                if (move_selected == 0)
                {
                    SimpleMove move;
                    move.from_row = Board::board_get_row(moves.origin_index);
                    move.from_col = Board::board_get_col(moves.origin_index);
                    move.to_col = Board::board_get_col(i);
                    move.to_row = Board::board_get_row(i);
                    PromotionPieceType promotion_type = PROMOTION_QUEEN;
                    if (b.board_pawn_is_being_promoted(move))
                    {
                        std::uniform_int_distribution<int32_t> promotion_dist(0, 3);
                        promotion_type = promotions_pieces[promotion_dist(gen)];
                    }
                    return analyzer_get_move_from_simple(&b, move, promotion_type);
                }
                move_selected--;
            }
        }
        return {};
    }

}
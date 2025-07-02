#include "player.hpp"
#include "analyzer.hpp"
namespace game
{

    SimpleMove
    Human::get_move(const Board &b)
    {
        return {};
    }

    std::random_device DrunkMan::rd;
    std::mt19937 DrunkMan::gen(DrunkMan::rd());

    SimpleMove
    DrunkMan::get_move(const Board &b)
    {
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
                    return move;
                }
                move_selected--;
            }
        }
        return {};
    }

}
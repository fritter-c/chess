#pragma once
#include "board.hpp"
#include "player.hpp"
namespace game
{
    struct Game
    {
        // Updated every move
        enum class GameStatus
        {
            WHITE_TURN,
            BLACK_TURN,
            WHITE_CHECKMATE,
            BLACK_CHECKMATE,
            WHITE_STALEMATE,
            BLACK_STALEMATE,
            INSUFFICIENT_MATERIAL,
            NONE
        };

        // Updated every game tick
        enum class GameWinner
        {
            WHITE,
            BLACK,
            DRAW,
            NONE
        };
        

        Board board;
        Player white_player;
        Player black_player;
        Color turn{PIECE_WHITE};
        GameStatus status{GameStatus::WHITE_TURN};
        GameWinner winner{GameWinner::NONE};
        uint64_t move_count{0};

        Game();

        template <typename T>
        void set_player(Color c, T player)
        {
            if (c == PIECE_WHITE)
            {
                white_player.emplace<T>(player);
                player_init(white_player, PIECE_WHITE);
            }
            else
            {
                black_player.emplace<T>(player);
                player_init(black_player, PIECE_BLACK);
            }
        }

        bool move(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col);
        bool move(const SimpleMove &sm)
        {
            return move(sm.from_row, sm.from_col, sm.to_row, sm.to_col);
        }
        bool board_in_check();
        void tick();

        const char* get_status_string() const;
        const char* get_winner_string() const;
    };
}
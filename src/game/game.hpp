#pragma once
#include "board.hpp"
#include "player.hpp"
#include "analyzer.hpp"
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
            INVALID
        };

        // Updated every game tick
        enum class GameWinner
        {
            WHITE,
            BLACK,
            DRAW,
            PLAYING
        };

        Board board{};
        Player white_player{};
        Player black_player{};
        Color turn{PIECE_WHITE};
        GameStatus status{GameStatus::WHITE_TURN};
        GameWinner winner{GameWinner::PLAYING};
        uint64_t move_count{0};

        Game();

        template <typename T>
        void set_player(const Color c, T player)
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

        inline bool
        move(int32_t from_row, int32_t from_col, int32_t to_row, int32_t to_col,
             PromotionPieceType promotion_type = PROMOTION_QUEEN)
        {
            return move(analyzer_get_move_from_simple(&board, SimpleMove{static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col), 
                                                       static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col)}, promotion_type));
        }

        bool move(const Move &move);
        bool board_in_check() const;
        void tick();

        const char *get_status_string() const;
        const char *get_winner_string() const;
    };
}
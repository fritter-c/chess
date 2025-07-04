#pragma once
#include "piece.hpp"
#include <variant>
#include "move.hpp"
#include "board.hpp"
#include <random>
namespace game
{
    struct PlayerStatus
    {
        Color color;
        uint64_t time_left;
        uint32_t moves_made;
        uint32_t piece_score;
    };

    struct Human /* non-AI */
    {
        PlayerStatus player;

        void init(Color c)
        {
            player.color = c;
        }
        Move get_move(const Board &b);
    };

    struct DrunkMan
    {
        PlayerStatus player;
        static std::random_device rd;
        static std::mt19937 gen;

        void init(Color c)
        {
            player.color = c;
        };
        Move get_move(const Board &b);
    };

    using Player = std::variant<Human, DrunkMan>;

    inline bool
    player_is_ai(Player &p)
    {
        return !std::holds_alternative<Human>(p);
    }

    inline Move
    player_get_move(Player &p, const Board &b)
    {
        return std::visit(
            [&b](auto &player)
            {
                return player.get_move(b);
            },
            p);
    }

    inline void
    player_init(Player &p, Color c)
    {
        return std::visit([c](auto &player)
                          { player.init(c); }, p);
    }
}
#pragma once
#include <random>
#include <variant>
#include "board.hpp"
#include "move.hpp"
#include "piece.hpp"

namespace game {
struct PlayerStatus {
    Color color;
    uint64_t time_left;
    uint32_t moves_made;
    uint32_t piece_score;
    PlayerStatus() : color(PIECE_WHITE), time_left(0), moves_made(0), piece_score(0) {}
    PlayerStatus(Color c) : color(c), time_left(0), moves_made(0), piece_score(0) {}
};


struct Human /* non-AI */
{
    PlayerStatus player;
    void init(const Color c) { player.color = c; }

    Move get_move(Board &b);
};

struct DrunkMan {
    PlayerStatus player;
    static std::random_device rd;
    static std::mt19937 gen;

    void init(const Color c) { player.color = c; };

    Move get_move(Board &b);
};

using Player = std::variant<Human, DrunkMan>;

inline bool player_is_ai(const Player &p) { return !std::holds_alternative<Human>(p); }


template <typename T>
inline Move player_get_move(T &p, Board &b) {
    return p.get_move(b);
}
template <>
inline Move player_get_move(Player &p, Board &b) {
    return std::visit([&b](auto &player) { return player.get_move(b); }, p);
}

inline void player_init(Player &p, Color c) {
    return std::visit([c](auto &player) { player.init(c); }, p);
}
} // namespace game

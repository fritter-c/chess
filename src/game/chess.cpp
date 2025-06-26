#include "chess.hpp"
#include <algorithm>

namespace game
{

    utils::short_string
    game_mode_to_string(ChessMode mode)
    {
        switch (mode)
        {
        case FREE_MODE:
            return "Free";
        case EVALUATION_MODE:
            return "Evaluation";
        case GAME_MODE:
            return "Game";
        }
        return "Unknown";
    }
} // namespace game

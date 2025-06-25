#pragma once
#include <cstdint>
#include "board.hpp"


namespace game{
    struct Position{
        Board board;
        int8_t last_moved_piece_index;
    };

}
#pragma once
#include "analyzer.hpp"
#include "piece.hpp"

namespace game {
    struct User {
        ChessPieceColor color;

        SimpleMove get_move(const Board *board) const {
            (void) color;
            return {};
        }
    };

    struct DrunkMan {
        ChessPieceColor color;

        SimpleMove get_move(const Board *board) const;
    };
}

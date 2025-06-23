#pragma once
#include "piece.hpp"
namespace game
{
    struct Player
    {
        ChessPieceColor color; // The color of the player's pieces
        uint64_t time_left;    // The time left for the player in milliseconds
        uint32_t moves_made;   // The number of moves made by the player
        uint32_t piece_score;  // The score of the player's pieces
    };
}
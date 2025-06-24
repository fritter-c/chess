#pragma once
#include "board.hpp"
#include "player.hpp"
#include "move.hpp"

namespace game
{
    struct ChessGame
    {
        Board board;                  // The chess board
        Player white_player;          // The player with white pieces
        Player black_player;          // The player with black pieces
        ChessPieceColor current_turn; // The color of the player whose turn it is
        Board *history;               // History of moves made in the game
        uint32_t history_size;        // The size of the history array
        uint32_t history_capacity;    // The capacity of the history array
        uint32_t history_index;       // The current index in the history array
        AlgebraicMove *moves;         // Array of moves made in the game
        uint32_t moves_size;          // The size of the moves array
        uint32_t moves_capacity;      // The capacity of the moves array
        uint32_t moves_index;         // The current index in the moves array
    };

    void
    chess_game_initialize(ChessGame *game);
}
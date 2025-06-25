#include "chess.hpp"
#include <algorithm>

namespace game {

    void
    chess_game_initialize(ChessGame *game) {
        memset(game, 0, sizeof(ChessGame));
        game->white_player.color = ChessPieceColor::PIECE_WHITE;
        game->black_player.color = ChessPieceColor::PIECE_BLACK;
    }
} // namespace game

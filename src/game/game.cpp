#include "game.hpp"
namespace game {
    Game::Game() {
        board.init();
        board.board_populate();
    }
}

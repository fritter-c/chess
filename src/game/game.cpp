#include "game.hpp"
#include "analyzer.hpp"
namespace game {
Game::Game() {
    board.init();
    board.populate();
    push_move(AlgebraicMove{""});
}

static Player &game_get_player(Game *g, Color c) { return c == PIECE_WHITE ? g->white_player : g->black_player; }

static void game_update_status(Game *g) {
    using enum Game::GameStatus;
    if (analyzer_is_color_in_checkmate(&g->board, PIECE_BLACK)) {
        g->status = BLACK_CHECKMATE;
    } else if (analyzer_is_color_in_checkmate(&g->board, PIECE_WHITE)) {
        g->status = WHITE_CHECKMATE;
    } else if (analyzer_get_is_stalemate(&g->board, PIECE_WHITE)) {
        g->status = WHITE_STALEMATE;
    } else if (analyzer_get_is_stalemate(&g->board, PIECE_BLACK)) {
        g->status = BLACK_STALEMATE;
    } else if (analyzer_is_insufficient_material(&g->board)) {
        g->status = INSUFFICIENT_MATERIAL;
    } else if (g->board.side_to_move == PIECE_WHITE) {
        g->status = WHITE_TURN;
    } else if (g->board.side_to_move  == PIECE_BLACK) {
        g->status = BLACK_TURN;
    } else {
        g->status = INVALID;
    }
}

bool game_is_playable(const Game *g) {
    using enum Game::GameStatus;
    return g->status != WHITE_CHECKMATE && g->status != BLACK_CHECKMATE && g->status != WHITE_STALEMATE && g->status != BLACK_STALEMATE && g->status != INSUFFICIENT_MATERIAL;
}

bool Game::move(const Move &move) {
    if (!game_is_playable(this)) {
        return false;
    }
    AvailableMoves moves = analyzer_get_legal_moves_for_piece(&board, move.from_row(), move.from_col());
    if (board.get_color(move.from_row(), move.from_col()) == board.side_to_move && moves.get(move.to_row(), move.to_col())) {
        AlgebraicMove algebraic_move;
        board.move(move, algebraic_move);
        game_update_status(this);
        push_move(algebraic_move);
        ++move_count;
        return true;
    }
    return false;
}

bool Game::redo() {
    if (board.redo()) {
        game_update_status(this);
        redo_move();
        ++move_count;
        return true;
    }
    return false;
}

bool Game::undo() {
    if (board.undo()) {
        game_update_status(this);
        undo_move();
        --move_count;
        return true;
    }
    return false;
}

bool Game::random_move() {
    DrunkMan player{PlayerStatus{board.side_to_move}};
    return move(player_get_move(player, board));
}

void Game::return_last_move() {
    while (redo()) {/** Redo until it cannot */}
}

void Game::return_first_move() {
    while (undo()) {/** Undo until it cannot */}
}

bool Game::board_in_check() { return analyzer_is_color_in_check(&board, PIECE_WHITE) || analyzer_is_color_in_check(&board, PIECE_BLACK); }

void Game::tick() {
    using enum GameStatus;
    using enum GameWinner;
    if (game_is_playable(this)) {
        winner = PLAYING;
        if (player_is_ai(game_get_player(this, board.side_to_move))) {
            move(player_get_move(game_get_player(this, board.side_to_move), board));
        }
    } else {
        if (status == WHITE_CHECKMATE) {
            winner = BLACK;
        } else if (status == BLACK_CHECKMATE) {
            winner = WHITE;
        } else if (status == WHITE_STALEMATE || status == BLACK_STALEMATE || status == INSUFFICIENT_MATERIAL) {
            winner = DRAW;
        }
    }
}

void Game::reset() {
    board = Board{};
    board.init();
    board.populate();
    status = GameStatus::WHITE_TURN;
    winner = GameWinner::PLAYING;
    move_count = 0;
    move_list.clear();
    push_move(AlgebraicMove{""});
}

const char *Game::get_status_string() const {
    using enum GameStatus;
    switch (status) {
    case WHITE_TURN           : return "White's turn";
    case BLACK_TURN           : return "Black's turn";
    case WHITE_CHECKMATE      : return "White is in checkmate";
    case BLACK_CHECKMATE      : return "Black is in checkmate";
    case WHITE_STALEMATE      : return "White is in stalemate";
    case BLACK_STALEMATE      : return "Black is in stalemate";
    case INSUFFICIENT_MATERIAL: return "Insufficient material";
    default                   : return "Unknown game status";
    }
}

const char *Game::get_winner_string() const {
    using enum GameWinner;
    switch (winner) {
    case WHITE: return "White wins";
    case BLACK: return "Black wins";
    case DRAW : return "Draw";
    default   : return "Playing";
    }
}
} // namespace game

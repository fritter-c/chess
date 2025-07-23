#pragma once
#include "analyzer.hpp"
#include "board.hpp"
#include "history.hpp"
#include "player.hpp"

namespace game {
struct Game {
    // Updated every move
    enum class GameStatus { WHITE_TURN, BLACK_TURN, WHITE_CHECKMATE, BLACK_CHECKMATE, WHITE_STALEMATE, BLACK_STALEMATE, INSUFFICIENT_MATERIAL, INVALID };

    // Updated every game tick
    enum class GameWinner { WHITE, BLACK, DRAW, PLAYING };

    Board board{};
    Player white_player{};
    Player black_player{};
    GameStatus status{GameStatus::WHITE_TURN};
    GameWinner winner{GameWinner::PLAYING};
    uint64_t move_count{0};
    history<AlgebraicMove> move_list{};

    Game();

    template <typename T> void set_player(const Color c, T player) {
        if (c == PIECE_WHITE) {
            white_player.emplace<T>(player);
            player_init(white_player, PIECE_WHITE);
        } else {
            black_player.emplace<T>(player);
            player_init(black_player, PIECE_BLACK);
        }
    }

    bool move(const int32_t from_row, const int32_t from_col, const int32_t to_row, const int32_t to_col, const PromotionPieceType promotion_type = PROMOTION_QUEEN) {
        return move(analyzer_get_move_from_simple(
            &board, SimpleMove{static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col), static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col)}, promotion_type));
    }
    
    bool random_move();
    bool move(const Move &move);
    bool undo();
    bool redo();
    void return_last_move();
    void return_first_move();
    bool board_in_check();
    void tick();
    void reset();
    const char *get_status_string() const;
    const char *get_winner_string() const;
    void push_move(const AlgebraicMove &move) { move_list.push(move); }
    void pop_move() { move_list.pop(); }
    void undo_move() { move_list.undo(); }
    void redo_move() { move_list.redo(); }
};
} // namespace game

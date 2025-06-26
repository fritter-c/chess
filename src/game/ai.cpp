#include "ai.hpp"
#include <cstdlib>
#include <cmath>

namespace game {
    SimpleMove DrunkMan::get_move(const Board *board) const {
        SimpleMove move{};
        const auto count = board_get_piece_count(board, color);
        int32_t current_color_piece = 0;
        const int32_t target_piece = std::abs(rand()) % count;
        for (int32_t i = 0; i < 64; ++i) {
            if (board->pieces[i].color != color) {
                continue;
            }
            if (target_piece == current_color_piece) {
                const AvailableSquares avail = analyzer_get_available_moves_for_piece(board, i);
                int32_t move_count = avail.move_count();
                const int32_t target_move = std::abs(rand()) % move_count;
                for (int32_t j = 0; j < 64; ++j) {
                    if (avail.get(j)) {
                        if (move_count == target_move) {
                            move.from_row = static_cast<uint8_t>(board_get_row(i));
                            move.from_col = static_cast<uint8_t>(board_get_col(i));
                            move.to_col = static_cast<uint8_t>(board_get_col(j));
                            move.to_row = static_cast<uint8_t>(board_get_col(j));
                            break;
                        }
                        --move_count;
                    }
                }

            }
            current_color_piece++;
        }
        return move;
    }
}

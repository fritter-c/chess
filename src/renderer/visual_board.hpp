#pragma once
#include "../game/board.hpp"
#include "imgui_extra.hpp"
#include "vec2.hpp"
#include "vector.hpp"
namespace game {
struct Game;
}
namespace renderer {

struct Arrow {
    uint32_t color{};
    int8_t from_square{-1};
    int8_t to_square{-1};
    uint8_t width{};
};
struct VisualBoard {
    static constexpr gtr::vec2 PIECE_SIZE = gtr::vec2(128.0f, 128.0f);
    float board_size{};
    float cell_size{};
    gtr::vec2 board_offset{};
    bool flipped{false};
    int32_t dragging_piece_index{-1};
    int32_t row_hovered{-1};
    int32_t col_hovered{-1};
    int32_t origin_arrow_square{-1};
    int32_t destination_arrow_square{-1};
    game::AvailableMoves available_squares_for_dragging{};
    bool waiting_promotion{false};
    game::SimpleMove promotion_move{};
    gtr::vector<Arrow> arrows{};
    game::BitBoard selected_squares_bitboard{};

    void render(game::Game *game, float width, float height);
    void flip_board() { flipped = !flipped; }
    void clear_arrows() { arrows.clear(); }
};

GLuint get_piece_texture(game::Piece piece);
bool load_board_resources();
void release_board_resources();
} // namespace renderer

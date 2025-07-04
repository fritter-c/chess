#include <array>
#include <filesystem>
#include "imgui.h"
#include "imgui_extra.hpp"
#include "../game/board.hpp"
#include "../third/miniaudio.h"

namespace game
{
    struct Game;
}

namespace renderer
{
    struct VisualBoard
    {
        static constexpr ImVec2 PIECE_SIZE = ImVec2(128.0f, 128.0f);
        std::array<GLuint, 12> chess_pieces_textures{ImGui::INVALID_TEXTURE_ID};
        float board_size{};
        float cell_size{};
        ImVec2 board_offset{};
        bool flipped{false};
        int32_t dragging_piece_index{-1};
        ma_engine sound_engine;
        ma_sound move_sound;
        ma_sound check_sound;
        int32_t row_hovered{-1};
        int32_t col_hovered{-1};
        game::AvailableSquares available_squares_for_dragging{};
        bool waiting_promotion{false};
        game::SimpleMove promotion_move{};

        VisualBoard() = default;
        ~VisualBoard() { cleanup(); }

        bool load_textures(const std::filesystem::path &res_path);
        void render(game::Game *game, float width, float height);
        void cleanup();

        inline void 
        flip_board() { flipped = !flipped; }

        inline GLuint
        get_piece_texture(game::Piece piece) const
        {
            static constexpr std::array piece_to_img_pos{0, 0, 1, 2, 3, 4, 5, 0, 0, 6, 7, 8, 9, 10, 11};
            return chess_pieces_textures[piece_to_img_pos[std::to_underlying(piece)]];
        }
    };
}

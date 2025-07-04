#include <array>
#include <filesystem>
#include "imgui.h"
#include "imgui_extra.hpp"
#include "../game/board.hpp"
#include "../third/miniaudio.h"

namespace game {
    struct Game;
}

namespace renderer {
    struct VisualBoard {
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

        VisualBoard() = default;

        ~VisualBoard() { cleanup(); }

        bool load_textures(const std::filesystem::path &res_path);

        void render(game::Game *game, float width, float height);

        void cleanup();

        inline void flip_board() { flipped = !flipped; }
    };
}

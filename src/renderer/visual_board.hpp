#pragma once
#include "imgui_impl_opengl3_loader.h"
#include <array>
#include <filesystem>
#include "imgui.h"

namespace game {
    struct Game;
}

namespace renderer {
    struct VisualBoard {
        static constexpr ImVec2 PIECE_SIZE = ImVec2(128.0f, 128.0f);
        std::array<GLuint, 12> chess_pieces_textures;
        float board_size{};
        float cell_size{};
        ImVec2 board_offset{};
        bool flipped{};

        bool load_textures(const std::filesystem::path &res_path);

        void render(game::Game *game, float width, float height);
    };
}

#pragma once
#include "imgui_impl_opengl3_loader.h"
#include <array>
#include <filesystem>
#include "imgui.h"
#include "imgui_extra.hpp"
#include "../game/piece.hpp"
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
        bool load_textures(const std::filesystem::path &res_path);
        void render(game::Game *game, float width, float height);
    };
}

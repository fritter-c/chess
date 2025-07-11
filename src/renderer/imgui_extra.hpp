#pragma once
#include <filesystem>
#include "imgui_impl_opengl3_loader.h"
namespace ImGui {
constexpr GLuint INVALID_TEXTURE_ID = 0;
GLuint LoadTexture(const std::filesystem::path &filename);
void LoadFont(const std::filesystem::path &filename, float size = 16.0f);
} // namespace ImGui

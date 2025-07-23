#pragma once
#include <filesystem>
#include "imgui_impl_opengl3_loader.h"
#include "GLFW/glfw3.h"
namespace ImGui {
constexpr GLuint INVALID_TEXTURE_ID = 0;
GLuint LoadTexture(const std::filesystem::path &filename);
GLFWimage LoadImage(const std::filesystem::path &filename);
void FreeImage(GLFWimage &image);
void LoadFont(const std::filesystem::path &filename, float size = 16.0f);
} // namespace ImGui

#pragma once
#include "imgui_impl_opengl3_loader.h"
#include <filesystem>
namespace ImGui {
    constexpr GLuint INVALID_TEXTURE_ID = 0;
    GLuint LoadTexture(const std::filesystem::path& filename);
}

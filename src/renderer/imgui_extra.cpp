#include "imgui_extra.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <fstream>
#include <string>
#include <vector>
#include "imgui.h"
#include "stb_image.h"

namespace ImGui {
bool load_texture_from_memory(const uint8_t *data, size_t data_size, GLuint *out_texture) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data = stbi_load_from_memory(data, static_cast<int32_t>(data_size), &image_width, &image_height, nullptr, 4);
    if (image_data == nullptr)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;

    return true;
}

bool load_texture_from_file(const std::filesystem::path &filename, GLuint &out_texture) {
    std::ifstream file{filename, std::ios::binary | std::ios::ate};
    if (!file)
        return false;

    const auto size = file.tellg();
    if (size <= 0)
        return false;

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char *>(data.data()), size))
        return false;

    return load_texture_from_memory(data.data(), data.size(), &out_texture);
}

GLuint LoadTexture(const std::filesystem::path &filename) {
    if (GLuint texture{}; load_texture_from_file(filename, texture)) {
        return texture;
    }
    return INVALID_TEXTURE_ID;
}

GLFWimage LoadImage(const std::filesystem::path &filename){
    GLFWimage image;
    image.pixels = stbi_load(filename.string().c_str(), &image.width, &image.height, nullptr, 4);
    return image;
}

void FreeImage(GLFWimage &image){
    if (image.pixels) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
    }
    image.width = 0;
    image.height = 0;
}

void LoadFont(const std::filesystem::path &filename, float size) {
    if (auto &io = GetIO(); io.Fonts->AddFontFromFileTTF(filename.string().c_str(), size) == nullptr) {
        fprintf(stderr, "Failed to load font: %s\n", filename.string().c_str());
    } else {
        io.FontDefault = io.Fonts->Fonts.back();
    }
}
} // namespace ImGui

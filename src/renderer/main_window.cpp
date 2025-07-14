#include "main_window.hpp"
#include "imgui.h"
namespace renderer {

MainWindow::MainWindow() {
    ImGui::LoadFont(std::filesystem::current_path().parent_path().parent_path() / "res" / "open_chess_font.ttf", 18.0f); // To draw pieces
    ImGui::LoadFont(std::filesystem::current_path().parent_path().parent_path() / "res" / "liberation_mono_regular.ttf", 18.0f); // Default;
}

void MainWindow::render() {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
    board_panel.render();
    ImGui::Render();
}

} // namespace renderer
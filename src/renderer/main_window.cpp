#include "main_window.hpp"
#include "imgui.h"
namespace renderer {

    void MainWindow::render() {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
        board_panel.render();
        ImGui::Render();
    }

} // namespace renderer
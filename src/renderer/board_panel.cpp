#include "board_panel.hpp"

namespace renderer {
    BoardPanel::BoardPanel() {
        if (!chess_board.load_textures(std::filesystem::current_path().parent_path().parent_path() / "res")) {
            exit(-1);
        }
    }

    void BoardPanel::render() {
        ImGui::SetNextWindowSize(ImVec2(1080, 1080), ImGuiCond_FirstUseEver);
        ImGui::Begin("Chess Board"); {

            ImGui::BeginChild("Chess Board");
            const float width = ImGui::GetWindowWidth();
            const float height = ImGui::GetWindowHeight();
            chess_board.render(&chess_game, width, height);
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

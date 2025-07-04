#include "board_panel.hpp"
#include "../game/player.hpp"
namespace renderer
{
    BoardPanel::BoardPanel()
    {
        if (!chess_board.load_textures(std::filesystem::current_path().parent_path().parent_path() / "res"))
        {
            exit(-1);
        }

       chess_game.set_player(game::PIECE_WHITE, game::DrunkMan{});
      chess_game.set_player(game::PIECE_BLACK, game::DrunkMan{});
    }

    void BoardPanel::render()
    {
        ImGui::SetNextWindowSize(ImVec2(1080, 1080), ImGuiCond_FirstUseEver);
        ImGui::Begin("Chess Board");
        {
            ImGui::BeginChild("Chess Board");
            const float width = ImGui::GetWindowWidth();
            const float height = ImGui::GetWindowHeight();
            chess_board.render(&chess_game, width, height);
            chess_game.tick();
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::Begin("Controls");
        {
#if 1
            ImGui::BeginChild("Debug", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
            const float fps = ImGui::GetIO().Framerate;
            ImGui::Text("Board FPS %.2f", fps);
            game::Piece p = chess_game.board[game::Board::board_get_index(chess_board.row_hovered, chess_board.col_hovered)];
            ImGui::Text("Rank (%d) File (%d) Piece (%s)", chess_board.row_hovered, chess_board.col_hovered, game::piece_to_string(p));
            if (chess_board.dragging_piece_index >= 0)
            {
                game::Piece dragging = chess_game.board[chess_board.dragging_piece_index];
                ImGui::Text("Dragging %s (%d)", game::piece_to_string(dragging), chess_board.available_squares_for_dragging.move_count());
            }
            else
            {
                ImGui::Text("Dragging None");
            }

            ImGui::Text("Move count %lu", chess_game.move_count);
            ImGui::Text("Status %s (%s)", chess_game.get_status_string(), chess_game.get_winner_string());

            ImGui::EndChild();
#endif
            ImGui::BeginChild("Control Buttons", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
            if (ImGui::Button("Reset Game"))
            {
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip Board"))
            {
                chess_board.flip_board();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

}

#include "board_panel.hpp"
#include "../game/analyzer.hpp"
#include "../game/player.hpp"

namespace renderer {
// Until I have a better font I have to split the output and change fonts back and forth
static void get_move_with_icon(const game::AlgebraicMove &move, gtr::string &before, gtr::string &icon, gtr::string &after) {

    auto contains_piece = [](const char piece) {
        static constexpr char piece_map[] = {'N', 'B', 'R', 'Q', 'K'};
        for (const auto piece_char : piece_map) {
            if (piece_char == piece) {
                return true;
            }
        }
        return false;
    };
    gtr::string *write_buffer = &before;
    for (const auto piece : move) {
        if (contains_piece(piece)) {
            icon.push_back(piece);
            write_buffer = &after;
        } else {
            write_buffer->push_back(piece);
        }
    }
}

static void write_move_with_icon(const game::AlgebraicMove &move) {
    gtr::string before;
    gtr::string after;
    gtr::string icon;
    get_move_with_icon(move, before, icon, after);
    constexpr float icon_shift = 7.0f;
    if (!before.empty()) {
        ImGui::Text("%s", before.c_str());
        ImGui::SameLine();
        // Align the icon better with normal font
        if (!icon.empty()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - icon_shift);
        }
    }
    if (!icon.empty()) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("%s", icon.c_str());
        ImGui::SameLine();
        ImGui::PopFont();
        // Align the icon better with normal font
        if (!after.empty()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - icon_shift);
            ImGui::Text("%s", after.c_str());
        }
    }
}

BoardPanel::BoardPanel() {
#if 0
    chess_game.set_player(game::PIECE_WHITE, game::DrunkMan{});
    chess_game.set_player(game::PIECE_BLACK, game::DrunkMan{});
#endif
}
void BoardPanel::render() {
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

        ImGui::BeginChild("Debug", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
        const float fps = ImGui::GetIO().Framerate;
        ImGui::Text("Board FPS %.2f", fps);
        if (chess_board.row_hovered >= 0 && chess_board.col_hovered >= 0) {
            const game::Piece p = chess_game.board[game::Board::get_index(chess_board.row_hovered, chess_board.col_hovered)];
            ImGui::Text("Rank (%d) File (%d) Piece (%s)", chess_board.row_hovered, chess_board.col_hovered, game::piece_to_string(p));
        } else {
            ImGui::Text("Rank (%d) File (%d) Piece (None)", chess_board.row_hovered, chess_board.col_hovered);
        }
        if (chess_board.dragging_piece_index >= 0) {
            const game::Piece dragging = chess_game.board[chess_board.dragging_piece_index];
            ImGui::Text("Dragging %s (%d)", game::piece_to_string(dragging), chess_board.available_squares_for_dragging.move_count());
        } else {
            ImGui::Text("Dragging None");
        }

        ImGui::Text("Move count %lu", chess_game.move_count);
        ImGui::Text("Status %s (%s)", chess_game.get_status_string(), chess_game.get_winner_string());

        ImGui::EndChild();
        ImGui::BeginChild("Control Buttons", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);

        if (ImGui::Button("Flip")) {
            chess_board.flip_board();
        }
        ImGui::SameLine();
        if (ImGui::Button("<<")) {
            chess_game.return_first_move();
        }
        ImGui::SameLine();
        if (ImGui::Button("<")) {
            chess_game.undo();
        }

        ImGui::SameLine();
        if (ImGui::Button(">")) {
            if (!chess_game.redo()){
                chess_game.random_move();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(">>")) {
            chess_game.return_last_move();
        }
        ImGui::SameLine();
        if (ImGui::Button("Debug")) {
            debug_chess_board = !debug_chess_board;
        }
        if (debug_chess_board) {
            ImGui::Checkbox("View as plain string", &debug_plain_string);
            if (debug_plain_string) {
                ImGui::TextUnformatted(chess_game.board.board_to_string().c_str());
            } else {
                constexpr float piece_font_size = 30.0f;
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0], piece_font_size);
                ImGui::TextUnformatted(chess_game.board.board_to_string().c_str());
                ImGui::PopFont();
            }
        }
        ImGui::EndChild();

        ImGui::BeginChild("Move List", ImVec2(0, 0));
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                                       ImGuiTableFlags_SizingFixedFit;
        constexpr int32_t column_count = 3;
        if (ImGui::BeginTable("Moves", column_count, flags)) {
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHeaderLabel);
            ImGui::TableSetupColumn("White", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Black", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (auto i = 1; i <= chess_game.move_list.read_index; i += 2) {

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d.", (i / 2) + 1);
                ImGui::TableNextColumn();
                write_move_with_icon(chess_game.move_list[i]);
                ImGui::TableNextColumn();
                if (i + 1 <= chess_game.move_list.read_index) {
                    write_move_with_icon(chess_game.move_list[i + 1]);
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace renderer

#include "visual_board.hpp"
#include "imgui_extra.hpp"
#include "../game/game.hpp"
#include "primitives.hpp"
#include "imgui_internal.h"
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

namespace renderer {
    bool
    VisualBoard::load_textures(const std::filesystem::path &res_path) {
        static constexpr std::array<const char *, 12> piece_filenames = {
            "white-pawn.png", "white-knight.png", "white-bishop.png",
            "white-rook.png", "white-queen.png", "white-king.png",
            "black-pawn.png", "black-knight.png", "black-bishop.png",
            "black-rook.png", "black-queen.png", "black-king.png"
        };

        for (int32_t i = 0; i < 12; ++i) {
            auto &tex = chess_pieces_textures[i];
            tex = ImGui::LoadTexture(res_path / piece_filenames[i]);
            if (tex == ImGui::INVALID_TEXTURE_ID) {
                return false;
            }
        }
        return true;
    }

    static ImVec2
    apply_window_offset(const ImVec2 vec) {
        return vec + ImGui::GetWindowPos();
    }

    static GLuint
    render_get_piece_texture(const VisualBoard *board, const game::Piece piece) {
        static constexpr std::array piece_to_img_pos{0, 0, 1, 2, 3, 4, 5, 0, 0, 6, 7, 8, 9, 10, 11};
        return board->chess_pieces_textures[piece_to_img_pos[std::to_underlying(piece)]];
    }

    static void
    visual_board_get_rect_for_cell(const VisualBoard *board, const int32_t row, const int32_t col, Rectangle &rect) {
        if (board->flipped) {
            rect.topLeft.x = board->board_offset.x + (7 - static_cast<float>(col)) * board->cell_size;
            rect.topLeft.y = board->board_offset.y + static_cast<float>(row) * board->cell_size;
        } else {
            rect.topLeft.x = board->board_offset.x + static_cast<float>(col) * board->cell_size;
            rect.topLeft.y = board->board_offset.y + static_cast<float>(7 - row) * board->cell_size;
        }
        rect.topLeft = apply_window_offset(rect.topLeft);
        rect.Size({board->cell_size, board->cell_size});
    }

    static void
    visual_board_get_rect_for_mouse_pos(const VisualBoard *board, const ImVec2 mouse_pos, Rectangle &rect) {
        rect.topLeft = mouse_pos - ImVec2(board->cell_size / 2.0f, board->cell_size / 2.0f);
        rect.Size({board->cell_size, board->cell_size});
    }

    static void
    render_chess_pieces(const game::Game *game, const VisualBoard *board) {
        for (int32_t row = 7; row >= 0; --row) {
            for (int32_t col = 7; col >= 0; --col) {
                const int32_t index = game::Board::board_get_index(row, col);
                const game::Piece piece = game->board.pieces[index];
                if (piece != game::PIECE_NONE) {
                    Rectangle piece_rect{};
                    visual_board_get_rect_for_cell(board, row, col, piece_rect);
                    const GLuint texture_id = render_get_piece_texture(board, piece);
                    ImGui::GetWindowDrawList()->AddImage(texture_id, piece_rect.topLeft, piece_rect.bottomRight);
                    if (index == board->dragging_piece_index) {
                        // Render a highlight around the piece being dragged
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            piece_rect.topLeft,
                            piece_rect.bottomRight,
                            IM_COL32(255, 255, 0, 128), // semi-transparent yellow
                            0.0f, ImDrawFlags_RoundCornersAll
                        );
                    }
                }
            }
        }
    }

    static void
    resize(VisualBoard *board, const float width, const float height) {
        if (width != board->board_size || height != board->board_size) {
            const float board_size = MIN(width, height);
            board->board_size = board_size;
            board->cell_size = board_size / 8.0f;
            board->board_offset = ImVec2((width - board_size) / 2.0f, (height - board_size) / 2.0f);
        }
    }

    static void
    render_chess_board(game::Game *game, VisualBoard *board) {
        const ImVec2 global_offset = apply_window_offset(board->board_offset);
        ImGui::SetCursorPos(ImVec2(board->board_offset.x, board->board_offset.y));
        // Grab the whole region so the window itself won't drag
        ImGui::InvisibleButton("##ChessBoard", ImVec2(board->board_size, board->board_size));

        constexpr ImU32 light_wood = IM_COL32(222, 184, 135, 255); // burlywood/tan
        constexpr ImU32 dark_wood = IM_COL32(160, 82, 45, 255); // sienna/brown

        // rank = 7 (top) down to 0 (bottom)
        for (int32_t rank = 7; rank >= 0; --rank) {
            const int32_t display_row = 7 - rank;
            for (int32_t file = 0; file < 8; ++file) {
                // compute cell position
                ImVec2 cell_pos(
                    global_offset.x + static_cast<float>(file) * board->cell_size,
                    global_offset.y + static_cast<float>(display_row) * board->cell_size
                );

                // standard chess parity: a1 (file=0, display_row=7) is dark
                const bool is_light_square = ((display_row + file) % 2 == 0);
                const ImU32 color = is_light_square ? light_wood : dark_wood;

                const auto draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(
                    cell_pos,
                    ImVec2(cell_pos.x + board->cell_size,
                           cell_pos.y + board->cell_size),
                    color
                );

                // hit-testing / clicks
                ImGui::PushID(game::Board::board_get_index(rank, file));
                ImRect cell_rect(
                    cell_pos,
                    ImVec2(cell_pos.x + board->cell_size,
                           cell_pos.y + board->cell_size)
                );
                ImGui::ItemAdd(cell_rect, ImGui::GetActiveID());
                if (ImGui::IsItemHovered()) {
                    draw_list->AddRect(
                        cell_rect.Min, cell_rect.Max,
                        IM_COL32(0, 255, 0, 255), 0.0f, ImDrawFlags_RoundCornersAll, 2.0f
                    );
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && board->dragging_piece_index == -1) {
                    board->dragging_piece_index = game::Board::board_get_index(rank, file);
                } else if (board->dragging_piece_index != -1 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    board->dragging_piece_index = -1;

                }
                ImGui::PopID();
            }
        }
    }

    static void
    render_dragging_piece( const game::Game *game, const VisualBoard *board) {
        if (board->dragging_piece_index < 0) {
            return;
        }
        Rectangle piece_rect{};
        visual_board_get_rect_for_mouse_pos(board, ImGui::GetMousePos(), piece_rect);
        const game::Piece piece = game->board.pieces[board->dragging_piece_index];
        const GLuint texture_id = render_get_piece_texture(board, piece);
        ImGui::GetWindowDrawList()->AddImage(texture_id, piece_rect.topLeft, piece_rect.bottomRight);
    }
    void
    VisualBoard::render(game::Game *game, const float width, const float height) {
        resize(this, width, height);
        render_chess_board(game, this);
        render_chess_pieces(game, this);
        render_dragging_piece(game, this);
    }
}

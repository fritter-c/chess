#include "visual_board.hpp"
#include "imgui_extra.hpp"
#include "../game/game.hpp"
#include "primitives.hpp"
#include "../game/analyzer.hpp"
#include "imgui_internal.h"
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

namespace renderer
{
    bool
    VisualBoard::load_textures(const std::filesystem::path &res_path)
    {
        if (ma_engine_init(nullptr, &sound_engine) != MA_SUCCESS)
        {
            return false;
        }

        const auto move = res_path / "move.mp3";
        // Need to convert first to string because windows uses wide char
        if (ma_sound_init_from_file(&sound_engine, move.string().c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                    &move_sound) != MA_SUCCESS)
        {
            return false;
        }
        const auto check = res_path / "check.mp3";
        if (ma_sound_init_from_file(&sound_engine, check.string().c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                    &check_sound) != MA_SUCCESS)
        {
            return false;
        }

        static constexpr std::array<const char *, 12> piece_filenames = {
            "white-pawn.png", "white-knight.png", "white-bishop.png",
            "white-rook.png", "white-queen.png", "white-king.png",
            "black-pawn.png", "black-knight.png", "black-bishop.png",
            "black-rook.png", "black-queen.png", "black-king.png"};

        for (int32_t i = 0; i < 12; ++i)
        {
            auto &tex = chess_pieces_textures[i];
            tex = ImGui::LoadTexture(res_path / piece_filenames[i]);
            if (tex == ImGui::INVALID_TEXTURE_ID)
            {
                return false;
            }
        }
        return true;
    }

    static void
    play_move_sound(VisualBoard *board)
    {
        ma_sound_seek_to_pcm_frame(&board->move_sound, 0);
        ma_sound_start(&board->move_sound);
    }

    static void
    play_check_sound(VisualBoard *board)
    {
        ma_sound_seek_to_pcm_frame(&board->check_sound, 0);
        ma_sound_start(&board->check_sound);
    }

    void
    VisualBoard::cleanup()
    {
        ma_sound_uninit(&move_sound);
        ma_sound_uninit(&check_sound);
        ma_engine_uninit(&sound_engine);
    }

    static ImVec2
    apply_window_offset(const ImVec2 vec)
    {
        return vec + ImGui::GetWindowPos();
    }

    static GLuint
    render_get_piece_texture(const VisualBoard *board, const game::Piece piece)
    {
        static constexpr std::array piece_to_img_pos{0, 0, 1, 2, 3, 4, 5, 0, 0, 6, 7, 8, 9, 10, 11};
        return board->chess_pieces_textures[piece_to_img_pos[std::to_underlying(piece)]];
    }

    static Rectangle
    visual_board_get_rect_for_cell(const VisualBoard *board, const int32_t row, const int32_t col)
    {
        Rectangle rect{};
        if (board->flipped)
        {
            rect.topLeft.x = board->board_offset.x + (7 - static_cast<float>(col)) * board->cell_size;
            rect.topLeft.y = board->board_offset.y + static_cast<float>(row) * board->cell_size;
        }
        else
        {
            rect.topLeft.x = board->board_offset.x + static_cast<float>(col) * board->cell_size;
            rect.topLeft.y = board->board_offset.y + static_cast<float>(7 - row) * board->cell_size;
        }
        rect.topLeft = apply_window_offset(rect.topLeft);
        rect.Size({board->cell_size, board->cell_size});
        return rect;
    }

    static Rectangle
    visual_board_get_rect_for_mouse_pos(const VisualBoard *board, const ImVec2 mouse_pos)
    {
        Rectangle rect{};
        rect.topLeft = mouse_pos - ImVec2(board->cell_size / 2.0f, board->cell_size / 2.0f);
        rect.Size({board->cell_size, board->cell_size});
        return rect;
    }

    static void
    render_available_squares(const VisualBoard *board)
    {
        for (int8_t i = 0; i < 64; ++i)
        {
            if (board->available_squares_for_dragging.get(i))
            {
                const int32_t row = game::Board::board_get_row(i);
                const int32_t col = game::Board::board_get_col(i);
                Rectangle cell_rect = visual_board_get_rect_for_cell(board, row, col);
                ImGui::GetWindowDrawList()->AddCircleFilled(cell_rect.Center(), cell_rect.Size().x / 5.0f,
                                                            IM_COL32(0, 255, 0, 128), 128);
            }
        }
    }

    static void
    render_chess_pieces(const game::Game *game, const VisualBoard *board)
    {
        // Draw the available square behind
        if (board->dragging_piece_index >= 0)
        {
            render_available_squares(board);
        }
        for (int32_t index = 63; index >= 0; --index)
        {
            const int32_t row = game::Board::board_get_row(index);
            const int32_t col = game::Board::board_get_col(index);
            if (const game::Piece piece = game->board.pieces[index]; piece != game::PIECE_NONE)
            {
                const Rectangle piece_rect = visual_board_get_rect_for_cell(board, row, col);

                const GLuint texture_id = render_get_piece_texture(board, piece);
                ImGui::GetWindowDrawList()->AddImage(texture_id, piece_rect.topLeft, piece_rect.bottomRight);
                if (index == board->dragging_piece_index)
                {
                    // Render a highlight around the piece being dragged
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        piece_rect.topLeft,
                        piece_rect.bottomRight,
                        IM_COL32(255, 255, 0, 128), // semi-transparent yellow
                        0.0f, ImDrawFlags_RoundCornersAll);
                }
            }
        }
    }

    static void
    resize(VisualBoard *board, const float width, const float height)
    {
        if (width != board->board_size || height != board->board_size)
        {
            const float board_size = MIN(width, height);
            board->board_size = board_size;
            board->cell_size = board_size / 8.0f;
            board->board_offset = ImVec2((width - board_size) / 2.0f, (height - board_size) / 2.0f);
        }
    }

    static void
    visual_board_piece_move(VisualBoard *board, game::Game *game, const int32_t from_row, const int32_t from_col,
                            const int32_t to_row, const int32_t to_col, game::PromotionPieceType promotion_type = game::PROMOTION_QUEEN)
    {
        const game::Color piece_color = game->board.get_color(from_row, from_col);
        if (game->turn != piece_color)
        {
            return;
        }

        if (game->board.board_pawn_is_being_promoted({static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col),
                                                      static_cast<uint8_t>(to_row), static_cast<uint8_t>(to_col)}))
        {
            board->promotion_move = {static_cast<uint8_t>(from_row), static_cast<uint8_t>(from_col), static_cast<uint8_t>(to_row),
                                     static_cast<uint8_t>(to_col)};
            board->waiting_promotion = true;
        }
        else
        {

            if (game->move(from_row, from_col, to_row, to_col, promotion_type))
            {
                if (game->board_in_check())
                {
                    play_check_sound(board);
                }
                else
                {
                    play_move_sound(board);
                }
            }
        }
    }

    static void
    render_promotion_modal(game::Game *game, VisualBoard *board)
    {
        auto apply_move = [&board, &game](game::PromotionPieceType promotion_type = game::PROMOTION_QUEEN)
        {
            board->waiting_promotion = false;
            if (game->move(board->promotion_move.from_row, board->promotion_move.from_col, board->promotion_move.to_row,
                           board->promotion_move.to_col, promotion_type))
            {
                if (game->board_in_check())
                {
                    play_check_sound(board);
                }
                else
                {
                    play_move_sound(board);
                }
            }
        };

        if (board->waiting_promotion)
        {
            ImGui::OpenPopup("Promotion");
            if (ImGui::BeginPopupModal("Promotion", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
            {
                const game::Color c = game->board.get_color(board->promotion_move.from_row, board->promotion_move.from_col);

                if (ImGui::ImageButton("##PromotionQueen", board->get_piece_texture(c ? game::BLACK_QUEEN : game::WHITE_QUEEN), ImVec2(128.0f, 128.0f)))
                {
                    apply_move(game::PROMOTION_QUEEN);
                }
                if (ImGui::ImageButton("##PromotionRook", board->get_piece_texture(c ? game::BLACK_ROOK : game::WHITE_ROOK), ImVec2(128.0f, 128.0f)))
                {
                    apply_move(game::PROMOTION_ROOK);
                }
                if (ImGui::ImageButton("##PromotionBishop", board->get_piece_texture(c ? game::BLACK_BISHOP : game::WHITE_BISHOP), ImVec2(128.0f, 128.0f)))
                {
                    apply_move(game::PROMOTION_BISHOP);
                }
                if (ImGui::ImageButton("##PromotionKnight", board->get_piece_texture(c ? game::BLACK_KNIGHT : game::WHITE_KNIGHT), ImVec2(128.0f, 128.0f)))
                {
                    apply_move(game::PROMOTION_KNIGHT);
                }

                ImGui::EndPopup();
            }
        }
    }

    static void
    render_chess_board(game::Game *game, VisualBoard *board)
    {
        const ImVec2 global_offset = apply_window_offset(board->board_offset);
        ImGui::SetCursorPos(ImVec2(board->board_offset.x, board->board_offset.y));
        // Grab the whole region so the window itself won't drag
        ImGui::InvisibleButton("##ChessBoard", ImVec2(board->board_size, board->board_size));
        constexpr ImU32 light_wood = IM_COL32(222, 184, 135, 255);
        constexpr ImU32 dark_wood = IM_COL32(160, 82, 45, 255);

        // rank = 7 (top) down to 0 (bottom)
        for (int32_t rank = 7; rank >= 0; --rank)
        {
            const int32_t display_row = 7 - rank;
            const int32_t flipped_rank = board->flipped ? display_row : rank;
            for (int32_t file = 0; file < 8; ++file)
            {
                const int32_t flipped_file = board->flipped ? 7 - file : file;
                // compute cell position
                ImVec2 cell_pos(
                    global_offset.x + static_cast<float>(file) * board->cell_size,
                    global_offset.y + static_cast<float>(display_row) * board->cell_size);

                // standard chess parity: a1 (file=0, display_row=7) is dark
                const bool is_light_square = ((display_row + file) % 2 == 0);
                const ImU32 color = is_light_square ? light_wood : dark_wood;

                const auto draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(
                    cell_pos,
                    ImVec2(cell_pos.x + board->cell_size,
                           cell_pos.y + board->cell_size),
                    color);

                // draw cell name at top-left corner of the cell
                const char *name = game::CellNames[flipped_rank][flipped_file];
                const float font_size = board->cell_size > 60.0f ? 20.0f : board->cell_size / 3.0f;
                draw_list->AddText(ImGui::GetFont(), font_size, cell_pos, IM_COL32(255, 255, 255, 255), name);

                // hit-testing / clicks
                const int32_t index = game::Board::board_get_index(flipped_rank, flipped_file);

                ImGui::PushID(index);
                ImRect cell_rect(
                    cell_pos,
                    ImVec2(cell_pos.x + board->cell_size,
                           cell_pos.y + board->cell_size));
                ImGui::ItemAdd(cell_rect, ImGui::GetActiveID());

                if (ImGui::IsItemHovered())
                {
                    draw_list->AddRect(
                        cell_rect.Min, cell_rect.Max,
                        IM_COL32(0, 255, 0, 255), 0.0f, ImDrawFlags_RoundCornersAll, 2.0f);
                    board->row_hovered = flipped_rank;
                    board->col_hovered = flipped_file;
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
                    board->dragging_piece_index == -1 &&
                    game->board.pieces[index] != game::PIECE_NONE &&
                    game->board.get_color(index) == game->turn)
                {
                    board->dragging_piece_index = index;
                    board->available_squares_for_dragging = game::analyzer_get_available_moves_for_piece(
                        &game->board, index);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
                else if (board->dragging_piece_index != -1 &&
                         ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                         ImGui::IsItemHovered())
                {

                    visual_board_piece_move(board, game,
                                            game::Board::board_get_row(board->dragging_piece_index),
                                            game::Board::board_get_col(board->dragging_piece_index),
                                            flipped_rank, flipped_file);

                    board->dragging_piece_index = -1;
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
                }

                ImGui::PopID();
            }
        }
        render_promotion_modal(game, board);
    }

    static void
    render_dragging_piece(const game::Game *game, const VisualBoard *board)
    {
        if (board->dragging_piece_index < 0)
        {
            return;
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        const Rectangle piece_rect = visual_board_get_rect_for_mouse_pos(board, ImGui::GetMousePos());
        const game::Piece piece = game->board.pieces[board->dragging_piece_index];
        const GLuint texture_id = render_get_piece_texture(board, piece);
        ImGui::GetWindowDrawList()->AddImage(texture_id, piece_rect.topLeft, piece_rect.bottomRight);
    }

    void VisualBoard::render(game::Game *game, const float width, const float height)
    {
        resize(this, width, height);
        render_chess_board(game, this);
        render_chess_pieces(game, this);
        render_dragging_piece(game, this);
    }
}

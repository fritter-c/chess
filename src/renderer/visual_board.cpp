#include "visual_board.hpp"
#include "raylib.h"
#include "../game/move.hpp"
#include <cstring>
#include "main_panel.hpp"
#include "../game/analyzer.hpp"
#include "../game/types.hpp"
#include <array>

namespace renderer
{
    static void
    visual_board_get_rect_for_cell(const VisualBoard *board, const int32_t row, const int32_t col, Rectangle *rect)
    {
        if (board->flipped)
        {
            rect->x = static_cast<float>(board->offset_x + (7 - col) * board->cell_size);
            rect->y = static_cast<float>(board->offset_y + row * board->cell_size);
        }
        else
        {
            rect->x = static_cast<float>(board->offset_x + col * board->cell_size);
            rect->y = static_cast<float>(board->offset_y + (7 - row) * board->cell_size);
        }
        rect->width = static_cast<float>(board->cell_size);
        rect->height = static_cast<float>(board->cell_size);
    }

    static void
    visual_board_get_rect_for_mouse_pos(const VisualBoard *board, const Vector2 mouse_pos, Rectangle *rect)
    {
        rect->x = mouse_pos.x - static_cast<float>(board->cell_size) / 2.0f;
        rect->y = mouse_pos.y - static_cast<float>(board->cell_size) / 2.0f;
        rect->width = static_cast<float>(board->cell_size);
        rect->height = static_cast<float>(board->cell_size);
    }

    static int32_t
    visual_board_get_board_index_for_mouse_pos(const VisualBoard *board, const Vector2 mouse_pos)
    {
        if (board->offset_y > static_cast<int32_t>(mouse_pos.y) || board->offset_x > static_cast<int32_t>(mouse_pos.x))
        {
            return -1; // Out of bounds
        }
        int32_t row;
        int32_t col;
        if (board->flipped)
        {
            row = (static_cast<int32_t>(mouse_pos.y) - board->offset_y) / board->cell_size;
            col = 7 - ((static_cast<int32_t>(mouse_pos.x) - board->offset_x) / board->cell_size);
        }
        else
        {
            col = (static_cast<int32_t>(mouse_pos.x) - board->offset_x) / board->cell_size;
            row = 7 - ((static_cast<int32_t>(mouse_pos.y) - board->offset_y) / board->cell_size);
        }

        if (row < 0 || row >= 8 || col < 0 || col >= 8)
        {
            return -1; // Out of bounds
        }
        return game::Board::board_get_index(row, col);
    }

    void
    visual_board_initialize(VisualBoard *board, MainPanel *panel)
    {
        std::memset(board, 0, sizeof(VisualBoard));
        board->board.init();
        board->board.board_populate();
        board->panel = panel;
    }

    void
    visual_board_resize(VisualBoard *board, const int32_t w, const int32_t h)
    {
        board->size = w < h ? w : h;
        board->cell_size = board->size / 8;
        board->offset_x = (w - board->size) / 2;
        board->offset_y = (h - board->size) / 2;
    }

    static const Rectangle &
    visual_board_get_piece_original_rect(const VisualBoard *board, const game::Piece piece)
    {
        // King, Queen, Bishop, Knight, Rook, Pawn on the PNG
        static constexpr std::array piece_to_img_pos{0, 5, 3, 2, 4, 1, 0, 0, 0, 11, 9, 8, 10, 7, 6};
        return board->piece_original_rects[piece_to_img_pos[piece]];
    }

    static void
    visual_board_draw_available_squares(const VisualBoard *board)
    {
        if (PIECE_TYPE(board->dragging_piece) == game::PieceType::NONE)
        {
            return;
        }

        const game::AvailableSquares available_moves = board->dragging_piece_available_moves;

        for (int32_t r = 0; r < 8; r++)
        {
            for (int32_t c = 0; c < 8; c++)
            {
                if (available_moves.get(r, c))
                {
                    Rectangle highlight_rect{};
                    visual_board_get_rect_for_cell(board, r, c, &highlight_rect);
                    DrawRectangleLinesEx(highlight_rect, 2.0f, YELLOW);
                }
            }
        }
    }

    static void
    visual_board_draw_pieces(const VisualBoard *board)
    {
        for (int32_t row = 7; row >= 0; --row)
        {
            for (int32_t col = 7; col >= 0; --col)
            {
                const int32_t index = game::Board::board_get_index(row, col);
                const game::Piece piece = board->board.pieces[index];
                Rectangle piece_rect{};
                visual_board_get_rect_for_cell(board, row, col, &piece_rect);
                if (piece_rect.width < 100.0f)
                {
                    DrawTextEx(board->font_small, game::CellNames[row][col], {piece_rect.x + 5.0f, piece_rect.y + 5.0f},
                               12, 0.0f,
                               WHITE);
                }
                else
                {
                    DrawTextEx(board->font_big, game::CellNames[row][col], {piece_rect.x + 5.0f, piece_rect.y + 5.0f},
                               20, 0.0f,
                               WHITE);
                }
                if (PIECE_TYPE(piece) == game::PieceType::NONE)
                {
                    continue;
                }

                const Rectangle &original_rect = visual_board_get_piece_original_rect(board, piece);
                constexpr Vector2 origin{0, 0};

                DrawTexturePro(board->piece_textures, original_rect, piece_rect, origin, 0.0f, WHITE);
            }
        }
    }

    static void
    visual_board_process_inputs(VisualBoard *board)
    {
        using enum game::PieceType;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PIECE_TYPE(board->dragging_piece) == NONE)
        {
            if (const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, GetMousePosition());
                board_index != -1)
            {
                if (const game::Piece piece = board->board.pieces[board_index];
                    PIECE_TYPE(piece) != NONE)
                {
                    board->dragging_piece = piece;
                    board->dragging_piece_row = static_cast<uint8_t>(game::Board::board_get_row(board_index));
                    board->dragging_piece_col = static_cast<uint8_t>(game::Board::board_get_col(board_index));
                    board->dragging_piece_available_moves = game::analyzer_get_available_moves_for_piece(
                        &board->board, board->dragging_piece_row, board->dragging_piece_col);
                }
            }
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && PIECE_TYPE(board->dragging_piece) !=
                                                                 NONE)
        {
            const Vector2 mouse_pos = GetMousePosition();
            game::AlgebraicMove move{};
            if (const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, mouse_pos);
                board_index != -1 && board->board.board_move(board->dragging_piece_row,
                                                             board->dragging_piece_col, game::Board::board_get_row(board_index),
                                                             game::Board::board_get_col(board_index), move))
            {
                board->has_moves = true;
                board->last_moved_piece_dest_row = game::Board::board_get_row(board_index);
                board->last_moved_piece_dest_col = game::Board::board_get_col(board_index);
                board->last_moved_piece_org_row = board->dragging_piece_row;
                board->last_moved_piece_org_col = board->dragging_piece_col;
                main_panel_push_move(board->panel, move);
                if (move.is_check)
                    PlaySound(board->check_sound);
                else
                {
                    PlaySound(board->move_sound);
                }
            }
            board->dragging_piece = game::chess_piece_make(NONE, game::Color::PIECE_WHITE);
        }
    }

    static void
    visual_board_draw_dragging_piece(const VisualBoard *board)
    {
        if (PIECE_TYPE(board->dragging_piece) == game::PieceType::NONE)
        {
            return;
        }

        const Vector2 mouse_pos = GetMousePosition();
        Rectangle piece_rect{};
        visual_board_get_rect_for_mouse_pos(board, mouse_pos, &piece_rect);
        const Rectangle &original_rect = visual_board_get_piece_original_rect(board, board->dragging_piece);
        constexpr Vector2 origin{0, 0};

        DrawTexturePro(board->piece_textures, original_rect, piece_rect, origin, 0.0f, WHITE);
    }

    void
    visual_board_draw(VisualBoard *board)
    {
        visual_board_process_inputs(board);
        for (int32_t row = 0; row < 8; row++)
        {
            for (int32_t col = 0; col < 8; col++)
            {
                const Color squareColor = (row + col) % 2 == 0 ? LIGHTGRAY : DARKGRAY;
                DrawRectangle(board->offset_x + col * board->cell_size, board->offset_y + row * board->cell_size,
                              board->cell_size, board->cell_size, squareColor);
            }
        }
        if (const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, GetMousePosition());
            board_index != -1)
        {
            const int32_t row = game::Board::board_get_row(board_index);
            const int32_t col = game::Board::board_get_col(board_index);
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, row, col, &highlight_rect);
        }

        if (PIECE_TYPE(board->dragging_piece) != game::PieceType::NONE)
        {
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, board->dragging_piece_row, board->dragging_piece_col,
                                           &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
        }

        if (board->has_moves && PIECE_TYPE(board->dragging_piece) == game::PieceType::NONE)
        {
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, board->last_moved_piece_dest_row, board->last_moved_piece_dest_col,
                                           &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
            visual_board_get_rect_for_cell(board, board->last_moved_piece_org_row,
                                           board->last_moved_piece_org_col, &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
        }
        visual_board_draw_available_squares(board);
        visual_board_draw_pieces(board);
        visual_board_draw_dragging_piece(board);
    }

    void
    visual_board_load_resources(VisualBoard *board, const std::filesystem::path &path)
    {
        const auto texture_path = path / "chess_pieces.png";
        board->piece_textures = LoadTexture(texture_path.string().c_str());
        if (board->piece_textures.id == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load piece textures from image");
            return;
        }

        const float piece_size = static_cast<float>(board->piece_textures.width) / 6.0f;
        const float piece_height = static_cast<float>(board->piece_textures.height) / 2.0f;

        for (int32_t i = 0; i < 12; ++i)
        {
            board->piece_original_rects[i] = Rectangle{
                static_cast<float>(i % 6) * piece_size, static_cast<float>(i / 6) * piece_height, piece_size,
                piece_height};
        }

        const auto font_path = path / "liberation_mono_regular.ttf";
        board->font_big = LoadFontEx(font_path.string().c_str(), 20, nullptr, 1024);
        if (board->font_big.baseSize == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load big font from memory");
        }

        board->font_small = LoadFontEx(font_path.string().c_str(), 12, nullptr, 1024);
        if (board->font_small.baseSize == 0)
        {
            TraceLog(LOG_ERROR, "Failed to load small font from memory");
        }

        board->move_sound = LoadSound((path / "move.mp3").string().c_str());
        board->check_sound = LoadSound((path / "check.mp3").string().c_str());
    }

    void
    visual_board_reset_pieces(VisualBoard *board)
    {
        board->board.board_populate();
        board->dragging_piece = {};
        board->dragging_piece_row = 0;
        board->dragging_piece_col = 0;
        board->last_moved_piece_dest_row = 0;
        board->last_moved_piece_dest_col = 0;
        board->last_moved_piece_org_row = 0;
        board->last_moved_piece_org_col = 0;
        board->has_moves = false;
    }
}

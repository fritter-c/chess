#include "visual_board.hpp"
#include "raylib.h"
#include <cstring>

namespace renderer {
    static void
    visual_board_get_rect_for_cell(const VisualBoard *board, const int32_t row, const int32_t col, Rectangle *rect) {
        rect->x = static_cast<float>(board->offset_x + col * board->cell_size);
        rect->y = static_cast<float>(board->offset_y + row * board->cell_size);
        rect->width = static_cast<float>(board->cell_size);
        rect->height = static_cast<float>(board->cell_size);
    }

    static void
    visual_board_get_rect_for_mouse_pos(const VisualBoard *board, const Vector2 mouse_pos, Rectangle *rect) {
        rect->x = mouse_pos.x - static_cast<float>(board->cell_size) / 2.0f;
        rect->y = mouse_pos.y - static_cast<float>(board->cell_size) / 2.0f;
        rect->width = static_cast<float>(board->cell_size);
        rect->height = static_cast<float>(board->cell_size);
    }

    static int32_t
    visual_board_get_board_index_for_mouse_pos(const VisualBoard *board, const Vector2 mouse_pos) {
        if (board->offset_y > static_cast<int32_t>(mouse_pos.y) || board->offset_x > static_cast<int32_t>(mouse_pos.
                x)) {
            return -1; // Out of bounds
        }

        const int32_t row = (static_cast<int32_t>(mouse_pos.y) - board->offset_y) / board->cell_size;
        const int32_t col = (static_cast<int32_t>(mouse_pos.x) - board->offset_x) / board->cell_size;

        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            return -1; // Out of bounds
        }
        return game::board_get_index(row, col);
    }

    void
    visual_board_initialize(VisualBoard *board) {
        std::memset(board, 0, sizeof(VisualBoard));
        game::board_populate(&board->board);
    }

    void
    visual_board_resize(VisualBoard *board, const int32_t w, const int32_t h) {
        board->size = w < h ? w : h;
        board->cell_size = board->size / 8;
        board->offset_x = (w - board->size) / 2;
        board->offset_y = (h - board->size) / 2;
    }

    static const Rectangle
    *visual_board_get_piece_original_rect(const VisualBoard *board, const game::ChessPiece piece) {
        const int32_t index = static_cast<int32_t>(piece.type) - 1 + (piece.color == game::ChessPieceColor::PIECE_BLACK
                                                                          ? 6
                                                                          : 0);
        return &board->piece_original_rects[index];
    }

    static void
    visual_board_draw_pieces(const VisualBoard *board) {
        for (int32_t row = 0; row < 8; row++) {
            for (int32_t col = 0; col < 8; col++) {
                const int32_t index = game::board_get_index(row, col);
                const game::ChessPiece piece = board->board.pieces[index];
                Rectangle piece_rect{};
                visual_board_get_rect_for_cell(board, row, col, &piece_rect);
                if (piece_rect.width < 100.0f) {
                    DrawTextEx(board->font_small, game::CellNames[row][col], {piece_rect.x + 5.0f, piece_rect.y + 5.0f},
                               12, 0.0f,
                               WHITE);
                } else {
                    DrawTextEx(board->font_big, game::CellNames[row][col], {piece_rect.x + 5.0f, piece_rect.y + 5.0f},
                               20, 0.0f,
                               WHITE);
                }
                if (piece.type == game::ChessPieceType::NONE) {
                    continue;
                }

                const Rectangle *original_rect = visual_board_get_piece_original_rect(board, piece);
                constexpr Vector2 origin{0, 0};

                DrawTexturePro(board->piece_textures, *original_rect, piece_rect, origin, 0.0f, WHITE);
            }
        }
    }

    static void
    visual_board_process_inputs(VisualBoard *board) {
        using enum game::ChessPieceType;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && board->dragging_piece.type == NONE) {
            const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, GetMousePosition());
            if (board_index != -1) {
                if (const game::ChessPiece piece = board->board.pieces[board_index];
                    piece.type != NONE) {
                    board->dragging_piece = piece;
                    board->dragging_piece_row = static_cast<uint8_t>(game::board_get_row(board_index));
                    board->dragging_piece_col = static_cast<uint8_t>(game::board_get_col(board_index));
                }
            }
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && board->dragging_piece.type !=
                   NONE) {
            const Vector2 mouse_pos = GetMousePosition();
            if (const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, mouse_pos);
                board_index != -1 && game::board_move(&board->board, board->dragging_piece_row,
                                                      board->dragging_piece_col, game::board_get_row(board_index),
                                                      game::board_get_col(board_index))) {
                board->has_moves = true;
                board->last_moved_piece_dest_row = game::board_get_row(board_index);
                board->last_moved_piece_dest_col = game::board_get_col(board_index);
                board->last_moved_piece_org_row = board->dragging_piece_row;
                board->last_moved_piece_org_col = board->dragging_piece_col;
                PlaySound(board->capture_sound);
            }
            board->dragging_piece = game::ChessPiece{NONE, game::ChessPieceColor::PIECE_WHITE};
        }
    }

    static void
    visual_board_draw_dragging_piece(const VisualBoard *board) {
        if (board->dragging_piece.type == game::ChessPieceType::NONE) {
            return;
        }

        const Vector2 mouse_pos = GetMousePosition();
        Rectangle piece_rect{};
        visual_board_get_rect_for_mouse_pos(board, mouse_pos, &piece_rect);
        const Rectangle *original_rect = visual_board_get_piece_original_rect(board, board->dragging_piece);
        constexpr Vector2 origin{0, 0};

        DrawTexturePro(board->piece_textures, *original_rect, piece_rect, origin, 0.0f, WHITE);
    }

    void
    visual_board_draw(VisualBoard *board) {
        visual_board_process_inputs(board);
        for (int32_t row = 0; row < 8; row++) {
            for (int32_t col = 0; col < 8; col++) {
                const Color squareColor = (row + col) % 2 == 0 ? LIGHTGRAY : DARKGRAY;
                DrawRectangle(board->offset_x + col * board->cell_size, board->offset_y + row * board->cell_size,
                              board->cell_size, board->cell_size, squareColor);
            }
        }
        if (const int32_t board_index = visual_board_get_board_index_for_mouse_pos(board, GetMousePosition());
            board_index != -1) {
            const int32_t row = game::board_get_row(board_index);
            const int32_t col = game::board_get_col(board_index);
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, row, col, &highlight_rect);
        }

        if (board->dragging_piece.type != game::ChessPieceType::NONE) {
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, board->dragging_piece_row, board->dragging_piece_col,
                                           &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
        }

        if (board->has_moves && board->dragging_piece.type == game::ChessPieceType::NONE) {
            Rectangle highlight_rect{};
            visual_board_get_rect_for_cell(board, board->last_moved_piece_dest_row, board->last_moved_piece_dest_col,
                                           &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
            visual_board_get_rect_for_cell(board, board->last_moved_piece_org_row,
                                           board->last_moved_piece_org_col, &highlight_rect);
            DrawRectangleRec(highlight_rect, ColorAlpha(MAGENTA, .5f));
        }
        visual_board_draw_pieces(board);
        visual_board_draw_dragging_piece(board);
    }

    void
    visual_board_load_resources(VisualBoard *board, const std::filesystem::path &path) {
        const auto texture_path = path / "chess_pieces.png";
        board->piece_textures = LoadTexture(texture_path.string().c_str());
        if (board->piece_textures.id == 0) {
            TraceLog(LOG_ERROR, "Failed to load piece textures from image");
            return;
        }

        const float piece_size = static_cast<float>(board->piece_textures.width) / 6.0f;
        const float piece_height = static_cast<float>(board->piece_textures.height) / 2.0f;

        for (int32_t i = 0; i < 12; ++i) {
            board->piece_original_rects[i] = Rectangle{
                static_cast<float>(i % 6) * piece_size, static_cast<float>(i / 6) * piece_height, piece_size,
                piece_height
            };
        }

        const auto font_path = path / "liberation_mono_regular.ttf";
        board->font_big = LoadFontEx(font_path.string().c_str(), 20, nullptr, 1024);
        if (board->font_big.baseSize == 0) {
            TraceLog(LOG_ERROR, "Failed to load big font from memory");
        }

        board->font_small = LoadFontEx(font_path.string().c_str(), 12, nullptr, 1024);
        if (board->font_small.baseSize == 0) {
            TraceLog(LOG_ERROR, "Failed to load small font from memory");
        }

        board->capture_sound = LoadSound((path / "capture.mp3").string().c_str());
    }
}

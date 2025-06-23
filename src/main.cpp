#include <cstdlib>
#include "raylib.h"
#include "renderer/visual_board.hpp"
#include <filesystem>
int main()
{
    // Create a resizable window
    InitWindow(800, 800, "Chess");
    InitAudioDevice();
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    renderer::VisualBoard board{};
    renderer::visual_board_initialize(&board);
    const auto res = std::filesystem::current_path().parent_path().parent_path() / "res";
    const Image logo = LoadImage((res / "logo.png").string().c_str());
    SetWindowIcon(logo);
    renderer::visual_board_load_resources(&board, res);

    // Main loop
    while (!WindowShouldClose())
    {
        renderer::visual_board_resize(&board, GetScreenWidth(), GetScreenHeight());
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Draw the chessboard
        renderer::visual_board_draw(&board);
        DrawFPS(0, 0);
        EndDrawing();
    }

    CloseWindow();
    UnloadImage(logo);
    return 0;
}
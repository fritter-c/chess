#include <cstdlib>
#include "raylib.h"
#include "renderer/main_panel.hpp"
#include <filesystem>
int main()
{
    // Create a resizable window
    InitWindow(1280, 960, "Chess");
    SetWindowState(FLAG_WINDOW_RESIZABLE );
    InitAudioDevice();
    SetWindowMinSize(1280, 960);
    renderer::MainPanel panel{};

    const auto res = std::filesystem::current_path().parent_path().parent_path() / "res";
    renderer::main_panel_initialize(&panel, res);
    const Image logo = LoadImage((res / "logo.png").string().c_str());
    SetWindowIcon(logo);

    // Main loop
    while (!WindowShouldClose())
    {
        renderer::main_panel_resize(&panel, GetScreenWidth(), GetScreenHeight());
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Draw the chessboard
        renderer::main_panel_draw(&panel);
        DrawFPS(0, 0);
        EndDrawing();
    }

    CloseWindow();
    UnloadImage(logo);
    return 0;
}
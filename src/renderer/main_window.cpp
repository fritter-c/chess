#include <cstdlib>
#include "raylib.h"
#include "main_panel.hpp"
#include <filesystem>
namespace renderer
{
    int render()
    {
        InitWindow(1280, 960, "Chess");
        SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
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
            ClearBackground(GRAY);
            // Draw the chessboard
            renderer::main_panel_draw(&panel);
            DrawFPS(0, 0);
            EndDrawing();
        }
        CloseWindow();
        UnloadImage(logo);
        return 0;
    }
}

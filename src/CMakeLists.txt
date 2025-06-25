add_executable(chess main.cpp game/board.cpp
        renderer/visual_board.cpp
        game/move.cpp
        game/chess.cpp
        renderer/main_panel.cpp
        raygui.c)
target_link_libraries(chess PRIVATE raylib)
add_executable(chess main.cpp game/board.cpp
        renderer/visual_board.cpp
        game/move.cpp
        game/chess.cpp
        game/analyzer.cpp
        renderer/main_panel.cpp
        raygui.c)

if (MSVC)
        target_compile_options(chess PRIVATE /ARCH:AVX2 /03)
        else()
        target_compile_options(chess PRIVATE -march=native -O3)
endif()
target_link_libraries(chess PRIVATE raylib)
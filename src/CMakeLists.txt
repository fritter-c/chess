add_executable(chess main.cpp game/board.cpp renderer/visual_board.cpp)
target_link_libraries(chess PRIVATE raylib)
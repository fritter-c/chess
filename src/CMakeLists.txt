add_executable(chess main.cpp game/board.cpp 
                              renderer/visual_board.cpp
                              game/move.cpp
                              game/chess.cpp)
target_link_libraries(chess PRIVATE raylib)
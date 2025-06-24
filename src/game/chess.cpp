#include "chess.hpp"
#include <algorithm>
namespace game
{
    template <typename T>
    void array_push(T *array, uint32_t &size, uint32_t &capacity, T &value)
    {
        if (array == nullptr || size >= capacity)
        {
            capacity = (capacity == 0) ? 1 : capacity * 2;
            T *new_array = new T[capacity];
            if (array != nullptr)
            {
                std::copy(array, array + size, new_array);
                delete[] array;
            }
            array = new_array;
        }
        array[size++] = value;
    }

    void
    chess_game_initialize(ChessGame *game)
    {
        memset(game, 0, sizeof(ChessGame));
        game->white_player.color = ChessPieceColor::PIECE_WHITE;
        game->black_player.color = ChessPieceColor::PIECE_BLACK;
    }
} // namespace game
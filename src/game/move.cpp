#include "move.hpp"

namespace game
{
    std::string
    algebraic_move_to_string(const AlgebraicMove &move)
    {
        using enum PieceType;

        std::string result;

        if (move.kingside_castle)
        {
            result.append("0-0");
            return result;
        }

        if (move.queen_side_castle)
        {
            result.append("0-0-0");
            return result;
        }

        if (move.piece_type != PAWN)
        {
            result.push_back(chess_piece_to_algebraic_letter(move.piece_type));
            if (move.need_col_disambiguation)
            {
                result.push_back(static_cast<char>('a' + move.from_col));
            }
            if (move.need_row_disambiguation)
            {
                result.push_back(static_cast<char>('1' + move.from_row)); 
            }
        }

        if (move.is_capture)
        {
            if (move.piece_type == PAWN)
            {
                result.push_back(static_cast<char>('a' + move.from_col));
            }
            result.push_back('x');
        }

        result.push_back(static_cast<char>('a' + move.to_col));
        result.push_back(static_cast<char>('1' + move.to_row));

        if (move.is_checkmate)
            result.push_back('#');
        else if (move.is_check)
            result.push_back('+');
        return result;
    }

}

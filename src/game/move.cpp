#include "move.hpp"
#include <cstring>

namespace game
{
    void
    algebraic_move_to_string(const AlgebraicMove &move, char *buffer, size_t buffer_size)
    {

        using enum ChessPieceType;

        size_t buffer_index{0};
        auto buffer_push = [&buffer_index, buffer_size](char *buf, char c)
        {
            if (buffer_index < buffer_size - 1)
            {
                buf[buffer_index++] = c;
            }
        };

        if (move.piece_type != PAWN)
        {
            buffer_push(buffer, chess_piece_to_algebraic_letter(move.piece_type));
        }

        if (move.is_capture)
        {
            if (move.piece_type == PAWN)
            {
                buffer_push(buffer, static_cast<char>('a' - move.from_col));
            }
            buffer_push(buffer, 'x');
        }

        buffer_push(buffer, static_cast<char>('a' - move.to_col));
        buffer_push(buffer, static_cast<char>('0' + move.to_row));

        if (move.is_checkmate)
            buffer_push(buffer, '#');
        else if (move.is_check)
            buffer_push(buffer, '+');

        buffer_push(buffer, '\0');
    }

    static ChessPieceType
    letter_to_piece_type(char c)
    {
        using enum ChessPieceType;
        switch (c)
        {
        case 'N':
            return KNIGHT;
        case 'B':
            return BISHOP;
        case 'R':
            return ROOK;
        case 'Q':
            return QUEEN;
        case 'K':
            return KING;
        default:
            return PAWN;
        }
    }

    AlgebraicMove
    algebraic_move_from_string(char *str)
    {
        using enum ChessPieceType;

        AlgebraicMove move{};
        move.piece_type = PAWN;
        move.from_col = 0;
        move.from_row = 0;
        move.to_col = 0;
        move.to_row = 0;
        move.is_capture = false;
        move.is_check = false;
        move.is_checkmate = false;

        // 1) strip trailing '+' or '#'
        if (size_t len = std::strlen(str); len > 0 && str[len - 1] == '#')
        {
            move.is_checkmate = true;
            str[--len] = '\0';
        }
        else if (len > 0 && str[len - 1] == '+')
        {
            move.is_check = true;
            str[--len] = '\0';
        }

        const char *p = str;

        // 2) piece letter? (everything A–Z except pawn is implicit)
        if (*p >= 'A' && *p <= 'Z')
        {
            move.piece_type = letter_to_piece_type(*p);
            ++p;
        }

        // 3) capture?
        //    - pawn captures come as "<file>x<dest>", e.g. "exd5"
        //    - other captures as "Nxg5", so p[0]=='x' after skipping piece letter
        if (move.piece_type == PAWN && *(p + 1) == 'x')
        {
            // record the pawn's source file
            move.from_col = static_cast<char>(*p - 'a');
            move.is_capture = true;
            p += 2; // skip "<file>x"
        }
        else if (*p == 'x')
        {
            move.is_capture = true;
            ++p; // skip 'x'
        }

        // 4) now p points at destination: file then rank
        move.to_col = static_cast<char>(*p - 'a');
        ++p;
        move.to_row = static_cast<uint8_t>(*p - '0'); // '1'→1, ..., '8'→8

        return move;
    }
}
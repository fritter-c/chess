#include "fen.hpp"
#include <utils.hpp>
#include "bitboard.hpp"
namespace game {

bool Fen::set_fen(const char *fen) {
    Assert(fen, "Fen::set_fen: fen string cannot be null");
    gtr::char_string<128> fen_str(fen);

    auto valid_piece = [](const char c) {
        static gtr::string piece_chars = "PNBRQKpnbrqk";
        return piece_chars.find(c) != gtr::string::npos;
    };

    bool white_king_found = false;
    bool black_king_found = false;

    if (fen_str.count(' ') != 5) {
        return false;
    }

    if (fen_str.count('/') != 7) {
        return false;
    }

    int32_t index = 0;
    for (auto i = 0ULL; i < fen_str.size(); i++) {
        if (fen_str[i] == ' ') {
            if (fen_str.size() == i + 1ULL) {
                return false;
            }
            fields_index[index] = static_cast<uint8_t>(i + 1);
            index++;
        }
    }

    gtr::string position = fen_str.substr(0, fields_index[0] - 1).c_str();
    // Validate position
    if (position.size() > 64 + 7) { // 64 squares + 7 slashes
        return false;
    }
    std::array<int16_t, 8> rank_squares{};
    int8_t rank_index = 7;
    for (auto piece : position) {
        if (piece == '/') {
            if (rank_squares[rank_index] != 8) {
                return false;
            }
            rank_index--;
            if (rank_index < 0) {
                return false;
            }
        } else {
            if (gtr::is_numeric(piece)) {
                rank_squares[rank_index] += piece - '0';
            } else if (valid_piece(piece)) {
                rank_squares[rank_index]++;
                if (piece == 'K') {
                    if (white_king_found) {
                        return false; // More than one white king
                    }
                    white_king_found = true;
                } else if (piece == 'k') {
                    if (black_king_found) {
                        return false; // More than one black king
                    }
                    black_king_found = true;
                }
            } else {
                return false;
            }
        }
    }

    char active_color = fen_str[fields_index[0]];

    if (active_color != 'w' && active_color != 'b') {
        return false;
    }

    gtr::string castling_rights = fen_str.substr(fields_index[1], (fields_index[2] - 1)).c_str();
    // Validate castling rights
    if (castling_rights.size() > 4) {
        return false;
    }
    for (const char right : castling_rights) {
        switch (right) {
        case '-':
        case 'K':
        case 'Q':
        case 'k':
        case 'q': break;
        default : return false;
        }
    }

    gtr::string ep_square = fen_str.substr(fields_index[2], (fields_index[3] - 1)).c_str();
    // Validate en passant square
    if (ep_square[0] != '-' && ep_square.size() != 2) {
        return false; // En passant square must be '-' or a valid square
    }
    if (ep_square[0] != '-' && (ep_square[0] < 'a' || ep_square[0] > 'h' || ep_square[1] < '1' || ep_square[1] > '8')) {
        return false;
    }

    gtr::string halfmove_clock = fen_str.substr(fields_index[3], (fields_index[4] - 1)).c_str();
    if (halfmove_clock.to_int() == static_cast<int32_t>(LONG_MAX)) {
        return false;
    }

    gtr::string fullmove_number = fen_str.substr(fields_index[4]).c_str();
    if (fullmove_number.to_int() == static_cast<int32_t>(LONG_MAX)) {
        return false;
    }

    clear();
    append(fen_str);
    return true;
}

SquareIndex Fen::en_passant_square() const {
    gtr::string ep_str = substr(fields_index[2], fields_index[3] - 1).c_str();
    if (ep_str == "-") {
        return OUT_OF_BOUNDS;
    }
    const auto file = ep_str[0] - 'a';
    const auto rank = ep_str[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7) {
        return OUT_OF_BOUNDS; // Invalid en passant square
    }
    return static_cast<SquareIndex>(rank * 8 + file);
}

std::byte Fen::castle_rights() const {
    std::byte rights{};
    for (char right = at(fields_index[1]); right != ' '; ++right) {
        switch (right) {
        case 'K': rights |= CASTLE_WHITE_KINGSIDE; break;
        case 'Q': rights |= CASTLE_WHITE_QUEENSIDE; break;
        case 'k': rights |= CASTLE_BLACK_KINGSIDE; break;
        case 'q': rights |= CASTLE_BLACK_QUEENSIDE; break;
        default : break;
        }
    }
    return rights;
}

Piece Fen::piece_at(const SquareIndex s) const {
    const Rank rank = rank_of(s);
    const File file = file_of(s);
    int32_t current_rank = RANK_8;
    int32_t current_file = FILE_A;
    for (const auto p : *this) {
        if (current_rank == rank) {
            if (current_file == file) {
                switch (p) {
                case 'P': return WHITE_PAWN;
                case 'N': return WHITE_KNIGHT;
                case 'B': return WHITE_BISHOP;
                case 'R': return WHITE_ROOK;
                case 'Q': return WHITE_QUEEN;
                case 'K': return WHITE_KING;
                case 'p': return BLACK_PAWN;
                case 'n': return BLACK_KNIGHT;
                case 'b': return BLACK_BISHOP;
                case 'r': return BLACK_ROOK;
                case 'q': return BLACK_QUEEN;
                case 'k': return BLACK_KING;
                default : return PIECE_NONE;
                }
            }
            if (gtr::is_numeric(p)) {
                current_file += p - '0';
            } else {
                current_file++;
            }

            if (current_file > file) {
                return PIECE_NONE; // No piece at this square
            }
        }
        if (p == '/') {
            current_file = FILE_A;
            current_rank--;
            if (current_rank < 0) {
                return PIECE_NONE;
            }
        }
    }
    return PIECE_NONE; // No piece found
}

Fen Fen::build(const gtr::array<Piece, SQUARE_COUNT> &pieces, const Color t, const std::byte rights, const int8_t index, const int32_t halfmove, const int32_t fullmove) {
    gtr::char_string<128> position;
    std::array<gtr::string, RANK_COUNT> ranks{};
    int32_t empty_acum{0};
    Rank current_rank = RANK_1;
    for (int32_t i = 0; i < SQUARE_COUNT; ++i) {
        Rank new_rank = rank_of(static_cast<SquareIndex>(i));
        if (new_rank != current_rank && empty_acum != 0) {
            auto value = gtr::format("%d", empty_acum);
            ranks[current_rank].append(value);
            empty_acum = 0;
        }
        current_rank = new_rank;
        auto &buffer = ranks[current_rank];
        auto piece = pieces[i];
        if (piece == PIECE_NONE) {
            empty_acum++;
        } else {
            if (empty_acum > 0) {
                buffer.append(gtr::format("%d", empty_acum));
                empty_acum = 0;
            }
            switch (piece) {
            case WHITE_PAWN  : buffer.append('P'); break;
            case WHITE_KNIGHT: buffer.append('N'); break;
            case WHITE_BISHOP: buffer.append('B'); break;
            case WHITE_ROOK  : buffer.append('R'); break;
            case WHITE_QUEEN : buffer.append('Q'); break;
            case WHITE_KING  : buffer.append('K'); break;
            case BLACK_PAWN  : buffer.append('p'); break;
            case BLACK_KNIGHT: buffer.append('n'); break;
            case BLACK_BISHOP: buffer.append('b'); break;
            case BLACK_ROOK  : buffer.append('r'); break;
            case BLACK_QUEEN : buffer.append('q'); break;
            case BLACK_KING  : buffer.append('k'); break;
            default          : buffer.append(' '); break; // Should not happen
            }
        }
    }

    if (empty_acum) {
        ranks[current_rank].append(gtr::format("%d", empty_acum));
    }

    for (int i = RANK_8; i >= RANK_1; --i) {
        const auto& rank = ranks[i];
        position.append(rank);
        if (i != RANK_1) { // Do not append '/' after the last rank
            position.append('/');
        }
    }

    position.push_back(' ');
    position.push_back(t == PIECE_WHITE ? 'w' : 'b');
    position.push_back(' ');

    if (rights == CASTLE_NONE) {
        position.append("-");
    } else {
        if ((rights & CASTLE_WHITE_KINGSIDE) != std::byte{0})
            position.push_back('K');
        if ((rights & CASTLE_WHITE_QUEENSIDE) != std::byte{0})
            position.push_back('Q');
        if ((rights & CASTLE_BLACK_KINGSIDE) != std::byte{0})
            position.push_back('k');
        if ((rights & CASTLE_BLACK_QUEENSIDE) != std::byte{0})
            position.push_back('q');
    }

    position.push_back(' ');

    if (index != EN_PASSANT_INVALID_INDEX) {
        position.append(CellNamesC[index]);
    } else {
        position.append("-");
    }

    position.append(gtr::format(" %d %d", halfmove, fullmove));

    Fen fen;
    AssertExecute(fen.set_fen(position.c_str()), "Fen::build: Failed to set FEN string");
    return fen;
}

} // namespace game
#pragma once
// Utility to build board configurations easily from string patterns
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace test_utils {

// Build a board from a string pattern
// Each line represents a row (y coordinate)
// Characters: 'X' or 'x' = Black, 'O' or 'o' = White, '.' = Empty
// Spaces are ignored for readability
// Example:
//   set_board(board, R"(
//       . . . . . . .
//       . . . X X . .
//       . . X O O X .
//       . . . X X . .
//       . . . . . . .
//   )", 5, 5);
//
// The offset_x and offset_y parameters specify where to place the pattern on the board
inline void set_board(gomoku::Board& board, const std::string& pattern,
    int offset_x = 0, int offset_y = 0)
{
    using namespace gomoku;

    std::istringstream iss(pattern);
    std::string line;
    int y = 0;

    while (std::getline(iss, line)) {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        int x = 0;
        for (char ch : line) {
            if (ch == ' ' || ch == '\t') {
                continue; // Skip whitespace
            }

            Cell cell = Cell::Empty;
            if (ch == 'X' || ch == 'x') {
                cell = Cell::Black;
            } else if (ch == 'O' || ch == 'o') {
                cell = Cell::White;
            } else if (ch != '.') {
                continue; // Skip unknown characters
            }

            int board_x = offset_x + x;
            int board_y = offset_y + y;

            if (board_x >= 0 && board_x < 19 && board_y >= 0 && board_y < 19) {
                if (cell != Cell::Empty) {
                    board.setStone(Pos { static_cast<uint8_t>(board_x),
                                       static_cast<uint8_t>(board_y) },
                        cell);
                }
            }

            x++;
        }
        y++;
    }
}

// Compact version: set a line pattern
// Example: set_line(board, "XXOOX", 5, 5, Direction::Horizontal)
enum class Direction {
    Horizontal,
    Vertical,
    DiagonalDesc, // backslash (\)
    DiagonalAsc // slash (/)
};

inline void set_line(gomoku::Board& board, const std::string& pattern,
    int start_x, int start_y, Direction dir)
{
    using namespace gomoku;

    int x = start_x;
    int y = start_y;

    for (char ch : pattern) {
        if (ch == ' ' || ch == '\t') {
            continue; // Skip whitespace
        }

        Cell cell = Cell::Empty;
        if (ch == 'X' || ch == 'x') {
            cell = Cell::Black;
        } else if (ch == 'O' || ch == 'o') {
            cell = Cell::White;
        } else if (ch != '.') {
            continue; // Skip unknown characters
        }

        if (x >= 0 && x < 19 && y >= 0 && y < 19) {
            if (cell != Cell::Empty) {
                board.setStone(Pos { static_cast<uint8_t>(x),
                                   static_cast<uint8_t>(y) },
                    cell);
            }
        }

        // Move to next position based on direction
        switch (dir) {
        case Direction::Horizontal:
            x++;
            break;
        case Direction::Vertical:
            y++;
            break;
        case Direction::DiagonalDesc:
            x++;
            y++;
            break;
        case Direction::DiagonalAsc:
            x++;
            y--;
            break;
        }
    }
}

// Quick helpers for common patterns
inline void set_horizontal(gomoku::Board& board, const std::string& pattern, int x, int y)
{
    set_line(board, pattern, x, y, Direction::Horizontal);
}

inline void set_vertical(gomoku::Board& board, const std::string& pattern, int x, int y)
{
    set_line(board, pattern, x, y, Direction::Vertical);
}

inline void set_diagonal_desc(gomoku::Board& board, const std::string& pattern, int x, int y)
{
    set_line(board, pattern, x, y, Direction::DiagonalDesc);
}

inline void set_diagonal_asc(gomoku::Board& board, const std::string& pattern, int x, int y)
{
    set_line(board, pattern, x, y, Direction::DiagonalAsc);
}

} // namespace test_utils

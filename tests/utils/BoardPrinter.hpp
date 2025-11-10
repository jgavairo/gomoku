#pragma once
// Utility to print board state for debugging tests
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include <iomanip>
#include <iostream>

namespace test_utils {

inline void print_board(const gomoku::Board& board, bool show_coords = true)
{
    using namespace gomoku;

    if (show_coords) {
        // Print column numbers
        std::cout << "    ";
        for (int x = 0; x < 19; x++) {
            std::cout << std::setw(2) << x << " ";
        }
        std::cout << "\n";
    }

    for (int y = 0; y < 19; y++) {
        if (show_coords) {
            // Print row number
            std::cout << std::setw(2) << y << "  ";
        }

        for (int x = 0; x < 19; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
            char ch = '.';
            if (c == Cell::Black) {
                ch = 'X';
            } else if (c == Cell::White) {
                ch = 'O';
            }
            std::cout << ch;
            if (x < 18) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

inline void print_board_region(const gomoku::Board& board, int x_min, int x_max,
    int y_min, int y_max)
{
    using namespace gomoku;

    // Print column numbers
    std::cout << "    ";
    for (int x = x_min; x <= x_max; x++) {
        std::cout << std::setw(2) << x << " ";
    }
    std::cout << "\n";

    for (int y = y_min; y <= y_max; y++) {
        // Print row number
        std::cout << std::setw(2) << y << "  ";

        for (int x = x_min; x <= x_max; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
            char ch = '.';
            if (c == Cell::Black) {
                ch = 'X';
            } else if (c == Cell::White) {
                ch = 'O';
            }
            std::cout << ch;
            if (x < x_max) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

} // namespace test_utils

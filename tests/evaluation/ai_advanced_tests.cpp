#include "../framework/test_framework.hpp"
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/ai/MinimaxSearchEngine.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <iomanip>
#include <iostream>

using namespace gomoku;
using namespace gomoku::ai;

// Forward declaration
void run_all_ai_advanced_tests();

// ANSI color codes
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

static void print_board_with_move(const Board& board, const Move& move, const std::string& title)
{
    std::cout << "\n"
              << YELLOW << "=== " << title << " ===" << RESET << std::endl;
    std::cout << "   ";
   for (char x = 'A'; x <= 'S'; x++) {
        std::cout << std::setw(2) << x;
    }

    std::cout << "\n";

    for (int y = 0; y < 19; y++) {
        std::cout << std::setw(2) << y << "  ";

        for (int x = 0; x < 19; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));

            if (x == move.pos.x && y == move.pos.y) {
                std::cout << RED << (move.by == Player::Black ? "X " : "O ") << RESET;
            } else {
                if (c == Cell::Black)
                    std::cout << "X ";
                else if (c == Cell::White)
                    std::cout << "O ";
                else
                    std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

static void printSearchStats(const SearchStats& stats)
{
    std::cout << "  Stats: Depth=" << stats.depthReached
              << " Nodes=" << stats.nodes
              << " Time=" << stats.timeMs << "ms" << std::endl;
}


TEST(ai_advanced_user_scenario_1)
{
    Board board;
    RuleSet rules;

    // Setup from user board using BoardBuilder
    // Offset x=4 (E), y=6 (Row 7)
    test_utils::set_board(board, R"(
        X . . . . . . . . .
        . O . . . X . . . .
        . . O . . . . . . .
        . . . O X X X . X .
        . . . . . . . . X .
        . . . . . . . . . .
    )",
        4, 6);

    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "User Scenario 1 (Block Broken Four)");
    printSearchStats(stats);

    // Expectation: White must block the broken four at L9 (11, 9)
    ASSERT_EQ(move->pos.x, 11);
    ASSERT_EQ(move->pos.y, 9);
    TEST_PASSED();
}

void run_all_ai_advanced_tests()
{
    test_framework::run_all_tests("AI Advanced Tests (Cluttered Board)");
}

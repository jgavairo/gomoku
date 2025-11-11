// Unit tests for captures
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "../framework/test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_capture_tests();

// ============================================================================
// Tests 3) Captures
// ============================================================================

// Test 3.1: Horizontal capture - Pattern X O O X
TEST(capture_horizontal)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O . (Black can capture by playing right)
    test_utils::set_horizontal(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Verify both O were captured
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // Verify capture counter for Black
    ASSERT_EQ(board.capturedPairs().black, 1);
    ASSERT_EQ(board.capturedPairs().white, 0);

    TEST_PASSED();
}

// Test 3.2: Vertical capture - Motif X O O X
TEST(capture_vertical)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Vertical configuration: X O O . (Black can capture by playing down)
    test_utils::set_vertical(board, "XOO", 7, 3);

    board.forceSide(Player::Black);
    Move capture { Pos { 7, 6 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Verify both O were captured
    ASSERT_EQ(board.at(7, 4), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // Vérifier le compteur de captures
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 3.3: Descending diagonal capture - Motif X O O X
TEST(capture_diagonal_desc)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Diagonal configuration: X O O . (Black can capture)
    test_utils::set_diagonal_desc(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 8 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Verify both O were captured
    ASSERT_EQ(board.at(6, 6), Cell::Empty);
    ASSERT_EQ(board.at(7, 7), Cell::Empty);

    TEST_PASSED();
}

// Test 3.4: Ascending diagonal capture - Motif X O O X
TEST(capture_diagonal_asc)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Ascending diagonal configuration: X O O . (Black can capture)
    test_utils::set_diagonal_asc(board, "XOO", 5, 8);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Verify both O were captured
    ASSERT_EQ(board.at(6, 7), Cell::Empty);
    ASSERT_EQ(board.at(7, 6), Cell::Empty);

    TEST_PASSED();
}

// Test 3.5: No capture of single stone - X O X captures nothing
TEST(no_capture_single_stone)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O . (only 1 stone between two X)
    test_utils::set_horizontal(board, "XO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 7, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // The O stone should NOT be captured
    ASSERT_EQ(board.at(6, 5), Cell::White);

    // No captures
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 3.6: No capture of three or more stones - X O O O X captures nothing
TEST(no_capture_three_or_more)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O O . (3 consecutive stones)
    test_utils::set_horizontal(board, "XOOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // The 3 O stones should NOT be captured
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::White);

    // No captures
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 3.7: Multi-directional captures - One move can capture in multiple directions
TEST(multi_directional_capture)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration in a cross:
    //        X
    //        O
    //        O
    //   X O O . O O X  (horizontal)
    //        O
    //        O
    //        X
    // Black plays in the center and captures horizontally + vertically = 2 pairs

    test_utils::set_horizontal(board, "XOO", 5, 7); // Left
    test_utils::set_horizontal(board, "OOX", 9, 7); // Right (continuation)
    test_utils::set_vertical(board, "XOO", 8, 4); // Top
    test_utils::set_vertical(board, "OOX", 8, 8); // Bottom (continuation)

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 7 }, Player::Black }; // Center of the cross
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Verify that all 4 stones were captured (2 pairs)
    ASSERT_EQ(board.at(6, 7), Cell::Empty); // Horizontal left
    ASSERT_EQ(board.at(7, 7), Cell::Empty);
    ASSERT_EQ(board.at(9, 7), Cell::Empty); // Horizontal right
    ASSERT_EQ(board.at(10, 7), Cell::Empty);
    ASSERT_EQ(board.at(8, 5), Cell::Empty); // Vertical top
    ASSERT_EQ(board.at(8, 6), Cell::Empty);
    ASSERT_EQ(board.at(8, 8), Cell::Empty); // Vertical bottom
    ASSERT_EQ(board.at(8, 9), Cell::Empty);

    // 4 pairs captured
    ASSERT_EQ(board.capturedPairs().black, 4);

    TEST_PASSED();
}

// Test 3.8: Cells freed after capture become playable again
TEST(freed_positions_playable)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O .
    test_utils::set_horizontal(board, "XOO", 5, 5);

    // Black capture
    board.forceSide(Player::Black);
    Move capture { Pos { 8, 5 }, Player::Black };
    PlayResult r1 = board.tryPlay(capture, rules);
    ASSERT_TRUE(r1.success);

    // Positions (6,5) and (7,5) are now free
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // White can play on these positions again
    board.forceSide(Player::White);
    Move reuse1 { Pos { 6, 5 }, Player::White };
    PlayResult r2 = board.tryPlay(reuse1, rules);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(board.at(6, 5), Cell::White);

    board.forceSide(Player::Black);
    Move dummy { Pos { 10, 10 }, Player::Black };
    board.tryPlay(dummy, rules);

    board.forceSide(Player::White);
    Move reuse2 { Pos { 7, 5 }, Player::White };
    PlayResult r3 = board.tryPlay(reuse2, rules);
    ASSERT_TRUE(r3.success);
    ASSERT_EQ(board.at(7, 5), Cell::White);

    TEST_PASSED();
}

// Test 3.9: Correct capture counter for both sides
TEST(capture_counter_both_sides)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Black capture 2 paires de White
    test_utils::set_horizontal(board, "XOO", 3, 3);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 6, 3 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 3, 4);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 6, 4 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 2);
    ASSERT_EQ(board.capturedPairs().white, 0);

    // White capture 1 paire de Black
    test_utils::set_horizontal(board, "OXX", 10, 10);
    board.forceSide(Player::White);
    board.tryPlay(Move { Pos { 13, 10 }, Player::White }, rules);

    ASSERT_EQ(board.capturedPairs().black, 2);
    ASSERT_EQ(board.capturedPairs().white, 1);

    TEST_PASSED();
}

// Test 3.10: Win by 5 captured pairs (10 stones)
TEST(win_by_ten_captures)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5; // Win by 5 pairs (10 stones)

    // Black captures 5 pairs from White (spaced to avoid alignment)
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 7, 4);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 10, 4 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 2, 7);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 7 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 12, 9);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 15, 9 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 7, 12);
    board.forceSide(Player::Black);
    PlayResult r = board.tryPlay(Move { Pos { 10, 12 }, Player::Black }, rules);
    ASSERT_TRUE(r.success);

    // Verify 5 pairs captured
    ASSERT_EQ(board.capturedPairs().black, 5);

    // Win by capture
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 3.11: Win by capture priority over alignment
TEST(capture_win_priority)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5;

    // Black has already captured 4 pairs (spaced to avoid alignment)
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 7, 4);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 10, 4 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 2, 7);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 7 }, Player::Black }, rules);

    test_utils::set_horizontal(board, "XOO", 12, 9);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 15, 9 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 4);

    // Black makes a move that completes a 5-in-a-row AND captures the 5th pair
    // Configuration: Black already has XXXX diagonally
    test_utils::set_diagonal_desc(board, "XXXX", 10, 10);
    // And can capture one last pair
    test_utils::set_horizontal(board, "XOO", 7, 12);

    board.forceSide(Player::Black);

    // Black plays to capture 5th pair (reaches 10 captures)
    // This move also completes a diagonal alignment
    PlayResult r = board.tryPlay(Move { Pos { 10, 12 }, Player::Black }, rules);
    ASSERT_TRUE(r.success);

    // Win by capture (priority over alignment according to the code)
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// ============================================================================
// Entry point for tests
// ============================================================================

void run_all_capture_tests()
{
    run_all_tests("Captures");
}

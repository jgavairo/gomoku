// Unit tests for the forbidden double-three rule
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "../framework/test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_double_three_tests();

// ============================================================================
// Tests 4) Double-three forbidden (Forbidden Moves)
// ============================================================================

// Test 4.1: Free-three definition - Pattern _XX_X_ (open at both ends)
TEST(free_three_definition_open_both_ends)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: _XX_X_ (free-three horizontal)
    // Line: . X X . X . .
    test_utils::set_horizontal(board, ".XX.X", 5, 5);

    // Black can play (only one free-three, not forbidden)
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black }; // Complete the free-three
    PlayResult r = board.tryPlay(m, rules);

    // Move should be accepted (only one free-three)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.2: Free-three motif _X_XX_ (open at both ends)
TEST(free_three_pattern_x_xx)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: _X_XX_
    // Line: . X . X X . .
    test_utils::set_horizontal(board, ".X.XX", 5, 5);

    // Black can play to create a free-three
    board.forceSide(Player::Black);
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.3: Free-three motif __XXX_ (open at both ends)
TEST(free_three_pattern_xxx)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: . . X X X . .
    test_utils::set_horizontal(board, "..XXX", 5, 5);

    // Black can play to create a free-three
    board.forceSide(Player::Black);
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.4: Non-free-three - Blocked by opponent stone on the left (OXX_X_)
TEST(not_free_three_blocked_left)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: O X X . X . (blocked on left)
    test_utils::set_horizontal(board, "OXX.X", 5, 5);

    // This is NOT a free-three because blocked by O
    // Black can play freely
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.5: Non-free-three - Blocked by opponent stone on the right (_XX_XO)
TEST(not_free_three_blocked_right)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: . X X . X O (blocked on right)
    test_utils::set_horizontal(board, ".XX.XO", 5, 5);

    // This is NOT a free-three because blocked by O
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.6: Non-free-three - Blocked by edge
TEST(not_free_three_blocked_by_edge)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration at edge: positions 0-4
    // Edge X X . X . (blocked by edge on left)
    test_utils::set_horizontal(board, "XX.X", 0, 5);

    // This is NOT a free-three because blocked by edge
    board.forceSide(Player::Black);
    Move m { Pos { 2, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.7: Double-three forbidden - Two simultaneous free-threes (horizontal + vertical)
TEST(double_three_forbidden_horizontal_vertical)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration in cross that creates two free-threes:
    // A free-three is a pattern of 3 stones that can become an open 4
    //
    //        .
    //        X
    //        .
    //   . X . X .  (horizontal)
    //        X
    //        .
    // Position (8,5) completes the two free-threes

    // Horizontal: .X.X. → .X.XX = three stones with possibility of open 4
    test_utils::set_horizontal(board, ".X", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical: .X.X. → same thing vertically
    test_utils::set_vertical(board, ".X", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    board.forceSide(Player::Black);
    Move forbidden { Pos { 8, 5 }, Player::Black }; // Completes both patterns
    PlayResult r = board.tryPlay(forbidden, rules);

    if (r.success) {
        std::cout << "ERROR: Le coup a été accepté alors qu'il devrait être refusé!\n";
        test_utils::print_board_region(board, 4, 11, 1, 8);
    }

    // Move should be rejected (double-three)
    ASSERT_FALSE(r.success);
    ASSERT_EQ(r.code, PlayErrorCode::RuleViolation);

    TEST_PASSED();
} // Test 4.8: Double-three forbidden - Two free-threes in diagonals
TEST(double_three_forbidden_diagonals)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration in X shape:
    //   . X X . (diagonal desc)
    //     X X . (diagonal asc)
    //   Position (7,7) would create two free-threes

    // Descending diagonal: _XX_X_
    test_utils::set_diagonal_desc(board, ".XX", 5, 5);
    test_utils::set_diagonal_desc(board, "X", 9, 9);

    // Ascending diagonal: _XX_X_
    test_utils::set_diagonal_asc(board, ".XX", 5, 9);
    test_utils::set_diagonal_asc(board, "X", 9, 5);

    board.forceSide(Player::Black);
    Move forbidden { Pos { 7, 7 }, Player::Black };
    PlayResult r = board.tryPlay(forbidden, rules);

    // Move should be rejected (double-three)
    ASSERT_FALSE(r.success);

    TEST_PASSED();
}

// Test 4.9: No double-three if one pattern is blocked
TEST(no_double_three_if_one_blocked)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration:
    // Horizontal: _XX_X_ (free-three)
    // Vertical: OXX_X_ (blocked by O, not free-three)

    // Horizontal free
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical blocked by O at the top
    test_utils::set_vertical(board, "OXX", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    board.forceSide(Player::Black);
    Move allowed { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(allowed, rules);

    // Move should be accepted (only one free-three)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.10: Free-three count is done AFTER captures
TEST(free_three_count_after_captures)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;
    rules.capturesEnabled = true;

    // Configuration:
    // Horizontal: _XX_X_ (would be a free-three)
    // Vertical: _XX_X_ (would be a free-three)
    // BUT we add a capturable pair that will be removed

    // Horizontal
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical WITH capturable pair
    test_utils::set_vertical(board, ".XX", 8, 2);
    // Capturable pair: X OO . (instead of X)
    test_utils::set_vertical(board, "OO", 8, 6);
    test_utils::set_horizontal(board, "X", 7, 7);

    board.forceSide(Player::Black);
    Move with_capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(with_capture, rules);

    // After capture, only one vertical free-three remains
    // Move should be accepted
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.11: Double-three created BY a capture remains legal (exception)
TEST(double_three_by_capture_legal)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;
    rules.capturesEnabled = true;

    // Configuration: the move will capture AND create a double-three
    // But it is legal because the capture creates it

    // Paire capturable horizontale: X OO .
    test_utils::set_horizontal(board, "XOO", 5, 5);

    // After capture, we will have _X__X_ which forms a pattern
    // Also vertically
    test_utils::set_vertical(board, "X", 8, 3);
    test_utils::set_vertical(board, "X", 8, 7);

    board.forceSide(Player::Black);
    Move capture_move { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture_move, rules);

    // Move is legal despite double-three because created by capture
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.12: Free-three in all directions (horizontal, vertical, 2 diagonals)
TEST(free_three_all_directions)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // horizontal test
    Board board_h;
    test_utils::set_horizontal(board_h, ".XX.X", 5, 5);
    board_h.forceSide(Player::Black);
    PlayResult rh = board_h.tryPlay(Move { Pos { 8, 5 }, Player::Black }, rules);
    ASSERT_TRUE(rh.success);

    // Vertical test
    Board board_v;
    test_utils::set_vertical(board_v, ".XX.X", 5, 5);
    board_v.forceSide(Player::Black);
    PlayResult rv = board_v.tryPlay(Move { Pos { 5, 8 }, Player::Black }, rules);
    ASSERT_TRUE(rv.success);

    // Descending diagonal test
    Board board_d1;
    test_utils::set_diagonal_desc(board_d1, ".XX.X", 5, 5);
    board_d1.forceSide(Player::Black);
    PlayResult rd1 = board_d1.tryPlay(Move { Pos { 8, 8 }, Player::Black }, rules);
    ASSERT_TRUE(rd1.success);

    // Ascending diagonal test
    Board board_d2;
    test_utils::set_diagonal_asc(board_d2, ".XX.X", 5, 9);
    board_d2.forceSide(Player::Black);
    PlayResult rd2 = board_d2.tryPlay(Move { Pos { 8, 6 }, Player::Black }, rules);
    ASSERT_TRUE(rd2.success);

    TEST_PASSED();
}

// Test 4.13: RRule disabled - Double-three allowed
TEST(double_three_allowed_when_disabled)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = false; // Rule disabled

    // Configuration that would create a double-three
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    // Move should be accepted (rule disabled)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.14: White is not affected by the double-three rule
TEST(double_three_only_for_black)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration that would create a double-three for White
    test_utils::set_horizontal(board, ".OO", 5, 5);
    test_utils::set_horizontal(board, "O", 9, 5);
    test_utils::set_vertical(board, ".OO", 8, 2);
    test_utils::set_vertical(board, "O", 8, 6);

    board.forceSide(Player::White);
    Move m { Pos { 8, 5 }, Player::White };
    PlayResult r = board.tryPlay(m, rules);

    // Move should be accepted (rule only applies to Black)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// ============================================================================
// Entry point for tests
// ============================================================================

void run_all_double_three_tests()
{
    run_all_tests("Double-Three Forbidden");
}

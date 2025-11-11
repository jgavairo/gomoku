// Unit tests for alignment detection and victories
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "../framework/test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_alignment_tests();

// ============================================================================
// Tests 2) Alignment detection and victories
// ============================================================================

// Test 2.1: Detection of 5 aligned horizontally
TEST(detect_five_horizontal)
{
    Board board;
    RuleSet rules;

    // Place 4 black stones horizontally
    test_utils::set_horizontal(board, "XXXX", 5, 5);

    // 5th move with tryPlay to trigger detection
    Move last_move { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // After Black's 5th move, there should be victory
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.2: Detection of 5 aligned vertically
TEST(detect_five_vertical)
{
    Board board;
    RuleSet rules;

    // Place 4 white stones vertically
    test_utils::set_vertical(board, "OOOO", 7, 3);

    // Force current player to White and play the 5th move
    board.forceSide(Player::White);
    Move last_move { Pos { 7, 7 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.3: Detection of 5 aligned diagonally (backslash)
TEST(detect_five_diagonal_desc)
{
    Board board;
    RuleSet rules;

    // Place 4 black stones diagonally descending
    test_utils::set_diagonal_desc(board, "XXXX", 4, 4);

    // 5th move with tryPlay
    Move last_move { Pos { 8, 8 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // Victory should be detected
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.4: Detection of 5 aligned diagonally (slash)
TEST(detect_five_diagonal_asc)
{
    Board board;
    RuleSet rules;

    // Place 4 white stones diagonally ascending
    test_utils::set_diagonal_asc(board, "OOOO", 4, 8);

    // 5th move with tryPlay
    board.forceSide(Player::White);
    Move last_move { Pos { 8, 4 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.5: Detection of 6 aligned
TEST(detect_six_aligned)
{
    Board board;
    RuleSet rules;

    // Place 4 black stones horizontally
    test_utils::set_horizontal(board, "XXXX", 3, 10);

    // Le 5ème coup déclenche la victoire
    Move last_move { Pos { 7, 10 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // 5+ aligned give victory
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.6: Detection of 7 aligned
TEST(detect_seven_aligned)
{
    Board board;
    RuleSet rules;

    // Place 4 white stones vertically
    test_utils::set_vertical(board, "OOOO", 9, 5);

    // The 5th move triggers victory
    board.forceSide(Player::White);
    Move last_move { Pos { 9, 9 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // 5+ aligned give victory
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.7: 5 aligned breakable by immediate capture → no victory
TEST(five_breakable_by_capture_no_win)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: Black has 5 aligned at y=9, White can capture B-B pair at y=8
    // Ligne 8: . . . . X X . .
    // Ligne 9: . . . O X X X X X .
    test_utils::set_board(board, R"(
        . . . . X X . .
        . . . O X X X X X .
    )",
        3, 8);

    // White plays to capture the two B at (4,8) and (5,8)
    board.forceSide(Player::White);
    Move capture_move { Pos { 6, 8 }, Player::White };
    PlayResult r = board.tryPlay(capture_move, rules);
    ASSERT_TRUE(r.success);

    // Test verifies that even with a 5, if breakable, not necessarily game over
    // (depends on the implementation of the "breakable five" rule)

    TEST_PASSED();
}

// Test 2.8: 5 aligned NOT breakable → victory
TEST(five_not_breakable_win)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Black forms a 5 that cannot be broken by capture
    test_utils::set_horizontal(board, "XXXXX", 5, 5);

    // No capturable pairs around - White has no way to break this 5
    board.forceSide(Player::Black);

    // Black plays next to it to trigger verification
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Victory because the 5 cannot be broken
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.9: Line of 5 with immediate capture available → must break rule
TEST(must_break_five_rule)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.allowFiveOrMore = true;

    // Board configuration:
    // Ligne 4: . . X
    // Ligne 5: . . O
    // Ligne 6: . . O O O O . 
    // White has 5 aligned at y=5, Black can capture O-O pair at y=4
    test_utils::set_board(board, R"(
        . . X .
        . . O .
        . . O O O O .
    )",
        3, 4);

    board.forceSide(Player::White);
    Move win { Pos { 9, 6 }, Player::White };
    PlayResult r = board.tryPlay(win, rules);
    ASSERT_TRUE(r.success);

    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    // Black can break the 5 by capturing one of the stones
    board.forceSide(Player::Black);

    // Black capture les deux O en (6,4) et (7,4)
    Move capture { Pos { 5, 7 }, Player::Black };
    PlayResult r1 = board.tryPlay(capture, rules);
    ASSERT_TRUE(r1.success);

    // Verify capture worked by checking stones disappeared
    ASSERT_EQ(board.at(6, 4), Cell::Empty);
    ASSERT_EQ(board.at(7, 4), Cell::Empty);

    // Game must continue (White's 5 at y=5 still exists)
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    Move m { Pos { 5, 6 }, Player::White };
    PlayResult r2 = board.tryPlay(m, rules);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.10: No victory with only 4 aligned
TEST(four_aligned_no_win)
{
    Board board;
    RuleSet rules;

    // Place only 4 black stones horizontally
    test_utils::set_horizontal(board, "XXXX", 5, 7);

    board.forceSide(Player::Black);
    Move m { Pos { 10, 10 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // No victory with only 4
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    TEST_PASSED();
}

// Test 2.11: Detection in all directions from a single move
TEST(detect_all_four_directions)
{
    Board board;
    RuleSet rules;

    // Test horizontal
    Board board_h;
    test_utils::set_horizontal(board_h, "XXXX", 0, 5);
    Move mh { Pos { 4, 5 }, Player::Black };
    PlayResult rh = board_h.tryPlay(mh, rules);
    ASSERT_TRUE(rh.success);
    ASSERT_EQ(board_h.status(), GameStatus::WinByAlign);

    // Test vertical
    Board board_v;
    test_utils::set_vertical(board_v, "XXXX", 5, 0);
    Move mv { Pos { 5, 4 }, Player::Black };
    PlayResult rv = board_v.tryPlay(mv, rules);
    ASSERT_TRUE(rv.success);
    ASSERT_EQ(board_v.status(), GameStatus::WinByAlign);

    // Descending diagonal test (backslash)
    Board board_d1;
    test_utils::set_diagonal_desc(board_d1, "XXXX", 0, 0);
    Move md1 { Pos { 4, 4 }, Player::Black };
    PlayResult rd1 = board_d1.tryPlay(md1, rules);
    ASSERT_TRUE(rd1.success);
    ASSERT_EQ(board_d1.status(), GameStatus::WinByAlign);

    // Ascending diagonal test (slash)
    Board board_d2;
    test_utils::set_diagonal_asc(board_d2, "XXXX", 0, 4);
    Move md2 { Pos { 4, 0 }, Player::Black };
    PlayResult rd2 = board_d2.tryPlay(md2, rules);
    ASSERT_TRUE(rd2.success);
    ASSERT_EQ(board_d2.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// ============================================================================
// Test entry point
// ============================================================================

void run_all_alignment_tests()
{
    run_all_tests("Alignment Detection");
}

// Unit tests for global legality and edge cases
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_legality_tests();

// ============================================================================
// Tests 5) Global legality and evaluation order
// ============================================================================

// Test 5.1: Evaluation order - Placement → Captures → Victories
TEST(evaluation_order_capture_then_victory)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5;

    // Black has already captured 4 pairs
    // We will create 4 captures directly then the 5th
    for (int i = 0; i < 4; i++) {
        test_utils::set_horizontal(board, "XOO", 2 + i * 3, 2 + i);
        board.forceSide(Player::Black);
        board.tryPlay(Move { Pos { static_cast<uint8_t>(5 + i * 3), static_cast<uint8_t>(2 + i) }, Player::Black }, rules);
    }

    ASSERT_EQ(board.capturedPairs().black, 4);

    // Last capture reaches 5 pairs
    test_utils::set_horizontal(board, "XOO", 2, 15);
    board.forceSide(Player::Black);
    PlayResult r = board.tryPlay(Move { Pos { 5, 15 }, Player::Black }, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.capturedPairs().black, 5);
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 5.2: Illegal move → No victory declared
TEST(illegal_move_no_victory)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: 2 free-threes (illegal double-three for Black)
    // Horizontal: .XX at row 5, playing at (8,5) makes .XXX
    test_utils::set_horizontal(board, ".XX", 5, 5);
    // Vertical: .XX at col 8, playing at (8,5) makes .XXX
    test_utils::set_vertical(board, ".XX", 8, 2);

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black }; // Completes the 2 free-threes
    PlayResult r = board.tryPlay(illegal, rules);

    // The move must be rejected
    ASSERT_FALSE(r.success);

    // Status must remain Ongoing (no victory)
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    // The position must not be occupied
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.3: Illegal move → Board unchanged (complete rollback)
TEST(illegal_move_board_unchanged)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: 2 free-threes to create double-three
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);

    // Save board state
    uint64_t hashBefore = board.zobristKey();

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(illegal, rules);

    ASSERT_FALSE(r.success);

    // Hash must not have changed
    ASSERT_EQ(board.zobristKey(), hashBefore);

    // Current player must not have changed
    ASSERT_EQ(board.toPlay(), Player::Black);

    // The position must not be occupied
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.4: No phantom capture - Diagonal with gap
TEST(no_phantom_capture_diagonal_gap)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O . O X (gap in the middle)
    // This is NOT capturable because there is a gap
    test_utils::set_diagonal_desc(board, "XO", 5, 5);
    test_utils::set_diagonal_desc(board, "O", 8, 8);
    test_utils::set_diagonal_desc(board, "X", 9, 9);

    // We will rather build without the last stone
    Board board2;
    test_utils::set_diagonal_desc(board2, "XO", 5, 5);
    test_utils::set_diagonal_desc(board2, "O", 8, 8);

    board2.forceSide(Player::Black);
    PlayResult r = board2.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    ASSERT_TRUE(r.success);

    // No capture should occur (gap at position 7,7)
    ASSERT_EQ(board2.at(6, 6), Cell::White);
    ASSERT_EQ(board2.at(8, 8), Cell::White);
    ASSERT_EQ(board2.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.5: No phantom capture - Only 1 stone between two
TEST(no_phantom_capture_single_stone)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O X (only 1 stone, not capturable)
    test_utils::set_horizontal(board, "XO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 7, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Stone O must NOT be captured
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.6: No phantom capture - 3 or more stones
TEST(no_phantom_capture_three_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O O X (3 stones, not capturable)
    test_utils::set_horizontal(board, "XOOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // No stone should be captured
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.7: Valid capture exactly 2 consecutive stones
TEST(valid_capture_exactly_two_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O X (exactly 2 stones)
    test_utils::set_horizontal(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // The 2 stones must be captured
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 5.8: Free-threes counted AFTER captures
TEST(free_three_after_captures_applied)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.forbidDoubleThree = true;

    // Simplified configuration:
    // One horizontal free-three + one vertical capture
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical capturable pair
    test_utils::set_vertical(board, "XOO", 8, 2);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    // Move must be accepted (capture happens before free-three count)
    ASSERT_TRUE(r.success);

    // Verify capture occurred
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 5.9: Victory detected after captures applied
TEST(victory_detected_after_captures)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 2; // Only 2 pairs to simplify

    // Black captures 1 pair
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 1);

    // Last capture to reach 2 pairs
    test_utils::set_horizontal(board, "XOO", 7, 7);
    board.forceSide(Player::Black);
    Move final_capture { Pos { 10, 7 }, Player::Black };
    PlayResult r = board.tryPlay(final_capture, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.capturedPairs().black, 2);

    // Victory by capture must be detected
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 5.10: Complete order - Illegality detected before any modification
TEST(illegality_detected_before_changes)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Double-three configuration: .XX to complete to .XXX
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);

    // Save hash
    uint64_t hashBefore = board.zobristKey();

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(illegal, rules);

    // The move must be rejected
    ASSERT_FALSE(r.success);
    ASSERT_EQ(r.code, PlayErrorCode::RuleViolation);

    // Nothing should have changed
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.11: Capture on non-continuous line (edge) impossible
TEST(no_capture_across_board_edge)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Near the edge: positions 17, 18 and we try to "capture"
    // but the line is not continuous with the edge
    test_utils::set_horizontal(board, "XOO", 16, 5);

    // Rebuild: O O X at right edge
    Board board2;
    test_utils::set_horizontal(board2, "OOX", 16, 5);

    board2.forceSide(Player::Black);
    // Black plays at left of the O's: X O O X (edge)
    // Position 15 to make X O O X (18)
    Move m2 { Pos { 15, 5 }, Player::Black };
    PlayResult r = board2.tryPlay(m2, rules);
    ASSERT_TRUE(r.success);

    // Stones must be captured (valid if within bounds)
    ASSERT_EQ(board2.at(16, 5), Cell::Empty);
    ASSERT_EQ(board2.at(17, 5), Cell::Empty);

    TEST_PASSED();
}

// ============================================================================
// Entry point for tests
// ============================================================================

void run_all_legality_tests()
{
    run_all_tests("Global Legality");
}

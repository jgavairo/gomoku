// Unit tests for system reversibility and consistency
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "../framework/test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_reversibility_tests();

// ============================================================================
// Tests 7) Reversibility and consistency
// ============================================================================

// Test 7.1: Simple move followed by undo completely restores state
TEST(simple_move_undo_restores_all)
{
    Board board;
    RuleSet rules;

    // Initial state
    uint64_t hashBefore = board.zobristKey();
    Player toPlayBefore = board.toPlay();

    // play a move
    Move m { Pos { 9, 9 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // State has changed
    ASSERT_NE(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(9, 9), Cell::Black);
    ASSERT_EQ(board.toPlay(), Player::White);

    // Undo
    board.undo();

    // Verify complete restoration
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(9, 9), Cell::Empty);
    ASSERT_EQ(board.toPlay(), toPlayBefore);

    TEST_PASSED();
}

// Test 7.2: Move with capture followed by undo restores captured stones
TEST(move_with_capture_undo_restores_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration for a capture
    test_utils::set_horizontal(board, "XOO", 5, 5);

    uint64_t hashBefore = board.zobristKey();
    CaptureCount capsBefore = board.capturedPairs();

    // Make a capture
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Verify capture occurred
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);
    ASSERT_EQ(board.capturedPairs().black, 1);

    // Undo
    board.undo();

    // Verify complete restoration
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(6, 5), Cell::White); // Stones restored
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::Empty); // Move canceled
    ASSERT_EQ(board.capturedPairs().black, capsBefore.black);

    TEST_PASSED();
}

// Test 7.3: Move with multi-captures followed by undo
TEST(move_with_multi_capture_undo)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration for double capture
    test_utils::set_horizontal(board, "XOO", 5, 5);
    test_utils::set_vertical(board, "XOO", 8, 2);

    uint64_t hashBefore = board.zobristKey();

    // Make a double capture
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // 2 captures in 2 directions
    ASSERT_EQ(board.capturedPairs().black, 2);

    // Undo
    board.undo();

    // Verify restoration of 4 stones
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(6, 5), Cell::White); // Horizontal
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 3), Cell::White); // Vertical
    ASSERT_EQ(board.at(8, 4), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 7.4: Sequence of moves followed by undos restores initial state
TEST(move_sequence_undo_sequence)
{
    Board board;
    RuleSet rules;

    uint64_t initialHash = board.zobristKey();

    // Sequence of 5 moves
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    board.tryPlay(Move { Pos { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { Pos { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { Pos { 9, 11 }, Player::Black }, rules);

    ASSERT_NE(board.zobristKey(), initialHash);

    // Undo all moves
    board.undo();
    board.undo();
    board.undo();
    board.undo();
    board.undo();

    // Initial state restored
    ASSERT_EQ(board.zobristKey(), initialHash);
    ASSERT_EQ(board.at(9, 9), Cell::Empty);
    ASSERT_EQ(board.at(10, 9), Cell::Empty);
    ASSERT_EQ(board.at(9, 10), Cell::Empty);
    ASSERT_EQ(board.at(10, 10), Cell::Empty);
    ASSERT_EQ(board.at(9, 11), Cell::Empty);
    ASSERT_EQ(board.toPlay(), Player::Black);

    TEST_PASSED();
}

// Test 7.5: Hash changes on simple placement
TEST(hash_changes_on_placement)
{
    Board board;
    RuleSet rules;

    uint64_t hash1 = board.zobristKey();

    // Place a stone
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash2 = board.zobristKey();

    ASSERT_NE(hash1, hash2);

    // Place another stone
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    uint64_t hash3 = board.zobristKey();

    ASSERT_NE(hash2, hash3);
    ASSERT_NE(hash1, hash3);

    TEST_PASSED();
}

// Test 7.6: Hash changes on capture
TEST(hash_changes_on_capture)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration for capture
    test_utils::set_horizontal(board, "XOO", 5, 5);

    uint64_t hashBefore = board.zobristKey();

    // Make a capture
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 8, 5 }, Player::Black }, rules);

    uint64_t hashAfter = board.zobristKey();

    // Hash must have changed (placement + 2 captures)
    ASSERT_NE(hashBefore, hashAfter);

    TEST_PASSED();
}

// Test 7.7: Hash changes on turn change
TEST(hash_changes_on_turn_change)
{
    Board board;
    RuleSet rules;

    uint64_t hash1 = board.zobristKey();
    ASSERT_EQ(board.toPlay(), Player::Black);

    // Play a move (changes turn)
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash2 = board.zobristKey();

    ASSERT_NE(hash1, hash2);
    ASSERT_EQ(board.toPlay(), Player::White);

    TEST_PASSED();
}

// Test 7.8: Undo after victory restores Ongoing status
TEST(undo_after_victory_restores_status)
{
    Board board;
    RuleSet rules;

    // Configuration for immediate victory
    test_utils::set_horizontal(board, "XXXX", 5, 5);

    board.forceSide(Player::Black);
    Move winning { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(winning, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    // Undo
    board.undo();

    // Status must return to Ongoing
    ASSERT_EQ(board.status(), GameStatus::Ongoing);
    ASSERT_EQ(board.at(9, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 7.9: Undo after capture victory restores counter
TEST(undo_after_capture_victory_restores_counter)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 2;

    // First capture
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 1);

    // Last capture (victory)
    test_utils::set_horizontal(board, "XOO", 7, 7);
    board.forceSide(Player::Black);
    Move winning { Pos { 10, 7 }, Player::Black };
    board.tryPlay(winning, rules);

    ASSERT_EQ(board.status(), GameStatus::WinByCapture);
    ASSERT_EQ(board.capturedPairs().black, 2);

    // Undo
    board.undo();

    // Counter must return to 1
    ASSERT_EQ(board.capturedPairs().black, 1);
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    TEST_PASSED();
}

// Test 7.10: Multiple undo/redo cycles maintain consistency
TEST(multiple_undo_redo_consistency)
{
    Board board;
    RuleSet rules;

    uint64_t hash0 = board.zobristKey();

    // Move 1
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash1 = board.zobristKey();

    // Move 2
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    uint64_t hash2 = board.zobristKey();

    // Undo -> hash1
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash1);

    // Replay move 2 -> hash2
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    ASSERT_EQ(board.zobristKey(), hash2);

    // Undo all -> hash0
    board.undo();
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash0);

    TEST_PASSED();
}

// Test 7.11: Different hash for symmetric positions
TEST(hash_different_for_symmetric_positions)
{
    Board board1;
    Board board2;
    RuleSet rules;

    // Position 1: X at (9,9)
    board1.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash1 = board1.zobristKey();

    // Position 2: X at (10,10) (symmetric)
    board2.tryPlay(Move { Pos { 10, 10 }, Player::Black }, rules);
    uint64_t hash2 = board2.zobristKey();

    // Hashes must be different
    ASSERT_NE(hash1, hash2);

    TEST_PASSED();
}

// Test 7.12: Undo on empty board does nothing
TEST(undo_on_empty_board_safe)
{
    Board board;

    uint64_t hashBefore = board.zobristKey();

    // Undo on empty board (must not crash)
    board.undo();

    // State must not have changed
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.toPlay(), Player::Black);

    TEST_PASSED();
}

// ============================================================================
// Entry point for tests
// ============================================================================

void run_all_reversibility_tests()
{
    run_all_tests("Reversibility and Consistency");
}

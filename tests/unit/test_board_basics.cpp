// Unit tests for board basics: size, indexing, occupancy
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "gomoku/core/Zobrist.hpp"
#include "../framework/test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_board_basic_tests();

// ============================================================================
// Tests 1) Board and placements
// ============================================================================

// Test 1.1: Verify board is 19×19
TEST(board_size_19x19)
{
    Board board;

    // BOARD_SIZE must be 19
    ASSERT_EQ(BOARD_SIZE, 19);

    // Verify all valid positions are within bounds
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
        for (uint8_t y = 0; y < BOARD_SIZE; y++) {
            ASSERT_TRUE(board.isInside(x, y));
            ASSERT_TRUE(board.isEmpty(x, y));
        }
    }

    TEST_PASSED();
}

// Test 1.2: Verify indexing 0..360 is correct
TEST(board_indexation_0_to_360)
{
    Board board;

    // For a 19x19 board, there are 361 cells (0 to 360)
    const int total_cells = BOARD_SIZE * BOARD_SIZE;
    ASSERT_EQ(total_cells, 361);

    // Verify each position (x,y) corresponds to a unique index
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            Pos p { x, y };
            uint16_t idx = p.toIndex();

            // Index must be between 0 and 360
            ASSERT_TRUE(idx >= 0 && idx < 361);

            // Inverse conversion must give the same position
            Pos recovered = Pos::fromIndex(idx);
            ASSERT_EQ(recovered.x, x);
            ASSERT_EQ(recovered.y, y);
        }
    }

    // Verify edge cases
    Pos topLeft { 0, 0 };
    ASSERT_EQ(topLeft.toIndex(), 0);

    Pos bottomRight { 18, 18 };
    ASSERT_EQ(bottomRight.toIndex(), 360);

    Pos middle { 9, 9 };
    ASSERT_EQ(middle.toIndex(), 9 * 19 + 9);

    TEST_PASSED();
}

// Test 1.3: Occupied cell → illegal move
TEST(occupied_cell_illegal)
{
    Board board;
    RuleSet rules;

    // Place a black stone at (5, 5)
    Move m1 { Pos { 5, 5 }, Player::Black };
    PlayResult r1 = board.tryPlay(m1, rules);
    ASSERT_TRUE(r1.success);
    ASSERT_FALSE(board.isEmpty(5, 5));
    ASSERT_EQ(board.at(5, 5), Cell::Black);

    // Try to place a white stone at the same location
    Move m2 { Pos { 5, 5 }, Player::White };
    PlayResult r2 = board.tryPlay(m2, rules);
    ASSERT_FALSE(r2.success);
    ASSERT_EQ(r2.code, PlayErrorCode::Occupied);

    // The cell must still contain the black stone
    ASSERT_EQ(board.at(5, 5), Cell::Black);

    // Reset the board
    board.reset();

    // Place a black stone at (10, 10)
    Move m3 { Pos { 10, 10 }, Player::Black };
    board.tryPlay(m3, rules);

    // Try to place a white stone at the same location (White's turn)
    Move m4 { Pos { 10, 10 }, Player::White };
    PlayResult r4 = board.tryPlay(m4, rules);
    ASSERT_FALSE(r4.success);
    ASSERT_EQ(r4.code, PlayErrorCode::Occupied);

    TEST_PASSED();
}

// Test 1.4: Out of bounds → illegal move
TEST(out_of_bounds_illegal)
{
    Board board;
    RuleSet rules;

    // Test out of bounds positions
    ASSERT_FALSE(board.isInside(19, 0));
    ASSERT_FALSE(board.isInside(0, 19));
    ASSERT_FALSE(board.isInside(19, 19));
    ASSERT_FALSE(board.isInside(255, 0));
    ASSERT_FALSE(board.isInside(0, 255));

    // Valid positions are [0..18]
    ASSERT_TRUE(board.isInside(0, 0));
    ASSERT_TRUE(board.isInside(18, 18));
    ASSERT_TRUE(board.isInside(9, 9));

    // Verify that at() returns Empty for out of bounds
    ASSERT_EQ(board.at(19, 0), Cell::Empty);
    ASSERT_EQ(board.at(0, 19), Cell::Empty);
    ASSERT_EQ(board.at(20, 20), Cell::Empty);

    // Try to play out of bounds (position is invalid)
    Move m_out { Pos { 19, 5 }, Player::Black };
    ASSERT_FALSE(m_out.isValid());

    Move m_out2 { Pos { 5, 19 }, Player::Black };
    ASSERT_FALSE(m_out2.isValid());

    // If we try to play anyway, isEmpty should return false
    ASSERT_FALSE(board.isEmpty(19, 5));
    ASSERT_FALSE(board.isEmpty(5, 19));

    TEST_PASSED();
}

// Test 1.5: Placement and undo are bit-perfect reversible
TEST(placement_and_undo_reversible)
{
    Board board;
    RuleSet rules;

    // Save initial state
    uint64_t initial_hash = board.zobristKey();
    ASSERT_EQ(board.stoneCount(Player::Black), 0);
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Place a black stone at (5, 5)
    Move m1 { Pos { 5, 5 }, Player::Black };
    PlayResult r1 = board.tryPlay(m1, rules);
    ASSERT_TRUE(r1.success);

    uint64_t hash_after_m1 = board.zobristKey();

    ASSERT_NE(hash_after_m1, initial_hash);
    ASSERT_EQ(board.at(5, 5), Cell::Black);
    ASSERT_EQ(board.stoneCount(Player::Black), 1);

    // Undo the move
    bool undone1 = board.undo();
    ASSERT_TRUE(undone1);

    // Verify state has returned to initial
    ASSERT_EQ(board.zobristKey(), initial_hash);
    ASSERT_TRUE(board.isEmpty(5, 5));
    ASSERT_EQ(board.stoneCount(Player::Black), 0);

    // Sequence of multiple moves
    Move m2 { Pos { 3, 3 }, Player::Black };
    Move m3 { Pos { 4, 4 }, Player::White };
    Move m4 { Pos { 5, 5 }, Player::Black };

    board.tryPlay(m2, rules);
    uint64_t hash_after_m2 = board.zobristKey();

    board.tryPlay(m3, rules);
    uint64_t hash_after_m3 = board.zobristKey();

    board.tryPlay(m4, rules);
    // hash_after_m4 is not used in this test

    ASSERT_EQ(board.stoneCount(Player::Black), 2);
    ASSERT_EQ(board.stoneCount(Player::White), 1);

    // Undo m4
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash_after_m3);
    ASSERT_TRUE(board.isEmpty(5, 5));
    ASSERT_EQ(board.stoneCount(Player::Black), 1);

    // Undo m3
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash_after_m2);
    ASSERT_TRUE(board.isEmpty(4, 4));
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Undo m2
    board.undo();
    ASSERT_EQ(board.zobristKey(), initial_hash);
    ASSERT_TRUE(board.isEmpty(3, 3));
    ASSERT_EQ(board.stoneCount(Player::Black), 0);

    TEST_PASSED();
}

// Test 1.6: Bit-perfect reversibility with Zobrist hash
TEST(zobrist_hash_reversibility)
{
    Board board;
    RuleSet rules;

    uint64_t h0 = board.zobristKey();

    // Sequence of moves
    std::vector<Move> moves = {
        { Pos { 9, 9 }, Player::Black },
        { Pos { 9, 10 }, Player::White },
        { Pos { 10, 9 }, Player::Black },
        { Pos { 10, 10 }, Player::White },
        { Pos { 8, 9 }, Player::Black }
    };

    std::vector<uint64_t> hashes;
    hashes.push_back(h0);

    // Play all moves and record hashes
    for (const auto& m : moves) {
        PlayResult r = board.tryPlay(m, rules);
        ASSERT_TRUE(r.success);
        hashes.push_back(board.zobristKey());
    }

    // Verify all hashes are different
    for (size_t i = 0; i < hashes.size(); i++) {
        for (size_t j = i + 1; j < hashes.size(); j++) {
            ASSERT_NE(hashes[i], hashes[j]);
        }
    }

    // Undo all moves and verify hash reversibility
    for (int i = static_cast<int>(moves.size()) - 1; i >= 0; i--) {
        bool undone = board.undo();
        ASSERT_TRUE(undone);
        ASSERT_EQ(board.zobristKey(), hashes[i]);
    }

    // Final hash must be identical to initial hash
    ASSERT_EQ(board.zobristKey(), h0);

    TEST_PASSED();
}

// Test 1.7: Verify stone counter is accurate
TEST(stone_count_accuracy)
{
    Board board;
    RuleSet rules;

    ASSERT_EQ(board.stoneCount(Player::Black), 0);
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Use setStone to place stones directly without turn constraints
    // Place 10 black stones and 9 white stones
    for (int i = 0; i < 10; i++) {
        board.setStone(Pos { static_cast<uint8_t>(i), 0 }, Cell::Black);
    }
    for (int i = 0; i < 9; i++) {
        board.setStone(Pos { static_cast<uint8_t>(i), 1 }, Cell::White);
    }

    ASSERT_EQ(board.stoneCount(Player::Black), 10);
    ASSERT_EQ(board.stoneCount(Player::White), 9);

    // Now test with tryPlay to verify undo works
    board.reset();

    // Place some stones alternating
    for (int i = 0; i < 6; i++) {
        Player p = (i % 2 == 0) ? Player::Black : Player::White;
        Move m { Pos { static_cast<uint8_t>(i), 0 }, p };
        PlayResult r = board.tryPlay(m, rules);
        ASSERT_TRUE(r.success);
    }

    ASSERT_EQ(board.stoneCount(Player::Black), 3);
    ASSERT_EQ(board.stoneCount(Player::White), 3);

    // Undo some moves
    board.undo(); // White
    board.undo(); // Black

    ASSERT_EQ(board.stoneCount(Player::Black), 2);
    ASSERT_EQ(board.stoneCount(Player::White), 2);

    TEST_PASSED();
}

// Test 1.8: Verify board occupancy tracking
TEST(board_occupancy_tracking)
{
    Board board;
    RuleSet rules;

    // Initially, no positions are occupied
    const auto& occupied = board.occupiedPositions();
    ASSERT_EQ(occupied.size(), 0);

    // Place some stones
    std::vector<Pos> placed = {
        { 0, 0 }, { 5, 5 }, { 18, 18 }, { 9, 9 }
    };

    for (size_t i = 0; i < placed.size(); i++) {
        Move m { placed[i], (i % 2 == 0) ? Player::Black : Player::White };
        board.tryPlay(m, rules);
    }

    // Verify the number of occupied positions is correct
    const auto& occupied2 = board.occupiedPositions();
    ASSERT_EQ(occupied2.size(), placed.size());

    // Verify all placed positions are in the list
    for (const auto& p : placed) {
        bool found = false;
        for (const auto& occ : occupied2) {
            if (occ == p) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found);
    }

    TEST_PASSED();
}

// ============================================================================
// Test entry point
// ============================================================================

void run_all_board_basic_tests()
{
    run_all_tests("Board Basics");
}

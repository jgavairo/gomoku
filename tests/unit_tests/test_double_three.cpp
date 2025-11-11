// Tests unitaires pour la règle du double-trois interdit
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_double_three_tests();

// ============================================================================
// Tests 4) Double-trois interdit (Forbidden Moves)
// ============================================================================

// Test 4.1: Définition d'un free-three - Motif _XX_X_ (ouvert aux deux bouts)
TEST(free_three_definition_open_both_ends)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: _XX_X_ (free-three horizontal)
    // Ligne: . X X . X . .
    test_utils::set_horizontal(board, ".XX.X", 5, 5);

    // Black peut jouer (c'est un seul free-three, pas interdit)
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black }; // Compléter le free-three
    PlayResult r = board.tryPlay(m, rules);

    // Le coup devrait être accepté (un seul free-three)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.2: Free-three motif _X_XX_ (ouvert aux deux bouts)
TEST(free_three_pattern_x_xx)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: _X_XX_
    // Ligne: . X . X X . .
    test_utils::set_horizontal(board, ".X.XX", 5, 5);

    // Black peut jouer pour créer un free-three
    board.forceSide(Player::Black);
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.3: Free-three motif __XXX_ (ouvert aux deux bouts)
TEST(free_three_pattern_xxx)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: . . X X X . .
    test_utils::set_horizontal(board, "..XXX", 5, 5);

    // Black peut jouer pour créer un free-three
    board.forceSide(Player::Black);
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.4: Non-free-three - Bloqué par pierre adverse à gauche (OXX_X_)
TEST(not_free_three_blocked_left)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: O X X . X . (bloqué à gauche)
    test_utils::set_horizontal(board, "OXX.X", 5, 5);

    // Ce n'est PAS un free-three car bloqué par O
    // Black peut jouer librement
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.5: Non-free-three - Bloqué par pierre adverse à droite (_XX_XO)
TEST(not_free_three_blocked_right)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: . X X . X O (bloqué à droite)
    test_utils::set_horizontal(board, ".XX.XO", 5, 5);

    // Ce n'est PAS un free-three car bloqué par O
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.6: Non-free-three - Bloqué par le bord
TEST(not_free_three_blocked_by_edge)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration au bord: positions 0-4
    // Bord X X . X . (bloqué par le bord à gauche)
    test_utils::set_horizontal(board, "XX.X", 0, 5);

    // Ce n'est PAS un free-three car bloqué par le bord
    board.forceSide(Player::Black);
    Move m { Pos { 2, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.7: Double-trois interdit - Deux free-threes simultanés (horizontal + vertical)
TEST(double_three_forbidden_horizontal_vertical)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration en croix qui crée deux free-threes:
    // Un free-three est un motif de 3 pierres qui peut devenir un 4 ouvert
    //
    //        .
    //        X
    //        .
    //   . X . X .  (horizontal)
    //        X
    //        .
    // Position (8,5) complète les deux free-threes

    // Horizontal: .X.X. → .X.XX = trois pierres avec possibilité de 4 ouvert
    test_utils::set_horizontal(board, ".X", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical: .X.X. → même chose verticalement
    test_utils::set_vertical(board, ".X", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    std::cout << "\n=== Configuration double-three test ===\n";
    test_utils::print_board_region(board, 4, 11, 1, 8);

    board.forceSide(Player::Black);
    Move forbidden { Pos { 8, 5 }, Player::Black }; // Complète les deux patterns
    PlayResult r = board.tryPlay(forbidden, rules);

    if (r.success) {
        std::cout << "ERROR: Le coup a été accepté alors qu'il devrait être refusé!\n";
        test_utils::print_board_region(board, 4, 11, 1, 8);
    }

    // Le coup devrait être refusé (double-trois)
    ASSERT_FALSE(r.success);
    ASSERT_EQ(r.code, PlayErrorCode::RuleViolation);

    TEST_PASSED();
} // Test 4.8: Double-trois interdit - Deux free-threes en diagonales
TEST(double_three_forbidden_diagonals)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration en X:
    //   . X X . (diagonal desc)
    //     X X . (diagonal asc)
    //   Position (7,7) créerait deux free-threes

    // Diagonal descendante: _XX_X_
    test_utils::set_diagonal_desc(board, ".XX", 5, 5);
    test_utils::set_diagonal_desc(board, "X", 9, 9);

    // Diagonal ascendante: _XX_X_
    test_utils::set_diagonal_asc(board, ".XX", 5, 9);
    test_utils::set_diagonal_asc(board, "X", 9, 5);

    board.forceSide(Player::Black);
    Move forbidden { Pos { 7, 7 }, Player::Black };
    PlayResult r = board.tryPlay(forbidden, rules);

    // Le coup devrait être refusé (double-trois)
    ASSERT_FALSE(r.success);

    TEST_PASSED();
}

// Test 4.9: Pas de double-trois si un des motifs est bloqué
TEST(no_double_three_if_one_blocked)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration:
    // Horizontal: _XX_X_ (free-three)
    // Vertical: OXX_X_ (bloqué par O, pas free-three)

    // Horizontal libre
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical bloqué par O en haut
    test_utils::set_vertical(board, "OXX", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    board.forceSide(Player::Black);
    Move allowed { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(allowed, rules);

    // Le coup devrait être accepté (un seul free-three)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.10: Le compte des free-threes se fait APRÈS les captures
TEST(free_three_count_after_captures)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;
    rules.capturesEnabled = true;

    // Configuration:
    // Horizontal: _XX_X_ (serait un free-three)
    // Vertical: _XX_X_ (serait un free-three)
    // MAIS on ajoute une paire capturable qui sera enlevée

    // Horizontal
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Vertical AVEC paire capturable
    test_utils::set_vertical(board, ".XX", 8, 2);
    // Paire capturable: X OO . (au lieu de X)
    test_utils::set_vertical(board, "OO", 8, 6);
    test_utils::set_horizontal(board, "X", 7, 7);

    board.forceSide(Player::Black);
    Move with_capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(with_capture, rules);

    // Après la capture, il ne reste qu'un free-three vertical
    // Le coup devrait être accepté
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.11: Double-trois créé PAR une capture reste légal (exception)
TEST(double_three_by_capture_legal)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;
    rules.capturesEnabled = true;

    // Configuration: le coup va capturer ET créer un double-trois
    // Mais c'est légal car c'est la capture qui le crée

    // Paire capturable horizontale: X OO .
    test_utils::set_horizontal(board, "XOO", 5, 5);

    // Après capture, on aura _X__X_ qui forme un motif
    // En vertical aussi
    test_utils::set_vertical(board, "X", 8, 3);
    test_utils::set_vertical(board, "X", 8, 7);

    board.forceSide(Player::Black);
    Move capture_move { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture_move, rules);

    // Le coup est légal malgré le double-trois car créé par capture
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.12: Free-three en toutes directions (horizontal, vertical, 2 diagonales)
TEST(free_three_all_directions)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Test horizontal
    Board board_h;
    test_utils::set_horizontal(board_h, ".XX.X", 5, 5);
    board_h.forceSide(Player::Black);
    PlayResult rh = board_h.tryPlay(Move { Pos { 8, 5 }, Player::Black }, rules);
    ASSERT_TRUE(rh.success);

    // Test vertical
    Board board_v;
    test_utils::set_vertical(board_v, ".XX.X", 5, 5);
    board_v.forceSide(Player::Black);
    PlayResult rv = board_v.tryPlay(Move { Pos { 5, 8 }, Player::Black }, rules);
    ASSERT_TRUE(rv.success);

    // Test diagonal descendante
    Board board_d1;
    test_utils::set_diagonal_desc(board_d1, ".XX.X", 5, 5);
    board_d1.forceSide(Player::Black);
    PlayResult rd1 = board_d1.tryPlay(Move { Pos { 8, 8 }, Player::Black }, rules);
    ASSERT_TRUE(rd1.success);

    // Test diagonal ascendante
    Board board_d2;
    test_utils::set_diagonal_asc(board_d2, ".XX.X", 5, 9);
    board_d2.forceSide(Player::Black);
    PlayResult rd2 = board_d2.tryPlay(Move { Pos { 8, 6 }, Player::Black }, rules);
    ASSERT_TRUE(rd2.success);

    TEST_PASSED();
}

// Test 4.13: Règle désactivée - Double-trois autorisé
TEST(double_three_allowed_when_disabled)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = false; // Règle désactivée

    // Configuration qui créerait un double-trois
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);
    test_utils::set_vertical(board, "X", 8, 6);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    // Le coup devrait être accepté (règle désactivée)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// Test 4.14: White n'est pas affecté par la règle du double-trois
TEST(double_three_only_for_black)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration qui créerait un double-trois pour White
    test_utils::set_horizontal(board, ".OO", 5, 5);
    test_utils::set_horizontal(board, "O", 9, 5);
    test_utils::set_vertical(board, ".OO", 8, 2);
    test_utils::set_vertical(board, "O", 8, 6);

    board.forceSide(Player::White);
    Move m { Pos { 8, 5 }, Player::White };
    PlayResult r = board.tryPlay(m, rules);

    // Le coup devrait être accepté (la règle ne s'applique qu'à Black)
    ASSERT_TRUE(r.success);

    TEST_PASSED();
}

// ============================================================================
// Point d'entrée des tests
// ============================================================================

void run_all_double_three_tests()
{
    run_all_tests("Double-Three Forbidden");
}

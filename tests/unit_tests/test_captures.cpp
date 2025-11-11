// Tests unitaires pour les captures
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_capture_tests();

// ============================================================================
// Tests 3) Captures
// ============================================================================

// Test 3.1: Capture horizontale - Motif X O O X
TEST(capture_horizontal)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O . (Black peut capturer en jouant à droite)
    test_utils::set_horizontal(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que les deux O ont été capturés
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // Vérifier le compteur de captures pour Black
    ASSERT_EQ(board.capturedPairs().black, 1);
    ASSERT_EQ(board.capturedPairs().white, 0);

    TEST_PASSED();
}

// Test 3.2: Capture verticale - Motif X O O X
TEST(capture_vertical)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration verticale: X O O . (Black peut capturer en jouant en bas)
    test_utils::set_vertical(board, "XOO", 7, 3);

    board.forceSide(Player::Black);
    Move capture { Pos { 7, 6 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que les deux O ont été capturés
    ASSERT_EQ(board.at(7, 4), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // Vérifier le compteur de captures
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 3.3: Capture diagonale descendante - Motif X O O X
TEST(capture_diagonal_desc)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration diagonale: X O O . (Black peut capturer)
    test_utils::set_diagonal_desc(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 8 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que les deux O ont été capturés
    ASSERT_EQ(board.at(6, 6), Cell::Empty);
    ASSERT_EQ(board.at(7, 7), Cell::Empty);

    TEST_PASSED();
}

// Test 3.4: Capture diagonale ascendante - Motif X O O X
TEST(capture_diagonal_asc)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration diagonale ascendante: X O O . (Black peut capturer)
    test_utils::set_diagonal_asc(board, "XOO", 5, 8);

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que les deux O ont été capturés
    ASSERT_EQ(board.at(6, 7), Cell::Empty);
    ASSERT_EQ(board.at(7, 6), Cell::Empty);

    TEST_PASSED();
}

// Test 3.5: Pas de capture d'une seule pierre - X O X ne capture rien
TEST(no_capture_single_stone)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O . (seulement 1 pierre entre deux X)
    test_utils::set_horizontal(board, "XO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 7, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // La pierre O ne doit PAS être capturée
    ASSERT_EQ(board.at(6, 5), Cell::White);

    // Aucune capture
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 3.6: Pas de capture de 3 pierres ou plus - X O O O X ne capture rien
TEST(no_capture_three_or_more)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O O . (3 pierres consécutives)
    test_utils::set_horizontal(board, "XOOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Les 3 pierres O ne doivent PAS être capturées
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::White);

    // Aucune capture
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 3.7: Captures multi-directions - Un coup peut capturer dans plusieurs directions
TEST(multi_directional_capture)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration en croix:
    //        X
    //        O
    //        O
    //   X O O . O O X  (horizontale)
    //        O
    //        O
    //        X
    // Black joue au centre et capture horizontal + vertical = 2 paires

    test_utils::set_horizontal(board, "XOO", 5, 7); // Gauche
    test_utils::set_horizontal(board, "OOX", 9, 7); // Droite (continuation)
    test_utils::set_vertical(board, "XOO", 8, 4); // Haut
    test_utils::set_vertical(board, "OOX", 8, 8); // Bas (continuation)

    board.forceSide(Player::Black);
    Move capture { Pos { 8, 7 }, Player::Black }; // Centre de la croix
    PlayResult r = board.tryPlay(capture, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que les 4 pierres ont été capturées (2 paires)
    ASSERT_EQ(board.at(6, 7), Cell::Empty); // Horizontal gauche
    ASSERT_EQ(board.at(7, 7), Cell::Empty);
    ASSERT_EQ(board.at(9, 7), Cell::Empty); // Horizontal droite
    ASSERT_EQ(board.at(10, 7), Cell::Empty);
    ASSERT_EQ(board.at(8, 5), Cell::Empty); // Vertical haut
    ASSERT_EQ(board.at(8, 6), Cell::Empty);
    ASSERT_EQ(board.at(8, 8), Cell::Empty); // Vertical bas
    ASSERT_EQ(board.at(8, 9), Cell::Empty);

    // 4 paires capturées
    ASSERT_EQ(board.capturedPairs().black, 4);

    TEST_PASSED();
}

// Test 3.8: Cases libérées après capture redeviennent jouables
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

    // Les positions (6,5) et (7,5) sont maintenant libres
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);

    // White peut rejouer sur ces positions
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

// Test 3.9: Compteur de captures correct pour les deux camps
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

// Test 3.10: Victoire par 5 paires capturées (10 pierres)
TEST(win_by_ten_captures)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5; // Victoire à 5 paires (10 pierres)

    // Black capture 5 paires de White (espacées pour éviter alignement)
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

    // Vérifier 5 paires capturées
    ASSERT_EQ(board.capturedPairs().black, 5);

    // Victoire par capture
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 3.11: Victoire par capture prioritaire sur alignement
TEST(capture_win_priority)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5;

    // Black a déjà 4 paires capturées (espacées pour éviter alignement)
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

    // Black fait un coup qui complète un alignement de 5 ET capture la 5ème paire
    // Configuration: Black a déjà XXXX en diagonal
    test_utils::set_diagonal_desc(board, "XXXX", 10, 10);
    // Et peut capturer une dernière paire
    test_utils::set_horizontal(board, "XOO", 7, 12);

    board.forceSide(Player::Black);

    // Black joue pour capturer la 5ème paire (atteint 10 captures)
    // Ce coup complète aussi un alignement diagonal
    PlayResult r = board.tryPlay(Move { Pos { 10, 12 }, Player::Black }, rules);
    ASSERT_TRUE(r.success);

    // Victoire par capture (prioritaire sur alignement selon le code)
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// ============================================================================
// Point d'entrée des tests
// ============================================================================

void run_all_capture_tests()
{
    run_all_tests("Captures");
}

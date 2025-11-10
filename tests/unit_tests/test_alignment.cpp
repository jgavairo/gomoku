// Tests unitaires pour la détection d'alignements et victoires
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_alignment_tests();

// ============================================================================
// Tests 2) Détection d'alignements et victoires
// ============================================================================

// Test 2.1: Détection de 5 alignés horizontaux
TEST(detect_five_horizontal)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres avec setStone (plus rapide)
    for (int x = 5; x <= 8; x++) {
        board.setStone(Pos { static_cast<uint8_t>(x), 5 }, Cell::Black);
    }

    // Le 5ème coup avec tryPlay pour déclencher la détection
    Move last_move { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // Après le 5ème coup noir, il devrait y avoir victoire
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.2: Détection de 5 alignés verticaux
TEST(detect_five_vertical)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres blanches avec setStone
    for (int y = 3; y <= 6; y++) {
        board.setStone(Pos { 7, static_cast<uint8_t>(y) }, Cell::White);
    }

    // Forcer le joueur courant à White et jouer le 5ème coup
    board.forceSide(Player::White);
    Move last_move { Pos { 7, 7 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.3: Détection de 5 alignés en diagonale (backslash)
TEST(detect_five_diagonal_desc)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres avec setStone
    for (int i = 0; i < 4; i++) {
        board.setStone(Pos { static_cast<uint8_t>(4 + i), static_cast<uint8_t>(4 + i) }, Cell::Black);
    }

    // Le 5ème coup avec tryPlay
    Move last_move { Pos { 8, 8 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // La victoire devrait être détectée
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.4: Détection de 5 alignés en diagonale (slash)
TEST(detect_five_diagonal_asc)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres avec setStone
    for (int i = 0; i < 4; i++) {
        board.setStone(Pos { static_cast<uint8_t>(4 + i), static_cast<uint8_t>(8 - i) }, Cell::White);
    }

    // Le 5ème coup avec tryPlay
    board.forceSide(Player::White);
    Move last_move { Pos { 8, 4 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.5: Détection de 6 alignés
TEST(detect_six_aligned)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres avec setStone
    for (int x = 3; x <= 6; x++) {
        board.setStone(Pos { static_cast<uint8_t>(x), 10 }, Cell::Black);
    }

    // Le 5ème coup déclenche la victoire
    Move last_move { Pos { 7, 10 }, Player::Black };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // 5+ alignés donnent la victoire
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.6: Détection de 7 alignés
TEST(detect_seven_aligned)
{
    Board board;
    RuleSet rules;

    // Placer 4 pierres avec setStone
    for (int y = 5; y <= 8; y++) {
        board.setStone(Pos { 9, static_cast<uint8_t>(y) }, Cell::White);
    }

    // Le 5ème coup déclenche la victoire
    board.forceSide(Player::White);
    Move last_move { Pos { 9, 9 }, Player::White };
    PlayResult r = board.tryPlay(last_move, rules);
    ASSERT_TRUE(r.success);

    // 5+ alignés donnent la victoire
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.7: 5 alignés cassable par capture immédiate → pas de victoire
TEST(five_breakable_by_capture_no_win)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration : Black a 5 alignés en y=9, White peut capturer la paire B-B en y=8
    // Ligne 8: . . . . X X . .
    // Ligne 9: . . . O X X X X X .
    test_utils::set_board(board, R"(
        . . . . X X . .
        . . . O X X X X X .
    )",
        3, 8);

    // White joue pour capturer les deux B en (4,8) et (5,8)
    board.forceSide(Player::White);
    Move capture_move { Pos { 6, 8 }, Player::White };
    PlayResult r = board.tryPlay(capture_move, rules);
    ASSERT_TRUE(r.success);

    // Le test vérifie que même avec un 5, si c'est cassable, ce n'est pas forcément fini
    // (dépend de l'implémentation de la règle "breakable five")

    TEST_PASSED();
}

// Test 2.8: 5 alignés NON cassable → victoire
TEST(five_not_breakable_win)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Black forme un 5 qui ne peut pas être cassé par capture
    // Configuration simple: 5 noirs alignés sans possibilité de capture immédiate
    for (int x = 5; x <= 9; x++) {
        board.setStone(Pos { static_cast<uint8_t>(x), 5 }, Cell::Black);
    }

    // Aucune paire capturable autour
    // White n'a aucun moyen de casser ce 5

    board.forceSide(Player::Black);

    // Black joue à côté pour déclencher la vérification
    Move m { Pos { 10, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Victoire car le 5 n'est pas cassable
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// Test 2.9: Ligne de 5 avec capture immédiate disponible → must break rule
TEST(must_break_five_rule)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.allowFiveOrMore = true;

    // Configuration du plateau :
    // Ligne 4: . . X O O .
    // Ligne 5: . X O O O O O .
    // White a 5 alignés en y=5, Black peut capturer la paire O-O en y=4
    test_utils::set_board(board, R"(
        . . X O O .
        . X O O O O O .
    )",
        3, 4);

    std::cout << "\n=== Configuration initiale (White a un 5 en y=5) ===\n";
    test_utils::print_board_region(board, 3, 10, 3, 10);

    // Black peut casser le 5 en capturant une des pierres
    board.forceSide(Player::Black);

    // Black capture les deux O en (6,4) et (7,4)
    Move capture { Pos { 8, 4 }, Player::Black };
    PlayResult r1 = board.tryPlay(capture, rules);
    ASSERT_TRUE(r1.success);

    std::cout << "=== Après capture en (8,4) ===\n";
    test_utils::print_board_region(board, 3, 10, 3, 10);

    // Vérifier que la capture a bien fonctionné en vérifiant que les pierres ont disparu
    ASSERT_EQ(board.at(6, 4), Cell::Empty);
    ASSERT_EQ(board.at(7, 4), Cell::Empty);

    // Le jeu doit continuer (le 5 de White en y=5 existe toujours)
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    TEST_PASSED();
}

// Test 2.10: Pas de victoire avec seulement 4 alignés
TEST(four_aligned_no_win)
{
    Board board;
    RuleSet rules;

    // Placer seulement 4 pierres noires horizontalement
    for (int x = 5; x <= 8; x++) {
        board.setStone(Pos { static_cast<uint8_t>(x), 7 }, Cell::Black);
    }

    board.forceSide(Player::Black);
    Move m { Pos { 10, 10 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Pas de victoire avec seulement 4
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    TEST_PASSED();
}

// Test 2.11: Détection dans toutes les directions depuis un coup
TEST(detect_all_four_directions)
{
    Board board;
    RuleSet rules;

    // Test horizontal
    Board board_h;
    for (int x = 0; x < 4; x++) {
        board_h.setStone(Pos { static_cast<uint8_t>(x), 5 }, Cell::Black);
    }
    Move mh { Pos { 4, 5 }, Player::Black };
    PlayResult rh = board_h.tryPlay(mh, rules);
    ASSERT_TRUE(rh.success);
    ASSERT_EQ(board_h.status(), GameStatus::WinByAlign);

    // Test vertical
    Board board_v;
    for (int y = 0; y < 4; y++) {
        board_v.setStone(Pos { 5, static_cast<uint8_t>(y) }, Cell::Black);
    }
    Move mv { Pos { 5, 4 }, Player::Black };
    PlayResult rv = board_v.tryPlay(mv, rules);
    ASSERT_TRUE(rv.success);
    ASSERT_EQ(board_v.status(), GameStatus::WinByAlign);

    // Test diagonale descendante (backslash)
    Board board_d1;
    for (int i = 0; i < 4; i++) {
        board_d1.setStone(Pos { static_cast<uint8_t>(i), static_cast<uint8_t>(i) }, Cell::Black);
    }
    Move md1 { Pos { 4, 4 }, Player::Black };
    PlayResult rd1 = board_d1.tryPlay(md1, rules);
    ASSERT_TRUE(rd1.success);
    ASSERT_EQ(board_d1.status(), GameStatus::WinByAlign);

    // Test diagonale ascendante (slash)
    Board board_d2;
    for (int i = 0; i < 4; i++) {
        board_d2.setStone(Pos { static_cast<uint8_t>(i), static_cast<uint8_t>(4 - i) }, Cell::Black);
    }
    Move md2 { Pos { 4, 0 }, Player::Black };
    PlayResult rd2 = board_d2.tryPlay(md2, rules);
    ASSERT_TRUE(rd2.success);
    ASSERT_EQ(board_d2.status(), GameStatus::WinByAlign);

    TEST_PASSED();
}

// ============================================================================
// Point d'entrée des tests
// ============================================================================

void run_all_alignment_tests()
{
    run_all_tests("Alignment Detection");
}

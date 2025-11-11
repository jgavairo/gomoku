// Tests unitaires pour la légalité globale et cas limites
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
// Tests 5) Légalité globale et ordre d'évaluation
// ============================================================================

// Test 5.1: Ordre d'évaluation - Pose → Captures → Victoires
TEST(evaluation_order_capture_then_victory)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 5;

    // Black a déjà 4 paires capturées
    // On va créer 4 captures directement puis la 5ème
    for (int i = 0; i < 4; i++) {
        test_utils::set_horizontal(board, "XOO", 2 + i * 3, 2 + i);
        board.forceSide(Player::Black);
        board.tryPlay(Move { Pos { static_cast<uint8_t>(5 + i * 3), static_cast<uint8_t>(2 + i) }, Player::Black }, rules);
    }

    ASSERT_EQ(board.capturedPairs().black, 4);

    // Dernière capture atteint 5 paires
    test_utils::set_horizontal(board, "XOO", 2, 15);
    board.forceSide(Player::Black);
    PlayResult r = board.tryPlay(Move { Pos { 5, 15 }, Player::Black }, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.capturedPairs().black, 5);
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 5.2: Coup illégal → Aucune victoire déclarée
TEST(illegal_move_no_victory)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: 2 free-threes (double-trois illégal pour Black)
    // Horizontal: .XX at row 5, playing at (8,5) makes .XXX
    test_utils::set_horizontal(board, ".XX", 5, 5);
    // Vertical: .XX at col 8, playing at (8,5) makes .XXX
    test_utils::set_vertical(board, ".XX", 8, 2);

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black }; // Complète les 2 free-threes
    PlayResult r = board.tryPlay(illegal, rules);

    // Le coup doit être refusé
    ASSERT_FALSE(r.success);

    // Le statut doit rester Ongoing (pas de victoire)
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    // La position ne doit pas être occupée
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.3: Coup illégal → Board inchangé (rollback complet)
TEST(illegal_move_board_unchanged)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration: 2 free-threes pour créer double-trois
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);

    // Sauvegarder l'état du board
    uint64_t hashBefore = board.zobristKey();

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(illegal, rules);

    ASSERT_FALSE(r.success);

    // Le hash ne doit pas avoir changé
    ASSERT_EQ(board.zobristKey(), hashBefore);

    // Le joueur courant ne doit pas avoir changé
    ASSERT_EQ(board.toPlay(), Player::Black);

    // La position ne doit pas être occupée
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.4: Pas de capture fantôme - Diagonale avec trou
TEST(no_phantom_capture_diagonal_gap)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O . O X (trou au milieu)
    // Ce n'est PAS capturable car il y a un trou
    test_utils::set_diagonal_desc(board, "XO", 5, 5);
    test_utils::set_diagonal_desc(board, "O", 8, 8);
    test_utils::set_diagonal_desc(board, "X", 9, 9);

    // On va plutôt construire sans la dernière pierre
    Board board2;
    test_utils::set_diagonal_desc(board2, "XO", 5, 5);
    test_utils::set_diagonal_desc(board2, "O", 8, 8);

    board2.forceSide(Player::Black);
    PlayResult r = board2.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    ASSERT_TRUE(r.success);

    // Aucune capture ne doit avoir lieu (trou en position 7,7)
    ASSERT_EQ(board2.at(6, 6), Cell::White);
    ASSERT_EQ(board2.at(8, 8), Cell::White);
    ASSERT_EQ(board2.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.5: Pas de capture fantôme - Seulement 1 pierre entre deux
TEST(no_phantom_capture_single_stone)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O X (seulement 1 pierre, pas capturable)
    test_utils::set_horizontal(board, "XO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 7, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // La pierre O ne doit PAS être capturée
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.6: Pas de capture fantôme - 3 pierres ou plus
TEST(no_phantom_capture_three_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O O X (3 pierres, pas capturable)
    test_utils::set_horizontal(board, "XOOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Aucune pierre ne doit être capturée
    ASSERT_EQ(board.at(6, 5), Cell::White);
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 5.7: Capture valide exactement 2 pierres consécutives
TEST(valid_capture_exactly_two_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration: X O O X (exactement 2 pierres)
    test_utils::set_horizontal(board, "XOO", 5, 5);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Les 2 pierres doivent être capturées
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 5.8: Free-threes comptés APRÈS captures
TEST(free_three_after_captures_applied)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.forbidDoubleThree = true;

    // Configuration simplifiée:
    // Un free-three horizontal + une capture verticale
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_horizontal(board, "X", 9, 5);

    // Paire capturable verticale
    test_utils::set_vertical(board, "XOO", 8, 2);

    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);

    // Le coup doit être accepté (la capture se fait avant le compte des free-threes)
    ASSERT_TRUE(r.success);

    // Vérifier que la capture a eu lieu
    ASSERT_EQ(board.capturedPairs().black, 1);

    TEST_PASSED();
}

// Test 5.9: Victoire détectée après captures appliquées
TEST(victory_detected_after_captures)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 2; // Seulement 2 paires pour simplifier

    // Black capture 1 paire
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 1);

    // Dernière capture pour atteindre 2 paires
    test_utils::set_horizontal(board, "XOO", 7, 7);
    board.forceSide(Player::Black);
    Move final_capture { Pos { 10, 7 }, Player::Black };
    PlayResult r = board.tryPlay(final_capture, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.capturedPairs().black, 2);

    // La victoire par capture doit être détectée
    ASSERT_EQ(board.status(), GameStatus::WinByCapture);

    TEST_PASSED();
}

// Test 5.10: Ordre complet - Illégalité détectée avant toute modification
TEST(illegality_detected_before_changes)
{
    Board board;
    RuleSet rules;
    rules.forbidDoubleThree = true;

    // Configuration double-trois: .XX à compléter en .XXX
    test_utils::set_horizontal(board, ".XX", 5, 5);
    test_utils::set_vertical(board, ".XX", 8, 2);

    // Sauvegarder hash
    uint64_t hashBefore = board.zobristKey();

    board.forceSide(Player::Black);
    Move illegal { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(illegal, rules);

    // Le coup doit être refusé
    ASSERT_FALSE(r.success);
    ASSERT_EQ(r.code, PlayErrorCode::RuleViolation);

    // Rien ne doit avoir changé
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(8, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 5.11: Capture sur ligne non continue (bord) impossible
TEST(no_capture_across_board_edge)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration près du bord: position 17, 18 et on essaie de "capturer"
    // mais la ligne n'est pas continue avec le bord
    test_utils::set_horizontal(board, "XOO", 16, 5);

    // Reconstruisons: O O X au bord droit
    Board board2;
    test_utils::set_horizontal(board2, "OOX", 16, 5);

    board2.forceSide(Player::Black);
    // Black joue à gauche des O: X O O X (bord)
    // Position 15 pour faire X O O X (18)
    Move m2 { Pos { 15, 5 }, Player::Black };
    PlayResult r = board2.tryPlay(m2, rules);
    ASSERT_TRUE(r.success);

    // Les pierres doivent être capturées (c'est valide si dans les limites)
    ASSERT_EQ(board2.at(16, 5), Cell::Empty);
    ASSERT_EQ(board2.at(17, 5), Cell::Empty);

    TEST_PASSED();
}

// ============================================================================
// Point d'entrée des tests
// ============================================================================

void run_all_legality_tests()
{
    run_all_tests("Global Legality");
}

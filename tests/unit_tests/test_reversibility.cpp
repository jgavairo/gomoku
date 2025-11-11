// Tests unitaires pour la réversibilité et cohérence du système
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_reversibility_tests();

// ============================================================================
// Tests 7) Réversibilité et cohérence
// ============================================================================

// Test 7.1: Move simple suivi de undo restaure complètement l'état
TEST(simple_move_undo_restores_all)
{
    Board board;
    RuleSet rules;

    // État initial
    uint64_t hashBefore = board.zobristKey();
    Player toPlayBefore = board.toPlay();

    // Effectuer un coup
    Move m { Pos { 9, 9 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // L'état a changé
    ASSERT_NE(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(9, 9), Cell::Black);
    ASSERT_EQ(board.toPlay(), Player::White);

    // Undo
    board.undo();

    // Vérifier restauration complète
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(9, 9), Cell::Empty);
    ASSERT_EQ(board.toPlay(), toPlayBefore);

    TEST_PASSED();
}

// Test 7.2: Move avec capture suivi de undo restaure les pierres capturées
TEST(move_with_capture_undo_restores_stones)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration pour une capture
    test_utils::set_horizontal(board, "XOO", 5, 5);

    uint64_t hashBefore = board.zobristKey();
    CaptureCount capsBefore = board.capturedPairs();

    // Faire une capture
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // Vérifier que la capture a eu lieu
    ASSERT_EQ(board.at(6, 5), Cell::Empty);
    ASSERT_EQ(board.at(7, 5), Cell::Empty);
    ASSERT_EQ(board.capturedPairs().black, 1);

    // Undo
    board.undo();

    // Vérifier restauration complète
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(6, 5), Cell::White); // Pierres restaurées
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 5), Cell::Empty); // Coup annulé
    ASSERT_EQ(board.capturedPairs().black, capsBefore.black);

    TEST_PASSED();
}

// Test 7.3: Move avec multi-captures suivi de undo
TEST(move_with_multi_capture_undo)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration pour double capture
    test_utils::set_horizontal(board, "XOO", 5, 5);
    test_utils::set_vertical(board, "XOO", 8, 2);

    uint64_t hashBefore = board.zobristKey();

    // Faire une double capture
    board.forceSide(Player::Black);
    Move m { Pos { 8, 5 }, Player::Black };
    PlayResult r = board.tryPlay(m, rules);
    ASSERT_TRUE(r.success);

    // 2 captures dans 2 directions
    ASSERT_EQ(board.capturedPairs().black, 2);

    // Undo
    board.undo();

    // Vérifier restauration des 4 pierres
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.at(6, 5), Cell::White); // Horizontal
    ASSERT_EQ(board.at(7, 5), Cell::White);
    ASSERT_EQ(board.at(8, 3), Cell::White); // Vertical
    ASSERT_EQ(board.at(8, 4), Cell::White);
    ASSERT_EQ(board.capturedPairs().black, 0);

    TEST_PASSED();
}

// Test 7.4: Séquence de moves suivie de undos restaure l'état initial
TEST(move_sequence_undo_sequence)
{
    Board board;
    RuleSet rules;

    uint64_t initialHash = board.zobristKey();

    // Séquence de 5 coups
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    board.tryPlay(Move { Pos { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { Pos { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { Pos { 9, 11 }, Player::Black }, rules);

    ASSERT_NE(board.zobristKey(), initialHash);

    // Annuler tous les coups
    board.undo();
    board.undo();
    board.undo();
    board.undo();
    board.undo();

    // État initial restauré
    ASSERT_EQ(board.zobristKey(), initialHash);
    ASSERT_EQ(board.at(9, 9), Cell::Empty);
    ASSERT_EQ(board.at(10, 9), Cell::Empty);
    ASSERT_EQ(board.at(9, 10), Cell::Empty);
    ASSERT_EQ(board.at(10, 10), Cell::Empty);
    ASSERT_EQ(board.at(9, 11), Cell::Empty);
    ASSERT_EQ(board.toPlay(), Player::Black);

    TEST_PASSED();
}

// Test 7.5: Hash change sur pose simple
TEST(hash_changes_on_placement)
{
    Board board;
    RuleSet rules;

    uint64_t hash1 = board.zobristKey();

    // Placer une pierre
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash2 = board.zobristKey();

    ASSERT_NE(hash1, hash2);

    // Placer une autre pierre
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    uint64_t hash3 = board.zobristKey();

    ASSERT_NE(hash2, hash3);
    ASSERT_NE(hash1, hash3);

    TEST_PASSED();
}

// Test 7.6: Hash change sur capture
TEST(hash_changes_on_capture)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;

    // Configuration pour capture
    test_utils::set_horizontal(board, "XOO", 5, 5);

    uint64_t hashBefore = board.zobristKey();

    // Faire une capture
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 8, 5 }, Player::Black }, rules);

    uint64_t hashAfter = board.zobristKey();

    // Le hash doit avoir changé (pose + 2 captures)
    ASSERT_NE(hashBefore, hashAfter);

    TEST_PASSED();
}

// Test 7.7: Hash change sur changement de trait
TEST(hash_changes_on_turn_change)
{
    Board board;
    RuleSet rules;

    uint64_t hash1 = board.zobristKey();
    ASSERT_EQ(board.toPlay(), Player::Black);

    // Jouer un coup (change le trait)
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash2 = board.zobristKey();

    ASSERT_NE(hash1, hash2);
    ASSERT_EQ(board.toPlay(), Player::White);

    TEST_PASSED();
}

// Test 7.8: Undo après victoire restaure le statut Ongoing
TEST(undo_after_victory_restores_status)
{
    Board board;
    RuleSet rules;

    // Configuration pour victoire immédiate
    test_utils::set_horizontal(board, "XXXX", 5, 5);

    board.forceSide(Player::Black);
    Move winning { Pos { 9, 5 }, Player::Black };
    PlayResult r = board.tryPlay(winning, rules);

    ASSERT_TRUE(r.success);
    ASSERT_EQ(board.status(), GameStatus::WinByAlign);

    // Undo
    board.undo();

    // Le statut doit revenir à Ongoing
    ASSERT_EQ(board.status(), GameStatus::Ongoing);
    ASSERT_EQ(board.at(9, 5), Cell::Empty);

    TEST_PASSED();
}

// Test 7.9: Undo après victoire par capture restaure le compteur
TEST(undo_after_capture_victory_restores_counter)
{
    Board board;
    RuleSet rules;
    rules.capturesEnabled = true;
    rules.captureWinPairs = 2;

    // Première capture
    test_utils::set_horizontal(board, "XOO", 2, 2);
    board.forceSide(Player::Black);
    board.tryPlay(Move { Pos { 5, 2 }, Player::Black }, rules);

    ASSERT_EQ(board.capturedPairs().black, 1);

    // Dernière capture (victoire)
    test_utils::set_horizontal(board, "XOO", 7, 7);
    board.forceSide(Player::Black);
    Move winning { Pos { 10, 7 }, Player::Black };
    board.tryPlay(winning, rules);

    ASSERT_EQ(board.status(), GameStatus::WinByCapture);
    ASSERT_EQ(board.capturedPairs().black, 2);

    // Undo
    board.undo();

    // Le compteur doit revenir à 1
    ASSERT_EQ(board.capturedPairs().black, 1);
    ASSERT_EQ(board.status(), GameStatus::Ongoing);

    TEST_PASSED();
}

// Test 7.10: Multiple undo/redo cycles maintiennent la cohérence
TEST(multiple_undo_redo_consistency)
{
    Board board;
    RuleSet rules;

    uint64_t hash0 = board.zobristKey();

    // Coup 1
    board.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash1 = board.zobristKey();

    // Coup 2
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    uint64_t hash2 = board.zobristKey();

    // Undo -> hash1
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash1);

    // Rejouer coup 2 -> hash2
    board.tryPlay(Move { Pos { 10, 9 }, Player::White }, rules);
    ASSERT_EQ(board.zobristKey(), hash2);

    // Undo tout -> hash0
    board.undo();
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash0);

    TEST_PASSED();
}

// Test 7.11: Hash différent pour positions symétriques
TEST(hash_different_for_symmetric_positions)
{
    Board board1;
    Board board2;
    RuleSet rules;

    // Position 1: X en (9,9)
    board1.tryPlay(Move { Pos { 9, 9 }, Player::Black }, rules);
    uint64_t hash1 = board1.zobristKey();

    // Position 2: X en (10,10) (symétrique)
    board2.tryPlay(Move { Pos { 10, 10 }, Player::Black }, rules);
    uint64_t hash2 = board2.zobristKey();

    // Les hash doivent être différents
    ASSERT_NE(hash1, hash2);

    TEST_PASSED();
}

// Test 7.12: Undo sur board vide ne fait rien
TEST(undo_on_empty_board_safe)
{
    Board board;

    uint64_t hashBefore = board.zobristKey();

    // Undo sur board vide (ne doit pas crasher)
    board.undo();

    // L'état ne doit pas avoir changé
    ASSERT_EQ(board.zobristKey(), hashBefore);
    ASSERT_EQ(board.toPlay(), Player::Black);

    TEST_PASSED();
}

// ============================================================================
// Point d'entrée des tests
// ============================================================================

void run_all_reversibility_tests()
{
    run_all_tests("Reversibility and Consistency");
}

// Test unitaires pour les bases du plateau : taille, indexation, occupations
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp"
#include "gomoku/core/Zobrist.hpp"
#include "test_framework.hpp"
#include <iostream>

using namespace gomoku;
using namespace test_framework;

// Forward declaration
void run_all_board_basic_tests();

// ============================================================================
// Tests 1) Plateau et placements
// ============================================================================

// Test 1.1: Vérifier que le plateau est bien 19×19
TEST(board_size_19x19)
{
    Board board;

    // BOARD_SIZE doit être 19
    ASSERT_EQ(BOARD_SIZE, 19);

    // Vérifier que toutes les positions valides sont bien dans les limites
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
        for (uint8_t y = 0; y < BOARD_SIZE; y++) {
            ASSERT_TRUE(board.isInside(x, y));
            ASSERT_TRUE(board.isEmpty(x, y));
        }
    }

    TEST_PASSED();
}

// Test 1.2: Vérifier l'indexation 0..360 correcte
TEST(board_indexation_0_to_360)
{
    Board board;

    // Pour un plateau 19x19, il y a 361 cases (0 à 360)
    const int total_cells = BOARD_SIZE * BOARD_SIZE;
    ASSERT_EQ(total_cells, 361);

    // Vérifier que chaque position (x,y) correspond à un index unique
    for (uint8_t y = 0; y < BOARD_SIZE; y++) {
        for (uint8_t x = 0; x < BOARD_SIZE; x++) {
            Pos p { x, y };
            uint16_t idx = p.toIndex();

            // L'index doit être entre 0 et 360
            ASSERT_TRUE(idx >= 0 && idx < 361);

            // La conversion inverse doit donner la même position
            Pos recovered = Pos::fromIndex(idx);
            ASSERT_EQ(recovered.x, x);
            ASSERT_EQ(recovered.y, y);
        }
    }

    // Vérifier les cas limites
    Pos topLeft { 0, 0 };
    ASSERT_EQ(topLeft.toIndex(), 0);

    Pos bottomRight { 18, 18 };
    ASSERT_EQ(bottomRight.toIndex(), 360);

    Pos middle { 9, 9 };
    ASSERT_EQ(middle.toIndex(), 9 * 19 + 9);

    TEST_PASSED();
}

// Test 1.3: Case occupée → coup illégal
TEST(occupied_cell_illegal)
{
    Board board;
    RuleSet rules;

    // Placer une pierre noire en (5, 5)
    Move m1 { Pos { 5, 5 }, Player::Black };
    PlayResult r1 = board.tryPlay(m1, rules);
    ASSERT_TRUE(r1.success);
    ASSERT_FALSE(board.isEmpty(5, 5));
    ASSERT_EQ(board.at(5, 5), Cell::Black);

    // Tenter de placer une pierre blanche au même endroit
    Move m2 { Pos { 5, 5 }, Player::White };
    PlayResult r2 = board.tryPlay(m2, rules);
    ASSERT_FALSE(r2.success);
    ASSERT_EQ(r2.code, PlayErrorCode::Occupied);

    // La case doit toujours contenir la pierre noire
    ASSERT_EQ(board.at(5, 5), Cell::Black);

    // Réinitialiser le plateau
    board.reset();

    // Placer une pierre noire en (10, 10)
    Move m3 { Pos { 10, 10 }, Player::Black };
    board.tryPlay(m3, rules);

    // Tenter de placer une pierre blanche au même endroit (tour de White)
    Move m4 { Pos { 10, 10 }, Player::White };
    PlayResult r4 = board.tryPlay(m4, rules);
    ASSERT_FALSE(r4.success);
    ASSERT_EQ(r4.code, PlayErrorCode::Occupied);

    TEST_PASSED();
}

// Test 1.4: Hors-plateau → coup illégal
TEST(out_of_bounds_illegal)
{
    Board board;
    RuleSet rules;

    // Tester les positions hors limites
    ASSERT_FALSE(board.isInside(19, 0));
    ASSERT_FALSE(board.isInside(0, 19));
    ASSERT_FALSE(board.isInside(19, 19));
    ASSERT_FALSE(board.isInside(255, 0));
    ASSERT_FALSE(board.isInside(0, 255));

    // Les positions valides sont [0..18]
    ASSERT_TRUE(board.isInside(0, 0));
    ASSERT_TRUE(board.isInside(18, 18));
    ASSERT_TRUE(board.isInside(9, 9));

    // Vérifier que at() retourne Empty pour hors limites
    ASSERT_EQ(board.at(19, 0), Cell::Empty);
    ASSERT_EQ(board.at(0, 19), Cell::Empty);
    ASSERT_EQ(board.at(20, 20), Cell::Empty);

    // Tenter de jouer hors plateau (la position n'est pas valide)
    Move m_out { Pos { 19, 5 }, Player::Black };
    ASSERT_FALSE(m_out.isValid());

    Move m_out2 { Pos { 5, 19 }, Player::Black };
    ASSERT_FALSE(m_out2.isValid());

    // Si on essaie quand même de jouer, isEmpty devrait retourner false
    ASSERT_FALSE(board.isEmpty(19, 5));
    ASSERT_FALSE(board.isEmpty(5, 19));

    TEST_PASSED();
}

// Test 1.5: Pose et annulation réversibles bit-à-bit
TEST(placement_and_undo_reversible)
{
    Board board;
    RuleSet rules;

    // Sauvegarder l'état initial
    uint64_t initial_hash = board.zobristKey();
    ASSERT_EQ(board.stoneCount(Player::Black), 0);
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Placer une pierre noire en (5, 5)
    Move m1 { Pos { 5, 5 }, Player::Black };
    PlayResult r1 = board.tryPlay(m1, rules);
    ASSERT_TRUE(r1.success);

    uint64_t hash_after_m1 = board.zobristKey();

    ASSERT_NE(hash_after_m1, initial_hash);
    ASSERT_EQ(board.at(5, 5), Cell::Black);
    ASSERT_EQ(board.stoneCount(Player::Black), 1);

    // Annuler le coup
    bool undone1 = board.undo();
    ASSERT_TRUE(undone1);

    // Vérifier que l'état est revenu à l'initial
    ASSERT_EQ(board.zobristKey(), initial_hash);
    ASSERT_TRUE(board.isEmpty(5, 5));
    ASSERT_EQ(board.stoneCount(Player::Black), 0);

    // Séquence de plusieurs coups
    Move m2 { Pos { 3, 3 }, Player::Black };
    Move m3 { Pos { 4, 4 }, Player::White };
    Move m4 { Pos { 5, 5 }, Player::Black };

    board.tryPlay(m2, rules);
    uint64_t hash_after_m2 = board.zobristKey();

    board.tryPlay(m3, rules);
    uint64_t hash_after_m3 = board.zobristKey();

    board.tryPlay(m4, rules);
    // hash_after_m4 n'est pas utilisé dans ce test

    ASSERT_EQ(board.stoneCount(Player::Black), 2);
    ASSERT_EQ(board.stoneCount(Player::White), 1);

    // Annuler m4
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash_after_m3);
    ASSERT_TRUE(board.isEmpty(5, 5));
    ASSERT_EQ(board.stoneCount(Player::Black), 1);

    // Annuler m3
    board.undo();
    ASSERT_EQ(board.zobristKey(), hash_after_m2);
    ASSERT_TRUE(board.isEmpty(4, 4));
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Annuler m2
    board.undo();
    ASSERT_EQ(board.zobristKey(), initial_hash);
    ASSERT_TRUE(board.isEmpty(3, 3));
    ASSERT_EQ(board.stoneCount(Player::Black), 0);

    TEST_PASSED();
}

// Test 1.6: Réversibilité bit-à-bit avec hash Zobrist
TEST(zobrist_hash_reversibility)
{
    Board board;
    RuleSet rules;

    uint64_t h0 = board.zobristKey();

    // Séquence de coups
    std::vector<Move> moves = {
        { Pos { 9, 9 }, Player::Black },
        { Pos { 9, 10 }, Player::White },
        { Pos { 10, 9 }, Player::Black },
        { Pos { 10, 10 }, Player::White },
        { Pos { 8, 9 }, Player::Black }
    };

    std::vector<uint64_t> hashes;
    hashes.push_back(h0);

    // Jouer tous les coups et enregistrer les hash
    for (const auto& m : moves) {
        PlayResult r = board.tryPlay(m, rules);
        ASSERT_TRUE(r.success);
        hashes.push_back(board.zobristKey());
    }

    // Vérifier que tous les hash sont différents
    for (size_t i = 0; i < hashes.size(); i++) {
        for (size_t j = i + 1; j < hashes.size(); j++) {
            ASSERT_NE(hashes[i], hashes[j]);
        }
    }

    // Annuler tous les coups et vérifier la réversibilité des hash
    for (int i = static_cast<int>(moves.size()) - 1; i >= 0; i--) {
        bool undone = board.undo();
        ASSERT_TRUE(undone);
        ASSERT_EQ(board.zobristKey(), hashes[i]);
    }

    // Le hash final doit être identique au hash initial
    ASSERT_EQ(board.zobristKey(), h0);

    TEST_PASSED();
}

// Test 1.7: Vérifier que le compteur de pierres est correct
TEST(stone_count_accuracy)
{
    Board board;
    RuleSet rules;

    ASSERT_EQ(board.stoneCount(Player::Black), 0);
    ASSERT_EQ(board.stoneCount(Player::White), 0);

    // Utiliser setStone pour placer directement des pierres sans contrainte de tour
    // Placer 10 pierres noires et 9 pierres blanches
    for (int i = 0; i < 10; i++) {
        board.setStone(Pos { static_cast<uint8_t>(i), 0 }, Cell::Black);
    }
    for (int i = 0; i < 9; i++) {
        board.setStone(Pos { static_cast<uint8_t>(i), 1 }, Cell::White);
    }

    ASSERT_EQ(board.stoneCount(Player::Black), 10);
    ASSERT_EQ(board.stoneCount(Player::White), 9);

    // Maintenant testons avec tryPlay pour vérifier que undo fonctionne
    board.reset();

    // Placer quelques pierres en alternance
    for (int i = 0; i < 6; i++) {
        Player p = (i % 2 == 0) ? Player::Black : Player::White;
        Move m { Pos { static_cast<uint8_t>(i), 0 }, p };
        PlayResult r = board.tryPlay(m, rules);
        ASSERT_TRUE(r.success);
    }

    ASSERT_EQ(board.stoneCount(Player::Black), 3);
    ASSERT_EQ(board.stoneCount(Player::White), 3);

    // Annuler quelques coups
    board.undo(); // White
    board.undo(); // Black

    ASSERT_EQ(board.stoneCount(Player::Black), 2);
    ASSERT_EQ(board.stoneCount(Player::White), 2);

    TEST_PASSED();
}

// Test 1.8: Vérifier l'occupation du plateau
TEST(board_occupancy_tracking)
{
    Board board;
    RuleSet rules;

    // Au début, aucune position n'est occupée
    const auto& occupied = board.occupiedPositions();
    ASSERT_EQ(occupied.size(), 0);

    // Placer quelques pierres
    std::vector<Pos> placed = {
        { 0, 0 }, { 5, 5 }, { 18, 18 }, { 9, 9 }
    };

    for (size_t i = 0; i < placed.size(); i++) {
        Move m { placed[i], (i % 2 == 0) ? Player::Black : Player::White };
        board.tryPlay(m, rules);
    }

    // Vérifier que le nombre de positions occupées est correct
    const auto& occupied2 = board.occupiedPositions();
    ASSERT_EQ(occupied2.size(), placed.size());

    // Vérifier que toutes les positions placées sont dans la liste
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
// Point d'entrée des tests
// ============================================================================

void run_all_board_basic_tests()
{
    run_all_tests("Board Basics");
}

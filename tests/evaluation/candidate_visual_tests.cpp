// Tests visuels du CandidateGenerator
#include "../framework/test_framework.hpp"
#include "../utils/BoardBuilder.hpp"
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <iomanip>
#include <iostream>
#include <set>

// Forward declaration
void run_all_candidate_visual_tests();

using namespace gomoku;

// ANSI color codes
#define GREEN "\033[32m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Helper pour afficher le board avec les candidats en couleur
static void print_board_with_candidates(const Board& board, const std::vector<Move>& candidates)
{
    // Créer un set des positions candidates pour lookup O(log n)
    std::set<std::pair<int, int>> candidatePos;
    for (const auto& move : candidates) {
        candidatePos.insert({ move.pos.x, move.pos.y });
    }

    // Header avec numéros de colonnes
    std::cout << "    ";
    for (int x = 0; x < 19; x++) {
        std::cout << std::setw(2) << x << " ";
    }
    std::cout << "\n";

    // Afficher le plateau
    for (int y = 0; y < 19; y++) {
        std::cout << std::setw(2) << y << "  ";

        for (int x = 0; x < 19; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));

            if (c == Cell::Black) {
                std::cout << "X";
            } else if (c == Cell::White) {
                std::cout << "O";
            } else if (candidatePos.count({ x, y })) {
                // Afficher les candidats en vert
                std::cout << GREEN << "+" << RESET;
            } else {
                std::cout << ".";
            }

            if (x < 18) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

// Test 1: Plateau vide (doit retourner le centre)
TEST(candidate_empty_board)
{
    std::cout << "\n"
              << CYAN << "=== Test: Plateau vide ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;

    auto candidates = CandidateGenerator::generate(board, rules, Player::Black, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Position(s): ";
    for (const auto& move : candidates) {
        std::cout << "(" << (int)move.pos.x << ", " << (int)move.pos.y << ") ";
    }
    std::cout << "\n"
              << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() == 1);
    ASSERT_TRUE(candidates[0].pos.x == 9 && candidates[0].pos.y == 9);

    TEST_PASSED();
}

// Test 2: Une seule pierre (ring autour)
TEST(candidate_single_stone)
{
    std::cout << "\n"
              << CYAN << "=== Test: Une pierre unique ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1; // Anneau de rayon 1

    // Placer une pierre au centre
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 3: Ligne horizontale
TEST(candidate_horizontal_line)
{
    std::cout << "\n"
              << CYAN << "=== Test: Ligne horizontale ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 1;

    // Créer une ligne horizontale XXX
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 6 }, Player::White }, rules);
    board.tryPlay(Move { { 11, 9 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 4: Ligne verticale
TEST(candidate_vertical_line)
{
    std::cout << "\n"
              << CYAN << "=== Test: Ligne verticale ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 1;

    // Créer une ligne verticale
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 6 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 11 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 5: Diagonale
TEST(candidate_diagonal_line)
{
    std::cout << "\n"
              << CYAN << "=== Test: Ligne diagonale ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 1;

    // Créer une diagonale
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 6 }, Player::White }, rules);
    board.tryPlay(Move { { 11, 11 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 6: Deux groupes séparés
TEST(candidate_two_groups)
{
    std::cout << "\n"
              << CYAN << "=== Test: Deux groupes séparés ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 2;
    cfg.groupGap = 3; // Distance pour grouper les îlots

    // Groupe 1 (haut gauche)
    board.tryPlay(Move { { 5, 5 }, Player::Black }, rules);
    board.tryPlay(Move { { 14, 14 }, Player::White }, rules);
    board.tryPlay(Move { { 6, 5 }, Player::Black }, rules);
    board.tryPlay(Move { { 14, 15 }, Player::White }, rules);
    board.tryPlay(Move { { 5, 6 }, Player::Black }, rules);

    // Groupe 2 (bas droite) déjà créé avec les coups White

    auto candidates = CandidateGenerator::generate(board, rules, Player::Black, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin
              << ", groupGap=" << (int)cfg.groupGap << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 7: Formation en croix
TEST(candidate_cross_pattern)
{
    std::cout << "\n"
              << CYAN << "=== Test: Formation en croix ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 2;

    // Créer une croix
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // Centre
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 8 }, Player::Black }, rules); // Haut
    board.tryPlay(Move { { 5, 6 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules); // Bas
    board.tryPlay(Move { { 5, 7 }, Player::White }, rules);
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules); // Gauche
    board.tryPlay(Move { { 5, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // Droite

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 8: Carré de pierres
TEST(candidate_square_pattern)
{
    std::cout << "\n"
              << CYAN << "=== Test: Carré de pierres ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 1;

    // Créer un carré 3x3
    board.tryPlay(Move { { 8, 8 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 8 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 6 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 8 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 7 }, Player::White }, rules);
    board.tryPlay(Move { { 8, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 9: Coin du plateau
TEST(candidate_corner_position)
{
    std::cout << "\n"
              << CYAN << "=== Test: Position dans un coin ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 2;

    // Pierres dans le coin
    board.tryPlay(Move { { 0, 0 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 1, 0 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 0, 1 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 10: Configuration avec ringR=2 (anneau plus large)
TEST(candidate_large_ring)
{
    std::cout << "\n"
              << CYAN << "=== Test: Anneau large (ringR=2) ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 2; // Anneau plus large
    cfg.margin = 2;

    // Quelques pierres au centre
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << "  (Notez l'anneau plus large autour des pierres)" << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);

    TEST_PASSED();
}

// Test 11: Limite de maxCandidates
TEST(candidate_max_limit)
{
    std::cout << "\n"
              << CYAN << "=== Test: Limite maxCandidates ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 2;
    cfg.maxCandidates = 15; // Limite basse pour tester le cap

    // Créer plusieurs groupes pour générer beaucoup de candidats
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 14, 14 }, Player::White }, rules);
    board.tryPlay(Move { { 11, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 5, 14 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 14, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 11 }, Player::Black }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::White, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  maxCandidates configuré: " << cfg.maxCandidates << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() <= cfg.maxCandidates);

    TEST_PASSED();
}

// Test 12: Position complexe de mi-partie
TEST(candidate_midgame_position)
{
    std::cout << "\n"
              << CYAN << "=== Test: Position complexe de mi-partie ===" << RESET << std::endl;

    Board board;
    RuleSet rules;
    CandidateConfig cfg;
    cfg.ringR = 1;
    cfg.margin = 1;
    cfg.maxCandidates = 42;

    // Simuler une partie en cours avec plusieurs formations
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 11, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 8 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 11 }, Player::White }, rules);
    board.tryPlay(Move { { 7, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 12, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 7 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 12 }, Player::White }, rules);

    auto candidates = CandidateGenerator::generate(board, rules, Player::Black, cfg);

    std::cout << "\n  Candidats générés: " << YELLOW << candidates.size() << RESET << std::endl;
    std::cout << "  Config: ringR=" << (int)cfg.ringR << ", margin=" << (int)cfg.margin
              << ", maxCandidates=" << cfg.maxCandidates << std::endl;
    std::cout << std::endl;

    print_board_with_candidates(board, candidates);

    ASSERT_TRUE(candidates.size() > 0);
    ASSERT_TRUE(candidates.size() <= cfg.maxCandidates);

    TEST_PASSED();
}

// ============================================================================
// Test entry point
// ============================================================================

void run_all_candidate_visual_tests()
{
    test_framework::run_all_tests("CandidateGenerator Visual Tests");
}

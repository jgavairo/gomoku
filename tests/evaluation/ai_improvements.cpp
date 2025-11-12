// Test des améliorations de l'IA : configuration, profondeur, ordering
#include "../framework/test_framework.hpp"
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/ai/MinimaxSearchEngine.hpp"
#include "gomoku/ai/MoveOrderer.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <iomanip>
#include <iostream>

// Forward declaration
void run_all_ai_improvements_tests();

using namespace gomoku;
using namespace gomoku::ai;

// Helper pour afficher les stats de recherche
static void printSearchStats(const SearchStats& stats, const std::string& context)
{
    std::cout << "\n  [" << context << "]" << std::endl;
    std::cout << "    Depth:     " << stats.depthReached << std::endl;
    std::cout << "    Nodes:     " << stats.nodes << std::endl;
    std::cout << "    Q-nodes:   " << stats.qnodes << " ("
              << (stats.nodes > 0 ? (stats.qnodes * 100 / stats.nodes) : 0) << "%)" << std::endl;
    std::cout << "    TT hits:   " << stats.ttHits << " ("
              << (stats.nodes > 0 ? (stats.ttHits * 100 / stats.nodes) : 0) << "%)" << std::endl;
    std::cout << "    Time:      " << stats.timeMs << "ms" << std::endl;
    if (stats.timeMs > 0) {
        long long nps = (stats.nodes * 1000LL) / stats.timeMs;
        std::cout << "    NPS:       " << nps << std::endl;
    }
    std::cout << "    PV length: " << stats.principalVariation.size() << std::endl;
}

// Test 1: Vérifier la configuration par défaut améliorée
TEST(ai_default_config_improved)
{
    std::cout << "\n=== Test: Configuration par défaut améliorée ===" << std::endl;

    SearchConfig config;

    std::cout << "  timeBudgetMs:  " << config.timeBudgetMs << "ms (attendu: 500ms)" << std::endl;
    std::cout << "  maxDepthHint:  " << config.maxDepthHint << " (attendu: >= 10)" << std::endl;
    std::cout << "  Aspiration:    " << (config.useAspirationWindows ? "enabled" : "disabled") << std::endl;

    // Vérifier que les valeurs par défaut sont configurées pour l'efficacité
    ASSERT_TRUE(config.timeBudgetMs == 500); // 500ms pour être rapide
    ASSERT_TRUE(config.maxDepthHint >= 10); // Au moins profondeur 10 pour voir loin
    ASSERT_TRUE(config.useAspirationWindows); // Aspiration activée

    TEST_PASSED();
}

// Test 2: Vérifier la configuration du MoveOrderer
TEST(ai_move_orderer_config)
{
    std::cout << "\n=== Test: Configuration MoveOrderer améliorée ===" << std::endl;

    MoveOrdererConfig config;

    std::cout << "  capDeepRoot:   " << config.capDeepRoot << " (attendu: >= 35)" << std::endl;
    std::cout << "  capMid:        " << config.capMid << " (attendu: >= 25)" << std::endl;
    std::cout << "  capShallow:    " << config.capShallow << " (attendu: >= 20)" << std::endl;
    std::cout << "  capNearLeaf:   " << config.capNearLeaf << " (attendu: >= 12)" << std::endl;

    // Vérifier que les caps sont fortement augmentés
    ASSERT_TRUE(config.capDeepRoot >= 35);
    ASSERT_TRUE(config.capMid >= 25);
    ASSERT_TRUE(config.capShallow >= 20);
    ASSERT_TRUE(config.capNearLeaf >= 12);

    TEST_PASSED();
}

// Test 3: L'IA atteint-elle des profondeurs plus élevées?
TEST(ai_reaches_deeper_depth)
{
    std::cout << "\n=== Test: Profondeur atteinte avec config par défaut ===" << std::endl;

    Board board;
    RuleSet rules {};

    // Position simple avec quelques coups
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);

    MinimaxSearchEngine engine; // Config par défaut
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Profondeur avec config par défaut");

    // Avec la nouvelle config, on devrait atteindre au moins depth 4-5
    std::cout << "\n  Vérification: profondeur >= 4" << std::endl;
    ASSERT_TRUE(stats.depthReached >= 4);

    // Et explorer un nombre significatif de noeuds
    std::cout << "  Vérification: nodes >= 1000" << std::endl;
    ASSERT_TRUE(stats.nodes >= 1000);

    TEST_PASSED();
}

// Test 4: L'IA explore-t-elle plus de noeuds?
TEST(ai_explores_more_nodes)
{
    std::cout << "\n=== Test: Nombre de noeuds explorés ===" << std::endl;

    Board board;
    RuleSet rules {};

    // Position d'ouverture
    MinimaxSearchEngine engine;
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Ouverture - exploration");

    // Avec config améliorée, on devrait explorer beaucoup plus
    std::cout << "\n  Vérification: nodes >= 5000" << std::endl;
    ASSERT_TRUE(stats.nodes >= 5000);

    // Le NPS devrait être raisonnable
    if (stats.timeMs > 0) {
        long long nps = (stats.nodes * 1000LL) / stats.timeMs;
        std::cout << "  Vérification: NPS >= 10000" << std::endl;
        ASSERT_TRUE(nps >= 10000); // Au moins 10k nps
    }

    TEST_PASSED();
}

// Test 5: L'IA trouve-t-elle un gain immédiat?
TEST(ai_finds_immediate_win)
{
    std::cout << "\n=== Test: Détection de gain immédiat ===" << std::endl;

    Board board;
    RuleSet rules {};

    // Créer une situation de gain en 1 coup pour Blanc (toPlay)
    // . X X X X . (Blanc doit bloquer ou Noir gagne au prochain coup)
    // On fait jouer Noir pour créer la menace, puis c'est au tour de Blanc
    board.tryPlay(Move { { 7, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 7, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    // C'est maintenant au tour de Blanc - il doit bloquer

    std::cout << "\n  Position (Blanc doit bloquer la menace de Noir):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Gain immédiat");

    // Blanc devrait bloquer en (11,9) ou (6,9)
    std::cout << "\n  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (11, 9) ou (6, 9) [bloquer la menace]" << std::endl;
    ASSERT_TRUE((move->pos.x == 11 || move->pos.x == 6) && move->pos.y == 9);

    TEST_PASSED();
}

// Test 6: Comparaison rapide vs lente
TEST(ai_quick_vs_slow_search)
{
    std::cout << "\n=== Test: Comparaison recherche rapide vs lente ===" << std::endl;

    Board board;
    RuleSet rules {};

    // Position de milieu de jeu
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 10 }, Player::White }, rules);

    std::cout << "\n  Position:" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;

    // Recherche rapide (500ms)
    SearchStats statsQuick;
    auto moveQuick = engine.suggestMove(board, rules, 500, &statsQuick);
    ASSERT_TRUE(moveQuick.has_value());
    printSearchStats(statsQuick, "Recherche rapide (500ms)");

    // Clear TT pour la recherche lente
    engine.clearTranspositionTable();

    // Recherche lente (3000ms)
    SearchStats statsSlow;
    auto moveSlow = engine.suggestMove(board, rules, 3000, &statsSlow);
    ASSERT_TRUE(moveSlow.has_value());
    printSearchStats(statsSlow, "Recherche lente (3000ms)");

    // La recherche lente devrait aller beaucoup plus profond
    std::cout << "\n  Vérification: recherche lente plus profonde et explore plus" << std::endl;
    ASSERT_TRUE(statsSlow.depthReached >= statsQuick.depthReached);
    ASSERT_TRUE(statsSlow.nodes > statsQuick.nodes);

    TEST_PASSED();
}

// Test 7: Principal Variation non vide
TEST(ai_principal_variation_populated)
{
    std::cout << "\n=== Test: Principal Variation remplie ===" << std::endl;

    Board board;
    RuleSet rules {};

    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);

    MinimaxSearchEngine engine;
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "PV Check");

    // La PV devrait contenir au moins quelques coups
    std::cout << "\n  Vérification: PV contient >= 3 coups" << std::endl;
    ASSERT_TRUE(stats.principalVariation.size() >= 3);

    // Afficher la PV
    std::cout << "  PV: ";
    for (size_t i = 0; i < std::min((size_t)5, stats.principalVariation.size()); ++i) {
        const auto& m = stats.principalVariation[i];
        std::cout << char('A' + m.pos.x) << (m.pos.y + 1) << " ";
    }
    if (stats.principalVariation.size() > 5) {
        std::cout << "...";
    }
    std::cout << std::endl;

    TEST_PASSED();
}

// ============================================================================
// Test entry point
// ============================================================================

void run_all_ai_improvements_tests()
{
    test_framework::run_all_tests("AI Improvements");
}

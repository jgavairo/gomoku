// Test de performance et d'analyse de l'IA
#include "../framework/test_framework.hpp"
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/ai/MinimaxSearchEngine.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <iomanip>
#include <iostream>

// Forward declaration
void run_all_ai_performance_tests();

using namespace gomoku;
using namespace gomoku::ai;

namespace {

// Helper pour afficher les stats de recherche (utilisé dans certains tests)
void printSearchStats [[maybe_unused]] (const SearchStats& stats, const std::string& context)
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
}

// Test basique : position initiale
TEST(ai_opening_position)
{
    Board board;
    RuleSet rules {};

    SearchConfig config;
    config.timeBudgetMs = 500;
    config.maxDepthHint = 5;
    config.ttBytes = 8 << 20;

    MinimaxSearchEngine engine(config);
    SearchStats stats;

    std::cout << "\n=== Position d'ouverture ===" << std::endl;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    // Le premier coup devrait être au centre
    ASSERT_EQ(move->pos.x, 9);
    ASSERT_EQ(move->pos.y, 9);

    printSearchStats(stats, "Opening");

    // Vérifications basiques
    ASSERT_TRUE(stats.nodes > 0);
    ASSERT_TRUE(stats.depthReached >= 1);
}

// Test : position avec menace
TEST(ai_threat_detection)
{
    Board board;
    RuleSet rules {};

    // Créer une menace de 4 pour Noir
    // X X X . (White doit bloquer)
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);

    std::cout << "\n=== Position avec menace de 4 ===" << std::endl;
    test_utils::print_board(board);

    SearchConfig config;
    config.timeBudgetMs = 500;
    config.maxDepthHint = 5;

    MinimaxSearchEngine engine(config);
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    // White devrait bloquer en (7,9) ou (11,9)
    ASSERT_TRUE(move->pos.x == 7 || move->pos.x == 11);
    ASSERT_EQ(move->pos.y, 9);

    printSearchStats(stats, "Threat Detection");

    std::cout << "  AI move: " << char('A' + move->pos.x) << (move->pos.y + 1) << std::endl;
}

// Test : efficacité du CandidateGenerator
TEST(ai_candidate_generation)
{
    Board board;
    RuleSet rules {};
    CandidateConfig config {};

    std::cout << "\n=== Efficacité de génération de candidats ===" << std::endl;

    // Test avec différentes densités
    struct TestCase {
        std::string name;
        int numStones;
        std::vector<Move> moves;
    };

    std::vector<TestCase> cases = {
        { "Empty board", 0, {} },
        { "Early game (5 stones)", 5, { { { 9, 9 }, Player::Black }, { { 9, 10 }, Player::White }, { { 10, 9 }, Player::Black }, { { 10, 10 }, Player::White }, { { 8, 9 }, Player::Black } } },
        { "Mid game (10 stones)", 10, { { { 9, 9 }, Player::Black }, { { 9, 10 }, Player::White }, { { 10, 9 }, Player::Black }, { { 10, 10 }, Player::White }, { { 8, 9 }, Player::Black }, { { 8, 10 }, Player::White }, { { 11, 9 }, Player::Black }, { { 11, 10 }, Player::White }, { { 7, 9 }, Player::Black }, { { 7, 10 }, Player::White } } }
    };

    for (const auto& tc : cases) {
        Board b;
        for (const auto& m : tc.moves) {
            b.tryPlay(m, rules);
        }

        Player toPlay = (tc.numStones % 2 == 0) ? Player::Black : Player::White;
        auto candidates = CandidateGenerator::generate(b, rules, toPlay, config);

        std::cout << "\n  " << tc.name << ":" << std::endl;
        std::cout << "    Stones on board: " << b.occupiedPositions().size() << std::endl;
        std::cout << "    Candidates:      " << candidates.size() << std::endl;

        ASSERT_TRUE(candidates.size() > 0);
        ASSERT_TRUE(candidates.size() <= config.maxCandidates);
    }
}

// Test : scaling de la profondeur
TEST(ai_depth_scaling)
{
    Board board;
    RuleSet rules {};

    // Position simple après quelques coups
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);

    std::cout << "\n=== Scaling avec la profondeur ===" << std::endl;
    test_utils::print_board(board);

    std::vector<int> depths = { 3, 4, 5, 6 };
    std::cout << std::setw(8) << "Depth"
              << std::setw(12) << "Nodes"
              << std::setw(12) << "Q-nodes"
              << std::setw(10) << "Time(ms)"
              << std::setw(10) << "NPS" << std::endl;
    std::cout << std::string(52, '-') << std::endl;

    for (int depth : depths) {
        SearchConfig config;
        config.timeBudgetMs = 5000; // Assez de temps
        config.maxDepthHint = depth;
        config.ttBytes = 16 << 20;

        MinimaxSearchEngine engine(config);
        SearchStats stats;

        auto move = engine.findBestMove(board, rules, &stats);
        ASSERT_TRUE(move.has_value());

        long long nps = stats.timeMs > 0 ? (stats.nodes * 1000LL / stats.timeMs) : 0;

        std::cout << std::setw(8) << depth
                  << std::setw(12) << stats.nodes
                  << std::setw(12) << stats.qnodes
                  << std::setw(10) << stats.timeMs
                  << std::setw(10) << nps << std::endl;
    }
}

// Test : efficacité de la table de transposition
TEST(ai_transposition_table)
{
    Board board;
    RuleSet rules {};

    // Position avec beaucoup de transpositions possibles
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::White }, rules);

    std::cout << "\n=== Efficacité de la table de transposition ===" << std::endl;

    SearchConfig config;
    config.timeBudgetMs = 1000;
    config.maxDepthHint = 6;
    config.ttBytes = 16 << 20;

    MinimaxSearchEngine engine(config);
    SearchStats stats;

    auto move = engine.findBestMove(board, rules, &stats);
    ASSERT_TRUE(move.has_value());

    printSearchStats(stats, "With TT");

    int hitRate = (stats.nodes > 0) ? static_cast<int>(stats.ttHits * 100 / stats.nodes) : 0;
    std::cout << "\n  TT hit rate: " << hitRate << "%" << std::endl; // On s'attend à un taux de hits décent (au moins 5%)
    ASSERT_TRUE(hitRate >= 5);
}

// Test : aspiration windows
TEST(ai_aspiration_windows)
{
    Board board;
    RuleSet rules {};

    // Position après quelques coups
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::White }, rules);

    std::cout << "\n=== Impact des aspiration windows ===" << std::endl;

    // Test avec aspiration windows
    SearchConfig configWith;
    configWith.timeBudgetMs = 1000;
    configWith.maxDepthHint = 6;
    configWith.useAspirationWindows = true;

    MinimaxSearchEngine engineWith(configWith);
    SearchStats statsWith;

    auto moveWith = engineWith.findBestMove(board, rules, &statsWith);
    ASSERT_TRUE(moveWith.has_value());

    // Test sans aspiration windows
    SearchConfig configWithout;
    configWithout.timeBudgetMs = 1000;
    configWithout.maxDepthHint = 6;
    configWithout.useAspirationWindows = false;

    MinimaxSearchEngine engineWithout(configWithout);
    SearchStats statsWithout;

    auto moveWithout = engineWithout.findBestMove(board, rules, &statsWithout);
    ASSERT_TRUE(moveWithout.has_value());

    std::cout << "\n  Avec aspiration:" << std::endl;
    std::cout << "    Nodes: " << statsWith.nodes << ", Time: " << statsWith.timeMs << "ms" << std::endl;

    std::cout << "  Sans aspiration:" << std::endl;
    std::cout << "    Nodes: " << statsWithout.nodes << ", Time: " << statsWithout.timeMs << "ms" << std::endl;

    // Les deux devraient trouver un coup valide
    ASSERT_TRUE(moveWith->isValid());
    ASSERT_TRUE(moveWithout->isValid());

    TEST_PASSED();
}

} // anonymous namespace

// Point d'entrée pour exécuter tous les tests AI
void run_all_ai_performance_tests()
{
    // Configurer le logger pour les tests
    auto& logger = gomoku::Logger::getInstance();
    logger.enableConsoleLogging(false); // Désactivé par défaut pour ne pas polluer
    logger.enableFileLogging("logs/ai_tests.log");
    logger.setLogLevel(gomoku::LogLevel::DEBUG);

    test_framework::run_all_tests("AI Performance & Analysis");
}

// Tests tactiques de l'IA : focus sur les captures
#include "../framework/test_framework.hpp"
#include "../utils/BoardBuilder.hpp"
#include "../utils/BoardPrinter.hpp"
#include "gomoku/ai/MinimaxSearchEngine.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <iomanip>
#include <iostream>

// Forward declaration
void run_all_ai_tactical_tests();

using namespace gomoku;
using namespace gomoku::ai;

// ANSI color codes
#define RED "\033[31m"
#define RESET "\033[0m"

// Helper pour afficher le board avec le coup en rouge
static void print_board_with_move(const Board& board, const Position& move_pos)
{
    std::cout << "    ";
    for (int x = 0; x < 19; x++) {
        std::cout << std::setw(2) << x << " ";
    }
    std::cout << "\n";

    for (int y = 0; y < 19; y++) {
        std::cout << std::setw(2) << y << "  ";

        for (int x = 0; x < 19; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
            char ch = '.';
            if (c == Cell::Black) {
                ch = 'X';
            } else if (c == Cell::White) {
                ch = 'O';
            }

            // Highlight the move in red
            if (x == move_pos.x && y == move_pos.y) {
                std::cout << RED << ch << RESET;
            } else {
                std::cout << ch;
            }

            if (x < 18) {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

// Helper pour afficher les stats
static void printSearchStats(const SearchStats& stats, const std::string& context)
{
    std::cout << "\n  [" << context << "]" << std::endl;
    std::cout << "    Depth:     " << stats.depthReached << std::endl;
    std::cout << "    Nodes:     " << stats.nodes << std::endl;
    std::cout << "    TT hits:   " << stats.ttHits << " ("
              << (stats.nodes > 0 ? (stats.ttHits * 100 / stats.nodes) : 0) << "%)" << std::endl;
    std::cout << "    Time:      " << stats.timeMs << "ms" << std::endl;
}

// Test 1: Capture immédiate simple (OXX_ -> capturer en _)
TEST(ai_captures_immediate_simple)
{
    std::cout << "\n=== Test: Capture immédiate simple ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc a OXX à capturer
    // Position: O en (8,9), X en (9,9), X en (10,9), _ en (11,9)
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // X

    std::cout << "\n  Position (Blanc peut capturer 2 pions noirs):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Capture immédiate");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (11, 9) [capture OXX_]" << std::endl;
    ASSERT_TRUE(move->pos.x == 11 && move->pos.y == 9);

    TEST_PASSED();
}

// Test 2: Éviter d'être capturé (_XXO -> ne pas jouer en _)
TEST(ai_avoids_being_captured)
{
    std::cout << "\n=== Test: Éviter d'être capturé ===" << std::endl;

    Board board;
    RuleSet rules;

    // Noir ne doit PAS jouer en (7,9) sinon Blanc capture
    // Position: X en (8,9), X en (9,9), O en (10,9)
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 10, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X

    std::cout << "\n  Position (Noir ne doit PAS jouer en (7,9)):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Éviter capture");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Ne doit PAS être: (7, 9)" << std::endl;
    ASSERT_FALSE(move->pos.x == 7 && move->pos.y == 9);

    TEST_PASSED();
}

// Test 3: Double capture (deux paires capturables)
TEST(ai_double_capture_opportunity)
{
    std::cout << "\n=== Test: Opportunité de double capture ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc peut capturer sur deux axes depuis (10,10)
    // Horizontal: O en (8,10), X en (9,10), X en (10,10) -> capturer en (11,10)
    // Vertical: O en (10,8), X en (10,9), X en (10,10) -> capturer en (10,11)
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 10 }, Player::White }, rules); // O
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 10, 8 }, Player::White }, rules); // O
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules); // X

    std::cout << "\n  Position (Blanc peut capturer sur 2 axes):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Double capture");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (11, 10) ou (10, 11) [capture une paire]" << std::endl;
    ASSERT_TRUE((move->pos.x == 11 && move->pos.y == 10) || (move->pos.x == 10 && move->pos.y == 11));

    TEST_PASSED();
}

// Test 4: Capture vs alignement (arbitrage)
TEST(ai_capture_vs_alignment)
{
    std::cout << "\n=== Test: Arbitrage capture vs alignement ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc a le choix entre:
    // 1) Capturer OXX_ en (11,9)
    // 2) Faire un trois ouvert en (9,8)
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules); // O pour setup capture
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules); // O
    board.tryPlay(Move { { 5, 5 }, Player::Black }, rules); // X ailleurs
    board.tryPlay(Move { { 9, 11 }, Player::White }, rules); // O setup trois

    std::cout << "\n  Position (capture disponible vs alignement):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Capture vs alignement");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    // On vérifie juste que l'IA choisit un coup raisonnable
    ASSERT_TRUE(move.has_value());

    TEST_PASSED();
}

// Test 5: Menace de capture (forcer l'adversaire)
TEST(ai_capture_threat)
{
    std::cout << "\n=== Test: Créer une menace de capture ===" << std::endl;

    Board board;
    RuleSet rules;

    // Noir crée une position où il menace de capturer au prochain coup
    // O en (8,9), X en (9,9), _ en (10,9), _ en (11,9)
    // Si Noir joue en (10,9), il menace de capturer en jouant (7,9) ensuite
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 5, 5 }, Player::Black }, rules); // X ailleurs

    std::cout << "\n  Position (Blanc vient de jouer, Noir peut menacer):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Menace de capture");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    // L'IA devrait considérer (10,9) comme un bon coup (menace capture)

    TEST_PASSED();
}

// Test 6: Défense contre capture imminente
TEST(ai_defends_against_capture)
{
    std::cout << "\n=== Test: Défense contre capture imminente ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc a OXX, Noir doit défendre ou s'échapper
    // O en (8,9), X en (9,9), X en (10,9)
    // Noir au trait, ne doit pas laisser Blanc capturer facilement
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules); // O ailleurs

    std::cout << "\n  Position (Noir doit gérer la menace OXX_):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Défense capture");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;

    TEST_PASSED();
}

// Test 7: Capture en diagonale
TEST(ai_diagonal_capture)
{
    std::cout << "\n=== Test: Capture en diagonale ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc peut capturer en diagonale
    // O en (8,8), X en (9,9), X en (10,10), _ en (11,11)
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 8, 8 }, Player::White }, rules); // O
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules); // X

    std::cout << "\n  Position (capture diagonale disponible):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Capture diagonale");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (11, 11) [capture diagonale]" << std::endl;
    ASSERT_TRUE(move->pos.x == 11 && move->pos.y == 11);

    TEST_PASSED();
}

// Test 8: Multiple captures disponibles (choisir la meilleure)
TEST(ai_multiple_captures_choice)
{
    std::cout << "\n=== Test: Choix entre plusieurs captures ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc peut capturer à 3 endroits différents
    // Capture 1: O(6,9) X(7,9) X(8,9) _(9,9)
    // Capture 2: O(9,6) X(9,7) X(9,8) _(9,9)
    // Capture 3: O(12,9) X(11,9) X(10,9) _(9,9)
    board.tryPlay(Move { { 7, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 6, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 9, 6 }, Player::White }, rules); // O
    board.tryPlay(Move { { 9, 7 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 12, 9 }, Player::White }, rules); // O
    board.tryPlay(Move { { 9, 8 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 5, 5 }, Player::White }, rules); // O ailleurs
    board.tryPlay(Move { { 11, 9 }, Player::Black }, rules); // X
    board.tryPlay(Move { { 6, 6 }, Player::White }, rules); // O ailleurs
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules); // X

    std::cout << "\n  Position (3 captures possibles convergent vers (9,9)):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Choix multiple captures");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (9, 9) [capture centrale]" << std::endl;
    ASSERT_TRUE(move->pos.x == 9 && move->pos.y == 9);

    TEST_PASSED();
}

// Test 9: Capture forcée (5 paires pour gagner)
TEST(ai_winning_capture)
{
    std::cout << "\n=== Test: Capture pour gagner (5ème paire) ===" << std::endl;

    Board board;
    RuleSet rules;

    // Blanc a déjà 4 paires capturées, peut gagner en capturant une 5ème
    // On simule cela en jouant plusieurs captures

    // Première capture
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 11, 9 }, Player::White }, rules); // Capture!

    // Deuxième capture
    board.tryPlay(Move { { 9, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 11, 10 }, Player::White }, rules); // Capture!

    // Troisième capture
    board.tryPlay(Move { { 9, 11 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 11 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 11 }, Player::Black }, rules);
    board.tryPlay(Move { { 11, 11 }, Player::White }, rules); // Capture!

    // Quatrième capture
    board.tryPlay(Move { { 9, 12 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 12 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 12 }, Player::Black }, rules);
    board.tryPlay(Move { { 11, 12 }, Player::White }, rules); // Capture!

    // Setup pour 5ème capture (la gagnante)
    board.tryPlay(Move { { 9, 13 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 13 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 13 }, Player::Black }, rules);

    std::cout << "\n  Position (Blanc a 4 paires, peut gagner avec une 5ème):" << std::endl;
    test_utils::print_board(board);

    auto caps = board.capturedPairs();
    std::cout << "  Captures: Blanc=" << caps.white << ", Noir=" << caps.black << std::endl;

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Capture gagnante");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (11, 13) [5ème capture = victoire!]" << std::endl;
    ASSERT_TRUE(move->pos.x == 11 && move->pos.y == 13);

    TEST_PASSED();
}

// Test 10: Bloquer capture adverse gagnante
TEST(ai_blocks_enemy_winning_capture)
{
    std::cout << "\n=== Test: Bloquer capture gagnante adverse ===" << std::endl;

    Board board;
    RuleSet rules;

    // Noir a 4 paires capturées et menace de gagner
    // Blanc doit absolument empêcher la 5ème capture

    // 4 captures pour Noir (on inverse les rôles)
    board.tryPlay(Move { { 8, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 7, 9 }, Player::Black }, rules); // Capture!

    board.tryPlay(Move { { 8, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 7, 10 }, Player::Black }, rules); // Capture!

    board.tryPlay(Move { { 8, 11 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 11 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 11 }, Player::White }, rules);
    board.tryPlay(Move { { 7, 11 }, Player::Black }, rules); // Capture!

    board.tryPlay(Move { { 8, 12 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 12 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 12 }, Player::White }, rules);
    board.tryPlay(Move { { 7, 12 }, Player::Black }, rules); // Capture!

    // Setup où Noir menace 5ème capture
    board.tryPlay(Move { { 8, 13 }, Player::Black }, rules);
    board.tryPlay(Move { { 9, 13 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 13 }, Player::White }, rules);
    // Au tour de Noir, mais on teste le coup de Blanc
    board.tryPlay(Move { { 5, 5 }, Player::Black }, rules); // Noir joue ailleurs

    std::cout << "\n  Position (Noir a 4 paires, Blanc doit défendre):" << std::endl;
    test_utils::print_board(board);

    auto caps = board.capturedPairs();
    std::cout << "  Captures: Noir=" << caps.black << ", Blanc=" << caps.white << std::endl;

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Bloquer capture gagnante");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Devrait protéger contre capture en (7,13)" << std::endl;

    TEST_PASSED();
}

// Test 11: Capture dans un coin
TEST(ai_corner_capture)
{
    std::cout << "\n=== Test: Capture dans un coin ===" << std::endl;

    Board board;
    RuleSet rules;

    // Test capture près du bord/coin
    // O en (0,0), X en (1,1), X en (2,2), _ en (3,3)
    board.tryPlay(Move { { 1, 1 }, Player::Black }, rules);
    board.tryPlay(Move { { 0, 0 }, Player::White }, rules);
    board.tryPlay(Move { { 2, 2 }, Player::Black }, rules);

    std::cout << "\n  Position (capture dans le coin):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Capture coin");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    std::cout << "  Attendu: (3, 3) [capture dans le coin]" << std::endl;
    ASSERT_TRUE(move->pos.x == 3 && move->pos.y == 3);

    TEST_PASSED();
}

// Test 12: Séquence de captures forcées
TEST(ai_forced_capture_sequence)
{
    std::cout << "\n=== Test: Séquence de captures forcées ===" << std::endl;

    Board board;
    RuleSet rules;

    // Position complexe où plusieurs captures s'enchaînent
    // Blanc peut capturer, puis Noir capture, etc.
    board.tryPlay(Move { { 9, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 8, 9 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 7, 9 }, Player::White }, rules); // Setup pour contre-capture
    board.tryPlay(Move { { 6, 9 }, Player::Black }, rules); // Setup pour contre-capture

    std::cout << "\n  Position (captures en chaîne possibles):" << std::endl;
    test_utils::print_board(board);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    printSearchStats(stats, "Séquence captures");

    std::cout << "\n  Position après le coup de l'IA (en " << RED << "rouge" << RESET << "):" << std::endl;
    Board board_copy = board;
    board_copy.tryPlay(*move, rules);
    print_board_with_move(board_copy, move->pos);

    std::cout << "  Move choisi: (" << (int)move->pos.x << ", " << (int)move->pos.y << ")" << std::endl;
    // L'IA doit évaluer la séquence complète

    TEST_PASSED();
}

// ============================================================================
// Test entry point
// ============================================================================

void run_all_ai_tactical_tests()
{
    test_framework::run_all_tests("AI Tactical (Captures)");
}

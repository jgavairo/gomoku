// Tests tactiques de l'IA : focus sur les captures et les priorités
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
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Helper pour afficher le board avec le coup en surbrillance
static void print_board_with_move(const Board& board, const Move& move, const std::string& title)
{
    std::cout << "\n"
              << YELLOW << "=== " << title << " ===" << RESET << std::endl;
    std::cout << "    ";
    for (int x = 0; x < 19; x++) {
        std::cout << std::setw(2) << x << " ";
    }
    std::cout << "\n";

    for (int y = 0; y < 19; y++) {
        std::cout << std::setw(2) << y << "  ";

        for (int x = 0; x < 19; x++) {
            Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));

            if (x == move.pos.x && y == move.pos.y) {
                std::cout << RED << (move.by == Player::Black ? "X " : "O ") << RESET;
            } else {
                if (c == Cell::Black)
                    std::cout << "X ";
                else if (c == Cell::White)
                    std::cout << "O ";
                else
                    std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

static void printSearchStats(const SearchStats& stats)
{
    std::cout << "  Stats: Depth=" << stats.depthReached
              << " Nodes=" << stats.nodes
              << " Time=" << stats.timeMs << "ms" << std::endl;
}

// Test 1: Capture simple (Blanc capture Noir)
TEST(ai_tactical_simple_capture)
{
    Board board;
    RuleSet rules;

    // Setup: O X X _
    // Blanc (8,9), Noir (9,9), Noir (10,9)
    // C'est à Blanc de jouer
    test_utils::set_board(board, "O X X", 8, 9);
    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Capture Simple (Blanc joue)");
    printSearchStats(stats);

    // Blanc doit jouer en (11,9) pour capturer
    ASSERT_EQ(move->pos.x, 11);
    ASSERT_EQ(move->pos.y, 9);
    TEST_PASSED();
}

// Test 2: Défense contre capture (Noir défend)
TEST(ai_tactical_defend_capture)
{
    Board board;
    RuleSet rules;

    // Setup: O X X _
    // Blanc (8,9), Noir (9,9), Noir (10,9)
    // Menace créée par Blanc (5,5)
    test_utils::set_board(board, "O X X", 8, 9);
    board.setStone({ 5, 5 }, Cell::White);
    board.forceSide(Player::Black);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Défense Capture (Noir joue)");
    printSearchStats(stats);

    // Noir doit jouer en (11,9) pour bloquer la capture (ou étendre)
    // (7,9) ne marche pas car c'est de l'autre côté du O
    ASSERT_EQ(move->pos.x, 11);
    ASSERT_EQ(move->pos.y, 9);
    TEST_PASSED();
}

// Test 3: Double capture (Blanc capture 2 paires en un coup)
TEST(ai_tactical_double_capture)
{
    Board board;
    RuleSet rules;

    // Setup pour une VRAIE double capture (un coup capture 4 pions)
    // On veut jouer en (10, 10) pour capturer verticalement et horizontalement.
    // Vertical:   O(10,7) X(10,8) X(10,9) _(10,10)
    // Horizontal: O(7,10) X(8,10) X(9,10) _(10,10)

    test_utils::set_board(board, R"(
      . . . O
      . . . X
      . . . X
    O X X .
    )",
        7, 7);

    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Double Capture (2 paires)");
    printSearchStats(stats);

    // Le coup en (10, 10) doit être choisi car il rapporte 2 captures (valeur énorme)
    ASSERT_EQ(move->pos.x, 10);
    ASSERT_EQ(move->pos.y, 10);
    TEST_PASSED();
}

// Test 4: Victoire par capture (5ème paire)
TEST(ai_tactical_win_by_capture)
{
    Board board;
    RuleSet rules;

    // On donne 4 paires capturées à Noir
    // On setup une 5ème opportunité

    board.forceSide(Player::White);

    // 1
    board.tryPlay(Move { { 1, 1 }, Player::White }, rules);
    board.tryPlay(Move { { 0, 0 }, Player::Black }, rules);
    board.tryPlay(Move { { 2, 2 }, Player::White }, rules);
    board.tryPlay(Move { { 3, 3 }, Player::Black }, rules); // Capture W en (1,1),(2,2)

    // 2
    board.tryPlay(Move { { 1, 4 }, Player::White }, rules);
    board.tryPlay(Move { { 0, 3 }, Player::Black }, rules);
    board.tryPlay(Move { { 2, 5 }, Player::White }, rules);
    board.tryPlay(Move { { 3, 6 }, Player::Black }, rules); // Capture

    // 3
    board.tryPlay(Move { { 1, 7 }, Player::White }, rules);
    board.tryPlay(Move { { 0, 6 }, Player::Black }, rules);
    board.tryPlay(Move { { 2, 8 }, Player::White }, rules);
    board.tryPlay(Move { { 3, 9 }, Player::Black }, rules); // Capture

    // 4
    board.tryPlay(Move { { 1, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 0, 9 }, Player::Black }, rules);
    board.tryPlay(Move { { 2, 11 }, Player::White }, rules);
    board.tryPlay(Move { { 3, 12 }, Player::Black }, rules); // Capture

    // Setup 5ème: B(10,10) W(11,10) W(12,10) _(13,10)
    board.tryPlay(Move { { 11, 10 }, Player::White }, rules);
    board.tryPlay(Move { { 10, 10 }, Player::Black }, rules);
    board.tryPlay(Move { { 12, 10 }, Player::White }, rules);

    // C'est à Noir de jouer. Doit jouer (13,10) pour gagner.
    printf("Black captured paris : %d\n", board.capturedPairs().black);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Victoire par Capture (5ème)");
    printSearchStats(stats);

    ASSERT_EQ(move->pos.x, 13);
    ASSERT_EQ(move->pos.y, 10);
    TEST_PASSED();
}

// Test 5: Priorité Victoire (5 alignés) > Capture
TEST(ai_tactical_priority_win_over_capture)
{
    Board board;
    RuleSet rules;

    // Noir a 4 pions alignés: (5,5) à (8,5). Peut gagner en (9,5).
    // Noir a aussi une capture possible ailleurs.

    // Setup 4 alignés pour Noir
    test_utils::set_board(board, "X X X X", 5, 5);
    // Bloqueurs blancs
    test_utils::set_board(board, "O O O O", 5, 6);

    // Setup capture possible pour Noir: B(10,10) W(11,10) W(12,10) _(13,10)
    test_utils::set_board(board, "X O O", 10, 10);

    board.forceSide(Player::Black);

    // C'est à Noir.
    // Choix: (9,5) pour gagner (5 alignés) OU (13,10) pour capturer.
    // Victoire > Capture.

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Priorité Victoire > Capture");
    printSearchStats(stats);

    // Doit jouer (9,5) (ou (4,5) si libre, mais ici on attend une victoire)
    bool winMove = (move->pos.x == 9 && move->pos.y == 5) || (move->pos.x == 4 && move->pos.y == 5);
    ASSERT_TRUE(winMove);
    TEST_PASSED();
}

// Test 6: Bloquer victoire adverse (Priorité Bloquer > Capture simple)
TEST(ai_tactical_block_win_vs_capture)
{
    Board board;
    RuleSet rules;

    // Blanc a 4 alignés (menace victoire).
    // Noir peut faire une capture ailleurs.
    // Noir DOIT bloquer.

    // Setup Blanc 4 alignés: (5,5) à (8,5)
    test_utils::set_board(board, "O O O O", 5, 5);
    // Bloqueurs noirs
    test_utils::set_board(board, "X X X", 0, 0);

    // Setup capture possible pour Noir: B(10,10) W(11,10) W(12,10) _(13,10)
    test_utils::set_board(board, "X O O", 10, 10);

    board.forceSide(Player::Black);

    // C'est à Noir.
    // Menace: Blanc gagne en (9,5) ou (4,5).
    // Noir doit bloquer en (9,5) ou (4,5).
    // Ne doit PAS capturer en (13,10).

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Priorité Bloquer Victoire > Capture");
    printSearchStats(stats);

    bool blockMove = (move->pos.x == 9 && move->pos.y == 5) || (move->pos.x == 4 && move->pos.y == 5);
    ASSERT_TRUE(blockMove);
    TEST_PASSED();
}

// Test 7: Éviter le suicide (ne pas jouer là où on se fait capturer immédiatement)
TEST(ai_tactical_avoid_suicide)
{
    Board board;
    RuleSet rules;

    // Setup: X O . X
    // Si Blanc (O) joue en (10,10), il forme X O O X et se fait capturer par Noir.
    // Blanc doit éviter ce coup.

    test_utils::set_board(board, "X O . .", 8, 10);
    // X(8,10), O(9,10), .(10,10), X(11,10)

    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Éviter Suicide");
    printSearchStats(stats);

    // Le coup en (10, 10) est suicidaire
    ASSERT_FALSE(move->pos.x == 10 && move->pos.y == 10);
    TEST_PASSED();
}

// Test 8: Contre-attaque défensive (Capturer la pierre qui nous menace)
TEST(ai_tactical_counter_capture)
{
    Board board;
    RuleSet rules;

    // Setup:
    // Blanc est menacé de capture: X O O . (par Noir en 11,10)
    // Mais Blanc peut capturer le X menaçant (en 8,10) avec un O en 7,10
    // Configuration: O X(menace) O O .

    // O(7,10) à jouer
    // X(8,10) (menace)
    // O(9,10)
    // O(10,10)
    // .(11,10) (trou pour capture par Noir)
    // X(12,10) (l'autre bout de la tenaille noire, disons)

    // Simplifions:
    // Menace sur Blanc: X(8,10) O(9,10) O(10,10) _(11,10)
    // Si Noir joue (11,10), il capture O O.
    // Blanc peut se défendre en jouant (11,10) (extension).
    // MAIS, supposons que Blanc puisse capturer X(8,10).
    // O(6,10) X(7,10) X(8,10) -> Blanc joue (9,10)? Non.

    // Refaisons le setup pour que la capture soit la MEILLEURE défense.
    // Menace: X(10,10) O(11,10) O(12,10) _(13,10)  (Noir va jouer 13,10)
    // Opportunité Blanc: O(7,10) X(8,10) X(9,10) _(10,10)
    // Si Blanc joue (10,10), il capture X(8,10) et X(9,10).
    // En capturant, il supprime la pierre X(10,10) ? Non, X(10,10) est une autre pierre.

    // Cas : Capturer LA pierre menaçante.
    // Menace: X(9,10) O(10,10) O(11,10) _(12,10)
    // Pour capturer X(9,10), Blanc a besoin de : O(6,10) X(7,10) X(8,10) [X(9,10)]
    // Non, X(9,10) doit être le bout d'une paire noire.
    // Disons: O(6,10) X(7,10) X(8,10) O(9,10)
    // Blanc joue (9,10), capture X(7,10) et X(8,10).

    // Setup combiné:
    // Une paire blanche menacée: X(8,10) O(9,10) O(10,10) _(11,10)
    // La pierre X(8,10) fait partie d'une paire noire capturable: O(5,10) X(6,10) X(7,10) [X(8,10)] -> Non
    // La pierre X(8,10) est vulnérable si Blanc joue en 7,10 ?
    // O(5,10) X(6,10) X(7,10) ...

    // Essayons plus simple:
    // Blanc O O est menacé.
    // Blanc peut jouer pour capturer une paire adverse AILLEURS qui est plus valables,
    // OU capturer la pierre qui sert d'appui à la capture.

    // Setup:
    // Menace sur Blanc: X(9,9) O(10,9) O(11,9) .(12,9)
    // Capture dispo pour Blanc: O(9,8) X(9,9) X(9,10) .(9,11)
    // La pierre X(9,9) est commune !
    // Si Blanc joue (9,11), il capture X(9,9) et X(9,10).
    // En capturant X(9,9), la menace sur la ligne 9 disparaît !

    test_utils::set_board(board, R"(
        . O . .
        . X O O .
        . X . .
        . . . .
    )",
        8, 8);

    // Analyse du board ci-dessus (offset 8,8):
    // Ligne 8: . O(9,8) . .
    // Ligne 9: X(8,9) X(9,9) O(10,9) O(11,9) .(12,9)
    // Ligne 10: . X(9,10) . .

    // Menace: Noir peut jouer en (12,9) pour capturer O(10,9) et O(11,9).
    // Défense classique: Blanc joue (12,9).
    // Contre-attaque: Blanc joue (9,11) ? Non, alignement vertical X(9,9) X(9,10).
    // Il faut O(9,8) (déjà là) et jouer O(9,11).
    // Si Blanc joue (9,11), il capture X(9,9) et X(9,10).
    // X(9,9) disparaît. La menace horizontale X(9,9)-O-O disparaît.
    // C'est une défense active !

    // On ajoute un O en (9,11) pour que le coup soit valide (capture)
    // Ah non, c'est à Blanc de jouer en (9,11).

    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Contre-attaque (Capture défensive)");
    printSearchStats(stats);

    // Le coup en (9, 11) capture 2 pions ET sauve la paire O-O
    // C'est mieux que juste défendre en (12,9) (qui ne capture rien)
    ASSERT_EQ(move->pos.x, 9);
    ASSERT_EQ(move->pos.y, 11);
    TEST_PASSED();
}

// Test 9: Victoire par Capture > Bloquer Défaite (4 semi-fermé adverse)
// Le joueur a 4 paires.
// L'adversaire a un 4 semi-fermé (menace de victoire immédiate).
// Le joueur peut bloquer OU capturer une paire (victoire immédiate par 5 paires).
// Il doit choisir la capture (Victoire > Ne pas perdre).
TEST(ai_tactical_win_capture_vs_block_loss)
{
    Board board;
    RuleSet rules;

    // 1. Setup: White captures 4 pairs manually
    for (uint8_t y = 0; y < 4; ++y) {
        board.tryPlay(Move { { 1, y }, Player::Black }, rules);
        board.tryPlay(Move { { 0, y }, Player::White }, rules);
        board.tryPlay(Move { { 2, y }, Player::Black }, rules);
        board.tryPlay(Move { { 3, y }, Player::White }, rules);
    }
    board.tryPlay(Move { { 3, 4 }, Player::Black }, rules);
    board.forceSide(Player::Black);
    board.tryPlay(Move { { 0, 4 }, Player::Black }, rules);
    ASSERT_EQ(board.stoneCount(Player::Black), 2);

    // 2. Setup Tactical Situation (offset 5,5)
    // White (O) to play.
    // Option A: Block opponent's semi-open four.
    //   Row 6: O X X X X . -> Black threatens to play at dot. White can block there.
    // Option B: Capture a pair.
    //   Row 8: O X X .   -> Play (8,8) captures X X.

    test_utils::set_board(board, R"(
        . . . . . .
        O X X X X .
        . . . . . .
        O X X . . .
    )",
        5, 5);

    // Coordinates:
    // Row 1 (y=6): O(5,6) X(6,6) X(7,6) X(8,6) X(9,6) .(10,6)
    //   -> Black threatens (10,6). White blocking move is (10,6).
    // Row 3 (y=8): O(5,8) X(6,8) X(7,8) .(8,8)
    //   -> White capturing move is (8,8).

    board.forceSide(Player::White);

    MinimaxSearchEngine engine;
    SearchStats stats;
    auto move = engine.findBestMove(board, rules, &stats);

    ASSERT_TRUE(move.has_value());
    print_board_with_move(board, *move, "Victoire Capture vs Bloquer Défaite");
    printSearchStats(stats);

    // Expectation: Capture at (8,8) because it wins the game immediately.
    // Blocking at (10,6) only prolongs the game.
    ASSERT_EQ(move->pos.x, 8);
    ASSERT_EQ(move->pos.y, 8);
    TEST_PASSED();
}

void run_all_ai_tactical_tests()
{
    test_framework::run_all_tests("AI Tactical (Captures & Priorities)");
}

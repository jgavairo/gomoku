// Test runner principal pour les tests unitaires du Gomoku
#include "gomoku/core/Zobrist.hpp"
#include <iostream>

// Ce fichier sert de point d'entrée pour exécuter tous les tests.
// Les tests individuels sont enregistrés et exécutés après l'initialisation.

// Déclarations externes (C++ linkage)
extern void run_all_board_basic_tests();
extern void run_all_alignment_tests();
extern void run_all_capture_tests();
extern void run_all_double_three_tests();
extern void run_all_legality_tests();
extern void run_all_reversibility_tests();

int main(int argc, char** argv)
{
    // Initialiser les tables Zobrist AVANT que les tests ne s'exécutent
    gomoku::zobrist::init();

    bool verbose = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose") {
            verbose = true;
        }
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║                 Gomoku Unit Tests                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    if (verbose) {
        std::cout << "Running tests in verbose mode...\n\n";
    }

    // Exécuter tous les tests enregistrés
    run_all_board_basic_tests();
    run_all_alignment_tests();
    run_all_capture_tests();
    run_all_double_three_tests();
    run_all_legality_tests();
    run_all_reversibility_tests();

    return 0;
}

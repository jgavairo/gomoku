// Runner pour les tests d'amélioration de l'IA
#include "framework/test_framework.hpp"
#include "gomoku/core/Zobrist.hpp"
#include <iostream>

// Forward declaration
extern void run_all_ai_improvements_tests();

int main()
{
    // Initialiser Zobrist
    gomoku::zobrist::init();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  TESTS D'AMÉLIORATION DE L'IA GOMOKU" << std::endl;
    std::cout << "================================================\n"
              << std::endl;

    run_all_ai_improvements_tests();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  FIN DES TESTS" << std::endl;
    std::cout << "================================================\n"
              << std::endl;

    return (test_framework::tests_failed > 0) ? 1 : 0;
}

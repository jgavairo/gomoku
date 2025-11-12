// Runner pour les tests visuels du CandidateGenerator
#include <iostream>

// Forward declaration
void run_all_candidate_visual_tests();

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  CANDIDATEGENERATOR VISUAL TESTS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nLégende:" << std::endl;
    std::cout << "  X = Pierre noire" << std::endl;
    std::cout << "  O = Pierre blanche" << std::endl;
    std::cout << "  \033[32m+\033[0m = Candidat potentiel (en vert)" << std::endl;
    std::cout << "  . = Case vide" << std::endl;
    std::cout << "\n========================================\n"
              << std::endl;

    run_all_candidate_visual_tests();

    return 0;
}

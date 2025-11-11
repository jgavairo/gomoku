// Runner pour l'évaluation et l'analyse de l'IA
#include "gomoku/core/Zobrist.hpp"
#include "util/Logger.hpp"
#include <iostream>

// Déclaration externe de la fonction d'évaluation AI
extern void run_all_ai_performance_tests();

int main(int argc, char** argv)
{
    // Initialiser les tables Zobrist
    gomoku::zobrist::init();

    // Configuration du logger pour l'évaluation
    auto& logger = gomoku::Logger::getInstance();
    logger.enableConsoleLogging(true);
    logger.enableColoredOutput(true);
    logger.enableFileLogging("logs/ai_evaluation.log");

    // Mode verbose par défaut pour l'évaluation
    bool verbose = true;
    bool debug = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-q" || arg == "--quiet") {
            verbose = false;
        } else if (arg == "-d" || arg == "--debug") {
            debug = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "\nUsage: " << argv[0] << " [options]\n";
            std::cout << "\nOptions:\n";
            std::cout << "  -d, --debug    Enable debug logs (shows detailed AI internals)\n";
            std::cout << "  -q, --quiet    Quiet mode (less verbose output)\n";
            std::cout << "  -h, --help     Show this help message\n";
            std::cout << "\n";
            return 0;
        }
    }

    if (debug) {
        logger.setLogLevel(gomoku::LogLevel::DEBUG);
        std::cout << "Debug mode enabled - detailed AI logs will be shown\n";
    } else {
        logger.setLogLevel(gomoku::LogLevel::INFO);
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║          Gomoku AI Evaluation & Analysis          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    if (verbose) {
        std::cout << "Running AI performance tests and analysis...\n";
        if (debug) {
            std::cout << "Note: Debug logs show CandidateGenerator, MoveOrderer, and search internals\n";
        }
        std::cout << "\n";
    }

    // Exécuter les tests d'évaluation AI
    run_all_ai_performance_tests();

    std::cout << "\n";
    std::cout << "Evaluation complete. Detailed logs saved to: logs/ai_evaluation.log\n";
    std::cout << "\n";

    return 0;
}

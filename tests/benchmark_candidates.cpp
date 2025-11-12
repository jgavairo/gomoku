// Benchmark précis du CandidateGenerator
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/core/Board.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace gomoku;
using namespace std::chrono;

namespace {

struct BenchResult {
    int stones;
    int candidates;
    long long totalNs;
    int iterations;
    double avgUs;
};

BenchResult benchmark(Board& board, int iterations = 1000)
{
    RuleSet rules {};
    CandidateConfig config {};
    Player toPlay = Player::Black;

    // Warmup
    for (int i = 0; i < 10; i++) {
        CandidateGenerator::generate(board, rules, toPlay, config);
    }

    auto start = high_resolution_clock::now();
    int candidateCount = 0;

    for (int i = 0; i < iterations; i++) {
        auto candidates = CandidateGenerator::generate(board, rules, toPlay, config);
        candidateCount = static_cast<int>(candidates.size());
    }

    auto end = high_resolution_clock::now();
    long long totalNs = duration_cast<nanoseconds>(end - start).count();

    return {
        (int)board.occupiedPositions().size(),
        candidateCount,
        totalNs,
        iterations,
        static_cast<double>(totalNs) / static_cast<double>(iterations) / 1000.0 // microseconds
    };
}

void printHeader()
{
    std::cout << std::setw(12) << "Stones"
              << std::setw(12) << "Candidates"
              << std::setw(15) << "Avg Time (µs)"
              << std::setw(15) << "Per Call (ns)"
              << std::endl;
    std::cout << std::string(54, '-') << std::endl;
}

void printResult(const BenchResult& r)
{
    std::cout << std::setw(12) << r.stones
              << std::setw(12) << r.candidates
              << std::setw(15) << std::fixed << std::setprecision(2) << r.avgUs
              << std::setw(15) << static_cast<long long>(r.avgUs * 1000.0)
              << std::endl;
}

} // namespace

int main()
{
    std::cout << "\n=== CANDIDATE GENERATOR BENCHMARK ===" << std::endl;
    std::cout << "\nMeasuring average generation time over 1000 iterations\n"
              << std::endl;

    RuleSet rules {};

    // Test 1: Empty board
    std::cout << "Test 1: Empty board" << std::endl;
    printHeader();
    {
        Board board;
        auto result = benchmark(board);
        printResult(result);
    }

    // Test 2: Early game (3-10 stones)
    std::cout << "\nTest 2: Early game progression" << std::endl;
    printHeader();
    for (int numMoves : { 3, 5, 7, 10 }) {
        Board board;
        for (int i = 0; i < numMoves; i++) {
            int x = 9 + (i % 3);
            int y = 9 + (i / 3);
            Player p = (i % 2 == 0) ? Player::Black : Player::White;
            board.tryPlay(Move { { (uint8_t)x, (uint8_t)y }, p }, rules);
        }
        auto result = benchmark(board);
        printResult(result);
    }

    // Test 3: Mid game (15-30 stones)
    std::cout << "\nTest 3: Mid game progression" << std::endl;
    printHeader();
    for (int numMoves : { 15, 20, 25, 30 }) {
        Board board;
        for (int i = 0; i < numMoves; i++) {
            int x = 9 + (i % 6) - 2;
            int y = 9 + (i / 6) - 1;
            Player p = (i % 2 == 0) ? Player::Black : Player::White;
            board.tryPlay(Move { { (uint8_t)x, (uint8_t)y }, p }, rules);
        }
        auto result = benchmark(board);
        printResult(result);
    }

    // Test 4: Late game (40-50 stones)
    std::cout << "\nTest 4: Late game" << std::endl;
    printHeader();
    for (int numMoves : { 40, 50 }) {
        Board board;
        for (int i = 0; i < numMoves; i++) {
            int x = 9 + (i % 8) - 3;
            int y = 9 + (i / 8) - 2;
            Player p = (i % 2 == 0) ? Player::Black : Player::White;
            board.tryPlay(Move { { (uint8_t)x, (uint8_t)y }, p }, rules);
        }
        auto result = benchmark(board);
        printResult(result);
    }

    // Test 5: Worst case - scattered stones
    std::cout << "\nTest 5: Worst case - Scattered stones" << std::endl;
    printHeader();
    {
        Board board;
        // Pierres dispersées maximisant le nombre de rectangles
        std::vector<std::pair<int, int>> scattered = {
            { 0, 0 }, { 0, 18 }, { 18, 0 }, { 18, 18 }, // coins
            { 9, 0 }, { 9, 18 }, { 0, 9 }, { 18, 9 }, // milieux
            { 4, 4 }, { 4, 14 }, { 14, 4 }, { 14, 14 }, // intermédiaires
            { 9, 9 }, { 10, 10 }, { 8, 8 } // centre
        };
        for (size_t i = 0; i < scattered.size(); i++) {
            Player p = (i % 2 == 0) ? Player::Black : Player::White;
            board.tryPlay(Move { { (uint8_t)scattered[i].first, (uint8_t)scattered[i].second }, p }, rules);
        }
        auto result = benchmark(board);
        printResult(result);
    }

    return 0;
}

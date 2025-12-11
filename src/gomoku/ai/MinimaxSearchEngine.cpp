#include "gomoku/ai/MinimaxSearchEngine.hpp"
#include "gomoku/core/Board.hpp"
#include "gomoku/interfaces/IBoardView.hpp"
#include <iostream>
#include <stdexcept>

namespace gomoku::ai {

MinimaxSearchEngine::MinimaxSearchEngine()
    : searchImpl_(SearchConfig {})
    , config_ {}
{
}

MinimaxSearchEngine::MinimaxSearchEngine(const SearchConfig& config)
    : searchImpl_(config)
    , config_(config)
{
}

void MinimaxSearchEngine::setTimeLimit(int milliseconds)
{
    config_.timeBudgetMs = milliseconds;
    searchImpl_.setTimeBudgetMs(milliseconds);
}

void MinimaxSearchEngine::setDepthLimit(int maxDepth)
{
    config_.maxDepthHint = maxDepth;
    searchImpl_.setMaxDepthHint(maxDepth);
}

void MinimaxSearchEngine::setTranspositionTableSize(size_t bytes)
{
    config_.ttBytes = bytes;
    searchImpl_.setTranspositionTableSize(bytes);
}

std::optional<Move> MinimaxSearchEngine::findBestMove(const IBoardView& board, const RuleSet& rules, SearchStats* stats)
{
    // Convert IBoardView to concrete Board for MinimaxSearch class
    Board concreteBoard = boardFromView(board);

    // Use existing MinimaxSearch implementation
    auto result = searchImpl_.bestMove(concreteBoard, rules, stats);
    lastStats_ = stats ? *stats : SearchStats {};

    return result;
}

std::optional<Move> MinimaxSearchEngine::suggestMove(const IBoardView& board, const RuleSet& rules, int timeMs, SearchStats* stats)
{
    int oldTimeMs = config_.timeBudgetMs;
    searchImpl_.setTimeBudgetMs(timeMs);
    config_.timeBudgetMs = timeMs;
    auto res = findBestMove(board, rules, stats);
    // restore
    searchImpl_.setTimeBudgetMs(oldTimeMs);
    config_.timeBudgetMs = oldTimeMs;
    return res;
}

int MinimaxSearchEngine::evaluatePosition(const IBoardView& board, Player perspective) const
{
    // Pas de copie, juste un cast de référence/pointeur
    if (auto concreteBoard = dynamic_cast<const gomoku::Board*>(&board)) {
        return searchImpl_.evaluatePublic(*concreteBoard, perspective);
    }
    return 0; // ou exception
}

std::vector<Move> MinimaxSearchEngine::getOrderedMoves(const IBoardView& board, const RuleSet& rules) const
{
    if (auto concreteBoard = dynamic_cast<const gomoku::Board*>(&board)) {
        Player toPlay = concreteBoard->toPlay();
        return searchImpl_.orderedMovesPublic(*concreteBoard, rules, toPlay);
    }
    return {};
}

void MinimaxSearchEngine::clearTranspositionTable()
{
    searchImpl_.clearTranspositionTable();
}

SearchStats MinimaxSearchEngine::getLastSearchStats() const
{
    return lastStats_;
}

// Helper method to convert IBoardView to concrete Board
gomoku::Board MinimaxSearchEngine::boardFromView(const IBoardView& view) const
{
    const auto* concreteBoard = static_cast<const gomoku::Board*>(&view);
    return *concreteBoard;
}

} // namespace gomoku::ai
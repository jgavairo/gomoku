#pragma once
#include "gomoku/ai/SearchStats.hpp"
#include "gomoku/ai/TranspositionTable.hpp"
#include "gomoku/core/Types.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace gomoku {
class Board;

struct SearchConfig {
    int timeBudgetMs = 450; // Budget temps (ms) pour la recherche
    int maxDepthHint = 11; // Profondeur max d'itération
    std::size_t ttBytes = (64ull << 20); // Taille allouée à la table de transposition
    unsigned long long nodeCap = 0; // Limite de nœuds dure (0 = désactivée)
};

class MinimaxSearch {
public:
    explicit MinimaxSearch(const SearchConfig& conf)
        : cfg(conf)
    {
    }

    std::optional<Move> bestMove(Board& board, const RuleSet& rules, SearchStats* stats);

    // Configuration helpers used by MinimaxSearchEngine
    void setTimeBudgetMs(int ms) { cfg.timeBudgetMs = ms; }
    void setMaxDepthHint(int d) { cfg.maxDepthHint = d; }

    void setTranspositionTableSize(std::size_t bytes)
    {
        cfg.ttBytes = bytes;
        tt.resizeBytes(bytes);
    }

    void clearTranspositionTable() { tt.resizeBytes(cfg.ttBytes); }

    // Lightweight public helpers for tooling/analysis
    int evaluatePublic(const Board& board, Player perspective) const;
    std::vector<Move> orderedMovesPublic(const Board& board, const RuleSet& rules, Player toPlay) const;

private:
    // --- Core search primitives (signatures only) ---

    // Negamax with alpha-beta pruning and PVS. Returns best score and fills PV.
    // - depth: remaining plies to search (>= 0)
    // - alpha/beta: current search window
    // - toMove: side to move at this node
    // - ply: distance from root (for mate distance correction)
    // - stats: optional collector for node/qnode counters
    // - pvOut: principal variation to be populated with best line
    // - deadline: stop time for time management
    int negamax(Board& board, int depth, int alpha, int beta, int ply, std::vector<Move>& pvOut, const SearchContext& ctx);

    // Quiescence search to stabilize evaluations in tactical positions.
    // Searches only tactical moves (captures/menaces fortes) until a quiet position.
    int qsearch(Board& board, int alpha, int beta, int ply, const SearchContext& ctx);

    // Move ordering at a node: combines TT move, tactical generator, killers/history, etc.
    // depth parameter: use expensive evaluation only at root (high depth), cheap heuristics elsewhere
    std::vector<Move> orderMoves(const Board& board, const RuleSet& rules, Player toMove, const std::optional<Move>& ttMove, int depth) const;

    // --- Helpers extracted from bestMove for readability ---
    // Runs one iterative-deepening step at a given depth; fills best, bestScore, pv and updates nodes.
    bool runDepth(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx);

    SearchConfig cfg {};
    TranspositionTable tt;
};

} // namespace gomoku

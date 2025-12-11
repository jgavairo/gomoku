#pragma once
#include "gomoku/ai/Evaluator.hpp"
#include "gomoku/ai/MoveOrderer.hpp"
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
    int timeBudgetMs = 500; // Budget temps (ms) pour la recherche
    int maxDepthHint = 11; // Profondeur max d'itération - augmentée pour forcer l'efficacité
    std::size_t ttBytes = (128ull << 20); // Taille TT : 128MB (doublée) pour meilleur hit rate
    unsigned long long nodeCap = 0; // Limite de nœuds dure (0 = désactivée)

    // Aspiration window parameters
    bool useAspirationWindows = true; // Enable/disable aspiration windows
    int aspirationDelta = 400; // Fenêtre plus étroite pour forcer plus de re-recherches précises
    int aspirationWidenFactor = 6; // Élargissement plus modéré
    int aspirationDepthThreshold = 5; // Profondeur min pour activer l'aspiration
    int maxReSearches = 2; // Max tentatives de ré-élargissement avant full window

    // LMR (Late Move Reduction) parameters
    bool useLMR = true;
    int lmrMinDepth = 2; // Profondeur min pour activer LMR
    int lmrMinMoveIndex = 3; // Index de coup min pour activer LMR
};
class MinimaxSearch {
public:
    explicit MinimaxSearch(const SearchConfig& conf, const eval::EvalConfig& evalConf = {})
        : cfg(conf)
        , evaluator_(evalConf)
    {
        tt.resizeBytes(cfg.ttBytes); // Initialize TT with configured size
    }

    std::optional<Move> bestMove(Board& board, const RuleSet& rules, SearchStats* stats);

    // Configuration helpers used by MinimaxSearchEngine
    void setTimeBudgetMs(int ms) { cfg.timeBudgetMs = ms; }
    void setMaxDepthHint(int d) { cfg.maxDepthHint = d; }
    void setEvalConfig(const eval::EvalConfig& ec) { evaluator_.setConfig(ec); }

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

    // --- Helpers extracted from bestMove for readability ---
    // Runs one iterative-deepening step at a given depth with specified alpha-beta window
    // fills best, bestScore, pv and updates nodes.
    bool runDepthWithWindow(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx, int alpha, int beta);

    // Legacy wrapper for backwards compatibility (uses full window)
    bool runDepth(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx)
    {
        constexpr int INF = 1'000'000;
        return runDepthWithWindow(depth, board, rules, toPlay, rootCandidates, best, bestScore, pv, ctx, -INF, INF);
    }

    SearchConfig cfg {};
    TranspositionTable tt;
    MoveOrderer orderer_ { MoveOrdererConfig {} };
    eval::Evaluator evaluator_;
};

} // namespace gomoku

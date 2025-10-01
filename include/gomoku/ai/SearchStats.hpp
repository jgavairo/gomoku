#pragma once
#include "gomoku/core/Types.hpp"
#include <chrono>
#include <vector>

namespace gomoku {

// Forward declaration
struct SearchStats;

// Context passed through recursive search to avoid long parameter lists
// Provides fluent API for incrementing counters and checking time
struct SearchContext {
    const RuleSet& rules;
    std::chrono::steady_clock::time_point deadline;
    SearchStats* stats { nullptr };
    unsigned long long nodeCap { 0 };

    // Fluent API for incrementing counters during search
    inline void recordNode() const;
    inline void recordQNode() const;
    inline void recordTTHit() const;

    // Time management
    inline bool isTimeUp() const
    {
        return std::chrono::steady_clock::now() >= deadline;
    }
};

// Statistics collected during and after a search
// Design principle: counters (nodes, qnodes, ttHits) are incremented during search,
// metadata (depth, time, PV) is set at the end of each iteration
struct SearchStats {
    // Counters incremented during search (via ctx.recordNode(), etc.)
    long long nodes = 0;
    long long qnodes = 0;
    int ttHits = 0;

    // Metadata set at end of iteration (via finalize())
    int depthReached = 0;
    int timeMs = 0;
    std::vector<Move> principalVariation;

    // Reset all stats to zero
    void clear()
    {
        nodes = 0;
        qnodes = 0;
        ttHits = 0;
        depthReached = 0;
        timeMs = 0;
        principalVariation.clear();
    }

    // Finalize metadata after completing an iteration
    // Does NOT touch counters (nodes/qnodes/ttHits) - they are already accurate
    void finalize(std::chrono::steady_clock::time_point startTime,
        int depth,
        const std::vector<Move>& pv)
    {
        depthReached = depth;
        timeMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime)
                .count());
        principalVariation = pv;
    }

    // Convenience: set empty stats (for failed searches)
    static void setEmpty(SearchStats* stats,
        std::chrono::steady_clock::time_point startTime)
    {
        if (!stats)
            return;
        stats->clear();
        stats->timeMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime)
                .count());
    }
};

// Helper functions for counter increments (used by SearchContext fluent API)
inline void recordNodeVisit(SearchStats* stats)
{
    if (stats)
        ++stats->nodes;
}

inline void recordQNodeVisit(SearchStats* stats)
{
    if (stats)
        ++stats->qnodes;
}

inline void recordTTHit(SearchStats* stats)
{
    if (stats)
        ++stats->ttHits;
}

// SearchContext fluent API implementations
inline void SearchContext::recordNode() const { recordNodeVisit(stats); }
inline void SearchContext::recordQNode() const { recordQNodeVisit(stats); }
inline void SearchContext::recordTTHit() const { gomoku::recordTTHit(stats); }

} // namespace gomoku

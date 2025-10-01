#pragma once
#include "gomoku/core/Types.hpp"
#include <chrono>
#include <vector>

namespace gomoku {

// Represents the statistics for a search
struct SearchStats {
    long long nodes = 0;
    long long qnodes = 0;
    int depthReached = 0;
    int timeMs = 0;
    int ttHits = 0;
    std::vector<Move> principalVariation;

    // Helper to update stats from a time point
    void update(std::chrono::steady_clock::time_point startTime,
        long long nodeCount,
        long long qnodeCount,
        int depth,
        int ttHitCount,
        const std::vector<Move>& pv)
    {
        nodes = nodeCount;
        qnodes = qnodeCount;
        depthReached = depth;
        timeMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime)
                .count());
        ttHits = ttHitCount;
        principalVariation = pv;
    }

    // Helper to clear/reset stats
    void clear()
    {
        nodes = 0;
        qnodes = 0;
        depthReached = 0;
        timeMs = 0;
        ttHits = 0;
        principalVariation.clear();
    }

    // Convenience method for empty stats
    static void setEmpty(SearchStats* stats,
        std::chrono::steady_clock::time_point startTime)
    {
        if (stats)
            stats->update(startTime, 0, 0, 0, 0, {});
    }
};

} // namespace gomoku

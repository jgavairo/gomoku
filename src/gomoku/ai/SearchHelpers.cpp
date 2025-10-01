// SearchHelpers.cpp - Utility functions for minimax search
#include "gomoku/ai/SearchHelpers.hpp"
#include "gomoku/core/Board.hpp"

namespace gomoku::search {

bool isTerminal(const Board& board, int ply, int& outScore) noexcept
{
    const auto st = board.status();
    switch (st) {
    case GameStatus::Ongoing:
        outScore = 0;
        return false;
    case GameStatus::WinByAlign:
    case GameStatus::WinByCapture:
        // Side to move has just been given a losing position (opponent won on previous move)
        outScore = -MATE_SCORE + ply; // mate distance correction
        return true;
    case GameStatus::Draw:
        outScore = 0;
        return true;
    }
    outScore = 0;
    return false;
}

bool ttProbe(const TranspositionTable& tt, const Board& board, int depth, int alpha, int beta, int& outScore, std::optional<Move>& ttMove, TranspositionTable::Flag& outFlag) noexcept
{
    // Probe by Zobrist key; if depth is sufficient, return bound/value and best move when present.
    const uint64_t key = board.zobristKey();
    auto* e = tt.probe(key);
    if (!e || e->key != key)
        return false;

    // Provide TT move for ordering if valid
    if (e->best.isValid())
        ttMove = e->best;
    else
        ttMove.reset();

    // Only use the bound/value if entry depth is sufficient
    if (e->depth < depth)
        return false;

    outFlag = e->flag;
    const int s = e->score;
    switch (e->flag) {
    case TranspositionTable::Flag::Exact:
        outScore = s;
        return true;
    case TranspositionTable::Flag::Lower:
        if (s >= beta) {
            outScore = s;
            return true;
        }
        return false;
    case TranspositionTable::Flag::Upper:
        if (s <= alpha) {
            outScore = s;
            return true;
        }
        return false;
    }
    return false;
}

void ttStore(TranspositionTable& tt, const Board& board, int depth, int score, TranspositionTable::Flag flag, const std::optional<Move>& best) noexcept
{
    // Store by Zobrist key; rely on TT's internal replacement policy (prefer deeper entries).
    const uint64_t key = board.zobristKey();
    tt.store(key, depth, score, flag, best);
}

std::optional<Move> tryImmediateWin(Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& candidates)
{
    // Early plausibility checks to avoid unnecessary speculative plays
    const bool plausibleAlign = board.stoneCount(toPlay) >= 4;
    const auto caps = board.capturedPairs();
    const int pairs = (toPlay == Player::Black) ? caps.black : caps.white;
    const bool plausibleCaptureWin = pairs >= 4;

    // Only attempt if at least one path to immediate win is plausible
    if (!plausibleAlign && !plausibleCaptureWin)
        return std::nullopt;

    // Try each candidate speculatively
    for (const auto& m : candidates) {
        auto pr = board.tryPlay(m, rules);
        if (!pr.success)
            continue; // skip illegal candidates, don't abort early
        const auto st = board.status();
        board.undo();
        if (st == GameStatus::WinByAlign || st == GameStatus::WinByCapture)
            return m;
    }
    return std::nullopt;
}

} // namespace gomoku::search

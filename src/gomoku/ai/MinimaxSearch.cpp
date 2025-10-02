// MinimaxSearch.cpp
#include "gomoku/ai/MinimaxSearch.hpp"
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/ai/Evaluator.hpp"
#include "gomoku/ai/SearchHelpers.hpp"
#include "gomoku/ai/SearchStats.hpp"
#include "gomoku/core/Board.hpp"
#include "util/Logger.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <sstream>

namespace gomoku {

namespace {
    using Clock = std::chrono::steady_clock;

    // Generate root candidates with fallback to legal moves
    inline std::vector<Move> genRootCandidates(const Board& board, const RuleSet& rules, Player toPlay)
    {
        auto cands = CandidateGenerator::generate(board, rules, toPlay, CandidateConfig {});
        if (cands.empty())
            cands = board.legalMoves(toPlay, rules);
        return cands;
    }

    // Helper to format move as string (e.g., "A1", "K11")
    inline std::string moveToString(const Move& m)
    {
        std::ostringstream oss;
        char col = static_cast<char>('A' + static_cast<int>(m.pos.x));
        int row = static_cast<int>(m.pos.y) + 1;
        oss << col << row;
        return oss.str();
    }

} // namespace

// Note: cellOf and other are now available as playerToCell and opponent in Types.hpp
std::optional<Move> MinimaxSearch::bestMove(Board& board, const RuleSet& rules, SearchStats* stats)
{
    using namespace std::chrono;
    auto start = steady_clock::now();
    auto deadline = start + milliseconds(cfg.timeBudgetMs);
    SearchContext ctx { rules, deadline, stats, cfg.nodeCap };

    Player toPlay = board.toPlay();

    // Helper for early returns with empty stats
    auto returnEmpty = [&]() -> std::optional<Move> {
        SearchStats::setEmpty(stats, start);
        return std::nullopt;
    };

    // Initialize stats if provided
    if (stats)
        stats->clear();

    // Early terminal check
    int terminalScore = 0;
    if (search::isTerminal(board, /*ply*/ 0, terminalScore))
        return returnEmpty();

    std::vector<Move> candidates = genRootCandidates(board, rules, toPlay);

    if (candidates.empty())
        return returnEmpty();

    // 1) Immediate win shortcut (only if situation permits)
    if (auto iw = search::tryImmediateWin(board, rules, toPlay, candidates)) {
        if (stats) {
            // Note: not counting nodes here as this is a shortcut, not a full search
            stats->finalize(start, /*depth*/ 1, { *iw });
        }
        Logger::getInstance().info("AI: Immediate win found!");
        return iw;
    }

    // 2) Iterative deepening skeleton with aspiration windows
    std::optional<Move> best;
    std::vector<Move> pv;
    int maxDepth = cfg.maxDepthHint;
    int bestScore = -search::INF;

    Logger::getInstance().debug("AI: Starting iterative deepening - max depth {}, aspiration {} (delta {}), TT size {} MB",
        maxDepth, cfg.useAspirationWindows ? "enabled" : "disabled", cfg.aspirationDelta, cfg.ttBytes / (1024 * 1024));

    for (int depth = 1; depth <= maxDepth; ++depth) {
        int alpha, beta;

        // First iteration or aspiration disabled: use full window
        if (depth == 1 || !cfg.useAspirationWindows) {
            alpha = -search::INF;
            beta = search::INF;
        } else {
            // Aspiration window: narrow search around previous score
            alpha = bestScore - cfg.aspirationDelta;
            beta = bestScore + cfg.aspirationDelta;
            Logger::getInstance().debug("AI: Depth {} starting with aspiration window [{}, {}] around score {}",
                depth, alpha, beta, bestScore);
        }

        bool searchComplete = false;
        int windowWidenCount = 0;
        const int maxReSearches = 3; // Prevent infinite loop
        long long nodesBefore = stats ? stats->nodes : 0;

        // Aspiration window re-search loop
        while (!searchComplete && windowWidenCount < maxReSearches) {
            long long nodesAtStart = stats ? stats->nodes : 0;

            if (!runDepthWithWindow(depth, board, rules, toPlay, candidates, best, bestScore, pv, ctx, alpha, beta)) {
                // Search failed (timeout or no moves)
                if (ctx.isTimeUp()) {
                    Logger::getInstance().debug("AI: Time budget exceeded at depth {}", depth);
                }
                goto depth_loop_end;
            }

            long long nodesThisSearch = (stats ? stats->nodes : 0) - nodesAtStart;

            // Check if score fell outside aspiration window
            if (cfg.useAspirationWindows && depth > 1) {
                if (bestScore <= alpha) {
                    // Failed low: widen window downward
                    int oldAlpha = alpha;
                    alpha = std::max(alpha - cfg.aspirationDelta * cfg.aspirationWidenFactor, -search::INF);
                    Logger::getInstance().debug("AI: Aspiration fail-low at depth {} (score {} <= {}), {} nodes, widening to [{}, {}]",
                        depth, bestScore, oldAlpha, nodesThisSearch, alpha, beta);
                    windowWidenCount++;
                } else if (bestScore >= beta) {
                    // Failed high: widen window upward
                    int oldBeta = beta;
                    beta = std::min(beta + cfg.aspirationDelta * cfg.aspirationWidenFactor, search::INF);
                    Logger::getInstance().debug("AI: Aspiration fail-high at depth {} (score {} >= {}), {} nodes, widening to [{}, {}]",
                        depth, bestScore, oldBeta, nodesThisSearch, alpha, beta);
                    windowWidenCount++;
                } else {
                    // Score within window: success!
                    if (windowWidenCount > 0) {
                        Logger::getInstance().debug("AI: Aspiration success at depth {} after {} re-searches, score {} in [{}, {}]",
                            depth, windowWidenCount, bestScore, alpha, beta);
                    }
                    searchComplete = true;
                }
            } else {
                searchComplete = true;
            }
        }

        // If we exhausted re-searches, do final full-window search
        if (!searchComplete) {
            Logger::getInstance().warning("AI: Aspiration window failed {} times at depth {}, re-searching with full window",
                windowWidenCount, depth);
            long long nodesFallback = stats ? stats->nodes : 0;
            if (!runDepthWithWindow(depth, board, rules, toPlay, candidates, best, bestScore, pv, ctx, -search::INF, search::INF)) {
                if (ctx.isTimeUp()) {
                    Logger::getInstance().debug("AI: Time budget exceeded at depth {}", depth);
                }
                break;
            }
            long long fallbackNodes = (stats ? stats->nodes : 0) - nodesFallback;
            Logger::getInstance().debug("AI: Fallback full-window search used {} nodes", fallbackNodes);
        }

        // Finalize metadata for this iteration (counters already updated via ctx.recordNode())
        if (stats) {
            stats->finalize(start, depth, pv);
            long long depthNodes = (stats->nodes - nodesBefore);
            Logger::getInstance().debug("AI: Depth {} complete - score {}, {} total nodes this depth, PV length {}",
                depth, bestScore, depthNodes, pv.size());
        }
    }
depth_loop_end:

    if (best) {
        // Final summary log with all statistics
        if (stats) {
            long long nps = stats->timeMs > 0 ? (stats->nodes * 1000LL / stats->timeMs) : 0;
            int ttHitRate = stats->nodes > 0 ? static_cast<int>(stats->ttHits * 100LL / stats->nodes) : 0;
            Logger::getInstance().info("AI: Search complete - depth {}, {} nodes ({} qnodes, {} TT hits, {}% hit rate), score {}, {}ms ({} nps)",
                stats->depthReached, stats->nodes, stats->qnodes, stats->ttHits, ttHitRate, bestScore, stats->timeMs, nps);

            // Log PV (first few moves)
            if (!pv.empty()) {
                std::string pvStr;
                int pvDisplay = std::min(static_cast<int>(pv.size()), 5);
                for (int i = 0; i < pvDisplay; ++i) {
                    pvStr += moveToString(pv[i]);
                    if (i < pvDisplay - 1)
                        pvStr += " ";
                }
                if (pv.size() > 5)
                    pvStr += " ...";
                Logger::getInstance().info("AI: Principal Variation: {}", pvStr);
            }
        }
        return best;
    }

    Logger::getInstance().warning("AI: Search returned no valid move");
    return returnEmpty();
}

int MinimaxSearch::evaluatePublic(const Board& board, Player perspective) const
{
    return eval::evaluate(board, perspective);
}

std::vector<Move> MinimaxSearch::orderedMovesPublic(const Board& board, const RuleSet& rules, Player toPlay) const
{
    auto moves = CandidateGenerator::generate(board, rules, toPlay, CandidateConfig {});
    if (moves.empty()) {
        moves = board.legalMoves(toPlay, rules);
    }
    return moves;
}

// Négamax récursif (Gomoku) avec alpha-beta pruning.
// Architecture prête pour extensions: PVS, TT probe/store, extensions tactiques, LMR.
int MinimaxSearch::negamax(Board& board, int depth, int alpha, int beta, int ply, std::vector<Move>& pvOut, const SearchContext& ctx)
{
    pvOut.clear();

    // Count this node
    ctx.recordNode();

    // 1) Check time budget
    if (ctx.isTimeUp())
        return 0; // Return neutral score on timeout

    // 2) Check node cap (hard limit)
    if (ctx.nodeCap > 0 && ctx.stats && ctx.stats->nodes >= static_cast<long long>(ctx.nodeCap))
        return 0;

    // 3) Terminal check (win/loss/draw)
    int terminalScore = 0;
    if (search::isTerminal(board, ply, terminalScore))
        return terminalScore;

    // 4) Leaf node: transition to quiescence search or static eval
    if (depth <= 0) {
        // For now, call qsearch to stabilize tactical positions
        // When qsearch is implemented, this will explore forced sequences
        return qsearch(board, alpha, beta, ply, ctx);
    }

    // 5) TT probe: check if we've seen this position before
    std::optional<Move> ttMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    if (search::ttProbe(tt, board, depth, alpha, beta, ttScore, ttMove, ttFlag)) {
        // TT hit with usable bound/value
        ctx.recordTTHit();
        // If exact score or cutoff, we can return immediately
        // Note: PV is not reconstructed from TT (would require storing full PV)
        pvOut.clear();
        if (ttMove)
            pvOut.push_back(*ttMove);
        return ttScore;
    }
    if (ttMove) {
        // Even without a cutoff, we got a move hint for ordering
        ctx.recordTTHit();
    }

    // 6) Generate and order moves
    Player toMove = board.toPlay();
    auto moves = orderMoves(board, ctx.rules, toMove, ttMove, depth); // Pass TT move and depth for adaptive ordering

    // No legal moves: should not happen if terminal check is correct, but handle gracefully
    if (moves.empty())
        return eval::evaluate(board, toMove);

    // 7) Alpha-beta search through child nodes
    int bestScore = -search::INF;
    std::vector<Move> bestPV;
    bool foundLegal = false;

    for (const auto& m : moves) {
        // Try to play the move
        auto pr = board.tryPlay(m, ctx.rules);
        if (!pr.success)
            continue; // Skip illegal moves

        foundLegal = true;

        // Recursive negamax call (negate score due to negamax property)
        std::vector<Move> childPV;
        int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, childPV, ctx);

        // Undo the move
        board.undo();

        // Update best score and PV
        if (score > bestScore) {
            bestScore = score;
            bestPV.clear();
            bestPV.push_back(m);
            bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
        }

        // Alpha-beta pruning
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break; // Beta cutoff

        // Check time during search
        if (ctx.isTimeUp())
            break;
    }

    // If no legal moves were found, return static evaluation
    if (!foundLegal)
        return eval::evaluate(board, toMove);

    // 8) TT store: save this position for future lookup
    // Determine the bound type based on alpha-beta window
    TranspositionTable::Flag storeFlag;
    if (bestScore <= alpha) {
        // Failed low: bestScore is an upper bound (all moves were <= alpha)
        storeFlag = TranspositionTable::Flag::Upper;
    } else if (bestScore >= beta) {
        // Failed high: bestScore is a lower bound (beta cutoff occurred)
        storeFlag = TranspositionTable::Flag::Lower;
    } else {
        // Exact score: within the [alpha, beta] window
        storeFlag = TranspositionTable::Flag::Exact;
    }

    // Extract best move from PV
    std::optional<Move> bestMove;
    if (!bestPV.empty())
        bestMove = bestPV.front();

    search::ttStore(tt, board, depth, bestScore, storeFlag, bestMove);

    pvOut = bestPV;
    return bestScore;
}

// Recherche de quiétude (Gomoku):
//  - Stabilise l'évaluation en explorant uniquement les coups tactiques pertinents Gomoku:
//    • gains immédiats (faire 5), parades immédiates (bloquer 5/adversaire),
//    • créations/bloquages de quatre ouverts,
//    • captures de paires qui gagnent ou évitent une défaite par captures,
//    • éventuellement prolongations locales de menaces fortes.
//  - Évite d’explorer des coups calmes qui n’affectent pas les menaces en cours.
// TODO: stand-pat (éval statique), delta pruning adapté aux marges de menaces, génération coups tactiques.
int MinimaxSearch::qsearch(Board& board, int alpha, int beta, int ply, const SearchContext& ctx)
{
    ctx.recordQNode();

    // Check for terminal positions first
    int terminalScore = 0;
    if (search::isTerminal(board, ply, terminalScore))
        return terminalScore;

    // Stand-pat: évaluation statique de la position
    Player toMove = board.toPlay();
    int standPat = eval::evaluate(board, toMove);

    // Beta cutoff sur stand-pat (position déjà trop bonne)
    if (standPat >= beta)
        return beta;

    // Raise alpha si stand-pat est meilleur
    if (standPat > alpha)
        alpha = standPat;

    // Pour l'instant, pas de recherche tactique supplémentaire
    // (peut être ajouté plus tard : captures, menaces de 4, etc.)

    return standPat;
}

// Ordonne les coups à explorer (Gomoku):
//  - 1) TT move (si disponible)
//  - 2) Évaluation spéculative avec tryPlay/undo
//  - 3) Limitation dynamique du nombre de coups selon la profondeur
std::vector<Move> MinimaxSearch::orderMoves(const Board& board, const RuleSet& rules, Player toMove, const std::optional<Move>& ttMove, int depth) const
{
    // Get base candidates (already well-ordered by CandidateGenerator)
    auto moves = orderedMovesPublic(board, rules, toMove);

    // Always place TT move first if available
    if (ttMove && ttMove->isValid()) {
        auto it = std::find_if(moves.begin(), moves.end(),
            [&ttMove](const Move& m) { return m.pos == ttMove->pos; });

        if (it != moves.end()) {
            Move ttm = *it;
            moves.erase(it);
            moves.insert(moves.begin(), ttm);
        }
    }

    // Evaluate all moves speculatively (critical for good alpha-beta performance)
    struct ScoredMove {
        Move move;
        int score;
    };

    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(moves.size());

    // Skip first move if it's the ttMove
    size_t startIdx = (ttMove && ttMove->isValid() && !moves.empty() && moves[0].pos == ttMove->pos) ? 1 : 0;

    for (size_t i = startIdx; i < moves.size(); ++i) {
        const auto& m = moves[i];
        int score = 0;

        auto pr = const_cast<Board&>(board).tryPlay(m, rules);
        if (pr.success) {
            if (board.status() == GameStatus::WinByAlign || board.status() == GameStatus::WinByCapture) {
                score = 1000000; // Immediate win
            } else {
                score = -eval::evaluate(board, opponent(toMove));
            }
            const_cast<Board&>(board).undo();
        } else {
            score = -1000000; // Illegal
        }

        scoredMoves.push_back({ m, score });
    }

    std::sort(scoredMoves.begin(), scoredMoves.end(),
        [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    std::vector<Move> ordered;
    ordered.reserve(moves.size());

    if (startIdx == 1) {
        ordered.push_back(moves[0]); // ttMove first
    }

    // Adaptive move count limit based on depth
    // Aggressive pruning to reach depth 11
    size_t maxMoves;
    if (depth >= 8) {
        maxMoves = 25; // Root: explore widely
    } else if (depth >= 5) {
        maxMoves = 15; // Mid-tree: aggressive selection
    } else if (depth >= 2) {
        maxMoves = 10; // Deep nodes: very selective
    } else {
        maxMoves = 6; // Leaf neighbors: ultra selective
    }

    size_t count = std::min(scoredMoves.size(), maxMoves);
    for (size_t i = 0; i < count; ++i) {
        ordered.push_back(scoredMoves[i].move);
    }

    return ordered;
}

bool MinimaxSearch::runDepthWithWindow(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx, int alpha, int beta)
{
    if (ctx.isTimeUp())
        return false;

    // Probe TT for this root position
    std::optional<Move> ttRootMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    bool ttHit = search::ttProbe(tt, board, depth, alpha, beta, ttScore, ttRootMove, ttFlag);

    if (ttHit) {
        ctx.recordTTHit();
        Logger::getInstance().debug("AI: TT hit at root depth {} - score {}, move {}",
            depth, ttScore, ttRootMove ? moveToString(*ttRootMove) : "none");
    }

    auto ordered = orderMoves(board, rules, toPlay, ttRootMove, depth);
    if (ordered.empty())
        ordered = rootCandidates; // fallback

    Logger::getInstance().debug("AI: Exploring {} root candidates at depth {}", ordered.size(), depth);

    std::optional<Move> depthBest;
    int depthBestScore = -search::INF;
    std::vector<Move> depthPV;
    int movesSearched = 0;
    int bestMoveIndex = -1;

    for (size_t i = 0; i < ordered.size(); ++i) {
        const auto& m = ordered[i];
        if (ctx.isTimeUp())
            break;
        auto pr = board.tryPlay(m, rules);
        if (!pr.success)
            continue;

        movesSearched++;
        std::vector<Move> childPV;
        int childScore = negamax(board, depth - 1, -beta, -alpha, /*ply*/ 1, childPV, ctx);
        int score = -childScore;
        board.undo();

        if (score > depthBestScore) {
            depthBestScore = score;
            depthBest = m;
            bestMoveIndex = static_cast<int>(i);
            depthPV.clear();
            depthPV.push_back(m);
            depthPV.insert(depthPV.end(), childPV.begin(), childPV.end());
            Logger::getInstance().debug("AI: New best move at depth {}: {} (score {}, move {}/{})",
                depth, moveToString(m), score, movesSearched, ordered.size());
        }
        if (score > alpha)
            alpha = score;
    }

    if (bestMoveIndex >= 0 && movesSearched > 0) {
        Logger::getInstance().debug("AI: Best move was #{} out of {} candidates searched", bestMoveIndex + 1, movesSearched);
    }

    if (!depthBest)
        return false;

    best = depthBest;
    bestScore = depthBestScore;
    pv = depthPV;

    // Determine TT flag based on final score vs window
    TranspositionTable::Flag storeFlag;
    if (bestScore <= alpha) {
        storeFlag = TranspositionTable::Flag::Upper; // Failed low
    } else if (bestScore >= beta) {
        storeFlag = TranspositionTable::Flag::Lower; // Failed high
    } else {
        storeFlag = TranspositionTable::Flag::Exact; // Exact score
    }

    // Store result in TT
    search::ttStore(tt, board, depth, bestScore, storeFlag, best);

    return true;
}
} // namespace gomoku

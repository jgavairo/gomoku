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
        return iw;
    }

    // 2) Iterative deepening skeleton with aspiration windows
    std::optional<Move> best;
    std::vector<Move> pv;
    int maxDepth = cfg.maxDepthHint;
    int bestScore = -search::INF;
    int reachedDepth = 0;

    for (int depth = 1; depth <= maxDepth; ++depth) {
        reachedDepth = depth;
        int alpha, beta;
        if (depth == 1)
            orderer_.clearForNewIteration(/*maxPly=*/64);
        // First few iterations or aspiration disabled: use full window
        // Aspiration windows are more effective at deeper depths when the tree is more stable
        if (depth <= 5 || !cfg.useAspirationWindows) {
            alpha = -search::INF;
            beta = search::INF;
        } else {
            // Aspiration window: narrow search around previous score
            alpha = bestScore - cfg.aspirationDelta;
            beta = bestScore + cfg.aspirationDelta;
        }

        bool searchComplete = false;
        int windowWidenCount = 0;
        const int maxReSearches = 2; // Limit re-searches (reduced from 3 to avoid wasted time)

        // Aspiration window re-search loop
        while (!searchComplete && windowWidenCount < maxReSearches) {

            if (!runDepthWithWindow(depth, board, rules, toPlay, candidates, best, bestScore, pv, ctx, alpha, beta)) {
                // Search failed (timeout or no moves)
                goto depth_loop_end;
            }

            // Check if score fell outside aspiration window
            if (cfg.useAspirationWindows && depth > 1) {
                if (bestScore <= alpha) {
                    // Failed low: widen window downward
                    alpha = std::max(alpha - cfg.aspirationDelta * cfg.aspirationWidenFactor, -search::INF);
                    windowWidenCount++;
                } else if (bestScore >= beta) {
                    // Failed high: widen window upward
                    beta = std::min(beta + cfg.aspirationDelta * cfg.aspirationWidenFactor, search::INF);
                    windowWidenCount++;
                } else {
                    // Score within window: success!
                    searchComplete = true;
                }
            } else {
                searchComplete = true;
            }
        }

        // If we exhausted re-searches, do final full-window search
        if (!searchComplete) {
            if (!runDepthWithWindow(depth, board, rules, toPlay, candidates, best, bestScore, pv, ctx, -search::INF, search::INF)) {
                break;
            }
        }

        // Finalize metadata for this iteration (counters already updated via ctx.recordNode())
        if (stats) {
            stats->finalize(start, depth, pv);
        }
    }
depth_loop_end:

    if (best) {
        // Final summary log with all statistics
        if (stats) {
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
            }
        }
        LOG_INFO("Search finished. Depth reached: " + std::to_string(reachedDepth));
        return best;
    }

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
    int alpha0 = alpha, beta0 = beta;
    pvOut.clear();

    // Count this node
    ctx.recordNode();

    // 1) Check time budget
    if (ctx.isTimeUp())
        return eval::evaluate(board, board.toPlay());

    // 2) Check node cap (hard limit)
    if (ctx.nodeCap > 0 && ctx.stats && ctx.stats->nodes >= static_cast<long long>(ctx.nodeCap))
        return eval::evaluate(board, board.toPlay());

    // 3) Terminal check (win/loss/draw)
    int terminalScore = 0;
    if (search::isTerminal(board, ply, terminalScore))
        return terminalScore;

    // 4) Leaf node: transition to quiescence search or static eval
    if (depth <= 0) {
        return qsearch(board, alpha, beta, ply, ctx);
    }

    // 5) TT probe: check if we've seen this position before
    std::optional<Move> ttMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    if (search::ttProbe(tt, board, depth, alpha, beta, ttScore, ttMove, ttFlag)) {
        // Hit exploitable (Exact ou borne coupante) garanti par ttProbe
        ctx.recordTTHit();
        pvOut.clear();
        if (ttMove)
            pvOut.push_back(*ttMove);
        return ttScore;
    }

    // 6) Generate and order moves
    Player toMove = board.toPlay();
    auto moves = orderer_.order(board, ctx.rules, toMove, depth, ttMove);

    // No legal moves: should not happen if terminal check is correct, but handle gracefully
    if (moves.empty())
        return eval::evaluate(board, toMove);

    // 7) Alpha-beta search through child nodes
    int bestScore = -search::INF;
    std::vector<Move> bestPV;
    bool foundLegalMove = false;

    for (size_t i = 0; i < moves.size(); ++i) {
        const auto& m = moves[i];
        auto pr = board.tryPlay(m, ctx.rules);
        if (!pr.success)
            continue;

        foundLegalMove = true;
        std::vector<Move> childPV;
        int score;

        if (bestScore == -search::INF) {
            // Premier coup (PV-node) : fenêtre pleine
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, childPV, ctx);
        } else {
            // Coups suivants (Cut-nodes) : fenêtre nulle (Null Window Search)

            // Late Move Reduction (LMR)
            int R = 0;
            // Start LMR earlier (depth 2) and be more aggressive
            if (depth >= 2 && i >= 3) {
                R = 1;
                if (depth >= 4 && i >= 8)
                    R = 2;
                if (depth >= 6 && i >= 15)
                    R = 3;

                // Ensure we don't drop below depth 0 (qsearch)
                if (depth - 1 - R < 0)
                    R = depth - 1;
            }

            // Recherche avec profondeur réduite (ou normale si R=0)
            score = -negamax(board, depth - 1 - R, -alpha - 1, -alpha, ply + 1, childPV, ctx);

            // Si LMR a échoué (le coup semble bon), on refait la recherche à pleine profondeur (toujours fenêtre nulle)
            if (R > 0 && score > alpha) {
                // Re-search only if the score is promising enough?
                // For now, standard re-search
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1, childPV, ctx);
            }

            // Si le pari fenêtre nulle est perdu (score > alpha), on doit refaire une recherche complète (fenêtre ouverte)
            if (score > alpha && score < beta) {
                if (!ctx.isTimeUp()) {
                    childPV.clear();
                    score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, childPV, ctx);
                }
            }
        }

        board.undo();

        // Si timeout après le premier coup, retourner le meilleur score trouvé
        if (ctx.isTimeUp()) {
            if (score > bestScore) {
                bestScore = score;
                bestPV.clear();
                bestPV.push_back(m);
                bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
            }
            break; // Sortir immédiatement
        }

        if (score > bestScore) {
            bestScore = score;
            bestPV.clear();
            bestPV.push_back(m);
            bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
        }

        if (score > alpha)
            alpha = score;
        if (alpha >= beta) {
            orderer_.onBetaCut(ply, m);
            break;
        }
    }

    // Si aucun coup légal trouvé, retourner évaluation statique
    if (!foundLegalMove) {
        return eval::evaluate(board, board.toPlay());
    }
    // 8) TT store: save this position for future lookup
    // Determine the bound type based on alpha-beta window
    TranspositionTable::Flag storeFlag;
    if (bestScore <= alpha0)
        storeFlag = TranspositionTable::Flag::Upper; // fail-low vs fenêtre d’origine
    else if (bestScore >= beta0)
        storeFlag = TranspositionTable::Flag::Lower; // fail-high
    else
        storeFlag = TranspositionTable::Flag::Exact;

    // Extract best move from PV
    std::optional<Move> bestMove;
    if (!bestPV.empty())
        bestMove = bestPV.front();

    if (!ctx.isTimeUp()) {
        search::ttStore(tt, board, depth, bestScore, storeFlag, bestMove);
    }

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

    // Recherche tactique : captures et menaces
    // On limite la profondeur de qsearch pour éviter l'explosion combinatoire
    // même sur les coups tactiques (ex: 4 plies max de qsearch)
    // Mais ici on n'a pas de paramètre depth_q. On suppose que les coups tactiques s'épuisent.
    // Pour sécurité, on peut checker ctx.isTimeUp()
    if (ctx.isTimeUp())
        return alpha;

    auto moves = CandidateGenerator::generateTactical(board, ctx.rules, toMove);

    // Tri basique : captures d'abord ?
    // Pour l'instant on fait confiance à l'ordre de génération (qui est spatial)

    for (const auto& m : moves) {
        auto pr = board.tryPlay(m, ctx.rules);
        if (!pr.success)
            continue;

        int score = -qsearch(board, -beta, -alpha, ply + 1, ctx);
        board.undo();

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

bool MinimaxSearch::runDepthWithWindow(int depth, Board& board, const RuleSet& rules, Player toPlay,
    const std::vector<Move>& rootCandidates,
    std::optional<Move>& best, int& bestScore, std::vector<Move>& pv,
    const SearchContext& ctx, int alpha, int beta)
{
    if (ctx.isTimeUp())
        return false;

    const int alpha0 = alpha;
    const int beta0 = beta;

    // TT probe root
    std::optional<Move> ttRootMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    const bool ttHit = search::ttProbe(tt, board, depth, alpha, beta, ttScore, ttRootMove, ttFlag);
    if (ttHit) {
        ctx.recordTTHit();

        // Ajuster alpha si on a une borne inférieure exploitable
        if (ttFlag == TranspositionTable::Flag::Lower || ttFlag == TranspositionTable::Flag::Exact) {
            alpha = std::max(alpha, ttScore);
        }
    }

    // IMPORTANT: réordonner la liste fournie, ne pas regénérer
    auto ordered = orderer_.order(board, rules, toPlay, depth, ttRootMove, &rootCandidates);

    if (ordered.empty())
        return false;

    std::optional<Move> depthBest;
    int depthBestScore = -search::INF;
    std::vector<Move> depthPV;
    int bestMoveIndex = -1;

    for (size_t i = 0; i < ordered.size(); ++i) {
        if (ctx.isTimeUp())
            break;

        const Move& m = ordered[i];
        auto pr = board.tryPlay(m, rules);
        if (!pr.success)
            continue;

        std::vector<Move> childPV;
        int childScore;

        if (i == 0) {
            // PVS: premier enfant en fenêtre pleine
            childScore = negamax(board, depth - 1, -beta, -alpha, /*ply*/ 1, childPV, ctx);
        } else {
            // PVS: fenêtre nulle
            childScore = negamax(board, depth - 1, -(alpha + 1), -alpha, /*ply*/ 1, childPV, ctx);
            // Re-search si améliore
            if (-childScore > alpha && !ctx.isTimeUp()) {
                childPV.clear();
                childScore = negamax(board, depth - 1, -beta, -alpha, /*ply*/ 1, childPV, ctx);
            }
        }

        const int score = -childScore;
        board.undo();

        if (score > depthBestScore) {
            depthBestScore = score;
            depthBest = m;
            bestMoveIndex = static_cast<int>(i);
            depthPV.clear();
            depthPV.push_back(m);
            depthPV.insert(depthPV.end(), childPV.begin(), childPV.end());

            Logger::getInstance().debug("AI: New best move at depth {}: {} (score {}, move {}/{})",
                depth, moveToString(m), score, static_cast<int>(i + 1), static_cast<int>(ordered.size()));
        }

        if (score > alpha) {
            alpha = score;
            // **Root beta-cut**
            if (alpha >= beta) {
                Logger::getInstance().debug("AI: Root beta-cut at depth {} after move #{}", depth, i + 1);
                break;
            }
        }
    }

    if (bestMoveIndex >= 0) {
        Logger::getInstance().debug("AI: Best move was #{} out of {} candidates considered",
            bestMoveIndex + 1, static_cast<int>(ordered.size()));
    }

    // Timeout au milieu mais on a un meilleur ? → renvoyer le partiel
    if (!depthBest)
        return false;

    best = depthBest;
    bestScore = depthBestScore;
    pv = std::move(depthPV);

    // Déterminer le flag TT vs fenêtre d’origine
    TranspositionTable::Flag storeFlag = (bestScore <= alpha0) ? TranspositionTable::Flag::Upper : (bestScore >= beta0) ? TranspositionTable::Flag::Lower
                                                                                                                        : TranspositionTable::Flag::Exact;

    // Option: ne pas stocker si timeUp (pour éviter d’empoisonner la TT)
    if (!ctx.isTimeUp()) {
        search::ttStore(tt, board, depth, bestScore, storeFlag, best);
    }

    return true;
}

} // namespace gomoku

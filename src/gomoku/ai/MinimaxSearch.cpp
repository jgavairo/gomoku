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

} // namespace

// Note: cellOf and other are now available as playerToCell and opponent in Types.hpp
std::optional<Move> MinimaxSearch::bestMove(Board& board, const RuleSet& rules, SearchStats* stats)
{
    using namespace std::chrono;
    auto start = steady_clock::now();
    auto deadline = start + milliseconds(cfg.timeBudgetMs);
    SearchContext ctx { rules, deadline, stats, cfg.nodeCap };

    Player toPlay = board.toPlay();

    // Initialize stats if provided
    if (stats)
        stats->clear();

    // Early terminal check
    int terminalScore = 0;
    if (search::isTerminal(board, /*ply*/ 0, terminalScore)) {
        SearchStats::setEmpty(stats, start);
        return std::nullopt;
    }

    std::vector<Move> candidates = genRootCandidates(board, rules, toPlay);

    if (candidates.empty()) {
        SearchStats::setEmpty(stats, start);
        return std::nullopt;
    }

    // 1) Immediate win shortcut (only if situation permits)
    if (auto iw = search::tryImmediateWin(board, rules, toPlay, candidates)) {
        if (stats) {
            ctx.recordNode(); // Count the immediate win node
            stats->finalize(start, /*depth*/ 1, { *iw });
        }
        Logger::getInstance().info("AI: Immediate win found!");
        return iw;
    }

    // 2) Iterative deepening skeleton
    std::optional<Move> best;
    std::vector<Move> pv;
    int maxDepth = cfg.maxDepthHint;
    int bestScore = -search::INF;

    for (int depth = 1; depth <= maxDepth; ++depth) {
        if (!runDepth(depth, board, rules, toPlay, candidates, best, bestScore, pv, ctx))
            break;
        // Finalize metadata for this iteration (counters already updated via ctx.recordNode())
        if (stats) {
            stats->finalize(start, depth, pv);
            // Log performance every depth completion
            Logger::getInstance().debug("AI: Depth {} completed - {} nodes, {} qnodes, {} TT hits, {}ms",
                depth, stats->nodes, stats->qnodes, stats->ttHits, stats->timeMs);
        }
    }

    if (best) {
        // Final summary log
        if (stats) {
            Logger::getInstance().info("AI: Search complete - depth {}, {} nodes, score {}, {}ms",
                stats->depthReached, stats->nodes, bestScore, stats->timeMs);
        }
        return best;
    }

    SearchStats::setEmpty(stats, start);
    return std::nullopt;
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

// --- Stubs for private methods declared in MinimaxSearch.hpp ---

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

bool MinimaxSearch::runDepth(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx)
{
    if (ctx.isTimeUp())
        return false;

    // Probe TT for this root position
    std::optional<Move> ttRootMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    bool ttHit = search::ttProbe(tt, board, depth, -search::INF, search::INF, ttScore, ttRootMove, ttFlag);

    if (ttHit)
        ctx.recordTTHit();

    auto ordered = orderMoves(board, rules, toPlay, ttRootMove, depth);
    if (ordered.empty())
        ordered = rootCandidates; // fallback

    int alpha = -search::INF, beta = search::INF;
    std::optional<Move> depthBest;
    int depthBestScore = -search::INF;
    std::vector<Move> depthPV;

    for (const auto& m : ordered) {
        if (ctx.isTimeUp())
            break;
        auto pr = board.tryPlay(m, rules);
        if (!pr.success)
            continue;
        std::vector<Move> childPV;
        int childScore = negamax(board, depth - 1, -beta, -alpha, /*ply*/ 1, childPV, ctx);
        int score = -childScore;
        board.undo();

        if (score > depthBestScore) {
            depthBestScore = score;
            depthBest = m;
            depthPV.clear();
            depthPV.push_back(m);
            depthPV.insert(depthPV.end(), childPV.begin(), childPV.end());
        }
        if (score > alpha)
            alpha = score;
    }

    if (!depthBest)
        return false;

    best = depthBest;
    bestScore = depthBestScore;
    pv = depthPV;

    // Store result in TT
    search::ttStore(tt, board, depth, bestScore, TranspositionTable::Flag::Exact, best);

    return true;
}
} // namespace gomoku

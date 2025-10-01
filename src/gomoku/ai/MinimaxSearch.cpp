// MinimaxSearch.cpp
#include "gomoku/ai/MinimaxSearch.hpp"
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/ai/Evaluator.hpp"
#include "gomoku/ai/SearchHelpers.hpp"

// --- Helpers extracted from bestMove ---
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
    if (auto iw = tryImmediateWinShortcut(board, rules, toPlay, candidates)) {
        if (stats) {
            ctx.recordNode(); // Count the immediate win node
            stats->finalize(start, /*depth*/ 1, { *iw });
        }
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
        if (stats)
            stats->finalize(start, depth, pv);
    }

    if (best) {
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

// Négamax récursif (Gomoku) avec extensions possibles plus tard (alpha-bêta/PVS/TT).
// Rôle:
//  - Arrêt immédiat si état terminal (cinq alignés, victoire par captures, nul).
//  - En feuille (profondeur 0): renvoyer evaluate(...).
//  - Sinon: ordonner les coups via orderMoves(...) en privilégiant les menaces Gomoku
//    (gains en 1, parades de menaces adverses, créations de quatre ouverts, captures de paires critiques),
//    puis explorer récursivement (tryPlay → negamax → undo), construire la PV.
// TODO (palier 3): implémentation depth-limitée simple (sans alpha-bêta), puis ajouter alpha-bêta/PVS.
int MinimaxSearch::negamax(Board& board, int depth, int alpha, int beta, int ply, std::vector<Move>& pvOut, const SearchContext& ctx)
{
    // TODO: Terminal check, éval en feuille, génération + boucle enfants (tryPlay/undo), build PV.
    // TODO (plus tard): alpha-bêta/PVS, TT probe/store, extensions sur menaces (quatre ouvert, capture gagnante), LMR ciblé.
    (void)board;
    (void)depth;
    (void)alpha;
    (void)beta;
    (void)ply;
    (void)ctx;
    pvOut.clear();
    return 0;
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
    // TODO: Stand pat, delta pruning, génération coups tactiques, tryPlay/undo, récursif, alpha/beta update.
    (void)board;
    (void)alpha;
    (void)beta;
    (void)ply;
    (void)ctx;
    return 0;
}

// Ordonne les coups à explorer (Gomoku):
//  - 1) Coups gagnants immédiats (faire 5, capture gagnante),
//  - 2) Parades immédiates (bloquer 5 adverse, empêcher capture gagnante),
//  - 3) Création de quatre ouverts / blocage des quatre adverses,
//  - 4) Captures de paires critiques,
//  - 5) Extensions de menaces (étendre 3→4, 4→5) près du front,
//  - 6) Heuristique géométrique (proximité des pierres existantes), killers/history en option.
// TODO: placer ttMove en tête si dispo, puis classer par criticité des menaces/captures; plafonner à N coups pour maitriser le branching.
std::vector<Move> MinimaxSearch::orderMoves(const Board& board, const RuleSet& rules, Player toMove, const std::optional<Move>& ttMove) const
{
    // TODO: Placer ttMove en tête, réordonner par menaces/captures Gomoku, limiter à un top-N.
    (void)ttMove;
    return orderedMovesPublic(board, rules, toMove);
}

// Renvoie true si le temps est écoulé ou nodeCap atteint (soft stop).
// Note: nodeCap doit être incrémenté côté recherche (negamax/qsearch) pour être effectif.
bool MinimaxSearch::cutoffByTime(const SearchContext& ctx) const
{
    // TODO: Ajouter nodeCap check si besoin.
    return ctx.isTimeUp();
}

// --- Helpers extracted from bestMove ---
// Raccourci “gain immédiat” (Gomoku):
//  - Ne teste que si plausible: ≥4 pierres posées (alignement possible en 1) ou ≥4 paires capturées (capture-win possible).
//  - Joue spéculativement chaque candidat; si status devient WinByAlign/WinByCapture, retourne ce coup immédiatement.
//  - Laisse les règles gérer les interdits (double-trois, overline (6+)) via tryPlay/status.
std::optional<Move> MinimaxSearch::tryImmediateWinShortcut(Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& candidates) const
{

    const bool plausibleAlign = board.stoneCount(toPlay) >= 4;
    const auto caps = board.capturedPairs();
    const int pairs = (toPlay == Player::Black) ? caps.black : caps.white;
    const bool plausibleCaptureWin = pairs >= 4;

    // Only attempt if at least one path to immediate win is plausible
    if (!plausibleAlign && !plausibleCaptureWin)
        return std::nullopt;

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

bool MinimaxSearch::runDepth(int depth, Board& board, const RuleSet& rules, Player toPlay, const std::vector<Move>& rootCandidates, std::optional<Move>& best, int& bestScore, std::vector<Move>& pv, const SearchContext& ctx)
{
    if (cutoffByTime(ctx) || Clock::now() >= ctx.deadline)
        return false;

    // Probe TT for this root position
    std::optional<Move> ttRootMove;
    int ttScore = 0;
    TranspositionTable::Flag ttFlag = TranspositionTable::Flag::Exact;
    bool ttHit = search::ttProbe(tt, board, depth, -search::INF, search::INF, ttScore, ttRootMove, ttFlag);

    if (ttHit) {
        ctx.recordTTHit();
        std::string flagStr = ttFlag == TranspositionTable::Flag::Exact ? "Exact" : ttFlag == TranspositionTable::Flag::Lower ? "Lower"
                                                                                                                              : "Upper";
        Logger::getInstance().info("[TT] Hit at depth {} - score: {}, flag: {}, move: {}",
            depth, ttScore, flagStr,
            ttRootMove ? "available" : "none");
    } else if (ttRootMove) {
        Logger::getInstance().info("[TT] Partial hit at depth {} - move available but no cutoff", depth);
    }

    auto ordered = orderMoves(board, rules, toPlay, ttRootMove);
    if (ordered.empty())
        ordered = rootCandidates; // fallback

    int alpha = -search::INF, beta = search::INF;
    std::optional<Move> depthBest;
    int depthBestScore = -search::INF;
    std::vector<Move> depthPV;

    for (const auto& m : ordered) {
        if (cutoffByTime(ctx) || Clock::now() >= ctx.deadline)
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
    Logger::getInstance().info("[TT] Stored at depth {} - score: {}, move: ({},{})",
        depth, bestScore,
        static_cast<int>(best->pos.x),
        static_cast<int>(best->pos.y));

    return true;
}
} // namespace gomoku

#include "gomoku/ai/MoveOrderer.hpp"
#include "gomoku/ai/CandidateGenerator.hpp"
#include "gomoku/ai/Evaluator.hpp"
#include "util/Logger.hpp"
#include <algorithm>
#include <limits>

namespace gomoku {
MoveOrderer::ScopedPlay::ScopedPlay(Board& board, const Move& move, const RuleSet& rules)
    : b(board)
{
    auto pr = b.tryPlay(move, rules);
    ok = pr.success;
}
MoveOrderer::ScopedPlay::~ScopedPlay()
{
    if (ok)
        b.undo();
}

void MoveOrderer::ensureCapacity(int maxPly)
{
    killerStride_ = MAX_KILLERS;
    if ((int)killers_.size() < maxPly * killerStride_)
        killers_.resize(maxPly * killerStride_, Move {});
    if ((int)history_.size() < 2 * BOARD_SIZE * BOARD_SIZE)
        history_.resize(2 * BOARD_SIZE * BOARD_SIZE, 0);
}

void MoveOrderer::clearForNewIteration(int maxPly)
{
    ensureCapacity(maxPly);
    std::fill(killers_.begin(), killers_.end(), Move {});
    std::fill(history_.begin(), history_.end(), 0);
}

void MoveOrderer::pushKiller(int ply, const Move& m)
{
    if (ply < 0 || (ply * killerStride_ + MAX_KILLERS - 1) >= (int)killers_.size())
        return;
    auto& k0 = killers_[idxKiller(ply, 0)];
    auto& k1 = killers_[idxKiller(ply, 1)];
    if (k0.pos == m.pos)
        return;
    k1 = k0;
    k0 = m;
}

void MoveOrderer::onBetaCut(int ply, const Move& m)
{
    ensureCapacity(ply + 1);
    pushKiller(ply, m);
    // History boost encore augmenté pour impact maximal
    int& h = history_[idxHistory(m.by, m.pos)];
    h = std::min(h + 256, 100'000); // Reduced max from 2M to 100k to prevent overshadowing tactical eval
}

int MoveOrderer::capForDepth(int depth) const
{
    if (depth >= 8)
        return cfg_.capDeepRoot;
    if (depth >= 5)
        return cfg_.capMid;
    if (depth >= 2)
        return cfg_.capShallow;
    return cfg_.capNearLeaf;
}

std::vector<Move> MoveOrderer::order(Board& board,
    const RuleSet& rules,
    Player toMove,
    int depth,
    const std::optional<Move>& ttMove,
    const eval::Evaluator& evaluator,
    const std::vector<Move>* baseMoves)
{
    // 1) Seed
    std::vector<Move> moves;
    if (baseMoves && !baseMoves->empty()) {
        moves = *baseMoves;
    } else {
        moves = CandidateGenerator::generate(board, rules, toMove, CandidateConfig {});
        if (moves.empty())
            moves = board.legalMoves(toMove, rules);
    }
    if (moves.empty())
        return moves;

    // 2) TT-first (si présent dans la liste)
    if (cfg_.forceTTFirst && ttMove && ttMove->isValid()) {
        auto it = std::find_if(moves.begin(), moves.end(),
            [&ttMove](const Move& m) { return m.pos == ttMove->pos; });
        if (it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        }
    }

    // 3) Score spéculatif + bonus heuristiques (killers/history)
    std::vector<Scored> scored;
    scored.reserve(moves.size());

    const bool ttFirst = (cfg_.forceTTFirst && ttMove && ttMove->isValid() && !moves.empty() && moves.front().pos == ttMove->pos);
    const size_t start = ttFirst ? 1 : 0;

    for (size_t i = start; i < moves.size(); ++i) {
        const Move& m = moves[i];

        // Essayage spéculatif
        ScopedPlay g(board, m, rules);
        if (!g.ok)
            continue;

        int s;
        if (board.status() == GameStatus::WinByAlign || board.status() == GameStatus::WinByCapture) {
            s = cfg_.winScore; // C'est un win pour toMove
        } else {
            // CORRECTION: evaluate() retourne un score du point de vue du joueur passé
            // On veut le score de toMove, donc on passe toMove (pas board.toPlay() qui a changé!)
            s = evaluator.evaluate(board, toMove);
        }
        // History bonus - diviseur réduit pour impact fort
        ensureCapacity(1); // s'assure que history_ existe
        s += history_[idxHistory(m.by, m.pos)] / 10; // Scaled to max ~10,000

        scored.push_back({ m, s, /*tie*/ 0 });
    }

    std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
        if (a.s != b.s)
            return a.s > b.s;
        return a.tie > b.tie;
    });

    // 4) Cap
    const int cap = capForDepth(depth);
    std::vector<Move> out;
    out.reserve((ttFirst ? 1 : 0) + std::min((int)scored.size(), cap));
    if (ttFirst)
        out.push_back(moves.front());
    for (int i = 0; i < (int)scored.size() && (int)out.size() < (ttFirst ? 1 : 0) + cap; ++i)
        out.push_back(scored[i].m);

    return out;
}

} // namespace gomoku

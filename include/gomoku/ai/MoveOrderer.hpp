#pragma once
#include "gomoku/core/Board.hpp"
#include "gomoku/core/Types.hpp" // Move, Player, Pos, BOARD_SIZE...
#include <cstdint>
#include <optional>
#include <vector>

namespace gomoku {
struct MoveOrdererConfig {
    // Caps augmentés drastiquement pour explorer tous les coups prometteurs
    int capDeepRoot = 40; // depth >= 8 - augmenté de 30 à 40
    int capMid = 30; // depth >= 5 - augmenté de 20 à 30
    int capShallow = 22; // depth >= 2 - augmenté de 15 à 22
    int capNearLeaf = 15; // depth  < 2 - augmenté de 10 à 15

    int winScore = 1'000'000; // score utilisé pour un gain immédiat
    bool forceTTFirst = true; // pousse TT move devant si présent
};

class MoveOrderer {
public:
    explicit MoveOrderer(const MoveOrdererConfig& cfg = {})
        : cfg_(cfg)
    {
    }

    // Unifie root et sous-arbre : si baseMoves est fourni, on réordonne cette base.
    // Sinon on génère avec CandidateGenerator/Board.
    std::vector<Move> order(Board& board,
        const RuleSet& rules,
        Player toMove,
        int depth,
        const std::optional<Move>& ttMove,
        const std::vector<Move>* baseMoves = nullptr);

    // Hooks heuristiques (à appeler depuis la recherche) :
    void onBetaCut(int ply, const Move& m); // killer + history++
    void onFailLow(int ply, const std::vector<Move>&); // optionnel: history--
    void clearForNewIteration(int maxPly); // reset killers/history

private:
    struct ScopedPlay {
        Board& b;
        bool ok { false };
        ScopedPlay(Board& board, const Move& m, const RuleSet& rules);
        ~ScopedPlay();
        ScopedPlay(const ScopedPlay&) = delete;
        ScopedPlay& operator=(const ScopedPlay&) = delete;
    };

    struct Scored {
        Move m;
        int s;
        int tie;
    }; // tie pour heuristiques secondaires

    const MoveOrdererConfig cfg_;

    // Heuristiques d’état (par itération / par thread)
    static constexpr int MAX_KILLERS = 2;
    std::vector<Move> killers_; // taille = maxPly * MAX_KILLERS
    int killerStride_ = 0;

    // History simple: par (player, case). On part large, tu pourras affiner.
    std::vector<int> history_; // taille = 2 * BOARD_SIZE * BOARD_SIZE

    inline int idxHistory(Player p, const Pos& pos) const
    {
        const int side = (p == Player::Black ? 0 : 1);
        return side * (BOARD_SIZE * BOARD_SIZE) + pos.y * BOARD_SIZE + pos.x;
    }
    inline int idxKiller(int ply, int slot) const { return ply * MAX_KILLERS + slot; }

    void ensureCapacity(int maxPly);
    void pushKiller(int ply, const Move& m);
    int capForDepth(int depth) const;
    void dedupeLinear(std::vector<Move>& moves) const;
};

} // namespace gomoku

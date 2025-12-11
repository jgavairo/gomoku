#pragma once
#include "gomoku/core/Types.hpp"

namespace gomoku {
class Board;
}

namespace gomoku::eval {

struct EvalConfig {
    int capturePairValue = 8000;
    int centerBase = 10;
    int centerWeight = 3;
    int frontBase = 6;
    int frontWeight = 5;

    // Pattern values
    int winValue = 100000;
    int openFour = 40000;
    int closedFour = 6000;
    int openThree = 3000;
    int closedThree = 500;
    int openTwo = 200;
    int closedTwo = 50;
    int openOne = 20;
    int closedOne = 5;

    // Strategic bonuses
    int doubleOpenFour = 50000;
    int openFourThree = 20000;
    int doubleOpenThree = 12000;
    int openThreeClosedFour = 5000;
    int tripleOpenThree = 15000;

    // Capture potential
    int captureSetupBonus = 400;
    int captureSetupPenalty = 600;
};

class Evaluator {
public:
    explicit Evaluator(const EvalConfig& cfg = {})
        : cfg_(cfg)
    {
    }

    // Évaluation statique rapide d'une position (Gomoku)
    int evaluate(const Board& board, Player perspective) const noexcept;

    void setConfig(const EvalConfig& cfg) { cfg_ = cfg; }
    const EvalConfig& getConfig() const { return cfg_; }

private:
    EvalConfig cfg_;
};

} // namespace gomoku::eval
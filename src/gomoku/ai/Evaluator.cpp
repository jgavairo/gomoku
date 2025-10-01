// Evaluator.cpp - Static position evaluation for Gomoku
#include "gomoku/ai/Evaluator.hpp"
#include "gomoku/core/Board.hpp"
#include <algorithm>
#include <cstdlib>

namespace gomoku::eval {

int evaluate(const Board& board, Player perspective) noexcept
{
    // Safety: terminal states are handled by isTerminal() in search, but keep neutral for draws here.
    if (board.status() == GameStatus::Draw)
        return 0;

    const Cell me = playerToCell(perspective);
    const Cell opp = playerToCell(opponent(perspective));

    int score = 0;

    // 1) Captures differential (pairs). Each pair is valuable tactically.
    const auto caps = board.capturedPairs();
    const int capDiff = (perspective == Player::Black) ? (caps.black - caps.white) : (caps.white - caps.black);
    constexpr int CAPTURE_PAIR_VALUE = 3000; // tuned later
    score += capDiff * CAPTURE_PAIR_VALUE;

    // 2) Centrality (manhattan distance to center). Encourages occupying the center early.
    constexpr int cx = BOARD_SIZE / 2;
    constexpr int cy = BOARD_SIZE / 2;
    constexpr int CENTER_BASE = 10; // max shells counted
    constexpr int CENTER_WEIGHT = 3; // per-shell weight multiplier
    int central = 0;
    const auto& occ = board.occupiedPositions();
    for (const auto& p : occ) {
        const int x = p.x, y = p.y;
        const Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        const int md = std::abs(x - cx) + std::abs(y - cy);
        const int w = std::max(0, CENTER_BASE - md);
        if (c == me)
            central += w;
        else if (c == opp)
            central -= w;
    }
    score += central * CENTER_WEIGHT;

    // 2b) Front proximity: bias towards stones near the recent front (last 3 moves, weighted).
    {
        constexpr int FRONT_BASE = 6; // radius-like shells
        constexpr int FRONT_WEIGHT = 5; // final multiplier
        // Weights for last moves: most recent gets highest weight
        constexpr int W1 = 3, W2 = 2, W3 = 1; // sum = 6
        const auto recents = board.lastMoves(3);
        if (!recents.empty()) {
            int frontAccum = 0;
            int weightSum = 0;
            for (std::size_t i = 0; i < recents.size(); ++i) {
                const int wMove = (i == 0 ? W1 : (i == 1 ? W2 : W3));
                weightSum += wMove;
                const int lx = recents[i].pos.x;
                const int ly = recents[i].pos.y;
                int frontLocal = 0;
                for (const auto& p : occ) {
                    const int x = p.x, y = p.y;
                    const int md = std::abs(x - lx) + std::abs(y - ly);
                    if (md > FRONT_BASE)
                        continue;
                    const Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
                    const int w = FRONT_BASE - md;
                    if (c == me)
                        frontLocal += w;
                    else if (c == opp)
                        frontLocal -= w;
                }
                frontAccum += frontLocal * wMove;
            }
            // Divide by sum of move weights (6) to get an average-like effect
            score += (frontAccum / (W1 + W2 + W3)) * FRONT_WEIGHT;
        }
    }

    // 3) Pattern runs in 4 directions (open/closed 2/3/4, 5+).
    auto runValue = [](int len, int openEnds) -> int {
        if (len >= 5)
            return 100000; // effectively winning pattern (search should catch terminal earlier)
        switch (len) {
        case 4:
            return (openEnds >= 2) ? 10000 : 2500;
        case 3:
            return (openEnds >= 2) ? 600 : 150;
        case 2:
            return (openEnds >= 2) ? 80 : 20;
        case 1:
        default:
            return (openEnds >= 2) ? 10 : 2;
        }
    };

    constexpr int DIRS[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
    int patternScore = 0;
    for (const auto& p : occ) {
        const int x = p.x, y = p.y;
        const Cell c = board.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        for (const auto& d : DIRS) {
            const int dx = d[0], dy = d[1];
            const int prevX = x - dx, prevY = y - dy;
            // Only start at the beginning of a run for this direction
            if (prevX >= 0 && prevX < BOARD_SIZE && prevY >= 0 && prevY < BOARD_SIZE) {
                if (board.at(static_cast<uint8_t>(prevX), static_cast<uint8_t>(prevY)) == c)
                    continue;
            }
            // Count run length
            int len = 0;
            int nx = x, ny = y;
            while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board.at(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)) == c) {
                ++len;
                nx += dx;
                ny += dy;
            }
            // Determine openness at both ends
            int openEnds = 0;
            if (prevX >= 0 && prevX < BOARD_SIZE && prevY >= 0 && prevY < BOARD_SIZE) {
                if (board.at(static_cast<uint8_t>(prevX), static_cast<uint8_t>(prevY)) == Cell::Empty)
                    ++openEnds;
            } else {
                // Off-board is closed
            }
            if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                if (board.at(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)) == Cell::Empty)
                    ++openEnds;
            } else {
                // Off-board is closed
            }

            const int val = runValue(len, openEnds);
            if (c == me)
                patternScore += val;
            else if (c == opp)
                patternScore -= val;
        }
    }
    score += patternScore;

    return score;
}

} // namespace gomoku::eval

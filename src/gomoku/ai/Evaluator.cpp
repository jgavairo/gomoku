// Evaluator.cpp - Static position evaluation for Gomoku
#include "gomoku/ai/Evaluator.hpp"
#include "gomoku/core/Board.hpp"
#include <algorithm>
#include <cstdlib>

namespace gomoku::eval {

namespace {
    // Helpers: Boundary and cell checks (wrappers for Board API with arithmetic-friendly overloads)

    // Check if position is valid - for signed int (used after arithmetic with deltas)
    inline bool inside(int x, int y) noexcept
    {
        return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
    }

    // Check if cell is empty - for signed int coordinates
    inline bool isEmpty(const Board& board, int x, int y) noexcept
    {
        return inside(x, y) && board.isEmpty(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
    }

    // Helper: Count empty spaces in a direction from a adjacent position
    inline int countEmptyRay(const Board& b, int x, int y, int dx, int dy, int cap = 4) noexcept
    {
        int n = 0;
        int nx = x, ny = y; // ← on commence à la case adjacente
        while (n < cap && isEmpty(b, nx, ny)) {
            ++n;
            nx += dx;
            ny += dy;
        }
        return n;
    }

    // Helper: Detect capture threat pattern _OOX or XOO_ (opponent stones can be captured)
    inline bool hasCapturePattern(const Board& b, int x, int y, int dx, int dy, Cell me, Cell opp) noexcept
    {
        // Pattern: X O O _ - can capture if we play at the empty spot
        if (inside(x + 3 * dx, y + 3 * dy)) {
            Cell c1 = b.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
            Cell c2 = b.at(static_cast<uint8_t>(x + dx), static_cast<uint8_t>(y + dy));
            Cell c3 = b.at(static_cast<uint8_t>(x + 2 * dx), static_cast<uint8_t>(y + 2 * dy));
            if (c1 == me && c2 == opp && c3 == opp && isEmpty(b, x + 3 * dx, y + 3 * dy))
                return true;
        }
        // Pattern: _ O O X - can capture if we play at the empty spot
        if (inside(x - 3 * dx, y - 3 * dy)) {
            Cell c1 = b.at(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
            Cell c2 = b.at(static_cast<uint8_t>(x - dx), static_cast<uint8_t>(y - dy));
            Cell c3 = b.at(static_cast<uint8_t>(x - 2 * dx), static_cast<uint8_t>(y - 2 * dy));
            if (c1 == me && c2 == opp && c3 == opp && isEmpty(b, x - 3 * dx, y - 3 * dy))
                return true;
        }
        return false;
    }

    // Helper: Detect if an alignment can potentially extend to 5
    inline bool canExtendToFive(int len, int spaceBefore, int spaceAfter)
    {
        if (len >= 5)
            return true; // Already 5
        return (len + spaceBefore >= 5) || (len + spaceAfter >= 5) || (len + spaceBefore + spaceAfter >= 5);
    }

    // Helper: Assess freedom level of an alignment
    enum class Freedom { Flanked,
        HalfFree,
        Free };
    inline Freedom assessFreedom(int openEnds, int spaceBefore, int spaceAfter)
    {
        if (openEnds == 2)
            return Freedom::Free;
        if (openEnds == 1 && (spaceBefore >= 2 || spaceAfter >= 2))
            return Freedom::HalfFree;
        return Freedom::Flanked;
    }
}

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
    constexpr int CAPTURE_PAIR_VALUE = 4500; // Increased to prioritize captures over simple blocks
    score += capDiff * CAPTURE_PAIR_VALUE;

    // 2) Centrality (manhattan distance to center). Encourages occupying the center early.
    constexpr int cx = BOARD_SIZE / 2;
    constexpr int cy = BOARD_SIZE / 2;
    constexpr int CENTER_BASE = 10; // max shells counted
    constexpr int CENTER_WEIGHT = 3; // per-shell weight multiplier
    int central = 0;
    const auto& occ = board.occupiedPositions();
    for (const auto& p : occ) {
        const Cell c = board.at(p.x, p.y);
        const int md = std::abs(static_cast<int>(p.x) - cx) + std::abs(static_cast<int>(p.y) - cy);
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
                    const int md = std::abs(static_cast<int>(p.x) - lx) + std::abs(static_cast<int>(p.y) - ly);
                    if (md > FRONT_BASE)
                        continue;
                    const Cell c = board.at(p.x, p.y);
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
    // Enhanced with freedom and potential win assessment
    auto runValue = [](int len, int openEnds, Freedom freedom, bool canWin) -> int {
        if (len >= 5)
            return 100000; // effectively winning pattern (search should catch terminal earlier)

        // Base values for alignments
        int baseValue = 0;
        switch (len) {
        case 4:
            baseValue = (openEnds >= 2) ? 40000 : 6000;
            break;
        case 3:
            baseValue = (openEnds >= 2) ? 3000 : 500;
            break;
        case 2:
            baseValue = (openEnds >= 2) ? 200 : 50;
            break;
        case 1:
        default:
            baseValue = (openEnds >= 2) ? 20 : 5;
            break;
        }

        // Bonus for freedom level
        int adjustedValue = baseValue;
        if (freedom == Freedom::Free) {
            adjustedValue = (baseValue * 13) / 10; // 30% bonus for totally free alignments
        } else if (freedom == Freedom::HalfFree) {
            adjustedValue = (baseValue * 11) / 10; // 10% bonus for half-free
        }

        // Penalty if cannot extend to 5
        if (!canWin && len < 5) {
            adjustedValue = (adjustedValue * 3) / 10; // Severe penalty for dead-end alignments
        }

        return adjustedValue;
    };

    constexpr int DIRS[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
    int patternScore = 0;
    int potentialCaptureScore = 0;

    // 4) Strategic figures (double threats, forks)
    // Count threatening patterns per player
    int myThreats[5] = { 0 }; // Index: [open_fours, closed_fours, open_threes, closed_threes, twos]
    int oppThreats[5] = { 0 };

    // UNIFIED LOOP: Count patterns, score runs, detect threats and capture patterns in one pass
    for (const auto& p : occ) {
        const Cell c = board.at(p.x, p.y);
        const int x = static_cast<int>(p.x);
        const int y = static_cast<int>(p.y);

        for (const auto& d : DIRS) {
            const int dx = d[0], dy = d[1];
            const int prevX = x - dx, prevY = y - dy;

            // Only start at the beginning of a run for this direction
            if (inside(prevX, prevY)) {
                if (board.at(static_cast<uint8_t>(prevX), static_cast<uint8_t>(prevY)) == c)
                    continue;
                // Avoid double counting split patterns (if we are the second part of X _ X)
                if (isEmpty(board, prevX, prevY) && inside(prevX - dx, prevY - dy) && board.at(static_cast<uint8_t>(prevX - dx), static_cast<uint8_t>(prevY - dy)) == c)
                    continue;
            }

            // Count run length
            int len = 0;
            int nx = x, ny = y;
            while (inside(nx, ny) && board.at(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)) == c) {
                ++len;
                nx += dx;
                ny += dy;
            }

            const bool leftOpen = isEmpty(board, prevX, prevY);
            const bool rightOpen = isEmpty(board, nx, ny);

            // Check for split pattern (broken four/five)
            int len2 = 0;
            int nx2 = nx + dx;
            int ny2 = ny + dy;
            if (rightOpen) {
                while (inside(nx2, ny2) && board.at(static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2)) == c) {
                    ++len2;
                    nx2 += dx;
                    ny2 += dy;
                }
            }

            const int leftSpace = leftOpen ? countEmptyRay(board, prevX, prevY, -dx, -dy) : 0;
            const int rightSpace = rightOpen ? countEmptyRay(board, nx, ny, dx, dy) : 0;

            const int openEnds = (leftOpen ? 1 : 0) + (rightOpen ? 1 : 0);

            // Peut-on atteindre 5 ? (les "spaces" incluent maintenant la case adjacente)
            const bool canWin = (len >= 5) || (len + leftSpace + rightSpace >= 5);

            Freedom freedom = assessFreedom(openEnds, leftSpace, rightSpace);

            // Score the pattern run
            const int val = runValue(len, openEnds, freedom, canWin);
            if (c == me)
                patternScore += val;
            else if (c == opp)
                patternScore -= val;

            // Score split patterns
            if (len2 > 0) {
                int splitTotal = len + len2;
                int splitVal = 0;
                int* threats = (c == me) ? myThreats : oppThreats;

                if (splitTotal >= 4) {
                    // Equivalent to a closed four (one winning spot in the gap)
                    splitVal = 8000;
                    threats[1]++; // closed_four
                } else if (splitTotal == 3) {
                    // Check if the far end is open
                    bool rightOpen2 = inside(nx2, ny2) && isEmpty(board, nx2, ny2);
                    int splitEnds = (leftOpen ? 1 : 0) + (rightOpen2 ? 1 : 0);

                    if (splitEnds >= 2) {
                        // Open broken three: . X X . X . -> becomes Open Four
                        splitVal = 2000;
                        threats[2]++; // open_three
                    } else {
                        // Closed broken three
                        splitVal = 500;
                        threats[3]++; // closed_three
                    }
                }

                if (c == me)
                    patternScore += splitVal;
                else if (c == opp)
                    patternScore -= splitVal;
            }

            // Check for capture patterns (X_OOX)
            if (c == me && hasCapturePattern(board, x, y, dx, dy, me, opp)) {
                potentialCaptureScore += 400; // Bonus for potential capture setup
            } else if (c == opp && hasCapturePattern(board, x, y, dx, dy, opp, me)) {
                potentialCaptureScore -= 600; // Penalty if opponent has capture setup
            }

            // Classify threat for strategic combinations
            int* threats = (c == me) ? myThreats : oppThreats;
            if (len == 4) {
                threats[(openEnds >= 2) ? 0 : 1]++; // open_four or closed_four
            } else if (len == 3) {
                threats[(openEnds >= 2) ? 2 : 3]++; // open_three or closed_three
            } else if (len == 2 && openEnds >= 2) {
                threats[4]++; // open_two
            }
        }
    }

    score += patternScore;
    score += potentialCaptureScore;

    // Evaluate strategic combinations
    int figureBonus = 0;

    // Double open-four: instant win threat
    if (myThreats[0] >= 2)
        figureBonus += 50000;
    if (oppThreats[0] >= 2)
        figureBonus -= 50000;

    // Open-four + open-three: very strong
    if (myThreats[0] >= 1 && myThreats[2] >= 1)
        figureBonus += 20000;
    if (oppThreats[0] >= 1 && oppThreats[2] >= 1)
        figureBonus -= 20000;

    // Double open-three: strong fork
    if (myThreats[2] >= 2)
        figureBonus += 12000;
    if (oppThreats[2] >= 2)
        figureBonus -= 12000;

    // Open-three + closed-four: forcing sequence
    if (myThreats[0] >= 1 && myThreats[3] >= 1)
        figureBonus += 5000;
    if (oppThreats[0] >= 1 && oppThreats[3] >= 1)
        figureBonus -= 5000;

    // Multiple open-threes (3+): overwhelming position
    if (myThreats[2] >= 3)
        figureBonus += 15000;
    if (oppThreats[2] >= 3)
        figureBonus -= 15000;

    score += figureBonus;

    return score;
}

} // namespace gomoku::eval
#include "gomoku/core/PatternAnalyzer.hpp"
#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/RayTables.hpp"

namespace gomoku::pattern {

namespace {
    constexpr uint16_t idx(uint8_t x, uint8_t y)
    {
        return static_cast<uint16_t>(y * BOARD_SIZE + x);
    }

    inline bool isInside(uint8_t x, uint8_t y) { return x < BOARD_SIZE && y < BOARD_SIZE; }
}

bool createsIllegalDoubleThree(const BoardState& state, Move m, const RuleSet& rules)
{
    if (!rules.forbidDoubleThree)
        return false;

    if (rules.capturesEnabled && capture::wouldCapture(state, m))
        return false;

    const Cell ME = playerToCell(m.by);
    const Cell OP = (ME == Cell::Black ? Cell::White : Cell::Black);

    uint16_t capturedIdx[8];
    int captured_n = 0;

    const int idx0 = m.pos.y * BOARD_SIZE + m.pos.x;
    for (int d = 0; d < 4; ++d) {
        const auto& ray = rays::capRaysByDir[d][idx0];

        if (ray.fwd[2] != 0xFFFF && state.cells[ray.fwd[0]] == OP && state.cells[ray.fwd[1]] == OP && state.cells[ray.fwd[2]] == ME) {
            capturedIdx[captured_n++] = ray.fwd[0];
            capturedIdx[captured_n++] = ray.fwd[1];
        }
        if (ray.bwd[2] != 0xFFFF && state.cells[ray.bwd[0]] == OP && state.cells[ray.bwd[1]] == OP && state.cells[ray.bwd[2]] == ME) {
            capturedIdx[captured_n++] = ray.bwd[0];
            capturedIdx[captured_n++] = ray.bwd[1];
        }
    }

    auto isVirtuallyCaptured = [&](uint16_t id) noexcept {
        for (int i = 0; i < captured_n; ++i)
            if (capturedIdx[i] == id)
                return true;
        return false;
    };

    auto vAt = [&](int x, int y) noexcept -> Cell {
        if ((unsigned)x >= BOARD_SIZE || (unsigned)y >= BOARD_SIZE)
            return OP;
        const uint16_t id = static_cast<uint16_t>(y * BOARD_SIZE + x);
        if (id == idx0)
            return ME;
        if (isVirtuallyCaptured(id))
            return Cell::Empty;
        return state.cells[id];
    };

    auto hasThreeInLine = [&](int dx, int dy) noexcept -> bool {
        int C[9];
        for (int off = -4, i = 0; off <= 4; ++off, ++i) {
            const int x = (int)m.pos.x + off * dx;
            const int y = (int)m.pos.y + off * dy;
            Cell c = vAt(x, y);
            C[i] = (c == Cell::Empty) ? 0 : (c == ME ? 1 : 2);
        }
        auto Z = [&](int k) noexcept { return C[k + 4] == 0; };
        auto O = [&](int k) noexcept { return C[k + 4] == 1; };

        if (Z(-1) && O(0) && O(+1) && O(+2) && Z(+3))
            return true;
        if (Z(-2) && O(-1) && O(0) && O(+1) && Z(+2))
            return true;
        if (Z(-3) && O(-2) && O(-1) && O(0) && Z(+1))
            return true;

        if (Z(-1) && O(0) && Z(+1) && O(+2) && O(+3) && Z(+4))
            return true;
        if (Z(-3) && O(-2) && Z(-1) && O(0) && O(+1) && Z(+2))
            return true;
        if (Z(-4) && O(-3) && O(-2) && Z(-1) && O(0) && Z(+1))
            return true;

        if (Z(-1) && O(0) && O(+1) && Z(+2) && O(+3) && Z(+4))
            return true;
        if (Z(-2) && O(-1) && O(0) && Z(+1) && O(+2) && Z(+3))
            return true;
        if (Z(-4) && O(-3) && O(-2) && Z(-1) && O(0) && Z(+1))
            return true;

        return false;
    };

    int threes = 0;
    if (hasThreeInLine(1, 0) && ++threes == 2)
        return true;
    if (hasThreeInLine(0, 1) && ++threes == 2)
        return true;
    if (hasThreeInLine(1, 1) && ++threes == 2)
        return true;
    if (hasThreeInLine(1, -1) && ++threes == 2)
        return true;

    return false;
}

bool checkFiveOrMoreFrom(const BoardState& state, Pos p, Cell who)
{
    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    auto at = [&](uint8_t x, uint8_t y) { return state.cells[idx(x, y)]; };
    for (int d = 0; d < 4; ++d) {
        int count = 1;
        for (int s = -1; s <= 1; s += 2) {
            int x = p.x, y = p.y;
            while (true) {
                x += s * DX[d];
                y += s * DY[d];
                if (!isInside((uint8_t)x, (uint8_t)y))
                    break;
                if (at((uint8_t)x, (uint8_t)y) == who)
                    ++count;
                else
                    break;
            }
        }
        if (count >= 5)
            return true;
    }
    return false;
}

bool hasAnyFive(const BoardState& state, Cell who)
{
    for (const auto& p : state.occupied_) {
        if (state.cells[idx(p.x, p.y)] == who) {
            if (checkFiveOrMoreFrom(state, p, who))
                return true;
        }
    }
    return false;
}

bool isFiveBreakableNow(const BoardState& stateIn, Player justPlayed, const RuleSet& rules)
{
    if (!rules.capturesEnabled)
        return false;

    Player opp = (justPlayed == Player::Black ? Player::White : Player::Black);
    Cell meC = playerToCell(justPlayed);

    // Copy state into a BoardState+context surrogate
    BoardState state = stateIn; // shallow copy (arrays/vectors copied)

    // We need side-to-move concept because capture::applyCapturesAround and hash/side don't depend on it.
    // Build candidate empties adjacent in aligned directions to stones of justPlayed (meC).
    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    std::vector<uint8_t> seen(static_cast<std::size_t>(BOARD_SIZE * BOARD_SIZE), 0);
    std::vector<Pos> cand;
    cand.reserve(128);
    auto at = [&](uint8_t x, uint8_t y) { return state.cells[idx(x, y)]; };
    for (const auto& s : state.occupied_) {
        if (at(s.x, s.y) != meC)
            continue;
        for (int d = 0; d < 4; ++d) {
            int nx1 = static_cast<int>(s.x) + DX[d];
            int ny1 = static_cast<int>(s.y) + DY[d];
            if (isInside((uint8_t)nx1, (uint8_t)ny1) && at((uint8_t)nx1, (uint8_t)ny1) == Cell::Empty) {
                uint16_t id = idx((uint8_t)nx1, (uint8_t)ny1);
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { (uint8_t)nx1, (uint8_t)ny1 });
                }
            }
            int nx2 = static_cast<int>(s.x) - DX[d];
            int ny2 = static_cast<int>(s.y) - DY[d];
            if (isInside((uint8_t)nx2, (uint8_t)ny2) && at((uint8_t)nx2, (uint8_t)ny2) == Cell::Empty) {
                uint16_t id = idx((uint8_t)nx2, (uint8_t)ny2);
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { (uint8_t)nx2, (uint8_t)ny2 });
                }
            }
        }
    }

    // Simulate only opponent capturing moves
    for (const auto& pos : cand) {
        Move mv { pos, opp };
        if (!capture::wouldCapture(state, mv))
            continue;

        // Make a working copy to apply captures
        BoardState sim = state;
        sim.placeStone(mv.pos, playerToCell(opp));
        std::vector<Pos> removed;
        int gained = capture::applyCapturesAround(sim, mv.pos, playerToCell(opp), rules, removed);
        int blackPairs = sim.blackPairs;
        int whitePairs = sim.whitePairs;
        if (gained) {
            if (opp == Player::Black)
                blackPairs += gained;
            else
                whitePairs += gained;
        }
        if ((opp == Player::Black ? blackPairs : whitePairs) >= rules.captureWinPairs)
            return true;
        if (!hasAnyFive(sim, meC))
            return true;
    }
    return false;
}

} // namespace gomoku::pattern

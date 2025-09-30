#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/Zobrist.hpp"

namespace gomoku::capture {

namespace {
    struct CapRay {
        uint16_t fwd[3] {};
        uint16_t bwd[3] {};
    };

    constexpr uint16_t encode(int x, int y)
    {
        return (unsigned)x < BOARD_SIZE && (unsigned)y < BOARD_SIZE
            ? static_cast<uint16_t>(y * BOARD_SIZE + x)
            : static_cast<uint16_t>(0xFFFF);
    }

    constexpr std::array<std::array<CapRay, BOARD_SIZE * BOARD_SIZE>, 4> makeCapRays()
    {
        std::array<std::array<CapRay, BOARD_SIZE * BOARD_SIZE>, 4> rays {};
        constexpr int DX[4] = { 1, 0, 1, 1 };
        constexpr int DY[4] = { 0, 1, 1, -1 };
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                const int i = y * BOARD_SIZE + x;
                for (int d = 0; d < 4; ++d) {
                    for (int k = 1; k <= 3; ++k) {
                        rays[d][i].fwd[k - 1] = encode(x + k * DX[d], y + k * DY[d]);
                        rays[d][i].bwd[k - 1] = encode(x - k * DX[d], y - k * DY[d]);
                    }
                }
            }
        }
        return rays;
    }

    constexpr auto capRaysByDir = makeCapRays();

    constexpr uint16_t idx(uint8_t x, uint8_t y)
    {
        return static_cast<uint16_t>(y * BOARD_SIZE + x);
    }
}

int applyCapturesAround(BoardState& state, Pos p, Cell who, const RuleSet& rules, std::vector<Pos>& removed)
{
    if (!rules.capturesEnabled)
        return 0;

    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    const Cell opp = (who == Cell::Black ? Cell::White : Cell::Black);
    int pairs = 0;

    auto isInside = [](uint8_t x, uint8_t y) { return x < BOARD_SIZE && y < BOARD_SIZE; };

    auto tryDir = [&](int sx, int sy, int dx, int dy) -> bool {
        int x1 = sx + dx, y1 = sy + dy;
        int x2 = sx + 2 * dx, y2 = sy + 2 * dy;
        int x3 = sx + 3 * dx, y3 = sy + 3 * dy;
        if (!isInside(static_cast<uint8_t>(x3), static_cast<uint8_t>(y3)))
            return false;
        auto at = [&](uint8_t x, uint8_t y) { return state.cells[idx(x, y)]; };
        if (at(static_cast<uint8_t>(x1), static_cast<uint8_t>(y1)) == opp && at(static_cast<uint8_t>(x2), static_cast<uint8_t>(y2)) == opp && at(static_cast<uint8_t>(x3), static_cast<uint8_t>(y3)) == who) {
            state.removeStone(Pos { (uint8_t)x1, (uint8_t)y1 });
            state.removeStone(Pos { (uint8_t)x2, (uint8_t)y2 });
            removed.push_back({ (uint8_t)x1, (uint8_t)y1 });
            removed.push_back({ (uint8_t)x2, (uint8_t)y2 });
            return true;
        }
        return false;
    };

    for (int d = 0; d < 4; ++d) {
        if (tryDir(p.x, p.y, DX[d], DY[d]))
            ++pairs; // forward
        if (tryDir(p.x, p.y, -DX[d], -DY[d]))
            ++pairs; // backward
    }
    return pairs;
}

bool wouldCapture(const BoardState& state, Move m) noexcept
{
    const Cell me = playerToCell(m.by);
    const Cell opp = (me == Cell::Black ? Cell::White : Cell::Black);
    const uint16_t i = idx(m.pos.x, m.pos.y);

    for (int d = 0; d < 4; ++d) {
        const auto& R = capRaysByDir[d][i];

        if (R.fwd[2] != 0xFFFF && state.cells[R.fwd[2]] == me && state.cells[R.fwd[0]] == opp && state.cells[R.fwd[1]] == opp)
            return true;

        if (R.bwd[2] != 0xFFFF && state.cells[R.bwd[2]] == me && state.cells[R.bwd[0]] == opp && state.cells[R.bwd[1]] == opp)
            return true;
    }
    return false;
}

} // namespace gomoku::capture

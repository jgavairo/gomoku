#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/RayTables.hpp"

namespace gomoku::capture {

int applyCapturesAround(BoardState& state, Pos p, Cell who,
                        const RuleSet& rules, std::vector<Pos>& removed)
{
    if (!rules.capturesEnabled)
        return 0;

    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    const Cell opp = (who == Cell::Black ? Cell::White : Cell::Black);
    int pairs = 0;

    auto isInside = [](uint8_t x, uint8_t y) {
        return x < BOARD_SIZE && y < BOARD_SIZE;
    };

    auto tryDir = [&](int sx, int sy, int dx, int dy) -> bool {
        int x1 = sx + dx,     y1 = sy + dy;
        int x2 = sx + 2*dx,   y2 = sy + 2*dy;
        int x3 = sx + 3*dx,   y3 = sy + 3*dy;

        if (!isInside((uint8_t)x3, (uint8_t)y3))
            return false;

        auto at = [&](int x, int y) {
            return state.getCell(Pos { static_cast<uint8_t>(x), static_cast<uint8_t>(y) });
        };

        if (at(x1,y1) == opp && at(x2,y2) == opp && at(x3,y3) == who) {
            state.removeStone({(uint8_t)x1, (uint8_t)y1});
            state.removeStone({(uint8_t)x2, (uint8_t)y2});
            removed.push_back({(uint8_t)x1, (uint8_t)y1});
            removed.push_back({(uint8_t)x2, (uint8_t)y2});
            return true;
        }
        return false;
    };

    for (int d = 0; d < 4; ++d) {
        if (tryDir(p.x, p.y, DX[d], DY[d])) ++pairs; // forward
        if (tryDir(p.x, p.y, -DX[d], -DY[d])) ++pairs; // backward
    }
    return pairs;
}

bool wouldCapture(const BoardState& state, Move m) noexcept
{
    const Cell me  = playerToCell(m.by);
    const Cell opp = (me == Cell::Black ? Cell::White : Cell::Black);
    const uint16_t i = BoardState::idx(m.pos);

    for (int d = 0; d < 4; ++d) {
        const auto& R = rays::capRaysByDir[d][i];

        if (R.fwd[2] != 0xFFFF &&
            state.getCell(Pos::fromIndex(R.fwd[2])) == me &&
            state.getCell(Pos::fromIndex(R.fwd[0])) == opp &&
            state.getCell(Pos::fromIndex(R.fwd[1])) == opp)
            return true;

        if (R.bwd[2] != 0xFFFF &&
            state.getCell(Pos::fromIndex(R.bwd[2])) == me &&
            state.getCell(Pos::fromIndex(R.bwd[0])) == opp &&
            state.getCell(Pos::fromIndex(R.bwd[1])) == opp)
            return true;
    }
    return false;
}

} // namespace gomoku::capture

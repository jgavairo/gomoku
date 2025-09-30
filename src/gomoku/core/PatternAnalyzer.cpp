#include "gomoku/core/PatternAnalyzer.hpp"
#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/RayTables.hpp"

namespace gomoku::pattern {

// --- helpers locaux sûrs (types int, cast explicite au dernier moment) ---
static inline bool inside_int(int x, int y) noexcept
{
    return x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE;
}
static inline Cell at_int(const BoardState& st, int x, int y) noexcept
{
    return st.getCell(Pos { static_cast<uint8_t>(x), static_cast<uint8_t>(y) });
}

bool createsIllegalDoubleThree(const BoardState& state, Move m, const RuleSet& rules)
{
    if (!rules.forbidDoubleThree)
        return false;

    // Exception : un coup capturant est autorisé même s'il crée un double-trois
    if (rules.capturesEnabled && capture::wouldCapture(state, m))
        return false;

    const Cell ME = playerToCell(m.by);
    const Cell OP = (ME == Cell::Black ? Cell::White : Cell::Black);

    // --- Captures virtuelles autour de m ---
    uint16_t capturedIdx[8];
    int captured_n = 0;

    const uint16_t idx0 = BoardState::idx(m.pos);
    for (int d = 0; d < 4; ++d) {
        const auto& ray = rays::capRaysByDir[d][idx0];

        // fwd: .. OP OP ME -> capture
        if (ray.fwd[2] != 0xFFFF && state.getCell(Pos::fromIndex(ray.fwd[0])) == OP && state.getCell(Pos::fromIndex(ray.fwd[1])) == OP && state.getCell(Pos::fromIndex(ray.fwd[2])) == ME) {
            capturedIdx[captured_n++] = ray.fwd[0];
            capturedIdx[captured_n++] = ray.fwd[1];
        }
        // bwd: ME OP OP .. -> capture
        if (ray.bwd[2] != 0xFFFF && state.getCell(Pos::fromIndex(ray.bwd[0])) == OP && state.getCell(Pos::fromIndex(ray.bwd[1])) == OP && state.getCell(Pos::fromIndex(ray.bwd[2])) == ME) {
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

    // lecture "virtuelle" tenant compte de la pierre posée et des captures virtuelles
    auto vAt = [&](int x, int y) noexcept -> Cell {
        if (!inside_int(x, y))
            return OP; // mur = adversaire
        const uint16_t id = BoardState::idx(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        if (id == idx0)
            return ME; // pierre qu'on pose
        if (isVirtuallyCaptured(id))
            return Cell::Empty; // retirée virtuellement
        return state.getCell(Pos::fromIndex(id));
    };

    // motifs autour de (m.pos) sans buffer secondaire
    auto hasThreeInLine = [&](int dx, int dy) noexcept -> bool {
        int C[9]; // valeurs 0=vide, 1=ME, 2=OP aux offsets -4..+4
        for (int off = -4, i = 0; off <= 4; ++off, ++i) {
            const int x = static_cast<int>(m.pos.x) + off * dx;
            const int y = static_cast<int>(m.pos.y) + off * dy;
            const Cell c = vAt(x, y);
            C[i] = (c == Cell::Empty) ? 0 : (c == ME ? 1 : 2);
        }
        auto Z = [&](int k) noexcept { return C[k + 4] == 0; };
        auto O = [&](int k) noexcept { return C[k + 4] == 1; };

        // 01110
        if (Z(-1) && O(0) && O(+1) && O(+2) && Z(+3))
            return true;
        if (Z(-2) && O(-1) && O(0) && O(+1) && Z(+2))
            return true;
        if (Z(-3) && O(-2) && O(-1) && O(0) && Z(+1))
            return true;

        // 010110
        if (Z(-1) && O(0) && Z(+1) && O(+2) && O(+3) && Z(+4))
            return true;
        if (Z(-3) && O(-2) && Z(-1) && O(0) && O(+1) && Z(+2))
            return true;
        if (Z(-4) && O(-3) && O(-2) && Z(-1) && O(0) && Z(+1))
            return true;

        // 011010
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

    for (int d = 0; d < 4; ++d) {
        int count = 1;
        for (int s = -1; s <= 1; s += 2) {
            int x = p.x, y = p.y;
            while (true) {
                x += s * DX[d];
                y += s * DY[d];
                if (!inside_int(x, y))
                    break;
                if (at_int(state, x, y) == who)
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
        if (state.getCell(p) == who) {
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

    const Player opp = (justPlayed == Player::Black ? Player::White : Player::Black);
    const Cell meC = playerToCell(justPlayed);

    // Copie de travail
    BoardState state = stateIn;

    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };

    std::vector<uint8_t> seen(static_cast<std::size_t>(BOARD_SIZE * BOARD_SIZE), 0);
    std::vector<Pos> cand;
    cand.reserve(128);

    for (const auto& s : state.occupied_) {
        if (state.getCell(s) != meC)
            continue;
        for (int d = 0; d < 4; ++d) {
            // + direction
            int nx1 = static_cast<int>(s.x) + DX[d];
            int ny1 = static_cast<int>(s.y) + DY[d];
            if (inside_int(nx1, ny1) && at_int(state, nx1, ny1) == Cell::Empty) {
                const uint16_t id = BoardState::idx(static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1));
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1) });
                }
            }
            // - direction
            int nx2 = static_cast<int>(s.x) - DX[d];
            int ny2 = static_cast<int>(s.y) - DY[d];
            if (inside_int(nx2, ny2) && at_int(state, nx2, ny2) == Cell::Empty) {
                const uint16_t id = BoardState::idx(static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2));
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2) });
                }
            }
        }
    }

    // Simule uniquement les coups adverses qui capturent
    for (const auto& pos : cand) {
        Move mv { pos, opp };
        if (!capture::wouldCapture(state, mv))
            continue;

        BoardState sim = state;
        sim.placeStone(mv.pos, playerToCell(opp));

        std::vector<Pos> removed;
        const int gained = capture::applyCapturesAround(sim, mv.pos, playerToCell(opp), rules, removed);

        const int oppPairsAfter = (opp == Player::Black ? sim.blackPairs + gained : sim.whitePairs + gained);

        if (oppPairsAfter >= rules.captureWinPairs)
            return true; // victoire immédiate par capture
        if (!hasAnyFive(sim, meC))
            return true; // l'alignement 5+ est cassé par la capture
    }
    return false;
}

} // namespace gomoku::pattern

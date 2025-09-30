#include "gomoku/core/Board.hpp"
#include <array>
#include <cassert>
#include <random>
#include <string>

namespace gomoku {

// ------------------ Zobrist ------------------
namespace {
    std::array<uint64_t, 2 * BOARD_SIZE * BOARD_SIZE> Z_PCS {};
    uint64_t Z_SIDE = 0;

    inline int flat(int x, int y) { return y * BOARD_SIZE + x; }
    inline uint64_t z_of(Cell c, int x, int y)
    {
        if (c == Cell::Black)
            return Z_PCS[0 * BOARD_SIZE * BOARD_SIZE + flat(x, y)];
        if (c == Cell::White)
            return Z_PCS[1 * BOARD_SIZE * BOARD_SIZE + flat(x, y)];
        return 0ull;
    }

    struct ZInit {
        ZInit()
        {
            std::mt19937_64 rng(0x9E3779B97F4A7C15ULL); // seed fixe (reproductible)
            for (auto& v : Z_PCS)
                v = rng();
            Z_SIDE = rng();
        }
    } ZINIT;
}
// ------------------------------------------------

namespace {
    struct CapRay {
        uint16_t fwd[3] {};
        uint16_t bwd[3] {};
    };

    // encode position -> index, ou 0xFFFF si hors plateau
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

    // Table calculée à la compilation
    constexpr auto capRaysByDir = makeCapRays();
}

Board::Board() { reset(); }

Cell Board::at(uint8_t x, uint8_t y) const
{
    if (!isInside(x, y))
        return Cell::Empty;
    return cells[idx(x, y)];
}

void Board::reset()
{
    cells.fill(Cell::Empty);
    currentPlayer = Player::Black;
    blackPairs = whitePairs = 0;
    blackStones = whiteStones = 0;
    gameState = GameStatus::Ongoing;
    moveHistory.clear();
    occupied_.clear();
    occIdx_.fill(-1);

    // Zobrist
    zobristHash = 0ull;
    // Encode le trait (Black to move)
    zobristHash ^= Z_SIDE;
}

// ------------------------------------------------
// Double-trois (free-threes) avec prise en compte des captures
bool Board::createsIllegalDoubleThree(Move m, const RuleSet& rules) const
{
    if (!rules.forbidDoubleThree)
        return false;

    // Exception : un coup capturant est autorisé même s'il crée un double-trois
    if (rules.capturesEnabled && wouldCapture(m))
        return false;

    const Cell ME = playerToCell(m.by);
    const Cell OP = (ME == Cell::Black ? Cell::White : Cell::Black);

    // --- Pré-calcul des captures virtuelles autour de m via capRaysByDir ---
    uint16_t capturedIdx[8];
    int captured_n = 0;

    const int idx0 = m.pos.y * BOARD_SIZE + m.pos.x;
    for (int d = 0; d < 4; ++d) {
        const auto& ray = capRaysByDir[d][idx0];

        // fwd: .. OP, OP, ME -> capture
        if (ray.fwd[2] != 0xFFFF && cells[ray.fwd[0]] == OP && cells[ray.fwd[1]] == OP && cells[ray.fwd[2]] == ME) {
            capturedIdx[captured_n++] = ray.fwd[0];
            capturedIdx[captured_n++] = ray.fwd[1];
        }
        // bwd: ME, OP, OP .. -> capture
        if (ray.bwd[2] != 0xFFFF && cells[ray.bwd[0]] == OP && cells[ray.bwd[1]] == OP && cells[ray.bwd[2]] == ME) {
            capturedIdx[captured_n++] = ray.bwd[0];
            capturedIdx[captured_n++] = ray.bwd[1];
        }
    }

    auto isVirtuallyCaptured = [&](uint16_t idx) noexcept {
        for (int i = 0; i < captured_n; ++i)
            if (capturedIdx[i] == idx)
                return true;
        return false;
    };

    auto vAt = [&](int x, int y) noexcept -> Cell {
        if ((unsigned)x >= BOARD_SIZE || (unsigned)y >= BOARD_SIZE)
            return OP; // mur = adversaire
        const uint16_t idx = static_cast<uint16_t>(y * BOARD_SIZE + x);
        if (idx == idx0)
            return ME; // pierre qu'on pose
        if (isVirtuallyCaptured(idx))
            return Cell::Empty; // retirée virtuellement
        return cells[idx];
    };

    // Teste des motifs autour de (0) sans construire de buffer :
    // on lit seulement les cases nécessaires pour les gabarits centrés.
    auto hasThreeInLine = [&](int dx, int dy) noexcept -> bool {
        // On code: 0=vide, 1=ME, 2=OP (mur compte comme OP via vAt)
        int C[9]; // offsets -4..+4
        for (int off = -4, i = 0; off <= 4; ++off, ++i) {
            const int x = (int)m.pos.x + off * dx;
            const int y = (int)m.pos.y + off * dy;
            Cell c = vAt(x, y);
            C[i] = (c == Cell::Empty) ? 0 : (c == ME ? 1 : 2);
        }
        auto Z = [&](int k) noexcept { return C[k + 4] == 0; }; // empty at offset k
        auto O = [&](int k) noexcept { return C[k + 4] == 1; }; // ME at offset k
        // NB: offset 0 est toujours O(0) par construction

        // --- "01110" (open three) : centre dans {gauche, milieu, droite} du trio ---
        if (Z(-1) && O(0) && O(+1) && O(+2) && Z(+3))
            return true; // centre = 1er du trio
        if (Z(-2) && O(-1) && O(0) && O(+1) && Z(+2))
            return true; // centre = milieu
        if (Z(-3) && O(-2) && O(-1) && O(0) && Z(+1))
            return true; // centre = 3e du trio

        // --- "010110" ---
        if (Z(-1) && O(0) && Z(+1) && O(+2) && O(+3) && Z(+4))
            return true; // centre index 1
        if (Z(-3) && O(-2) && Z(-1) && O(0) && O(+1) && Z(+2))
            return true; // centre index 3
        if (Z(-4) && O(-3) && O(-2) && Z(-1) && O(0) && Z(+1))
            return true; // centre index 4

        // --- "011010" ---
        if (Z(-1) && O(0) && O(+1) && Z(+2) && O(+3) && Z(+4))
            return true; // centre index 1
        if (Z(-2) && O(-1) && O(0) && Z(+1) && O(+2) && Z(+3))
            return true; // centre index 2
        if (Z(-4) && O(-3) && O(-2) && Z(-1) && O(0) && Z(+1))
            return true; // centre index 4 (même masque que ci-dessus pour certains cas)

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

// ------------------------------------------------
// Détecte 5+ alignés depuis p (8 directions)
bool Board::checkFiveOrMoreFrom(Pos p, Cell who) const
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
                if (!isInside(static_cast<uint8_t>(x), static_cast<uint8_t>(y)))
                    break;
                if (at(static_cast<uint8_t>(x), static_cast<uint8_t>(y)) == who)
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

// ------------------------------------------------
// Captures XOOX dans 4 directions et 2 sens
int Board::applyCapturesAround(Pos p, Cell who, const RuleSet& rules, std::vector<Pos>& removed)
{
    if (!rules.capturesEnabled)
        return 0;

    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };

    const Cell opp = (who == Cell::Black ? Cell::White : Cell::Black);
    int pairs = 0;

    auto tryDir = [&](int sx, int sy, int dx, int dy) -> bool {
        int x1 = sx + dx, y1 = sy + dy;
        int x2 = sx + 2 * dx, y2 = sy + 2 * dy;
        int x3 = sx + 3 * dx, y3 = sy + 3 * dy;
        if (!isInside(static_cast<uint8_t>(x3), static_cast<uint8_t>(y3)))
            return false;
        if (at(static_cast<uint8_t>(x1), static_cast<uint8_t>(y1)) == opp && at(static_cast<uint8_t>(x2), static_cast<uint8_t>(y2)) == opp && at(static_cast<uint8_t>(x3), static_cast<uint8_t>(y3)) == who) {
            cells[idx(static_cast<uint8_t>(x1), static_cast<uint8_t>(y1))] = Cell::Empty;
            cells[idx(static_cast<uint8_t>(x2), static_cast<uint8_t>(y2))] = Cell::Empty;
            removed.push_back({ (uint8_t)x1, (uint8_t)y1 });
            removed.push_back({ (uint8_t)x2, (uint8_t)y2 });
            return true;
        }
        return false;
    };

    for (int d = 0; d < 4; ++d) {
        if (tryDir(p.x, p.y, DX[d], DY[d]))
            ++pairs; // sens +
        if (tryDir(p.x, p.y, -DX[d], -DY[d]))
            ++pairs; // sens -
    }
    return pairs;
}

// ------------------------------------------------
PlayResult Board::applyCore(Move m, const RuleSet& rules, bool record)
{
    if (gameState != GameStatus::Ongoing) {
        return PlayResult::fail(PlayErrorCode::GameFinished, "Game already finished.");
    }
    if (m.by != currentPlayer) {
        return PlayResult::fail(PlayErrorCode::NotPlayersTurn, "Not this player's turn.");
    }
    if (!isEmpty(m.pos.x, m.pos.y)) {
        return PlayResult::fail(PlayErrorCode::Occupied, "Cell not empty.");
    }

    bool mustBreak = false;
    if (rules.allowFiveOrMore && rules.capturesEnabled) {
        Player justPlayed = opponent(currentPlayer);
        Cell meC = playerToCell(justPlayed);
        if (hasAnyFive(meC) && isFiveBreakableNow(justPlayed, rules))
            mustBreak = true;
    }

    bool allowDoubleThreeThisMove = false;
    if (mustBreak) {
        if (!wouldCapture(m)) {
            return PlayResult::fail(PlayErrorCode::RuleViolation, "Must break opponent's five.");
        }
        Board sim = *this;
        sim.cells[idx(m.pos.x, m.pos.y)] = playerToCell(m.by);
        std::vector<Pos> removedTmp;
        int gainedTmp = sim.applyCapturesAround(m.pos, playerToCell(m.by), rules, removedTmp);
        if (gainedTmp) {
            if (m.by == Player::Black)
                sim.blackPairs += gainedTmp;
            else
                sim.whitePairs += gainedTmp;
        }
        int myPairsAfter = (m.by == Player::Black ? sim.blackPairs : sim.whitePairs);
        Cell oppFiveColor = playerToCell(opponent(currentPlayer));
        bool breaks = (myPairsAfter >= rules.captureWinPairs) || (!sim.hasAnyFive(oppFiveColor));
        if (!breaks) {
            return PlayResult::fail(PlayErrorCode::RuleViolation, "Must break opponent's five.");
        }
        allowDoubleThreeThisMove = true;
    }

    if (!allowDoubleThreeThisMove && createsIllegalDoubleThree(m, rules)) {
        return PlayResult::fail(PlayErrorCode::RuleViolation, "Illegal double-three.");
    }

    // Préparation Undo (si record)
    UndoEntry u;
    if (record) {
        u.move = m;
        u.blackPairsBefore = blackPairs;
        u.whitePairsBefore = whitePairs;
        u.blackStonesBefore = blackStones;
        u.whiteStonesBefore = whiteStones;
        u.stateBefore = gameState;
        u.playerBefore = currentPlayer;
    }

    cells[idx(m.pos.x, m.pos.y)] = playerToCell(m.by);
    zobristHash ^= z_of(playerToCell(m.by), m.pos.x, m.pos.y);
    // Track stone counts
    if (m.by == Player::Black)
        ++blackStones;
    else
        ++whiteStones;
    // Sparse index add
    {
        const int id = idx(m.pos.x, m.pos.y);
        occIdx_[id] = static_cast<int16_t>(occupied_.size());
        occupied_.push_back(m.pos);
    }

    std::vector<Pos> capturedLocal; // utilisera u.capturedStones si record
    auto& capVec = record ? u.capturedStones : capturedLocal;
    int gained = applyCapturesAround(m.pos, playerToCell(m.by), rules, capVec);
    if (gained) {
        if (m.by == Player::Black)
            blackPairs += gained;
        else
            whitePairs += gained;
        Cell oppC = (m.by == Player::Black ? Cell::White : Cell::Black);
        for (auto rp : capVec) {
            zobristHash ^= z_of(oppC, rp.x, rp.y);
            // Decrement opponent stone counts for captured stones
            if (oppC == Cell::Black)
                --blackStones;
            else
                --whiteStones;
            // Sparse index remove via swap-pop
            const int id = idx(rp.x, rp.y);
            int16_t posIdx = occIdx_[id];
            if (posIdx >= 0) {
                const int lastIdx = static_cast<int>(occupied_.size()) - 1;
                if (posIdx != lastIdx) {
                    Pos moved = occupied_.back();
                    occupied_[posIdx] = moved;
                    occIdx_[moved.toIndex()] = posIdx;
                }
                occupied_.pop_back();
                occIdx_[id] = -1;
            }
        }
    }

    if (rules.allowFiveOrMore && checkFiveOrMoreFrom(m.pos, playerToCell(m.by))) {
        if (!isFiveBreakableNow(m.by, rules)) {
            gameState = GameStatus::WinByAlign;
        }
    }

    if (rules.capturesEnabled && gameState == GameStatus::Ongoing) {
        if (blackPairs >= rules.captureWinPairs || whitePairs >= rules.captureWinPairs)
            gameState = GameStatus::WinByCapture;
    }

    if (gameState == GameStatus::Ongoing && isBoardFull())
        gameState = GameStatus::Draw;

    if (record) {
        moveHistory.push_back(std::move(u));
    }
    currentPlayer = opponent(currentPlayer);
    zobristHash ^= Z_SIDE;

    return PlayResult::ok();
}

PlayResult Board::tryPlay(Move m, const RuleSet& rules)
{
    return applyCore(m, rules, true);
}

bool Board::speculativeTry(Move m, const RuleSet& rules, PlayResult* out)
{
    // Implémentation Option B (diff ciblé) :
    // On enregistre uniquement l'état minimal nécessaire pour restaurer :
    // - hash, joueur courant, compteurs captures, statut
    // - contenu des cellules susceptibles de changer (la case jouée + jusqu'à 8*2 candidates captures)

    uint64_t hashBefore = zobristHash;
    Player playerBefore = currentPlayer;
    int blackPairsBefore = blackPairs;
    int whitePairsBefore = whitePairs;
    int blackStonesBefore = blackStones;
    int whiteStonesBefore = whiteStones;
    GameStatus statusBefore = gameState;

    struct CellSnapshot {
        Pos p;
        Cell before;
    };
    CellSnapshot center { m.pos, at(m.pos.x, m.pos.y) }; // devrait être Empty si légal

    // Générer les positions candidates pouvant être capturées (x1,x2) pour chaque direction ±
    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    CellSnapshot candidates[16];
    int candCount = 0;
    bool seen[BOARD_SIZE * BOARD_SIZE] = { false };
    auto mark = [&](int x, int y) {
        if (!isInside(static_cast<uint8_t>(x), static_cast<uint8_t>(y)))
            return;
        int idFlat = y * BOARD_SIZE + x;
        if (seen[idFlat])
            return;
        seen[idFlat] = true;
        candidates[candCount++] = CellSnapshot { Pos { (uint8_t)x, (uint8_t)y }, at(static_cast<uint8_t>(x), static_cast<uint8_t>(y)) };
    };
    for (int d = 0; d < 4; ++d) {
        int x1 = m.pos.x + DX[d], y1 = m.pos.y + DY[d];
        int x2 = m.pos.x + 2 * DX[d], y2 = m.pos.y + 2 * DY[d];
        mark(x1, y1);
        mark(x2, y2);
        int X1 = m.pos.x - DX[d], Y1 = m.pos.y - DY[d];
        int X2 = m.pos.x - 2 * DX[d], Y2 = m.pos.y - 2 * DY[d];
        mark(X1, Y1);
        mark(X2, Y2);
    }

    PlayResult pr = applyCore(m, rules, false);
    if (out)
        *out = pr;
    if (!pr.success) {
        // Aucune mutation durable si échec (toutes les validations échouantes précèdent la pose).
        return false;
    }

    // ROLLBACK : retirer la pierre posée et restaurer les cellules capturées.
    // La pierre jouée est toujours à m.pos si succès.
    cells[idx(m.pos.x, m.pos.y)] = center.before; // normalement Empty
    // adjust stone counts for the removed placed stone
    if (m.by == Player::Black)
        --blackStones;
    else
        --whiteStones;

    // Restaurer chaque cellule candidate devenue vide alors qu'elle ne l'était pas avant
    for (int i = 0; i < candCount; ++i) {
        auto& snap = candidates[i];
        Cell after = at(snap.p.x, snap.p.y);
        // Si la cellule a été vidée (capture) on la restaure.
        if (snap.before != Cell::Empty && after == Cell::Empty) {
            // (snap.before devrait être oppC en pratique)
            cells[idx(snap.p.x, snap.p.y)] = snap.before;
            if (snap.before == Cell::Black)
                ++blackStones;
            else if (snap.before == Cell::White)
                ++whiteStones;
        }
    }

    // Restaurer compteurs & statut & trait & hash
    blackPairs = blackPairsBefore;
    whitePairs = whitePairsBefore;
    blackStones = blackStonesBefore;
    whiteStones = whiteStonesBefore;
    gameState = statusBefore;
    currentPlayer = playerBefore;
    zobristHash = hashBefore; // hash global cohérent (inclut side to move)

    return true;
}

// ------------------------------------------------
bool Board::undo()
{
    if (moveHistory.empty())
        return false;
    UndoEntry u = std::move(moveHistory.back());
    moveHistory.pop_back();

    // Zobrist: le trait redevient celui d'avant
    zobristHash ^= Z_SIDE;

    // Retirer la pierre jouée
    cells[idx(u.move.pos.x, u.move.pos.y)] = Cell::Empty;
    // Zobrist: retirer la pierre annulée
    zobristHash ^= z_of(playerToCell(u.move.by), u.move.pos.x, u.move.pos.y);
    // Update stone count for removed stone
    if (u.move.by == Player::Black)
        --blackStones;
    else
        --whiteStones;
    // Sparse index remove via swap-pop
    {
        const int id = idx(u.move.pos.x, u.move.pos.y);
        int16_t posIdx = occIdx_[id];
        if (posIdx >= 0) {
            const int lastIdx = static_cast<int>(occupied_.size()) - 1;
            if (posIdx != lastIdx) {
                Pos moved = occupied_.back();
                occupied_[posIdx] = moved;
                occIdx_[moved.toIndex()] = posIdx;
            }
            occupied_.pop_back();
            occIdx_[id] = -1;
        }
    }

    // Restaurer les pierres capturées
    Cell oppC = (u.move.by == Player::Black ? Cell::White : Cell::Black);
    for (auto rp : u.capturedStones) {
        cells[idx(rp.x, rp.y)] = oppC;
        // Zobrist: remettre les capturées
        zobristHash ^= z_of(oppC, rp.x, rp.y);
        if (oppC == Cell::Black)
            ++blackStones;
        else
            ++whiteStones;
        // Sparse index add back
        const int id = idx(rp.x, rp.y);
        occIdx_[id] = static_cast<int16_t>(occupied_.size());
        occupied_.push_back(rp);
    }
    blackPairs = u.blackPairsBefore;
    whitePairs = u.whitePairsBefore;
    blackStones = u.blackStonesBefore;
    whiteStones = u.whiteStonesBefore;
    gameState = u.stateBefore;
    currentPlayer = u.playerBefore;
    return true;
}

// ------------------------------------------------
std::vector<Move> Board::legalMoves(Player p, const RuleSet& rules) const
{
    std::vector<Move> out;
    // If the board is empty (no moves yet), fall back to scanning for all empties
    // to preserve initial-move behavior.
    if (moveHistory.empty()) {
        out.reserve(BOARD_SIZE * BOARD_SIZE);
        for (uint8_t y = 0; y < BOARD_SIZE; ++y) {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x) {
                if (at(x, y) != Cell::Empty)
                    continue;
                Move m { { x, y }, p };
                if (createsIllegalDoubleThree(m, rules))
                    continue;
                out.push_back(m);
            }
        }
        return out;
    }

    // Otherwise, restrict to empties within Chebyshev distance <= 2 of any occupied cell
    // using the sparse occupied index to avoid scanning the whole board.
    std::vector<uint8_t> mark(static_cast<std::size_t>(BOARD_SIZE * BOARD_SIZE), 0);
    out.reserve(256);
    for (const auto& s : occupied_) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int nx = static_cast<int>(s.x) + dx;
                int ny = static_cast<int>(s.y) + dy;
                if (!isInside(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)))
                    continue;
                if (at(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny)) != Cell::Empty)
                    continue;
                uint16_t id = idx(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny));
                if (mark[id])
                    continue;
                mark[id] = 1;
                Move m { { static_cast<uint8_t>(nx), static_cast<uint8_t>(ny) }, p };
                if (createsIllegalDoubleThree(m, rules))
                    continue;
                out.push_back(m);
            }
        }
    }
    return out;
}

// ------------------------------------------------
bool Board::hasAnyFive(Cell who) const
{
    // Iterate only on occupied cells to avoid scanning empty space
    for (const auto& p : occupied_) {
        if (at(p.x, p.y) == who) {
            if (checkFiveOrMoreFrom(p, who))
                return true;
        }
    }
    return false;
}

// Après que 'justPlayed' a posé sa pierre et que les captures ont été appliquées,
// vérifier si l'adversaire peut casser immédiatement le 5+ par capture
bool Board::isFiveBreakableNow(Player justPlayed, const RuleSet& rules) const
{
    if (!rules.capturesEnabled)
        return false;

    Player opp = (justPlayed == Player::Black ? Player::White : Player::Black);
    Cell meC = playerToCell(justPlayed);

    Board base = *this;
    base.forceSide(opp); // au trait pour l'adversaire dans la simulation

    // Build candidate empties adjacent (in the 4 aligned directions) to justPlayed's stones.
    // Any XOOX capture square is adjacent to at least one of the two opponent stones (here: meC),
    // so this neighborhood covers all immediate capture-breaking moves.
    static constexpr int DX[4] = { 1, 0, 1, 1 };
    static constexpr int DY[4] = { 0, 1, 1, -1 };
    std::vector<uint8_t> seen(static_cast<std::size_t>(BOARD_SIZE * BOARD_SIZE), 0);
    std::vector<Pos> cand;
    cand.reserve(128);
    for (const auto& s : base.occupied_) {
        if (base.at(s.x, s.y) != meC)
            continue;
        for (int d = 0; d < 4; ++d) {
            // + direction
            int nx1 = static_cast<int>(s.x) + DX[d];
            int ny1 = static_cast<int>(s.y) + DY[d];
            if (base.isInside(static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1)) && base.at(static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1)) == Cell::Empty) {
                uint16_t id = idx(static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1));
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { static_cast<uint8_t>(nx1), static_cast<uint8_t>(ny1) });
                }
            }
            // - direction
            int nx2 = static_cast<int>(s.x) - DX[d];
            int ny2 = static_cast<int>(s.y) - DY[d];
            if (base.isInside(static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2)) && base.at(static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2)) == Cell::Empty) {
                uint16_t id = idx(static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2));
                if (!seen[id]) {
                    seen[id] = 1;
                    cand.push_back(Pos { static_cast<uint8_t>(nx2), static_cast<uint8_t>(ny2) });
                }
            }
        }
    }

    for (const auto& pos : cand) {
        Move mv { pos, opp };
        if (!base.wouldCapture(mv))
            continue; // only capturing moves can break immediately

        Board sim = base;
        // place the stone and apply captures
        sim.cells[idx(mv.pos.x, mv.pos.y)] = playerToCell(opp);
        std::vector<Pos> removed;
        int gained = sim.applyCapturesAround(mv.pos, playerToCell(opp), rules, removed);
        if (gained) {
            if (opp == Player::Black)
                sim.blackPairs += gained;
            else
                sim.whitePairs += gained;
        }

        int oppPairsAfter = (opp == Player::Black ? sim.blackPairs : sim.whitePairs);
        if (oppPairsAfter >= rules.captureWinPairs)
            return true; // immediate win by capture
        if (!sim.hasAnyFive(meC))
            return true; // line is broken by the capture
    }
    return false;
}

void Board::forceSide(Player p)
{
    if (currentPlayer != p) {
        currentPlayer = p;
        // Maintenir la clé Zobrist alignée avec "side to move"
        zobristHash ^= Z_SIDE;
    }
}

bool Board::isBoardFull() const
{
    return (blackStones + whiteStones) == N;
}

// Détecte si m provoquerait une capture XOOX (±4 directions)
inline bool Board::wouldCapture(Move m) const noexcept
{
    const Cell me = playerToCell(m.by);
    const Cell opp = (me == Cell::Black ? Cell::White : Cell::Black);
    const uint16_t i = idx(m.pos.x, m.pos.y);

    for (int d = 0; d < 4; ++d) {
        const auto& R = capRaysByDir[d][i];

        if (R.fwd[2] != 0xFFFF && cells[R.fwd[2]] == me && cells[R.fwd[0]] == opp && cells[R.fwd[1]] == opp)
            return true;

        if (R.bwd[2] != 0xFFFF && cells[R.bwd[2]] == me && cells[R.bwd[0]] == opp && cells[R.bwd[1]] == opp)
            return true;
    }
    return false;
}

} // namespace gomoku

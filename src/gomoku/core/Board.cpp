#include "gomoku/core/Board.hpp"
#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/Zobrist.hpp"
#include <array>
#include <cassert>
#include <random>
#include <string>

namespace gomoku {

// ------------------ Zobrist via dedicated module ------------------
using gomoku::zobrist::piece;
using gomoku::zobrist::side;
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

// Define constexpr helper from header
constexpr uint16_t Board::idx(uint8_t x, uint8_t y)
{
    return static_cast<uint16_t>(y * BOARD_SIZE + x);
}

Cell Board::at(uint8_t x, uint8_t y) const
{
    if (!isInside(x, y))
        return Cell::Empty;
    return state.cells[idx(x, y)];
}

Player Board::toPlay() const { return currentPlayer; }
CaptureCount Board::capturedPairs() const { return { state.blackPairs, state.whitePairs }; }
GameStatus Board::status() const { return gameState; }
uint64_t Board::zobristKey() const { return state.zobristHash; }

bool Board::isInside(uint8_t x, uint8_t y) const { return x < BOARD_SIZE && y < BOARD_SIZE; }
bool Board::isEmpty(uint8_t x, uint8_t y) const { return isInside(x, y) && state.cells[idx(x, y)] == Cell::Empty; }

int Board::stoneCount(Player p) const { return (p == Player::Black) ? state.blackStones : state.whiteStones; }

std::optional<Move> Board::lastMove() const
{
    if (moveHistory.empty())
        return std::nullopt;
    return moveHistory.back().move;
}

std::vector<Move> Board::lastMoves(std::size_t k) const
{
    std::vector<Move> out;
    if (k == 0 || moveHistory.empty())
        return out;
    const std::size_t n = std::min(k, moveHistory.size());
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(moveHistory[moveHistory.size() - 1 - i].move);
    }
    return out;
}

const std::vector<Pos>& Board::occupiedPositions() const { return state.occupied_; }

void Board::reset()
{
    state.reset(true);
    currentPlayer = Player::Black;
    gameState = GameStatus::Ongoing;
    moveHistory.clear();
    // Side encoded in state.reset(true)
}

// ------------------------------------------------
// Double-trois (free-threes) avec prise en compte des captures
bool Board::createsIllegalDoubleThree(Move m, const RuleSet& rules) const
{
    if (!rules.forbidDoubleThree)
        return false;

    // Exception : un coup capturant est autorisé même s'il crée un double-trois
    if (rules.capturesEnabled && capture::wouldCapture(state, m))
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
        if (ray.fwd[2] != 0xFFFF && state.cells[ray.fwd[0]] == OP && state.cells[ray.fwd[1]] == OP && state.cells[ray.fwd[2]] == ME) {
            capturedIdx[captured_n++] = ray.fwd[0];
            capturedIdx[captured_n++] = ray.fwd[1];
        }
        // bwd: ME, OP, OP .. -> capture
        if (ray.bwd[2] != 0xFFFF && state.cells[ray.bwd[0]] == OP && state.cells[ray.bwd[1]] == OP && state.cells[ray.bwd[2]] == ME) {
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
        return state.cells[idx];
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
// applyCapturesAround moved to CaptureEngine

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
        if (!capture::wouldCapture(state, m)) {
            return PlayResult::fail(PlayErrorCode::RuleViolation, "Must break opponent's five.");
        }
        Board sim = *this;
        sim.state.setCell(m.pos.x, m.pos.y, playerToCell(m.by));
        std::vector<Pos> removedTmp;
        int gainedTmp = capture::applyCapturesAround(sim.state, m.pos, playerToCell(m.by), rules, removedTmp);
        if (gainedTmp) {
            if (m.by == Player::Black)
                sim.state.blackPairs += gainedTmp;
            else
                sim.state.whitePairs += gainedTmp;
        }
        int myPairsAfter = (m.by == Player::Black ? sim.state.blackPairs : sim.state.whitePairs);
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
        u.blackPairsBefore = state.blackPairs;
        u.whitePairsBefore = state.whitePairs;
        u.blackStonesBefore = state.blackStones;
        u.whiteStonesBefore = state.whiteStones;
        u.stateBefore = gameState;
        u.playerBefore = currentPlayer;
    }

    state.placeStone(m.pos, playerToCell(m.by));

    std::vector<Pos> capturedLocal; // utilisera u.capturedStones si record
    auto& capVec = record ? u.capturedStones : capturedLocal;
    int gained = capture::applyCapturesAround(state, m.pos, playerToCell(m.by), rules, capVec);
    if (gained) {
        if (m.by == Player::Black)
            state.blackPairs += gained;
        else
            state.whitePairs += gained;
    }

    if (rules.allowFiveOrMore && checkFiveOrMoreFrom(m.pos, playerToCell(m.by))) {
        if (!isFiveBreakableNow(m.by, rules)) {
            gameState = GameStatus::WinByAlign;
        }
    }

    if (rules.capturesEnabled && gameState == GameStatus::Ongoing) {
        if (state.blackPairs >= rules.captureWinPairs || state.whitePairs >= rules.captureWinPairs)
            gameState = GameStatus::WinByCapture;
    }

    if (gameState == GameStatus::Ongoing && isBoardFull())
        gameState = GameStatus::Draw;

    if (record) {
        moveHistory.push_back(std::move(u));
    }
    currentPlayer = opponent(currentPlayer);
    state.flipSide();

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

    uint64_t hashBefore = state.zobristHash;
    Player playerBefore = currentPlayer;
    int blackPairsBefore = state.blackPairs;
    int whitePairsBefore = state.whitePairs;
    int blackStonesBefore = state.blackStones;
    int whiteStonesBefore = state.whiteStones;
    GameStatus statusBefore = gameState;

    struct CellSnapshot {
        Pos p;
        Cell before;
    };
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
    state.removeStone(m.pos);

    // Restaurer chaque cellule candidate devenue vide alors qu'elle ne l'était pas avant
    for (int i = 0; i < candCount; ++i) {
        auto& snap = candidates[i];
        Cell after = at(snap.p.x, snap.p.y);
        // Si la cellule a été vidée (capture) on la restaure.
        if (snap.before != Cell::Empty && after == Cell::Empty) {
            // (snap.before devrait être oppC en pratique)
            state.placeStone(snap.p, snap.before);
        }
    }

    // Restaurer compteurs & statut & trait & hash
    state.blackPairs = blackPairsBefore;
    state.whitePairs = whitePairsBefore;
    state.blackStones = blackStonesBefore;
    state.whiteStones = whiteStonesBefore;
    gameState = statusBefore;
    currentPlayer = playerBefore;
    state.zobristHash = hashBefore; // hash global cohérent (inclut side to move)

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
    state.flipSide();

    // Retirer la pierre jouée (cell + occupied)
    state.removeStone(u.move.pos);

    // Restaurer les pierres capturées
    Cell oppC = (u.move.by == Player::Black ? Cell::White : Cell::Black);
    for (auto rp : u.capturedStones) {
        state.placeStone(rp, oppC);
    }
    state.blackPairs = u.blackPairsBefore;
    state.whitePairs = u.whitePairsBefore;
    state.blackStones = u.blackStonesBefore;
    state.whiteStones = u.whiteStonesBefore;
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
    for (const auto& s : state.occupied_) {
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
    for (const auto& p : state.occupied_) {
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
    for (const auto& s : base.state.occupied_) {
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
        if (!capture::wouldCapture(base.state, mv))
            continue; // only capturing moves can break immediately

        Board sim = base;
        // place the stone and apply captures
        sim.state.placeStone(mv.pos, playerToCell(opp));
        std::vector<Pos> removed;
        int gained = capture::applyCapturesAround(sim.state, mv.pos, playerToCell(opp), rules, removed);
        if (gained) {
            if (opp == Player::Black)
                sim.state.blackPairs += gained;
            else
                sim.state.whitePairs += gained;
        }
        // occupancy for captured stones already updated by applyCapturesAround

        int oppPairsAfter = (opp == Player::Black ? sim.state.blackPairs : sim.state.whitePairs);
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
        state.flipSide();
    }
}

bool Board::isBoardFull() const
{
    return (state.blackStones + state.whiteStones) == N;
}

// Détecte si m provoquerait une capture XOOX (±4 directions)
// wouldCapture moved to CaptureEngine

} // namespace gomoku

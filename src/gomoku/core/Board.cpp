#include "gomoku/core/Board.hpp"
#include "gomoku/core/BoardState.hpp"
#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/PatternAnalyzer.hpp"
#include "gomoku/core/Zobrist.hpp"
#include <array>
#include <cassert>
#include <string>

namespace gomoku {

// ------------------------------------------------

Board::Board() { reset(); }

// BoardState::idx/Pos::toIndex are used for linear indexing; no local helper needed.

Cell Board::at(uint8_t x, uint8_t y) const
{
    if (!isInside(x, y))
        return Cell::Empty;
    return state.cells[BoardState::idx(x, y)];
}

Player Board::toPlay() const { return currentPlayer; }
CaptureCount Board::capturedPairs() const { return { state.blackPairs, state.whitePairs }; }
GameStatus Board::status() const { return gameState; }
uint64_t Board::zobristKey() const { return state.zobristHash; }

bool Board::isInside(uint8_t x, uint8_t y) const { return x < BOARD_SIZE && y < BOARD_SIZE; }
bool Board::isEmpty(uint8_t x, uint8_t y) const { return isInside(x, y) && state.cells[BoardState::idx(x, y)] == Cell::Empty; }

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
        if (pattern::hasAnyFive(state, meC) && pattern::isFiveBreakableNow(state, justPlayed, rules))
            mustBreak = true;
    }

    bool allowDoubleThreeThisMove = false;
    if (mustBreak) {
        if (!capture::wouldCapture(state, m)) {
            return PlayResult::fail(PlayErrorCode::RuleViolation, "Must break opponent's five.");
        }
        Board sim = *this;
        sim.state.placeStone(m.pos, playerToCell(m.by));
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
        bool breaks = (myPairsAfter >= rules.captureWinPairs) || (!pattern::hasAnyFive(sim.state, oppFiveColor));
        if (!breaks) {
            return PlayResult::fail(PlayErrorCode::RuleViolation, "Must break opponent's five.");
        }
        allowDoubleThreeThisMove = true;
    }

    if (!allowDoubleThreeThisMove && pattern::createsIllegalDoubleThree(state, m, rules)) {
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

    if (rules.allowFiveOrMore && pattern::checkFiveOrMoreFrom(state, m.pos, playerToCell(m.by))) {
        if (!pattern::isFiveBreakableNow(state, m.by, rules)) {
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
                if (pattern::createsIllegalDoubleThree(state, m, rules))
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
                uint16_t id = BoardState::idx(static_cast<uint8_t>(nx), static_cast<uint8_t>(ny));
                if (mark[id])
                    continue;
                mark[id] = 1;
                Move m { { static_cast<uint8_t>(nx), static_cast<uint8_t>(ny) }, p };
                if (pattern::createsIllegalDoubleThree(state, m, rules))
                    continue;
                out.push_back(m);
            }
        }
    }
    return out;
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

} // namespace gomoku

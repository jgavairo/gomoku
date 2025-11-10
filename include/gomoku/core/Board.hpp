#pragma once
// Concrete board implementation backing the core game logic (libgomoku_logic.a)
#include "gomoku/core/BoardState.hpp"
#include "gomoku/core/CaptureEngine.hpp"
#include "gomoku/core/Types.hpp"
#include "gomoku/interfaces/IBoardView.hpp"
#include <array>
#include <cstdint>
#include <limits> // for std::numeric_limits in static_assert
#include <optional>
#include <string>
#include <vector>

namespace gomoku {

// Implémentation concrète de IBoardView pour libgomoku_logic.a
class Board final : public IBoardView {
public:
    Board();

    // ---- IBoardView interface ----
    Cell at(uint8_t x, uint8_t y) const override;
    Player toPlay() const override;
    CaptureCount capturedPairs() const override;
    GameStatus status() const override;
    bool isBoardFull() const override;
    std::vector<Move> legalMoves(Player p, const RuleSet& rules) const override;
    uint64_t zobristKey() const override;

    // ---- Board-specific API ----
    void reset();
    bool isInside(uint8_t x, uint8_t y) const;
    bool isEmpty(uint8_t x, uint8_t y) const;

    // Stone count (tracked incrementally)
    int stoneCount(Player p) const;

    // Last move played (if any)
    std::optional<Move> lastMove() const;

    // Last k moves (most recent first). Returns up to k moves.
    std::vector<Move> lastMoves(std::size_t k) const;

    PlayResult tryPlay(Move m, const RuleSet& rules);
    bool wouldCapture(Move m) const { return capture::wouldCapture(state, m); }
    bool undo();

    bool speculativeTry(Move m, const RuleSet& rules, PlayResult* out);

    // Force player turn (for specific game setups)
    void forceSide(Player p);

    // Test helper: place a stone directly without turn validation (bypasses game rules)
    // Useful for setting up specific board configurations in unit tests
    void setStone(Pos p, Cell c);

    // Sparse occupied cells accessor (for fast scans in generators/eval)
    const std::vector<Pos>& occupiedPositions() const;

private:
    static constexpr int N = BOARD_SIZE * BOARD_SIZE;

    // Internal state container (cells, occupied index, counters, zobrist)
    BoardState state;

    Player currentPlayer { Player::Black };
    GameStatus gameState { GameStatus::Ongoing };

    struct UndoEntry {
        Move move {};
        std::vector<Pos> capturedStones; // pierres capturées
        int blackPairsBefore { 0 }, whitePairsBefore { 0 };
        int blackStonesBefore { 0 }, whiteStonesBefore { 0 };
        GameStatus stateBefore { GameStatus::Ongoing };
        Player playerBefore { Player::Black };
        uint64_t zobristBefore { 0 }; // Hash Zobrist avant le coup
    };
    std::vector<UndoEntry> moveHistory;

    static_assert(BOARD_SIZE * BOARD_SIZE < std::numeric_limits<int16_t>::max(), "occIdx_ requires N < int16_t::max");

    // Facteur interne : logique partagée d'application. Si record=true, pousse UndoEntry.
    PlayResult applyCore(Move m, const RuleSet& rules, bool record);

    // capture logic moved to CaptureEngine (free functions)
};

} // namespace gomoku

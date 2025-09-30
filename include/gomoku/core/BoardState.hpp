#pragma once

#include "gomoku/core/Types.hpp"
#include "gomoku/core/Zobrist.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace gomoku {

// Lightweight container for the board's internal state.
// It stores only data and provides tiny helpers to maintain invariants.
//
// Invariants:
// - cells holds the content of every board position.
// - occIdx_[i] == -1 if and only if cells[i] == Cell::Empty.
// - For any non-empty cell at linear index i, occIdx_[i] is a valid index
//   into occupied_, and occupied_[occIdx_[i]] is the corresponding Pos.
// - Stone counters (blackStones/whiteStones) match the content of cells.
// - zobristHash is kept in sync by set/clear operations and flipSide().
//   It encodes the side-to-move bit if flipSide/reset was used appropriately.
class BoardState final {
public:
    static constexpr int N = BOARD_SIZE * BOARD_SIZE;
    static constexpr uint16_t idx(uint8_t x, uint8_t y) noexcept
    {
        return static_cast<uint16_t>(y * BOARD_SIZE + x);
    }
    static constexpr uint16_t idx(Pos p) noexcept
    {
        return idx(p.x, p.y);
    }

    // Raw storage
    std::array<Cell, N> cells {};
    std::vector<Pos> occupied_;
    std::array<int16_t, N> occIdx_ {};

    // Counters
    int blackPairs { 0 }, whitePairs { 0 };
    int blackStones { 0 }, whiteStones { 0 };

    // Zobrist hash (includes side-to-move bit when used via flipSide/reset)
    uint64_t zobristHash { 0 };

    BoardState();

    // Reset state, clear all structures, and set side-to-move bit
    // according to the provided parameter (default: Black to move).
    void reset(bool sideToMoveBlack = true) noexcept;

    // Basic queries
    bool isInside(uint8_t x, uint8_t y) const noexcept { return x < BOARD_SIZE && y < BOARD_SIZE; }
    bool isEmpty(uint8_t x, uint8_t y) const noexcept { return cells[idx(x, y)] == Cell::Empty; }

    // Cell operations (do NOT touch occupied_ by design; combine with add/removeOccupied)
    // - setCell: set a cell to c and keep stone counters and hash in sync
    // - clearCell: set a cell to Empty and keep counters/hash in sync
    Cell getCell(uint8_t x, uint8_t y) const noexcept { return cells[idx(x, y)]; }
    Cell getCell(Pos p) const noexcept { return cells[idx(p)]; }
    bool isFull() const noexcept { return occupied_.size() == N; }

    // High-level helpers keeping invariants automatically
    void placeStone(Pos p, Cell c) noexcept
    {
        setCell(p.x, p.y, c);
        addOccupied(p);
    }
    void removeStone(Pos p) noexcept
    {
        clearCell(p.x, p.y);
        removeOccupied(p);
    }

    // Toggle side-to-move bit in zobristHash
    void flipSide() noexcept { zobristHash ^= zobrist::side(); }

    static_assert(BOARD_SIZE * BOARD_SIZE < std::numeric_limits<int16_t>::max(),
        "occIdx_ requires N < int16_t::max");

private:
    // Occupancy tracking helpers (swap-pop removal)
    // Respect the invariant occIdx_[i] == -1 <=> empty when used consistently
    void setCell(uint8_t x, uint8_t y, Cell c) noexcept;
    void clearCell(uint8_t x, uint8_t y) noexcept;
    void addOccupied(Pos p) noexcept;
    void removeOccupied(Pos p) noexcept;
};

} // namespace gomoku

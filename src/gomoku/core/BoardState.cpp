#include "gomoku/core/BoardState.hpp"

namespace gomoku {

BoardState::BoardState()
{
    reset(true);
}

void BoardState::reset(bool sideToMoveBlack) noexcept
{
    cells.fill(Cell::Empty);
    occIdx_.fill(-1);
    occupied_.clear();
    blackPairs = whitePairs = 0;
    blackStones = whiteStones = 0;
    zobristHash = 0ull;
    if (sideToMoveBlack) {
        // Encode side-to-move (Black to move)
        zobristHash ^= zobrist::side();
    }
}

void BoardState::setCell(uint8_t x, uint8_t y, Cell c) noexcept
{
    const uint16_t i = idx(x, y);
    Cell prev = cells[i];
    if (prev == c)
        return;

    // Update hash: remove previous piece, add new one
    if (prev != Cell::Empty)
        zobristHash ^= zobrist::piece(prev, x, y);
    if (c != Cell::Empty)
        zobristHash ^= zobrist::piece(c, x, y);

    // Update counters
    if (prev == Cell::Black) --blackStones;
    else if (prev == Cell::White) --whiteStones;

    cells[i] = c;

    if (c == Cell::Black) ++blackStones;
    else if (c == Cell::White) ++whiteStones;
}

void BoardState::clearCell(uint8_t x, uint8_t y) noexcept
{
    const uint16_t i = idx(x, y);
    Cell prev = cells[i];
    if (prev == Cell::Empty)
        return;

    // Update hash and counters
    zobristHash ^= zobrist::piece(prev, x, y);
    if (prev == Cell::Black) --blackStones;
    else if (prev == Cell::White) --whiteStones;

    cells[i] = Cell::Empty;
}

void BoardState::addOccupied(Pos p) noexcept
{
    const uint16_t i = idx(p);
    if (occIdx_[i] >= 0)
        return; // already tracked
    occIdx_[i] = static_cast<int16_t>(occupied_.size());
    occupied_.push_back(p);
}

void BoardState::removeOccupied(Pos p) noexcept
{
    const uint16_t i = idx(p);
    int16_t posIdx = occIdx_[i];
    if (posIdx < 0)
        return; // not tracked
    const int lastIdx = static_cast<int>(occupied_.size()) - 1;
    if (posIdx != lastIdx) {
        Pos moved = occupied_.back();
        occupied_[posIdx] = moved;
        occIdx_[idx(moved)] = posIdx;
    }
    occupied_.pop_back();
    occIdx_[i] = -1;
}

} // namespace gomoku

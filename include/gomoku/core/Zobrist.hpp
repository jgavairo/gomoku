#pragma once

#include "gomoku/core/Types.hpp"
#include <cstdint>

namespace gomoku::zobrist {

// Returns the Zobrist key for placing cell 'c' at (x, y).
// For Empty cells, the key is 0.
uint64_t piece(Cell c, uint8_t x, uint8_t y) noexcept;

// Returns the Zobrist key for side to move.
uint64_t side() noexcept;

} // namespace gomoku::zobrist

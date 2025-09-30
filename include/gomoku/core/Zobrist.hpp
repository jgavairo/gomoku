#pragma once
#include "gomoku/core/Types.hpp"
#include <cstdint>

namespace gomoku::zobrist {

using FlatIdx = uint16_t;

// Zobrist key pour (c, index linéaire).
uint64_t piece(Cell c, FlatIdx idx) noexcept;

// Surcharges de confort (délèguent à la version FlatIdx)
inline uint64_t piece(Cell c, uint8_t x, uint8_t y) noexcept
{
    return piece(c, static_cast<FlatIdx>(y * BOARD_SIZE + x));
}
inline uint64_t piece(Cell c, Pos p) noexcept
{
    return piece(c, p.toIndex());
}

// Side-to-move toggle (à XOR à chaque changement de trait).
uint64_t side() noexcept;

// (Optionnel) pour tests : redéfinir les clés de façon déterministe
void reseed(uint64_t seed) noexcept;

} // namespace gomoku::zobrist

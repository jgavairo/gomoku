#pragma once

#include "gomoku/core/BoardState.hpp"
#include "gomoku/core/Types.hpp"

namespace gomoku::pattern {

// Returns true if move m would illegally create a double-three for the player, accounting for virtual captures.
bool createsIllegalDoubleThree(const BoardState& state, Move m, const RuleSet& rules);

// Returns true if there are 5 or more stones of 'who' aligned including position p.
bool checkFiveOrMoreFrom(const BoardState& state, Pos p, Cell who);

// Returns true if there exists anywhere on the board a 5+ line for 'who'.
bool hasAnyFive(const BoardState& state, Cell who);

// After player 'justPlayed' placed a stone and captures were applied, can the opponent immediately
// break the five-plus line by a capturing move? Also returns true if opponent can immediately
// win by capture (reach captureWinPairs).
bool isFiveBreakableNow(const BoardState& state, Player justPlayed, const RuleSet& rules);

} // namespace gomoku::pattern

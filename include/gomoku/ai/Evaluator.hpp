#pragma once
#include "gomoku/core/Types.hpp"

namespace gomoku {
class Board;
}

namespace gomoku::eval {

// Évaluation statique rapide d'une position (Gomoku):
//  - Patterns: cinq (win), overline (6+) (selon règles), quatre ouvert/fermé, trois ouverts, double-trois (interdit selon règles).
//  - Captures de paires: avantage/menace de capture, victoire par captures possible.
//  - Géométrie: centralité et densité locale autour du front de jeu (proximité des dernières pierres).
//  - Perspective: score positif si favorable au joueur 'perspective'.
//
// Note: Terminal states should be handled by the search (isTerminal) before calling this.
// This function returns 0 for drawn positions but does not detect ongoing games.
int evaluate(const Board& board, Player perspective) noexcept;

} // namespace gomoku::eval

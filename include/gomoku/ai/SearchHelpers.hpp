#pragma once
#include "gomoku/ai/TranspositionTable.hpp"
#include "gomoku/core/Types.hpp"
#include <optional>

namespace gomoku {
class Board;
}

namespace gomoku::search {

// Constants for search scoring
constexpr int INF = 1'000'000; // Generic infinity bound for alpha-beta
constexpr int MATE_SCORE = 900'000; // Base score for mate-like terminal outcomes

// Détecte si la position est terminale (Gomoku): victoire (5 alignés ou par captures) ou nul.
// Score retourné: négatif au trait si l'adversaire vient de gagner (correction distance-mate incluse).
// Returns true if terminal, false otherwise. Sets outScore to the evaluation from current player's perspective.
bool isTerminal(const Board& board, int ply, int& outScore) noexcept;

// Interroge la TT: si une entrée suffisante existe, fournit un bound et/ou un coup d'appoint.
// Returns true if a usable bound/cutoff is found, false otherwise.
// Always provides ttMove for move ordering if available (even when returning false).
bool ttProbe(const TranspositionTable& tt, const Board& board, int depth, int alpha, int beta, int& outScore, std::optional<Move>& ttMove, TranspositionTable::Flag& outFlag) noexcept;

// Stocke un résultat dans la TT (clé, profondeur, score, flag, meilleur coup).
// Relies on TT's internal replacement policy (prefer deeper entries).
void ttStore(TranspositionTable& tt, const Board& board, int depth, int score, TranspositionTable::Flag flag, const std::optional<Move>& best) noexcept;

} // namespace gomoku::search

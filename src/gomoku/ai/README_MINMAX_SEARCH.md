# Gomoku AI Engine

This directory contains the implementation Gomoku AI engine based on the Minimax algorithm (Negamax variant) with Alpha-Beta pruning and search optimizations.

## Architecture Overview

The AI is composed of several modular components designed for efficiency and extensibility:

- **MinimaxSearch**: The core search engine implementing the recursive Negamax algorithm.
- **Evaluator**: Static evaluation function assessing board positions based on patterns and heuristics.
- **CandidateGenerator**: Generates plausible moves to reduce the branching factor.
- **MoveOrderer**: Sorts moves to maximize Alpha-Beta pruning efficiency.
- **TranspositionTable**: Caches search results to avoid re-evaluating identical positions.

## Key Features

### 1. Search Algorithm (`MinimaxSearch`)

- **Negamax with Alpha-Beta Pruning**: Standard minimax variant for zero-sum games.
- **Iterative Deepening**: Searches progressively deeper (depth 1, 2, 3...) to ensure a best move is always available if time runs out and to improve move ordering for subsequent depths.
- **Principal Variation Search (PVS)**: Assumes the first move (best from previous iteration) is the best and searches others with a null window to prove they are inferior.
- **Aspiration Windows**: Searches within a narrow score window around the previous iteration's score to prune branches faster. Re-searches with wider windows if the score falls outside.
- **Late Move Reduction (LMR)**: Reduces search depth for moves that are late in the sorted list (assumed to be less promising), saving significant computation time.
- **Quiescence Search (`qsearch`)**: Continues searching beyond the horizon for "noisy" tactical moves (captures, immediate threats) to avoid the horizon effect.

### 2. Evaluation (`Evaluator`)

The static evaluation function analyzes the board from the perspective of the player to move. It considers:

- **Pattern Matching**: Detects open/closed fours, threes, twos, and ones.
- **Strategic Structures**: Rewards powerful shapes like double open threes or four-three combinations.
- **Capture Analysis**: Evaluates potential captures and capture setups (pairs).
- **Positional Factors**: Slight bias towards the center and front lines.

### 3. Move Generation & Ordering (`CandidateGenerator`, `MoveOrderer`)

To handle the large branching factor of Gomoku (19x19 board):

- **Candidate Generation**: Instead of all legal moves, generates moves only in the vicinity of existing stones (within a configurable radius).
- **Tactical Generation**: For Quiescence Search, generates only captures and strong threats.
- **Move Ordering**:
  - **Transposition Table Move**: The best move from a previous search is tried first (PV-move).
  - **Killer Heuristic**: Stores moves that caused a beta-cutoff at the same tree depth in other branches.
  - **History Heuristic**: Tracks moves that frequently cause cutoffs across the entire search tree.
  - **Captures & Promotions**: Captures are prioritized.

### 4. Transposition Table (`TranspositionTable`)

- Uses Zobrist hashing (implied by `uint64_t key`) to map board states to stored results.
- Stores:
  - **Score**: The evaluation of the position.
  - **Flag**: Exact score, Lower bound (beta cutoff), or Upper bound (alpha cutoff).
  - **Depth**: The depth at which this result was found.
  - **Best Move**: The move that led to this score.
- Replacement strategy prefers deeper searches and newer entries.

## Configuration

The engine is highly configurable via `SearchConfig` and `EvalConfig` structs:

- **Time Budget**: Max execution time per move.
- **Depth Limits**: Soft and hard depth limits.
- **Table Size**: Memory allocation for the Transposition Table.
- **Heuristic Weights**: Tunable values for patterns (Open Four, Closed Three, etc.) and strategic bonuses.

## Usage

The main entry point is `MinimaxSearch::bestMove`. It takes the current board state and returns the optimal move found within the time budget.

```cpp
gomoku::SearchConfig config;
config.timeBudgetMs = 1000;
gomoku::MinimaxSearch ai(config);

gomoku::SearchStats stats;
auto move = ai.bestMove(board, rules, &stats);
if (move) {
    board.play(*move);
}
```

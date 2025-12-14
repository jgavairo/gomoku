# Gomoku Heuristic Evaluator

This document details the static evaluation function implemented in `Evaluator.cpp`. The evaluator assigns a score to a given board state from the perspective of the current player. It is designed to be comprehensive, covering static board analysis, strategic pattern detection, and dynamic factors based on recent gameplay.

## Core Evaluation Logic

The evaluation function (`Evaluator::evaluate`) computes a linear combination of weighted features:
$$ Score = \sum (w_i \times feature_i) $$

The following sections demonstrate how the heuristic satisfies key technical requirements.

### 1. Static Analysis: Alignments & Patterns

The core of the heuristic is the detection of stone alignments (lines of 2, 3, 4, or 5 stones).

* **Alignments**: The board is scanned in all 4 directions (Horizontal, Vertical, Diagonal /, Diagonal \). Every continuous run of stones is measured and scored based on its length and type (Open vs Closed).
  * *Code Reference*: The main loop iterates over `occupiedPositions()` and checks `DIRS`.
* **Potential Win (Space to Develop)**: A crucial check ensures that an alignment is only valuable if it has enough space to become a 5-in-a-row. Dead lines (blocked by edges or opponent stones) are discarded.
  * *Logic*: `bool canWin = (len + leftSpace + rightSpace >= 5);` If `!canWin`, the pattern value is set to 0.
* **Freedom (Space Quality)**: Beyond just "open ends", the heuristic assesses the "freedom" of a pattern—how much empty space surrounds it.
  * **Free**: Open ends and ample space (bonus applied).
  * **Half-Free**: One open end but space to extend.
  * **Flanked**: Blocked or limited space.
  * *Logic*: `assessFreedom()` helper function applies a multiplier (e.g., 1.3x for fully free patterns) to prioritize flexible shapes.

### 2. Capture Mechanics

Gomoku rules (capture by pairs) are deeply integrated into the score.

* **Current Captures**: The difference in captured pairs is a massive score component.
  * *Logic*: `score += (myCaptures - oppCaptures) * capturePairValue`.
* **Potential Captures**: The heuristic proactively detects the "sandwich" pattern (`XOO_`) that threatens a capture on the next move.
  * *Logic*: `hasCapturePattern()` detects `XOO_` or `_OOX`. It awards a `captureSetupBonus` for creating threats and applies a `captureSetupPenalty` for ignoring opponent threats.

### 3. Strategic Figures & Combinations

The heuristic goes beyond individual lines to find "Figures"—combinations of threats that force a win (Forks).

* **Advantageous Combinations**: It counts the number of threats (Open 4s, Open 3s) for each player and checks for specific winning combinations:
  * **Double Open 4**: Immediate win.
  * **4-3 Combination**: Forcing move leading to a win.
  * **Double Open 3**: Strong fork.
  * *Logic*: The `myThreats[]` array tracks counts of each pattern type, and bonuses are added for overlapping threats (e.g., `if (myThreats[OPEN_3] >= 2) ...`).

### 4. Player Perspective

* **Both Players**: The evaluation is strictly zero-sum relative to the `perspective` player.
  * *Logic*: The loop iterates through all stones. If a stone/pattern belongs to `me`, the score is added. If it belongs to `opp` (opponent), the score is subtracted. This ensures the AI plays to maximize its advantage while minimizing the opponent's.

### 5. Dynamic Evaluation

* **Past Actions (Front Proximity)**: The heuristic is not purely static; it considers the "momentum" of the game.
  * *Logic*: It retrieves the last 3 moves (`board.lastMoves(3)`). Stones placed near these recent moves receive a `frontWeight` bonus. This helps the AI focus its search on the active area of the board ("The Front") rather than analyzing irrelevant distant stones, effectively weighing the board state based on the flow of the game.

## Tuning

The weights for all these components are defined in `EvalConfig` (in `Evaluator.hpp`) and can be tuned to adjust the AI's personality (aggressive vs defensive).

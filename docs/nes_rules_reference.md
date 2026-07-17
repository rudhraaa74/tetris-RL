# NES Tetris Rules & Constants Reference

This document serves as the single source of truth for all NES-accurate timings, scoring mechanisms, and rule constants used in `engine/Timing` and `engine/Scoring`. Every constant listed here maps directly to a hardcoded value in the engine, and every quirk must be backed by a unit test.

## 1. Gravity Table
Gravity in NES Tetris is determined by the current level and is measured in frames per row (grid cell drop). 

| Level | Frames | Level | Frames | Level | Frames |
|---|---|---|---|---|---|
| 0 | 48 | 10 | 5 | 20 | 2 |
| 1 | 43 | 11 | 5 | 21 | 2 |
| 2 | 38 | 12 | 5 | 22 | 2 |
| 3 | 33 | 13 | 4 | 23 | 2 |
| 4 | 28 | 14 | 4 | 24 | 2 |
| 5 | 23 | 15 | 4 | 25 | 2 |
| 6 | 18 | 16 | 3 | 26 | 2 |
| 7 | 13 | 17 | 3 | 27 | 2 |
| 8 | 8  | 18 | 3 | 28 | 2 |
| 9 | 6  | 19 | 2 | 29 | 1 |

*Source: Hard Drop Tetris Wiki's `Tetris_(NES,_Nintendo)` page. Reverse-engineered directly from the NES ROM (stored at address `$898E`, offset `$099E` in iNES format).*

## 2. DAS (Delayed Auto Shift)
- **Initial Delay:** 16 frames.
- **Auto-Repeat Rate (ARR):** Every 6 frames thereafter.
- **Reset Logic (Critical):** The DAS counter does NOT reset to 0 after an auto-shift fires. It resets to 10. The sequence is: hold input → first auto-shift at 16 frames → counter resets to 10 → next auto-shift at 10+6=16 frames later. It only resets to 0 when the input is fully released and pressed again.
- **Wall Charge/Block:** If a shift attempt is blocked by a wall or the stack, the counter instantly jumps to 16 (fully charged).
- **Dead/Frozen State:** The counter is fully frozen during ARE and line-clear delays. Pressing directions during these windows has no effect on the counter's value, but any charge already accumulated is retained and can be "redirected" once active play resumes.

*Source: Hard Drop Tetris Wiki, `Tetris_(NES,_Nintendo)` page (Kitaru's memory-value research).*

## 3. ARE (Entry Delay)
The delay between a piece locking and the next piece spawning depends on the lock height.
- **Lock Height 0-2 (bottom 2 rows):** 10 frames.
- **Lock Height 3-6:** 12 frames.
- **Lock Height 7-10:** 14 frames.
- **Lock Height 11-14:** 16 frames.
- **Lock Height 15-18:** 18 frames.
*(Formula: 10 frames for bottom 2 rows, +2 frames for each group of 4 rows above).*

*Source: Hard Drop Tetris Wiki, `Tetris_(NES,_Nintendo)` page.*

## 4. Line Clear Delay
An additional **17-20 frames** of delay is added beyond ARE when lines are cleared. This is driven by a 5-step animation that advances every 4 frames. The exact total (17, 18, 19, or 20) depends on which frame within the 4-frame cycle the piece locked on.

*Source: Hard Drop Tetris Wiki, `Tetris_(NES,_Nintendo)` page.*

## 5. Scoring System
Base points are awarded per line clear, multiplied by `(Level + 1)`.

| Clear Type | Base Points (Level 0) |
|---|---|
| Single | 40 |
| Double | 100 |
| Triple | 300 |
| Tetris | 1200 |

*Source: NES Tetris Instruction Manual.*

## 6. Level-Up Thresholds
- **First Transition:** `min(startLevel * 10 + 10, max(100, startLevel * 10 - 50))` lines cleared.
- **Subsequent Transitions:** Every 10 additional lines.

**Explicit Test Cases (must verify in tests):**
- Start Level 5 → Level 6 at 60 lines → Level 7 at 70 lines.
- Start Level 12 → Level 13 at 100 lines → Level 14 at 110 lines.
- Start Level 16 → Level 17 at 110 lines → Level 18 at 120 lines.

*Source: Hard Drop Tetris Wiki.*

---

## Resolved Decisions

**1. Level Multiplier Timing for Line Clears:**
NES uses the level *after* the line clear resolves as the scoring multiplier, not the level the piece locked at. For example, if clearing a Tetris pushes the player from level 5 to level 6, that Tetris scores at the level-6 multiplier (×7), not level-5 (×6). This rule is explicitly required and must be tested in `engine/Scoring`.

**2. Soft-Drop Scoring Behavior:**
*Decision: Implement the "intended" clean version (1 point per cell dropped, respecting the last-press-only rule).*
*Rationale:* Real hardware soft-drop scoring contains a BCD (binary-coded decimal) addition bug that results in inaccurate point awards in edge cases. We will respect the "last continuous press only" rule but omit the undocumented BCD bug. This favors a clean, consistent implementation that is easier to verify, while retaining the correct spirit of NES accuracy for RL purposes. Replicating an obfuscated hardware bug adds unnecessary reverse-engineering effort and test complexity for negligible impact on macro-agent performance.

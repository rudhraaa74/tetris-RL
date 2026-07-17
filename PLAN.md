# Tetris RL Project — Master Plan

**Goal:** Build an NES-accurate Tetris engine in C++, train a PPO agent (CPU-only, per-frame action space) to play it, and push toward a high score on NES-rule Tetris. Portfolio project demonstrating engine dev, RL, reward engineering, and (later) meta-optimization of reward weights via GA.

**Confirmed decisions:**
- Rendering: raylib
- Action space: **macro placement** — agent chooses (target column, rotation state) for the current piece; the engine internally resolves this into the actual frame-by-frame inputs (see Section 1a)
- Observation: board state (heights/holes/bumpiness) + current piece + next piece — no pixels, no CNN
- RL library: Stable-Baselines3 (PPO)
- Rules target: NES Tetris accuracy (right-handed Nintendo Rotation System, no hold, no wall kick, no lock delay, no hard drop) — accuracy lives in the *engine's mechanics and scoring*, not in how the agent issues actions
- Hardware: CPU only
- No hold piece, ever (not an NES mechanic)

---

## 0. Critical Risk Flags (read first)

1. **Macro placement is much faster to train than per-frame, but shifts complexity into the engine.** Since the agent no longer issues raw left/right/rotate inputs, the C++ engine must contain a "placement resolver" that translates (target column, rotation) into a sequence of legal NES-timed inputs (respecting DAS speed, gravity, no-wall-kick rotation rules) to actually execute the move. This is more engine work up front, but it's a one-time cost, not a per-training-step cost — and it means training itself (CPU-only) should now converge in a much more reasonable timeframe (hours, not days), since the action space and effective episode length are both drastically smaller.
   - Still vectorize environments (SB3 `SubprocVecEnv`) across CPU cores — free speedup regardless of action space.
   - Still checkpoint regularly.

2. **Not every (column, rotation) target is reachable given NES DAS speed and current piece position** — e.g., a piece may not be able to physically reach the far side of the board before locking, depending on gravity speed at the current level. The placement resolver must correctly reject/mask unreachable targets (action masking) rather than silently producing illegal or physically-impossible placements — this is the main correctness risk in this design.

3. **NES Tetris has no soft-ceiling on score at higher levels** — level 29+ ("kill screen") has gravity so fast that even top human players top out. Decide early whether "high score" means realistic sub-29 play or attempting super-human level-29 survival (much harder, likely out of scope for v1). Note: at kill-screen speeds, a piece may not be controllable long enough to reach many placements at all — this interacts directly with risk #2.

4. **Reward hacking risk is real** given how NES scoring works (see Phase 5) — an agent could learn to stall or avoid risk in ways that don't reflect "good Tetris." Needs explicit attention in reward design and evaluation.

---

## 1. NES Tetris Rules Reference (research summary — verify against source before coding)

This is what "NES-accurate" means concretely. Treat this section as the spec for Phase 1.

### Board & Pieces
- Board: 10 columns × 20 visible rows (plus buffer rows above for spawning).
- 7 tetrominoes: I, O, T, S, Z, J, L.
- **Randomizer**: NOT a modern 7-bag. NES uses a biased random roll — first roll picks 0–7 (8 values, where 7 is a "dummy"/reroll trigger), rerolling if it matches the previous piece or hits the dummy value; if the reroll also happens, a second roll (0–6, uniform) is used and dealt unconditionally. Net effect: same piece twice in a row is possible but suppressed, not impossible. This needs its own isolated `Randomizer` class since it directly affects difficulty and is a well-known point of NES-specific frustration ("droughts").
- **No preview piece info beyond the standard "next piece" box** (NES shows next piece; no hold, no multi-piece lookahead).

### Rotation System
- **Right-handed Nintendo Rotation System**: O has no rotation states. I has 2 states (horizontal favors the lower half of its bounding space, vertical favors the right half). J/L/T have 4 states, all centered on the piece's middle block.
- **No wall kicks.** A rotation that would collide is simply rejected — the piece stays in its current orientation and position.
- **No lock delay.** The piece locks immediately when gravity would move it into a collision (i.e., the moment it can't fall further, it locks on the next gravity tick — no "grace period" to slide it further, unlike modern SRS lock-delay behavior).
- **No hard drop.** Only soft drop exists (see below); there is no instant-drop input.

### Timing (frame-based; NES runs at ~60.0988 fps)
- **Gravity**: speed is level-dependent, defined by a lookup table of "frames per row of fall" keyed by level (e.g., level 0 is slow, gets faster each level, floors out at 1 frame/row around level 29 — the infamous "kill screen"). This table must be hardcoded from documented values (source: TetrisWiki / Hard Drop Wiki gravity tables) — do not guess it.
- **DAS (Delayed Auto Shift)**: holding left/right moves the piece once immediately-ish, then after an initial delay of 16 frames, auto-repeats every 6 frames as long as held. The DAS counter has specific reset behavior (resets to a nonzero charge value rather than 0 after a shift, and charge persists through ARE/line-clear in certain conditions) — this is a well-documented NES quirk (see "DAS" article, TetrisWiki) and directly affects how fast a human/agent can shift a piece across the board. Needs to be modeled accurately since it constrains what placements are even reachable before lock.
- **Soft drop**: doubles gravity speed (1/2G equivalent framing) while held; NES awards 1 point per cell soft-dropped (max 20).
- **ARE (entry delay)**: the pause between a piece locking and the next piece spawning; duration depends on how high the piece locked (roughly 10–18 frames, increasing the higher up the board).
- **Line clear delay**: an additional ~17–20 frame animation/delay when lines clear, before the next piece's ARE begins.

### Scoring (A-Type / Marathon)
- Points awarded per line-clear event, scaled by (level+1) and clear size (single/double/triple/tetris), per NES's documented scoring table — hardcode this table exactly, don't approximate.
- Soft-drop points as above.
- **Level progression**: starting level defined by player selection; advances after clearing a level-dependent line threshold (first threshold varies by start level, then +1 level per 10 lines cleared thereafter). Must replicate exact thresholds per documented behavior, since level directly controls gravity speed and thus difficulty and reachable score.

### Game Over
- Top-out condition: new piece cannot spawn without immediate collision (board filled to spawn rows).

**Action item before Phase 1 coding starts:** dedicate a short research pass (few hours) to pull the exact gravity table, DAS constants, ARE table, and scoring table from a primary technical source (TetrisWiki / Hard Drop Wiki data pages) and encode them as constants/tables in the engine — do not hand-derive these from general knowledge, since the exact numbers are what makes this "NES-accurate" rather than "NES-like."

---

## 1a. Placement Resolver (required for macro action space)

Since the agent's action is now "(target column, target rotation)" rather than raw inputs, the engine needs a component that converts that intent into a legal, NES-timed sequence of frame inputs, still fully respecting the rules in Section 1. This keeps the engine's internal simulation just as NES-accurate as a per-frame design — the macro action space only changes *what the agent controls*, not *how the game actually runs internally*.

Responsibilities:
- Given current piece, its spawn position/orientation, current gravity speed (level), and DAS constants, compute the fastest legal sequence of left/right/rotate/soft-drop inputs (frame-by-frame) that gets the piece to the target column and rotation before it locks.
- Reject/flag targets that are physically unreachable in time (see Risk #2) — this feeds directly into **action masking** on the Python/RL side, so the agent only ever selects from currently-legal placements rather than wasting learning signal on impossible ones.
- Execute the resolved input sequence through the *same* underlying frame-stepped simulation used internally (gravity table, DAS counter behavior, no wall kicks, no lock delay) — i.e., this is a scripted "autopilot" layered on top of the same low-level engine, not a shortcut that bypasses NES timing.
- This component is a natural candidate for its own class (`PlacementResolver`) and its own unit tests (given a board/piece/level, does it find the same input sequence a skilled human would use, and correctly reject truly unreachable targets?).

This effectively mirrors how strong existing NES Tetris bots operate: reason at the level of "where should this piece go," while a lower layer handles the mechanical execution under real NES constraints.

---

## 2. Tech Stack

| Layer | Tool |
|---|---|
| Engine | C++17 |
| Rendering | raylib |
| Bindings | pybind11 |
| RL | Python + PyTorch + Stable-Baselines3 (PPO) |
| Vectorized envs | SB3 `SubprocVecEnv` / `VecMonitor` |
| Logging | TensorBoard, Matplotlib |
| Later (Phase 9) | A GA library (e.g. DEAP) or hand-rolled GA for reward-coefficient search |

---

## 3. Folder Structure

```
tetris-rl/
  engine/
    Board.cpp / .h
    Piece.cpp / .h
    Randomizer.cpp / .h      # NES-accurate roll logic
    Timing.cpp / .h          # gravity table, DAS, ARE, line-clear delay constants
    Game.cpp / .h            # orchestrates the above, exposes reset()/step()/getState()
    RewardCalculator.cpp / .h
    Renderer.cpp / .h        # raylib, isolated from Game logic
  bindings/
    pybind11 module (tetris_env)
  python/
    env.py                   # Gymnasium-compatible wrapper around the pybind11 module
    train.py
    evaluate.py
    baseline_random.py
    ga_search.py              # Phase 9 only
  models/
  logs/
  videos/
  checkpoints/
  docs/
    nes_rules_reference.md    # the researched constants/tables, cited
  README.md
```

---

## 4. Phase-by-Phase Plan

### Phase 1 — NES-Accurate Engine (C++, no AI, no rendering dependency for logic)
- Implement `Board`, `Piece`, `Randomizer`, `Timing`, `Game` per Section 1.
- Frame-stepped simulation: `Game::tick()` advances exactly one NES frame; input (left/right/rotateCW/rotateCCW/softDrop/none) is sampled once per tick, matching real controller polling.
- Expose: `reset()`, `step(Action) -> (state, reward, done)`, `getState()`, `isGameOver()`, `getScore()`, `getLevel()`, `getLines()`.
- Unit tests against known NES behavior where possible (e.g., verified gravity frame counts per level, DAS timing, scoring table values for known line-clear/level combos).
- **Milestone check:** play it yourself via keyboard input (raylib renderer) and confirm it *feels* like NES Tetris (DAS charge, no hard drop, no wall kicks) before moving on.

### Phase 2 — Decouple for RL
- Split `Controller` from `Game`: `HumanController` (keyboard → raylib) vs `AIController` (receives action from Python side).
- `render` flag: off for training (raw simulation speed), on for demos/video capture.
- Confirm the engine can run with rendering fully disabled with no behavioral difference (critical for training throughput).

### Phase 3 — Python Interface (pybind11)
- Bind `Game` methods to Python.
- Build `python/env.py`: a Gymnasium-style wrapper (`reset()`, `step(action)`, `observation_space`, `action_space`) around the raw bindings — SB3 expects the Gymnasium API, not a bespoke one.
- **Observation space** (board-state, not pixels):
  - Column heights (10)
  - Aggregate height
  - Hole count
  - Bumpiness (sum of adjacent column height differences)
  - Current piece (one-hot, 7)
  - Next piece (one-hot, 7)
  - Current level (normalized scalar) — matters a lot in NES since gravity speed changes both difficulty and which placements are reachable
- **Action space**: `Discrete(N)` — one action per (column, rotation) combination for the current piece (N ≈ up to 10 columns × up to 4 rotations, fewer for O/I; exact N depends on how you enumerate invalid combos). Each action is resolved into a full placement by the C++ `PlacementResolver` (Section 1a) in a single `step()` call — one RL decision = one piece placed, not one frame.
- **Action masking is required**, not optional: at each `step()`, the environment must expose which (column, rotation) targets are currently reachable given the piece/level/DAS state, and mask out the rest. SB3 supports this via `MaskablePPO` (from `sb3-contrib`) — plan to use that rather than vanilla PPO, since an unmasked agent will waste significant training time selecting placements the engine then has to reject or reinterpret.
- Benchmark raw placements/sec of the environment (no NN) early — with macro actions this should be dramatically faster than per-frame throughput was going to be, but confirm empirically rather than assuming.

### Phase 4 — Random Agent Baseline
- Run a uniform-random policy over the action space.
- Log: average score, average lines cleared, average survival time (frames), score distribution.
- This is the floor every future agent is compared against — save these numbers into `docs/` for reference in the final writeup.

### Phase 5 — Initial Reward Function
- Isolate all reward logic in `RewardCalculator` (C++) so it can be swapped without touching engine internals.
- Starting point (placeholder, to be tuned):
  ```
  + line clear reward, scaled by clear size (mirror NES scoring shape: tetris >> triple > double > single)
  - hole created
  - increase in aggregate height / bumpiness
  - large penalty on game over
  ```
- **Reward hacking watch-list specific to NES rules:**
  - Since soft-drop grants points, make sure the RL reward isn't naively copying raw NES score if that encourages excessive/unsafe soft-dropping as a degenerate strategy.
  - Since there's no hard drop, an agent could in principle "wait out" gravity — make sure episode/step limits and no-op cost (if any) don't create weird incentives to stall.
  - Decide whether reward is based on raw NES score (interpretable, matches the actual goal) or fully shaped custom reward (better learning signal, less interpretable) — recommend training on shaped reward, but always *reporting* results in terms of true NES score.

### Phase 6 — Train with PPO (Stable-Baselines3)
- Use `MaskablePPO` (from `sb3-contrib`) + MLP policy (no CNN, since observations are structured features) — action masking (Section 3) is central to this design, not optional.
- Vectorize with `SubprocVecEnv` across available CPU cores — with macro actions, each env step is a full placement, so this should scale training throughput well even on CPU.
- Key hyperparameters to tune: `n_steps`, `batch_size`, `learning_rate`, `gamma` (should be high, e.g. 0.99+, since one episode is still many piece-placements long), `gae_lambda`, `ent_coef` (exploration).
- Save checkpoints on a schedule (e.g., every N timesteps) — training will likely be interrupted/resumed given CPU-only constraints.
- **Milestone check before scaling up:** first confirm the agent can learn *something* on a simplified/lower-level-only environment (e.g., always start at level 0, cap episode length) before training on full-game complexity — this validates the pipeline cheaply before expensive long-run training.

### Phase 7 — Visualization
- TensorBoard: reward curve, episode length, average score/lines, PPO loss terms (policy loss, value loss, entropy).
- Periodic gameplay video capture (turn `render = true` for eval episodes at checkpoints) — e.g., record at episode/timestep checkpoints 0, then geometric or fixed intervals through training, to show visible improvement over time (this is a core portfolio deliverable).

### Phase 8 — Reward Engineering
- Compare at least 2 hand-designed reward variants (e.g., raw-score-only vs. shaped board-features reward) under identical training budgets and identical evaluation seeds/piece sequences (needed for fair comparison).
- Document: learning speed, final score, stability (variance across seeds), and any reward-hacking behaviors observed.
- This is a written comparison as much as a code task — plan to keep a running log/notebook during this phase.

### Phase 9 — Advanced Extension: GA over Reward Coefficients (optional, last)
- Parameterize reward as `R = a*lines - b*holes - c*height - d*bumpiness [+ more terms as needed]`.
- GA (e.g. DEAP or hand-rolled) searches over `(a, b, c, d, ...)`.
- **Cost control is mandatory here**: each fitness evaluation = one full PPO training run. With CPU-only constraints, budget this tightly — e.g., small population (~10–20), short truncated training budget per individual (enough to rank relative performance, not to fully converge), and treat the final GA-discovered weights as "promising candidates to retrain fully," not final answers.
- Fitness = evaluation score (NES score or lines cleared) of the resulting agent over a fixed eval protocol.

---

## 5. Evaluation Protocol (needed from Phase 4 onward for fair comparisons)

- Fixed set of evaluation seeds (control the NES randomizer's RNG seed) reused across all agents/configs being compared.
- Report: mean score, median score, max score, mean lines cleared, mean survival frames, over N evaluation episodes (e.g., N=50-100).
- Always report **true NES score**, even when training reward is shaped differently, since that's the actual target metric ("high score on NES Tetris").
- Track level distribution reached, since level directly reflects skill/duration in NES rules.

---

## 6. Resolved Design Questions

1. **Action space enumeration**: Use a fixed `Discrete(40)` action space (10 columns × 4 rotations). Invalid or unreachable actions for the current piece shape and board state will be masked out. This is simpler for SB3 integration.
2. **PlacementResolver scoring**: The `PlacementResolver` will track and expose soft-drop points accumulated during a macro-action placement. This ensures true NES scoring accuracy is maintained during evaluation.
3. **Starting level for training**: Use a curriculum learning approach. Start training at level 0. Progressively raise the training start-level distribution as the agent improves (e.g., once survival/score crosses a threshold, sample from 0–9, then 0–19).
4. **High score target**: Focus on sub-kill-screen realistic play (up to level 28). This is a more achievable target for v1 and keeps masking logic standard (attempting level 29+ survival makes most placements physically unreachable).
5. **Build system**: Use CMake to configure and build both the standalone C++ engine and the pybind11 module.
6. **Rendering in Python**: Keep the `tetris_env` Python binding strictly headless (`tetris_core` only) to prevent GL context overhead or crashes in parallel `SubprocVecEnv` workers. For `env.render()`, serialize the state and use a lightweight Python-side renderer (e.g., `pygame`), reserving Raylib exclusively for the standalone C++ human-playable executable (Phases 1-2).
7. **Testing**: Write C++ unit tests using a framework like Google Test (gtest) or Catch2 to verify engine correctness before writing Python bindings.
8. **Observation state**: Stick to the features listed in the plan (heights, aggregate height, holes, bumpiness, pieces, level) and do not include the raw NES score.
9. **Engine RNG & Determinism**: The engine owns its explicit RNG (`std::mt19937`), seeded via `reset(seed)`. Vectorized environments use independent non-overlapping seeds, and the randomizer perfectly replicates NES mechanics.
10. **Placement Validation**: The `PlacementResolver` validates reachability via a 100% accurate internal "shadow simulation" of the drop, ensuring no physically impossible placements are approved.
11. **Action Mask Performance**: To keep training fast, the validity of all 40 macro actions is pre-calculated via shadow simulations the moment a new piece spawns, then cached. This provides O(1) retrieval when SB3 calls `env.action_masks()`.
---

## 7. Definition of Done (v1)

- Engine passes NES-behavior unit tests (gravity, DAS, scoring, rotation, randomizer) against documented reference values.
- Trained PPO agent significantly outperforms random baseline (Phase 4) on true NES score, lines cleared, and survival time, under the fixed evaluation protocol.
- TensorBoard curves + checkpoint gameplay videos showing visible improvement over training.
- Written comparison of at least 2 reward function variants (Phase 8).
- README tells the full story: engine → baseline → PPO → reward engineering → (optional) GA reward search.
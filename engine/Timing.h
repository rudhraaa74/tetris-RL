#pragma once

namespace tetris {
namespace timing {

// Returns the number of frames per grid cell drop at a given level.
// Valid levels are 0-29. Levels > 29 return the level 29 value (1 frame).
int getGravityFrames(int level);

// DAS constants
constexpr int DAS_INITIAL_DELAY = 16;
constexpr int DAS_AUTO_REPEAT_RATE = 6;
constexpr int DAS_RESET_CHARGE = 10;
constexpr int DAS_FULL_CHARGE = 16;

// ARE (Entry Delay) constants
// Lock height is 0-indexed from the bottom of the board.
// Bottom 2 rows = 10 frames, +2 frames for every 4 rows above.
int getAREFrames(int lockHeightFromBottom);

// Line clear animation delay
// We use 20 as a static value for the 17-20 frame animation range.
constexpr int LINE_CLEAR_DELAY = 20;

} // namespace timing
} // namespace tetris

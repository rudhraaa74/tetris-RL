#pragma once

namespace tetris {
namespace scoring {

// Returns the score awarded for clearing a given number of lines at a specific level.
// Note: As per NES rules, the level passed here MUST be the level *after* the line clear resolves.
int getLineClearScore(int linesCleared, int levelAfterClear);

// Returns the score for soft dropping.
int getSoftDropScore(int cellsDropped);

// Returns the total lines required to advance from the given start level to the next level.
int getFirstLevelUpLineThreshold(int startLevel);

// Returns the new level given a start level and the total lines cleared so far.
int computeLevel(int startLevel, int totalLinesCleared);

} // namespace scoring
} // namespace tetris

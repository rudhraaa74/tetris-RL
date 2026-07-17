#include "Scoring.h"
#include <algorithm>

namespace tetris {
namespace scoring {

int getLineClearScore(int linesCleared, int levelAfterClear) {
    int baseScore = 0;
    switch (linesCleared) {
        case 1: baseScore = 40; break;
        case 2: baseScore = 100; break;
        case 3: baseScore = 300; break;
        case 4: baseScore = 1200; break;
        default: return 0;
    }
    return baseScore * (levelAfterClear + 1);
}

int getSoftDropScore(int cellsDropped) {
    // Intended clean version: 1 point per cell of the last continuous soft-drop press.
    return std::max(0, cellsDropped);
}

int getFirstLevelUpLineThreshold(int startLevel) {
    return std::min(startLevel * 10 + 10, std::max(100, startLevel * 10 - 50));
}

int computeLevel(int startLevel, int totalLinesCleared) {
    int firstThreshold = getFirstLevelUpLineThreshold(startLevel);
    if (totalLinesCleared < firstThreshold) {
        return startLevel;
    }
    int additionalLines = totalLinesCleared - firstThreshold;
    return startLevel + 1 + (additionalLines / 10);
}

} // namespace scoring
} // namespace tetris

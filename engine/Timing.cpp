#include "Timing.h"
#include <algorithm>

namespace tetris {
namespace timing {

static const int GRAVITY_TABLE[] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 
    5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 1
};

int getGravityFrames(int level) {
    if (level < 0) return GRAVITY_TABLE[0];
    if (level > 29) return GRAVITY_TABLE[29];
    return GRAVITY_TABLE[level];
}

int getAREFrames(int lockHeightFromBottom) {
    int height = std::max(0, lockHeightFromBottom);
    if (height <= 1) return 10;
    if (height <= 5) return 12;
    if (height <= 9) return 14;
    if (height <= 13) return 16;
    return 18;
}

} // namespace timing
} // namespace tetris

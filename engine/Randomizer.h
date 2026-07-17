#pragma once
#include <random>
#include "Piece.h"

namespace tetris {

class Randomizer {
public:
    Randomizer();

    // Seeds the randomizer for reproducible sequences
    void seed(unsigned int seedValue);

    // Returns the next piece type using exact NES logic
    Tetromino nextPiece();

private:
    std::mt19937 m_rng;
    int m_history;
};

} // namespace tetris

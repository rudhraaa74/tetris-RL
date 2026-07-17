#include "Randomizer.h"

namespace tetris {

Randomizer::Randomizer() : m_history(7) {}

void Randomizer::seed(unsigned int seedValue) {
    m_rng.seed(seedValue);
    m_history = 7; // Reset history to dummy value
}

Tetromino Randomizer::nextPiece() {
    // Roll 1: 0-7
    std::uniform_int_distribution<int> dist07(0, 7);
    int roll = dist07(m_rng);

    // Reroll on repeat or dummy (7)
    if (roll == m_history || roll == 7) {
        // Roll 2: 0-6
        std::uniform_int_distribution<int> dist06(0, 6);
        roll = dist06(m_rng);
    }
    
    m_history = roll;
    
    // Map to Tetromino
    switch (roll) {
        case 0: return Tetromino::T;
        case 1: return Tetromino::J;
        case 2: return Tetromino::Z;
        case 3: return Tetromino::O;
        case 4: return Tetromino::S;
        case 5: return Tetromino::L;
        case 6: return Tetromino::I;
    }
    return Tetromino::T; // Fallback
}

} // namespace tetris

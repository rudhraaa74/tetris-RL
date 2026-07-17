#pragma once
#include "Board.h"
#include "Piece.h"
#include "Randomizer.h"
#include "Timing.h"
#include "Scoring.h"
#include <memory>

namespace tetris {

enum class Input {
    NONE,
    LEFT,
    RIGHT,
    ROTATE_CW,
    ROTATE_CCW,
    SOFT_DROP
};

enum class GameState {
    SPAWNING,
    FALLING,
    LOCKING,
    LINE_CLEARING,
    ARE,
    GAME_OVER
};

class Game {
public:
    Game(unsigned int seed = 0, int startLevel = 0);

    // Advances exactly one NES frame based on the given input primitive
    void tick(Input input);

    // Getters for current state
    GameState getState() const { return m_state; }
    int getScore() const { return m_score; }
    int getLevel() const { return m_level; }
    int getLines() const { return m_lines; }
    
    const Board& getBoard() const { return m_board; }
    const Piece* getPiece() const { return m_currentPiece.get(); } // Returns nullptr during ARE/LINE_CLEARING

private:
    void handleSpawning();
    void handleFalling(Input input);
    void handleLocking();
    void handleLineClearing();
    void handleARE();

    // DAS logic helper
    bool processDAS(Input input);

    Board m_board;
    Randomizer m_randomizer;
    std::unique_ptr<Piece> m_currentPiece;

    GameState m_state;
    int m_startLevel;
    int m_level;
    int m_lines;
    int m_score;

    int m_fallTimer;
    int m_dasTimer;
    Input m_lastDasInput;
    int m_areTimer;
    int m_lineClearTimer;
    int m_lockHeight;
    int m_softDropCounter;
    bool m_wasSoftDropping;
};

} // namespace tetris

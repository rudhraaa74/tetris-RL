#pragma once
#include "Board.h"
#include "Piece.h"
#include "Randomizer.h"
#include "Timing.h"
#include "Scoring.h"
#include <memory>
#include <array>

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
    const std::array<int, 7>& getPieceStats() const { return m_pieceStats; }
    
    const Board& getBoard() const { return m_board; }
    const Piece* getPiece() const { return m_currentPiece.get(); } // Returns nullptr during ARE/LINE_CLEARING
    Tetromino getNextPiece() const { return m_nextPiece; }


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
    Tetromino m_nextPiece;


    GameState m_state;
    int m_startLevel;
    int m_level;
    int m_lines;
    int m_score;

    std::array<int, 7> m_pieceStats{};

    // We can pull these from scoring namespace, but let's cache levelTimer;
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

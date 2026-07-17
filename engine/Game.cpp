#include "Game.h"

namespace tetris {

Game::Game(unsigned int seed, int startLevel)
    : m_startLevel(startLevel), m_level(startLevel), m_lines(0), m_score(0),
      m_state(GameState::SPAWNING), m_fallTimer(0), m_dasTimer(0),
      m_lastDasInput(Input::NONE), m_areTimer(0), m_lineClearTimer(0),
      m_lockHeight(0), m_softDropCounter(0), m_wasSoftDropping(false) {
    m_randomizer.seed(seed);
    m_nextPiece = m_randomizer.nextPiece();
}


void Game::tick(Input input) {
    switch (m_state) {
        case GameState::SPAWNING:
            handleSpawning();
            break;
        case GameState::FALLING:
            handleFalling(input);
            break;
        case GameState::LOCKING:
            handleLocking();
            break;
        case GameState::LINE_CLEARING:
            handleLineClearing();
            break;
        case GameState::ARE:
            handleARE();
            break;
        case GameState::GAME_OVER:
            break;
    }
}

void Game::handleSpawning() {
    Tetromino nextType = m_nextPiece;
    m_nextPiece = m_randomizer.nextPiece();
    m_currentPiece = std::make_unique<Piece>(nextType);


    if (!m_board.isValidPiece(*m_currentPiece)) {
        m_state = GameState::GAME_OVER;
    } else {
        m_state = GameState::FALLING;
        m_fallTimer = 0;
        m_softDropCounter = 0;
        m_wasSoftDropping = false;
    }
}

bool Game::processDAS(Input input) {
    if (input == Input::LEFT || input == Input::RIGHT) {
        if (input != m_lastDasInput) {
            // Fresh tap
            m_dasTimer = 0;
            m_lastDasInput = input;
            return true;
        } else {
            m_dasTimer++;
            if (m_dasTimer >= timing::DAS_FULL_CHARGE) {
                m_dasTimer = timing::DAS_RESET_CHARGE;
                return true;
            }
        }
    } else {
        m_dasTimer = 0;
        m_lastDasInput = Input::NONE;
    }
    return false;
}

void Game::handleFalling(Input input) {
    // 1. Process Horizontal Movement & Rotation
    bool shiftRequested = processDAS(input);
    int dx = 0;
    if (shiftRequested) {
        dx = (input == Input::LEFT) ? -1 : 1;
    }

    if (dx != 0) {
        m_currentPiece->move(dx, 0);
        if (!m_board.isValidPiece(*m_currentPiece)) {
            m_currentPiece->move(-dx, 0); // Revert
            m_dasTimer = timing::DAS_FULL_CHARGE; // Wall charge quirk
        }
    }

    if (input == Input::ROTATE_CW || input == Input::ROTATE_CCW) {
        bool cw = (input == Input::ROTATE_CW);
        auto rotatedBlocks = m_currentPiece->getRotatedBlocks(cw);
        if (m_board.areBlocksValid(rotatedBlocks)) {
            m_currentPiece->rotate(cw);
        }
    }

    // 2. Process Soft Drop
    bool isSoftDropping = (input == Input::SOFT_DROP);
    if (isSoftDropping) {
        if (!m_wasSoftDropping) {
            m_softDropCounter = 0; // Reset counter on fresh press
        }
    }
    
    int currentGravity = timing::getGravityFrames(m_level);
    if (isSoftDropping && currentGravity > 2) {
        // NES soft drop drops 1 cell per 2 frames. 
        m_fallTimer++;
        if (m_fallTimer >= 2) {
            m_fallTimer = currentGravity; // Force a drop this frame
        }
    } else {
        m_fallTimer++;
    }
    m_wasSoftDropping = isSoftDropping;

    // 3. Apply Gravity Drop
    if (m_fallTimer >= currentGravity) {
        m_fallTimer = 0;
        m_currentPiece->move(0, 1);
        if (!m_board.isValidPiece(*m_currentPiece)) {
            // Revert drop and lock
            m_currentPiece->move(0, -1);
            m_state = GameState::LOCKING;
        } else {
            // Successfully dropped a cell
            if (isSoftDropping) {
                m_softDropCounter++;
            }
        }
    }
}

void Game::handleLocking() {
    m_score += scoring::getSoftDropScore(m_softDropCounter);
    
    // Calculate lock height (0-indexed from bottom)
    auto blocks = m_currentPiece->getBlocks();
    int maxY = 0; // Highest Y index = lowest on screen
    for (auto b : blocks) {
        if (b.y > maxY) maxY = b.y;
    }
    m_lockHeight = (Board::HEIGHT - 1) - maxY;

    m_board.lockPiece(*m_currentPiece);
    m_currentPiece.reset();

    int cleared = m_board.clearLines();
    if (cleared > 0) {
        m_lines += cleared;
        m_level = scoring::computeLevel(m_startLevel, m_lines);
        m_score += scoring::getLineClearScore(cleared, m_level); // Uses level AFTER clear
        m_lineClearTimer = timing::LINE_CLEAR_DELAY;
        m_state = GameState::LINE_CLEARING;
    } else {
        m_areTimer = timing::getAREFrames(m_lockHeight);
        m_state = GameState::ARE;
    }
}

void Game::handleLineClearing() {
    m_lineClearTimer--;
    if (m_lineClearTimer <= 0) {
        m_areTimer = timing::getAREFrames(m_lockHeight);
        m_state = GameState::ARE;
    }
}

void Game::handleARE() {
    m_areTimer--;
    if (m_areTimer <= 0) {
        m_state = GameState::SPAWNING;
    }
}

} // namespace tetris

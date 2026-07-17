#pragma once
#include <vector>
#include "Piece.h"

namespace tetris {

class Board {
public:
    static constexpr int WIDTH = 10;
    static constexpr int HEIGHT = 22; // 2 buffer rows (0, 1), 20 visible rows (2-21)
    
    Board();

    // Returns true if the block at (x, y) is solid (occupied or out of bounds)
    bool isOccupied(int x, int y) const;

    // Checks if the piece is valid at its current position and rotation
    bool isValidPiece(const Piece& piece) const;

    // Checks if specific blocks are valid
    bool areBlocksValid(const std::vector<Point>& blocks) const;

    // Locks the piece into the board
    void lockPiece(const Piece& piece);

    // Clears full lines and returns the number of lines cleared
    // This immediately modifies the board state
    int clearLines();

    // Reset the board to empty
    void reset();

    // Get board data
    const std::vector<std::vector<int>>& getGrid() const { return m_grid; }

private:
    std::vector<std::vector<int>> m_grid;
};

} // namespace tetris

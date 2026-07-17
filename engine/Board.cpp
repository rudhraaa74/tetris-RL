#include "Board.h"

namespace tetris {

Board::Board() : m_grid(HEIGHT, std::vector<int>(WIDTH, 0)) {}

void Board::reset() {
    for (auto& row : m_grid) {
        std::fill(row.begin(), row.end(), 0);
    }
}

bool Board::isOccupied(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return true;
    }
    return m_grid[y][x] != 0;
}

bool Board::areBlocksValid(const std::vector<Point>& blocks) const {
    for (const auto& pt : blocks) {
        if (isOccupied(pt.x, pt.y)) {
            return false;
        }
    }
    return true;
}

bool Board::isValidPiece(const Piece& piece) const {
    return areBlocksValid(piece.getBlocks());
}

void Board::lockPiece(const Piece& piece) {
    auto blocks = piece.getBlocks();
    for (const auto& pt : blocks) {
        if (pt.y >= 0 && pt.y < HEIGHT && pt.x >= 0 && pt.x < WIDTH) {
            // Store type + 1 so empty is 0
            m_grid[pt.y][pt.x] = static_cast<int>(piece.getType()) + 1;
        }
    }
}

int Board::clearLines() {
    int linesCleared = 0;
    // Iterate from bottom up
    for (int y = HEIGHT - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < WIDTH; ++x) {
            if (m_grid[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            linesCleared++;
            // Shift everything above down by 1
            for (int shiftY = y; shiftY > 0; --shiftY) {
                for (int x = 0; x < WIDTH; ++x) {
                    m_grid[shiftY][x] = m_grid[shiftY - 1][x];
                }
            }
            // Clear top row
            for (int x = 0; x < WIDTH; ++x) {
                m_grid[0][x] = 0;
            }
            // Re-check the same row index since a new row shifted into this y
            y++; 
        }
    }
    return linesCleared;
}

} // namespace tetris

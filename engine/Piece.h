#pragma once
#include <vector>

namespace tetris {

enum class Tetromino { T, J, Z, O, S, L, I };

struct Point {
    int x;
    int y;
};

class Piece {
public:
    Piece(Tetromino type);

    Tetromino getType() const { return m_type; }

    std::vector<Point> getBlocks() const;
    std::vector<Point> getRotatedBlocks(bool cw) const;

    void rotate(bool cw);
    void move(int dx, int dy);

    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getRotation() const { return m_rotation; }

private:
    Tetromino m_type;
    int m_x, m_y;
    int m_rotation;
};

} // namespace tetris

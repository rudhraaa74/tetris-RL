#include "Piece.h"

namespace tetris {

static const std::vector<Point> T_STATES[4] = {
    {{0,1}, {1,1}, {2,1}, {1,2}}, // 0: Flat side down
    {{1,0}, {1,1}, {1,2}, {0,1}}, // 1: CW
    {{1,0}, {0,1}, {1,1}, {2,1}}, // 2: Flat side up
    {{1,0}, {1,1}, {1,2}, {2,1}}  // 3: CCW
};

static const std::vector<Point> J_STATES[4] = {
    {{0,1}, {1,1}, {2,1}, {2,2}}, // 0: Flat side down
    {{1,0}, {1,1}, {1,2}, {0,2}}, // 1: CW
    {{0,0}, {0,1}, {1,1}, {2,1}}, // 2: Flat side up
    {{1,0}, {2,0}, {1,1}, {1,2}}  // 3: CCW
};

static const std::vector<Point> L_STATES[4] = {
    {{0,1}, {1,1}, {2,1}, {0,2}}, // 0: Flat side down
    {{0,0}, {1,0}, {1,1}, {1,2}}, // 1: CW
    {{2,0}, {0,1}, {1,1}, {2,1}}, // 2: Flat side up
    {{1,0}, {1,1}, {1,2}, {2,2}}  // 3: CCW
};

static const std::vector<Point> Z_STATES[2] = {
    {{0,0}, {1,0}, {1,1}, {2,1}}, // 0: Horizontal
    {{2,0}, {1,1}, {2,1}, {1,2}}  // 1: Vertical
};

static const std::vector<Point> S_STATES[2] = {
    {{1,0}, {2,0}, {0,1}, {1,1}}, // 0: Horizontal
    {{1,0}, {1,1}, {2,1}, {2,2}}  // 1: Vertical
};

static const std::vector<Point> I_STATES[2] = {
    {{0,1}, {1,1}, {2,1}, {3,1}}, // 0: Horizontal
    {{2,0}, {2,1}, {2,2}, {2,3}}  // 1: Vertical
};

static const std::vector<Point> O_STATES[1] = {
    {{0,0}, {1,0}, {0,1}, {1,1}}  // 0: 2x2
};

static const std::vector<Point>* getStates(Tetromino type) {
    switch (type) {
        case Tetromino::T: return T_STATES;
        case Tetromino::J: return J_STATES;
        case Tetromino::L: return L_STATES;
        case Tetromino::Z: return Z_STATES;
        case Tetromino::S: return S_STATES;
        case Tetromino::I: return I_STATES;
        case Tetromino::O: return O_STATES;
    }
    return nullptr;
}

static int getNumStates(Tetromino type) {
    switch (type) {
        case Tetromino::T:
        case Tetromino::J:
        case Tetromino::L: return 4;
        case Tetromino::Z:
        case Tetromino::S:
        case Tetromino::I: return 2;
        case Tetromino::O: return 1;
    }
    return 1;
}

Piece::Piece(Tetromino type) : m_type(type), m_rotation(0) {
    // Determine spawn position
    if (type == Tetromino::O) {
        m_x = 4;
        m_y = 1; // dy=0 has highest block, so y=1 gives absolute y=1
    } else if (type == Tetromino::I) {
        m_x = 3;
        m_y = 0; // dy=1 has highest block, so y=0 gives absolute y=1
    } else if (type == Tetromino::Z || type == Tetromino::S) {
        m_x = 3;
        m_y = 1; // dy=0 has highest block, so y=1 gives absolute y=1
    } else {
        // T, J, L
        m_x = 3;
        m_y = 0; // dy=1 has highest block, so y=0 gives absolute y=1
    }
}

std::vector<Point> Piece::getBlocks() const {
    const std::vector<Point>* states = getStates(m_type);
    std::vector<Point> result = states[m_rotation];
    for (auto& pt : result) {
        pt.x += m_x;
        pt.y += m_y;
    }
    return result;
}

std::vector<Point> Piece::getRotatedBlocks(bool cw) const {
    int numStates = getNumStates(m_type);
    int nextRot = m_rotation;
    if (cw) {
        nextRot = (m_rotation + 1) % numStates;
    } else {
        nextRot = (m_rotation - 1 + numStates) % numStates;
    }
    const std::vector<Point>* states = getStates(m_type);
    std::vector<Point> result = states[nextRot];
    for (auto& pt : result) {
        pt.x += m_x;
        pt.y += m_y;
    }
    return result;
}

void Piece::rotate(bool cw) {
    int numStates = getNumStates(m_type);
    if (cw) {
        m_rotation = (m_rotation + 1) % numStates;
    } else {
        m_rotation = (m_rotation - 1 + numStates) % numStates;
    }
}

void Piece::move(int dx, int dy) {
    m_x += dx;
    m_y += dy;
}

} // namespace tetris

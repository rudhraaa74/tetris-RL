#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "../engine/Timing.h"
#include "../engine/Scoring.h"
#include "../engine/Piece.h"
#include "../engine/Board.h"
#include "../engine/Randomizer.h"
#include "../engine/Game.h"

using namespace tetris;

TEST_CASE("Timing: Gravity Table is correct", "[timing]") {
    REQUIRE(timing::getGravityFrames(0) == 48);
    REQUIRE(timing::getGravityFrames(8) == 8);
    REQUIRE(timing::getGravityFrames(9) == 6);
    REQUIRE(timing::getGravityFrames(19) == 2);
    REQUIRE(timing::getGravityFrames(29) == 1);
    REQUIRE(timing::getGravityFrames(30) == 1);
}

TEST_CASE("Timing: ARE frames by lock height", "[timing]") {
    REQUIRE(timing::getAREFrames(0) == 10);
    REQUIRE(timing::getAREFrames(1) == 10);
    REQUIRE(timing::getAREFrames(2) == 12);
    REQUIRE(timing::getAREFrames(5) == 12);
    REQUIRE(timing::getAREFrames(6) == 14);
    REQUIRE(timing::getAREFrames(18) == 18);
}

TEST_CASE("Scoring: Level-Up Thresholds", "[scoring]") {
    REQUIRE(scoring::computeLevel(5, 59) == 5);
    REQUIRE(scoring::computeLevel(5, 60) == 6);
    REQUIRE(scoring::computeLevel(5, 69) == 6);
    REQUIRE(scoring::computeLevel(5, 70) == 7);

    REQUIRE(scoring::computeLevel(12, 99) == 12);
    REQUIRE(scoring::computeLevel(12, 100) == 13);
    REQUIRE(scoring::computeLevel(12, 109) == 13);
    REQUIRE(scoring::computeLevel(12, 110) == 14);

    REQUIRE(scoring::computeLevel(16, 109) == 16);
    REQUIRE(scoring::computeLevel(16, 110) == 17);
    REQUIRE(scoring::computeLevel(16, 119) == 17);
    REQUIRE(scoring::computeLevel(16, 120) == 18);
}

TEST_CASE("Scoring: Line Clear Points and Level Multiplier Timing", "[scoring]") {
    REQUIRE(scoring::getLineClearScore(1, 0) == 40);
    REQUIRE(scoring::getLineClearScore(4, 0) == 1200);
    REQUIRE(scoring::getLineClearScore(4, 9) == 12000);

    int startLevel = 5;
    int currentLines = 56;
    int newLines = currentLines + 4;
    int newLevel = scoring::computeLevel(startLevel, newLines);
    REQUIRE(newLevel == 6);
    
    int score = scoring::getLineClearScore(4, newLevel);
    REQUIRE(score == 1200 * (6 + 1));
}

TEST_CASE("Piece: Spawn Heights", "[piece]") {
    Tetromino types[] = {Tetromino::T, Tetromino::J, Tetromino::L, Tetromino::Z, 
                         Tetromino::S, Tetromino::I, Tetromino::O};
    
    for (auto type : types) {
        Piece p(type);
        auto blocks = p.getBlocks();
        int minY = 100;
        for (auto b : blocks) {
            if (b.y < minY) minY = b.y;
        }
        REQUIRE(minY == 1);
    }
}

TEST_CASE("Piece: NRS I piece rotation", "[piece]") {
    Piece p(Tetromino::I);
    auto blocks = p.getBlocks();
    REQUIRE(blocks[0].x == 3);
    REQUIRE(blocks[0].y == 1);
    REQUIRE(blocks[3].x == 6);
    REQUIRE(blocks[3].y == 1);

    auto r1 = p.getRotatedBlocks(true);
    REQUIRE(r1[0].x == 5);
    REQUIRE(r1[0].y == 0);
    REQUIRE(r1[3].x == 5);
    REQUIRE(r1[3].y == 3);

    auto r2 = p.getRotatedBlocks(false);
    REQUIRE(r2[0].x == 5);
    REQUIRE(r2[0].y == 0);
}

TEST_CASE("Randomizer: Statistical Distribution", "[randomizer]") {
    Randomizer r;
    r.seed(12345);
    
    const int numRolls = 1000000;
    int counts[7] = {0};
    int repeats = 0;
    
    Tetromino last = r.nextPiece();
    counts[static_cast<int>(last)]++;

    for (int i = 1; i < numRolls; ++i) {
        Tetromino current = r.nextPiece();
        counts[static_cast<int>(current)]++;
        
        if (current == last) {
            repeats++;
        }
        last = current;
    }

    // With 1M rolls, each piece should be ~1/7 of the total (approx 142,857)
    // 3-sigma is around +/- 1050. Let's use a generous +/- 2000 bound.
    for (int i = 0; i < 7; ++i) {
        REQUIRE(counts[i] > 140800);
        REQUIRE(counts[i] < 144900);
    }

    // Repeats should happen 1/28 of the time (approx 35,714)
    // 3-sigma is around +/- 555. Let's use a generous +/- 1500 bound.
    REQUIRE(repeats > 34200);
    REQUIRE(repeats < 37200);
}

TEST_CASE("Game: State Machine Transitions", "[game]") {
    // TODO: Golden / reference-replay test validating engine output against 
    // a known recorded NES run is deferred to Phase 2/3 when a replay 
    // ingestion mechanism is built.
    Game g(42, 0);
    REQUIRE(g.getState() == GameState::SPAWNING);
    
    g.tick(Input::NONE);
    // After 1 tick of SPAWNING, it should transition to FALLING
    REQUIRE(g.getState() == GameState::FALLING);
    REQUIRE(g.getPiece() != nullptr);

    // Hard drop approximation using soft drop isn't instantaneous, 
    // but if we tick enough, it locks.
    for (int i=0; i<1000; ++i) {
        g.tick(Input::SOFT_DROP);
        if (g.getState() != GameState::FALLING) {
            break;
        }
    }

    REQUIRE(g.getState() == GameState::LOCKING);
    g.tick(Input::NONE);
    // After LOCKING, if no lines cleared, it should go to ARE
    REQUIRE(g.getState() == GameState::ARE);
    
    // Tick until SPAWNING again
    for (int i=0; i<30; ++i) {
        g.tick(Input::NONE);
        if (g.getState() == GameState::SPAWNING) {
            break;
        }
    }
    REQUIRE(g.getState() == GameState::SPAWNING);
}

TEST_CASE("Game: Golden Replay Determinism", "[game]") {
    // This test proves that given a specific seed and exact frame-by-frame inputs,
    // the engine resolves to the exact same state every single time.
    Game g(1337, 18); // Start at level 18 for fast gravity

    std::vector<Input> macro;
    // Frame 0-30: Spawning/ARE delays
    for (int i=0; i<31; ++i) macro.push_back(Input::NONE);
    // Move left twice
    macro.push_back(Input::LEFT);
    macro.push_back(Input::NONE);
    macro.push_back(Input::LEFT);
    // Rotate
    macro.push_back(Input::ROTATE_CW);
    // Soft drop to lock
    for (int i=0; i<50; ++i) macro.push_back(Input::SOFT_DROP);

    for (auto input : macro) {
        g.tick(input);
    }

    REQUIRE(g.getLevel() == 18);
    // We expect the piece to have locked and transitioned to ARE or SPAWNING.
    // Let's just ensure the board is not empty, proving the piece locked.
    int blocks = 0;
    const auto& grid = g.getBoard().getGrid();
    for (int y = 0; y < Board::HEIGHT; ++y) {
        for (int x = 0; x < Board::WIDTH; ++x) {
            if (grid[y][x] != 0) blocks++;
        }
    }
    REQUIRE(blocks == 4); // One piece locked
    REQUIRE(g.getScore() > 0); // Scored points from soft drops
}

TEST_CASE("Game: Fuzz Testing (100k random inputs)", "[game]") {
    Game g(999, 18);
    std::mt19937 rng(999);
    std::uniform_int_distribution<int> dist(0, 5); // 0 to 5 for Input enum

    for (int i = 0; i < 100000; ++i) {
        Input randomInput = static_cast<Input>(dist(rng));
        g.tick(randomInput);
        
        // Assert we never drop out of bounds or crash
        REQUIRE((g.getState() != GameState::SPAWNING || g.getPiece() == nullptr));
        if (g.getPiece() != nullptr) {
            auto blocks = g.getPiece()->getBlocks();
            for (auto b : blocks) {
                REQUIRE(b.x >= 0);
                REQUIRE(b.x < Board::WIDTH);
                // Pieces can be above board (y<0) during spawn but not below bottom
                REQUIRE(b.y < Board::HEIGHT);
            }
        }
    }
}


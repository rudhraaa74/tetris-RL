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

TEST_CASE("Randomizer: Logic test", "[randomizer]") {
    Randomizer r;
    r.seed(12345);
    
    // We just verify it generates valid pieces and runs without crashing
    for (int i=0; i<100; ++i) {
        Tetromino t = r.nextPiece();
        REQUIRE(static_cast<int>(t) >= 0);
        REQUIRE(static_cast<int>(t) <= 6);
    }
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

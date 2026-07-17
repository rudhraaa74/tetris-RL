#include "raylib.h"
#include "../engine/Game.h"
#include <string>

using namespace tetris;

const int CELL_SIZE = 30;
const int BOARD_X = 50;
const int BOARD_Y = 50;

Color getColor(Tetromino type) {
    switch (type) {
        case Tetromino::T: return MAGENTA;
        case Tetromino::J: return BLUE;
        case Tetromino::Z: return RED;
        case Tetromino::O: return YELLOW;
        case Tetromino::S: return GREEN;
        case Tetromino::L: return ORANGE;
        case Tetromino::I: return SKYBLUE;
    }
    return WHITE;
}

int main() {
    InitWindow(800, 800, "NES Tetris");
    SetTargetFPS(60);

    int currentStartLevel = 0;
    Game game(42, currentStartLevel);

    while (!WindowShouldClose()) {
        Input input = Input::NONE;
        
        // Change level and restart
        if (IsKeyPressed(KEY_EQUAL)) {
            currentStartLevel++;
            game = Game(42, currentStartLevel);
        } else if (IsKeyPressed(KEY_MINUS)) {
            if (currentStartLevel > 0) {
                currentStartLevel--;
                game = Game(42, currentStartLevel);
            }
        }
        
        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_UP)) input = Input::ROTATE_CW;
        else if (IsKeyPressed(KEY_Z)) input = Input::ROTATE_CCW;
        else if (IsKeyDown(KEY_LEFT)) input = Input::LEFT;
        else if (IsKeyDown(KEY_RIGHT)) input = Input::RIGHT;
        else if (IsKeyDown(KEY_DOWN)) input = Input::SOFT_DROP;

        game.tick(input);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw Board
        const auto& grid = game.getBoard().getGrid();
        for (int y = 2; y < Board::HEIGHT; ++y) {
            for (int x = 0; x < Board::WIDTH; ++x) {
                int cell = grid[y][x];
                Rectangle rect = { (float)(BOARD_X + x * CELL_SIZE), (float)(BOARD_Y + (y - 2) * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
                
                if (cell > 0) {
                    DrawRectangleRec(rect, getColor(static_cast<Tetromino>(cell - 1)));
                    DrawRectangleLinesEx(rect, 1, BLACK);
                } else {
                    DrawRectangleLinesEx(rect, 1, LIGHTGRAY);
                }
            }
        }

        // Draw Piece
        if (const Piece* p = game.getPiece()) {
            auto blocks = p->getBlocks();
            for (auto pt : blocks) {
                if (pt.y >= 2) {
                    Rectangle rect = { (float)(BOARD_X + pt.x * CELL_SIZE), (float)(BOARD_Y + (pt.y - 2) * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
                    DrawRectangleRec(rect, getColor(p->getType()));
                    DrawRectangleLinesEx(rect, 1, BLACK);
                }
            }
        }

        // Draw Next Piece
        Tetromino nextType = game.getNextPiece();
        DrawText("Next:", 400, 50, 20, BLACK);
        Piece nextP(nextType);
        auto nextBlocks = nextP.getBlocks();
        for (auto pt : nextBlocks) {
            // Shift it to display area (e.g. x=14, y=2)
            Rectangle rect = { (float)(400 + (pt.x - 3) * CELL_SIZE), (float)(80 + pt.y * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
            DrawRectangleRec(rect, getColor(nextType));
            DrawRectangleLinesEx(rect, 1, BLACK);
        }

        // Draw Stats
        DrawText(TextFormat("Level: %d", game.getLevel()), 400, 160, 20, BLACK);
        DrawText(TextFormat("Lines: %d", game.getLines()), 400, 190, 20, BLACK);
        DrawText(TextFormat("Score: %d", game.getScore()), 400, 220, 20, BLACK);

        const char* stateStr = "";

        switch (game.getState()) {
            case GameState::SPAWNING: stateStr = "SPAWNING"; break;
            case GameState::FALLING: stateStr = "FALLING"; break;
            case GameState::LOCKING: stateStr = "LOCKING"; break;
            case GameState::LINE_CLEARING: stateStr = "LINE_CLEARING"; break;
            case GameState::ARE: stateStr = "ARE"; break;
            case GameState::GAME_OVER: stateStr = "GAME_OVER"; break;
        }
        DrawText(TextFormat("State: %s", stateStr), 400, 250, 20, RED);
        
        DrawText("Controls:", 400, 310, 20, DARKGRAY);
        DrawText("Left/Right: Move", 400, 340, 20, DARKGRAY);
        DrawText("Up/X: Rotate CW", 400, 370, 20, DARKGRAY);
        DrawText("Z: Rotate CCW", 400, 400, 20, DARKGRAY);
        DrawText("Down: Soft Drop", 400, 430, 20, DARKGRAY);
        DrawText("+ / - : Change Level & Restart", 400, 460, 20, BLUE);


        EndDrawing();
    }

    CloseWindow();
    return 0;
}

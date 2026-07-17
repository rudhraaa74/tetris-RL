#include "raylib.h"
#include "../engine/Game.h"
#include <string>
#include <cstdio>

using namespace tetris;

void DrawNESBlock(int x, int y, Tetromino t) {
    Color light = WHITE;
    Color fill, shadow;
    
    switch (t) {
        case Tetromino::T: 
            fill = {160, 32, 240, 255}; // Purple
            shadow = {100, 20, 150, 255}; 
            break;
        case Tetromino::J: 
            fill = {0, 112, 236, 255}; // Blue
            shadow = {0, 0, 168, 255}; 
            break;
        case Tetromino::Z: 
            fill = {216, 40, 0, 255}; // Red
            shadow = {136, 20, 0, 255}; 
            break;
        case Tetromino::O: 
            fill = {255, 200, 0, 255}; // Yellow
            shadow = {180, 140, 0, 255}; 
            break;
        case Tetromino::S: 
            fill = {0, 168, 0, 255}; // Green
            shadow = {0, 100, 0, 255}; 
            break;
        case Tetromino::L: 
            fill = {255, 120, 0, 255}; // Orange
            shadow = {180, 80, 0, 255}; 
            break;
        case Tetromino::I: 
            fill = {0, 232, 216, 255}; // Cyan
            shadow = {0, 160, 140, 255}; 
            break;
    }
    
    // 8x8 Block
    DrawRectangle(x, y, 8, 8, BLACK);
    DrawRectangle(x, y, 7, 7, light);
    DrawRectangle(x+1, y+1, 6, 6, shadow);
    DrawRectangle(x+1, y+1, 5, 5, fill);
}

void DrawUIPiece(int x, int y, Tetromino type) {
    // Local coordinate offsets for each piece, perfectly aligned for the UI boxes
    struct Pt { int x, y; };
    const Pt* shape;
    
    const Pt shapeT[] = {{0,1}, {1,1}, {2,1}, {1,0}};
    const Pt shapeJ[] = {{0,1}, {1,1}, {2,1}, {0,0}};
    const Pt shapeZ[] = {{0,0}, {1,0}, {1,1}, {2,1}};
    const Pt shapeO[] = {{0,0}, {1,0}, {0,1}, {1,1}};
    const Pt shapeS[] = {{1,0}, {2,0}, {0,1}, {1,1}};
    const Pt shapeL[] = {{0,1}, {1,1}, {2,1}, {2,0}};
    const Pt shapeI[] = {{0,0}, {1,0}, {2,0}, {3,0}};
    
    switch (type) {
        case Tetromino::T: shape = shapeT; break;
        case Tetromino::J: shape = shapeJ; break;
        case Tetromino::Z: shape = shapeZ; break;
        case Tetromino::O: shape = shapeO; break;
        case Tetromino::S: shape = shapeS; break;
        case Tetromino::L: shape = shapeL; break;
        case Tetromino::I: shape = shapeI; break;
    }

    for (int i=0; i<4; ++i) {
        DrawNESBlock(x + shape[i].x * 8, y + shape[i].y * 8, type);
    }
}


void DrawUIBox(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, BLACK);
    Color cyan = {0, 232, 216, 255};
    DrawRectangleLines(x, y, w, h, cyan);
    DrawRectangleLines(x+2, y+2, w-4, h-4, WHITE);
}

void DrawBackground() {
    ClearBackground(BLACK);
    Color darkGray = {80, 80, 80, 255};
    Color lightGray = {120, 120, 120, 255};
    for(int y = 0; y < 240; y += 8) {
        for(int x = 0; x < 256; x += 8) {
            if ((x/8 + y/8) % 2 == 0) {
                DrawRectangle(x+1, y+1, 6, 6, darkGray);
                DrawRectangle(x+2, y+2, 4, 4, lightGray);
            }
        }
    }
}

int main() {
    // 3x scale of 256x240
    InitWindow(768, 720, "NES Tetris");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(256, 240);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    Font font = LoadFontEx("../assets/PressStart2P-Regular.ttf", 8, 0, 250);

    int currentStartLevel = 0;
    Game game(42, currentStartLevel);

    while (!WindowShouldClose()) {
        Input input = Input::NONE;
        
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

        BeginTextureMode(target);
        DrawBackground();

        // 1. Playfield Box
        DrawUIBox(92, 36, 88, 168);
        DrawRectangle(96, 40, 80, 160, BLACK); // Playfield background

        const auto& grid = game.getBoard().getGrid();
        for (int y = 2; y < Board::HEIGHT; ++y) {
            for (int x = 0; x < Board::WIDTH; ++x) {
                int cell = grid[y][x];
                if (cell > 0) {
                    DrawNESBlock(96 + x * 8, 40 + (y - 2) * 8, static_cast<Tetromino>(cell - 1));
                }
            }
        }

        if (const Piece* p = game.getPiece()) {
            auto blocks = p->getBlocks();
            for (auto pt : blocks) {
                if (pt.y >= 2) {
                    DrawNESBlock(96 + pt.x * 8, 40 + (pt.y - 2) * 8, p->getType());
                }
            }
        }

        // 2. A-TYPE Box
        DrawUIBox(8, 16, 72, 24);
        DrawTextEx(font, "A-TYPE", {12, 24}, 8, 0, WHITE);

        // 3. LINES Box
        DrawUIBox(92, 8, 88, 24);
        char linesText[32];
        snprintf(linesText, sizeof(linesText), "LINES-%03d", game.getLines());
        DrawTextEx(font, linesText, {100, 16}, 8, 0, WHITE);

        // 4. STATISTICS Box
        DrawUIBox(4, 56, 88, 180);
        DrawTextEx(font, "STATISTICS", {8, 64}, 8, 0, WHITE);
        
        auto stats = game.getPieceStats();
        Tetromino types[7] = { Tetromino::T, Tetromino::J, Tetromino::Z, Tetromino::O, Tetromino::S, Tetromino::L, Tetromino::I };
        for (int i=0; i<7; ++i) {
            int px = 12;
            if (types[i] == Tetromino::O) px = 16;
            else if (types[i] == Tetromino::I) px = 8;
            
            int py = 80 + i * 24;
            int py_piece = py;
            if (types[i] == Tetromino::I) py_piece += 4; // Center the I piece vertically
            
            DrawUIPiece(px, py_piece, types[i]);
            
            // Get the piece color for the text
            Color pieceColor = WHITE;
            switch (types[i]) {
                case Tetromino::T: pieceColor = {160, 32, 240, 255}; break;
                case Tetromino::J: pieceColor = {0, 112, 236, 255}; break;
                case Tetromino::Z: pieceColor = {216, 40, 0, 255}; break;
                case Tetromino::O: pieceColor = {255, 200, 0, 255}; break;
                case Tetromino::S: pieceColor = {0, 168, 0, 255}; break;
                case Tetromino::L: pieceColor = {255, 120, 0, 255}; break;
                case Tetromino::I: pieceColor = {0, 232, 216, 255}; break;
            }
            
            char countStr[8];
            snprintf(countStr, sizeof(countStr), "%03d", stats[static_cast<int>(types[i])]);
            DrawTextEx(font, countStr, {56, (float)py + 4}, 8, 0, pieceColor);
        }

        // 5. SCORE Box
        DrawUIBox(188, 16, 64, 56);
        DrawTextEx(font, "TOP", {192, 24}, 8, 0, WHITE);
        DrawTextEx(font, "010000", {192, 36}, 8, 0, WHITE);
        DrawTextEx(font, "SCORE", {192, 48}, 8, 0, WHITE);
        char scoreStr[16];
        snprintf(scoreStr, sizeof(scoreStr), "%06d", game.getScore());
        DrawTextEx(font, scoreStr, {192, 60}, 8, 0, WHITE);

        // 6. NEXT Box
        DrawUIBox(188, 104, 48, 48);
        DrawTextEx(font, "NEXT", {196, 112}, 8, 0, WHITE);
        // Next piece is drawn centered
        Tetromino nextType = game.getNextPiece();
        int nx = (nextType == Tetromino::I || nextType == Tetromino::O) ? 196 : 200;
        DrawUIPiece(nx, 124, nextType); 

        // 7. LEVEL Box
        DrawUIBox(192, 168, 48, 32);
        DrawTextEx(font, "LEVEL", {196, 176}, 8, 0, WHITE);
        char levelStr[8];
        snprintf(levelStr, sizeof(levelStr), "%02d", game.getLevel());
        DrawTextEx(font, levelStr, {208, 188}, 8, 0, WHITE);

        // Game Over Overlay
        if (game.getState() == GameState::GAME_OVER) {
            DrawRectangle(96, 110, 80, 20, BLACK);
            DrawTextEx(font, "GAME OVER", {100, 116}, 8, 0, RED);
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        // Draw the 256x240 render texture scaled up to 768x720
        DrawTexturePro(target.texture, 
            { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height }, 
            { 0.0f, 0.0f, 768.0f, 720.0f }, 
            { 0.0f, 0.0f }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadFont(font);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

#include "raylib.h"
#include "screen_difficulty.h"
#include "setting.h"

void InitScreenDifficulty(void) {
    // Initialization if necessary
}

GameScreen UpdateScreenDifficulty(void) {
    Vector2 mousePoint = GetVirtualMouse();
    float centerX = VIRTUAL_WIDTH / 2.0f;
    float centerY = VIRTUAL_HEIGHT / 2.0f;
    
    Rectangle easyBtn   = { centerX - 100, centerY - 60, 200, 50 };
    Rectangle normalBtn = { centerX - 100, centerY + 10, 200, 50 };
    Rectangle hardBtn   = { centerX - 100, centerY + 80, 200, 50 };
    Rectangle backBtn   = { centerX - 100, centerY + 150, 200, 50 };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePoint, easyBtn)) {
            currentDifficulty = DIFF_EASY;
            return DIALOGUE;
        } else if (CheckCollisionPointRec(mousePoint, normalBtn)) {
            currentDifficulty = DIFF_NORMAL;
            return DIALOGUE;
        } else if (CheckCollisionPointRec(mousePoint, hardBtn)) {
            currentDifficulty = DIFF_HARD;
            return DIALOGUE;
        } else if (CheckCollisionPointRec(mousePoint, backBtn)) {
            return MAIN_MENU;
        }
    }
    
    return DIFFICULTY_SELECT;
}

void DrawScreenDifficulty(void) {
    float centerX = VIRTUAL_WIDTH / 2.0f;
    float centerY = VIRTUAL_HEIGHT / 2.0f;
    Vector2 mousePoint = GetVirtualMouse();
    
    // Clear background to a dark red to fit the Hades theme
    ClearBackground((Color){ 139, 0, 0, 255 }); // Dark Red
    
    DrawText("SELECT DIFFICULTY", (int)centerX - MeasureText("SELECT DIFFICULTY", 40) / 2, (int)centerY - 150, 40, RAYWHITE);

    Rectangle easyBtn   = { centerX - 100, centerY - 60, 200, 50 };
    Rectangle normalBtn = { centerX - 100, centerY + 10, 200, 50 };
    Rectangle hardBtn   = { centerX - 100, centerY + 80, 200, 50 };
    Rectangle backBtn   = { centerX - 100, centerY + 150, 200, 50 };

    // Easy Button
    DrawRectangleRec(easyBtn, CheckCollisionPointRec(mousePoint, easyBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(easyBtn, 2, DARKGRAY);
    DrawText("EASY", (int)centerX - MeasureText("EASY", 20) / 2, (int)(easyBtn.y + 15), 20, DARKGRAY);

    // Normal Button
    DrawRectangleRec(normalBtn, CheckCollisionPointRec(mousePoint, normalBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(normalBtn, 2, DARKGRAY);
    DrawText("NORMAL", (int)centerX - MeasureText("NORMAL", 20) / 2, (int)(normalBtn.y + 15), 20, DARKGRAY);

    // Hard Button
    DrawRectangleRec(hardBtn, CheckCollisionPointRec(mousePoint, hardBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(hardBtn, 2, DARKGRAY);
    DrawText("HARD", (int)centerX - MeasureText("HARD", 20) / 2, (int)(hardBtn.y + 15), 20, DARKGRAY);

    // Back Button
    DrawRectangleRec(backBtn, CheckCollisionPointRec(mousePoint, backBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(backBtn, 2, DARKGRAY);
    DrawText("BACK", (int)centerX - MeasureText("BACK", 20) / 2, (int)(backBtn.y + 15), 20, DARKGRAY);
}

void UnloadScreenDifficulty(void) {
    // Nothing to unload
}

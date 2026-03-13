#include "raylib.h"
#include "screen_menu.h"
#include "setting.h"

void InitScreenMenu(void)
{
    // Nothing to init for now
}

GameScreen UpdateScreenMenu(void)
{
    const GameSettings* settings = GetSettings();
    Vector2 mousePoint = GetMousePosition();
    float centerX = settings->screenWidth / 2.0f;
    
    Rectangle startBtn = { centerX - 100, 200, 200, 50 };
    Rectangle settingsBtn = { centerX - 100, 280, 200, 50 };
    Rectangle quitBtn = { centerX - 100, 360, 200, 50 };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePoint, startBtn)) {
            return DIALOGUE;
        } else if (CheckCollisionPointRec(mousePoint, settingsBtn)) {
            return SETTINGS;
        } else if (CheckCollisionPointRec(mousePoint, quitBtn)) {
            return QUIT_GAME;
        }
    }
    
    return MAIN_MENU;
}

void DrawScreenMenu(void)
{
    const GameSettings* settings = GetSettings();
    int centerX = settings->screenWidth / 2;
    Vector2 mousePoint = GetMousePosition();
    
    Rectangle startBtn = { (float)centerX - 100, 200, 200, 50 };
    Rectangle settingsBtn = { (float)centerX - 100, 280, 200, 50 };
    Rectangle quitBtn = { (float)centerX - 100, 360, 200, 50 };

    DrawText("Protocol Hades", centerX - MeasureText("Protocol Hades", 60) / 2, 80, 60, DARKGRAY);

    // Start Button
    DrawRectangleRec(startBtn, CheckCollisionPointRec(mousePoint, startBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(startBtn, 2, DARKGRAY);
    DrawText("START", centerX - MeasureText("START", 20) / 2, 215, 20, DARKGRAY);

    // Settings Button
    DrawRectangleRec(settingsBtn, CheckCollisionPointRec(mousePoint, settingsBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(settingsBtn, 2, DARKGRAY);
    DrawText("SETTINGS", centerX - MeasureText("SETTINGS", 20) / 2, 295, 20, DARKGRAY);

    // Quit Button
    DrawRectangleRec(quitBtn, CheckCollisionPointRec(mousePoint, quitBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(quitBtn, 2, DARKGRAY);
    DrawText("QUIT", centerX - MeasureText("QUIT", 20) / 2, 375, 20, DARKGRAY);
}

void UnloadScreenMenu(void)
{
    // Nothing to unload
}

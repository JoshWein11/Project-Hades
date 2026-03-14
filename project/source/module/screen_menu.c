#include "raylib.h"
#include "screen_menu.h"
#include "setting.h"

static Texture2D menuLogo;

void InitScreenMenu(void)
{
    menuLogo = LoadTexture("../assets/images/menu.png");
}

GameScreen UpdateScreenMenu(void)
{
    const GameSettings* settings = GetSettings();
    Vector2 mousePoint = GetMousePosition();
    float centerX = settings->screenWidth / 2.0f;
    float centerY = settings->screenHeight / 2.0f;
    
    Rectangle startBtn = { centerX - 100, centerY - 10, 200, 50 };
    Rectangle settingsBtn = { centerX - 100, centerY + 60, 200, 50 };
    Rectangle quitBtn = { centerX - 100, centerY + 130, 200, 50 };

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
    float centerY = settings->screenHeight / 2.0f;
    Vector2 mousePoint = GetMousePosition();
    
    Rectangle startBtn = { (float)centerX - 100, centerY - 10, 200, 50 };
    Rectangle settingsBtn = { (float)centerX - 100, centerY + 60, 200, 50 };
    Rectangle quitBtn = { (float)centerX - 100, centerY + 130, 200, 50 };

    // Draw menu logo scaled 1.67x, centered
    float logoScale = 1.67f;
    int scaledWidth = (int)(menuLogo.width * logoScale);
    int logoX = centerX - scaledWidth / 2;
    DrawTextureEx(menuLogo, (Vector2){ (float)logoX, 20 }, 0.0f, logoScale, WHITE);

    // Start Button
    DrawRectangleRec(startBtn, CheckCollisionPointRec(mousePoint, startBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(startBtn, 2, DARKGRAY);
    DrawText("START", centerX - MeasureText("START", 20) / 2, (int)(startBtn.y + 15), 20, DARKGRAY);

    // Settings Button
    DrawRectangleRec(settingsBtn, CheckCollisionPointRec(mousePoint, settingsBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(settingsBtn, 2, DARKGRAY);
    DrawText("SETTINGS", centerX - MeasureText("SETTINGS", 20) / 2, (int)(settingsBtn.y + 15), 20, DARKGRAY);

    // Quit Button
    DrawRectangleRec(quitBtn, CheckCollisionPointRec(mousePoint, quitBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(quitBtn, 2, DARKGRAY);
    DrawText("QUIT", centerX - MeasureText("QUIT", 20) / 2, (int)(quitBtn.y + 15), 20, DARKGRAY);
}

void UnloadScreenMenu(void)
{
    UnloadTexture(menuLogo);
}

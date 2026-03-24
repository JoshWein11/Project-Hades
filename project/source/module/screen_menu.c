#include "raylib.h" //Code written by: Christopher 沈佳豪
#include "screen_menu.h"
#include "setting.h"
#include "savedata.h"

static Texture2D menuLogo;

void InitScreenMenu(void)
{
    menuLogo = LoadTexture("../assets/images/menu.png"); //Load Menu Logo from Assets
}

GameScreen UpdateScreenMenu(void)
{
    Vector2 mousePoint = GetVirtualMouse();
    float centerX = VIRTUAL_WIDTH / 2.0f;
    float centerY = VIRTUAL_HEIGHT / 2.0f;
    
    //Make buttons in the main menu
    Rectangle startBtn    = { centerX - 100, centerY - 10, 200, 50 };
    Rectangle loadBtn     = { centerX - 100, centerY + 60, 200, 50 };
    Rectangle settingsBtn = { centerX - 100, centerY + 130, 200, 50 };
    Rectangle quitBtn     = { centerX - 100, centerY + 200, 200, 50 };

    bool hasSave = SaveFileExists(SAVE_FILEPATH);

    //Check if the mouse is clicked
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePoint, startBtn)) {
            return DIALOGUE;
        } else if (hasSave && CheckCollisionPointRec(mousePoint, loadBtn)) {
            TraceLog(LOG_INFO, "MENU: Load Game clicked");
            return LOAD_GAME;
        } else if (CheckCollisionPointRec(mousePoint, settingsBtn)) {
            return SETTINGS;
        } else if (CheckCollisionPointRec(mousePoint, quitBtn)) {
            return QUIT_GAME;
        }
    }
    
    return MAIN_MENU;
}

void DrawScreenMenu(void) //Draw the Screen
{
    float centerX = VIRTUAL_WIDTH / 2.0f;
    float centerY = VIRTUAL_HEIGHT / 2.0f;
    Vector2 mousePoint = GetVirtualMouse();
    
    Rectangle startBtn    = { centerX - 100, centerY - 10, 200, 50 };
    Rectangle loadBtn     = { centerX - 100, centerY + 60, 200, 50 };
    Rectangle settingsBtn = { centerX - 100, centerY + 130, 200, 50 };
    Rectangle quitBtn     = { centerX - 100, centerY + 200, 200, 50 };

    bool hasSave = SaveFileExists(SAVE_FILEPATH);

    // Draw menu logo scaled 1.67x, centered
    float logoScale = 1.67f;
    int scaledWidth = (int)(menuLogo.width * logoScale);
    int logoX = (int)centerX - scaledWidth / 2;
    DrawTextureEx(menuLogo, (Vector2){ (float)logoX, 20 }, 0.0f, logoScale, WHITE);

    // Start Button
    DrawRectangleRec(startBtn, CheckCollisionPointRec(mousePoint, startBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(startBtn, 2, DARKGRAY);
    DrawText("NEW GAME", (int)centerX - MeasureText("NEW GAME", 20) / 2, (int)(startBtn.y + 15), 20, DARKGRAY);

    // Load Game Button (greyed out if no save exists)
    if (hasSave) {
        DrawRectangleRec(loadBtn, CheckCollisionPointRec(mousePoint, loadBtn) ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(loadBtn, 2, DARKGRAY);
        DrawText("LOAD GAME", (int)centerX - MeasureText("LOAD GAME", 20) / 2, (int)(loadBtn.y + 15), 20, DARKGRAY);
    } else {
        DrawRectangleRec(loadBtn, Fade(LIGHTGRAY, 0.4f));
        DrawRectangleLinesEx(loadBtn, 2, Fade(DARKGRAY, 0.4f));
        DrawText("LOAD GAME", (int)centerX - MeasureText("LOAD GAME", 20) / 2, (int)(loadBtn.y + 15), 20, Fade(GRAY, 0.5f));
    }

    // Settings Button
    DrawRectangleRec(settingsBtn, CheckCollisionPointRec(mousePoint, settingsBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(settingsBtn, 2, DARKGRAY);
    DrawText("SETTINGS", (int)centerX - MeasureText("SETTINGS", 20) / 2, (int)(settingsBtn.y + 15), 20, DARKGRAY);

    // Quit Button
    DrawRectangleRec(quitBtn, CheckCollisionPointRec(mousePoint, quitBtn) ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(quitBtn, 2, DARKGRAY);
    DrawText("QUIT", (int)centerX - MeasureText("QUIT", 20) / 2, (int)(quitBtn.y + 15), 20, DARKGRAY);
}

void UnloadScreenMenu(void)
{
    UnloadTexture(menuLogo);
}

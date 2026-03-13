#include "raylib.h"
#include "setting.h"
#include "screens.h"

// Include all screen modules
#include "screen_splash.h"
#include "screen_menu.h"
#include "screen_dialogue.h"
#include "screen_gameplay.h"

int main(void)
{
    // Initialize our settings module
    InitSettings();
    const GameSettings* settings = GetSettings();

    // Use settings for window initialization
    InitWindow(settings->screenWidth, settings->screenHeight, settings->title);

    // Initialize all screens
    InitScreenSplash();
    InitScreenMenu();
    InitScreenDialogue();
    InitScreenGameplay();

    
    GameScreen currentScreen = SPLASH;
    GameScreen previousScreen = MAIN_MENU;
    bool exitGame = false;
    
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose() && !exitGame && currentScreen != QUIT_GAME)
    {
        // Global Input: Toggle settings menu
        if (IsKeyPressed(KEY_M)) {
            if (currentScreen == GAMEPLAY || currentScreen == MAIN_MENU) {
                previousScreen = currentScreen;
                currentScreen = SETTINGS;
            } else if (currentScreen == SETTINGS) {
                currentScreen = previousScreen;
            }
        }

        // --- UPDATE ---
        switch (currentScreen) {
            case SPLASH:
                currentScreen = UpdateScreenSplash();
                break;
            case MAIN_MENU:
                currentScreen = UpdateScreenMenu();
                // Special case for settings toggle from within the menu module
                if (currentScreen == SETTINGS) previousScreen = MAIN_MENU;
                break;
            case DIALOGUE:
                currentScreen = UpdateScreenDialogue();
                break;
            case GAMEPLAY:
                currentScreen = UpdateScreenGameplay();
                break;
            case SETTINGS:
                // Settings logic
                if (IsKeyPressed(KEY_ONE)) ApplyResolution(800, 600);
                else if (IsKeyPressed(KEY_TWO)) ApplyResolution(1280, 720);
                else if (IsKeyPressed(KEY_THREE)) ApplyResolution(1920, 1080);
                break;
            default: break;
        }

        // --- DRAW ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            switch (currentScreen) {
                case SPLASH:
                    DrawScreenSplash();
                    break;
                case MAIN_MENU:
                    DrawScreenMenu();
                    break;
                case DIALOGUE:
                    DrawScreenDialogue();
                    break;
                case GAMEPLAY:
                    DrawScreenGameplay();
                    break;
                case SETTINGS: {
                    // Dim background
                    DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.8f));
                    
                    int centerX = settings->screenWidth / 2;
                    DrawText("SETTINGS MENU", centerX - MeasureText("SETTINGS MENU", 40) / 2, 100, 40, RAYWHITE);
                    DrawText("1. 800x600", centerX - MeasureText("1. 800x600", 20) / 2, 200, 20, RAYWHITE);
                    DrawText("2. 1280x790", centerX - MeasureText("2. 1280x720", 20) / 2, 250, 20, RAYWHITE);
                    DrawText("3. 1920x1080", centerX - MeasureText("3. 1920x1080", 20) / 2, 300, 20, RAYWHITE);
                    DrawText("Press 'M' to return to game", centerX - MeasureText("Press 'M' to return to game", 20) / 2, 400, 20, LIGHTGRAY);
                } break;
                default: break;
            }

        EndDrawing();
    }

    // --- DE-INITIALIZATION ---
    UnloadScreenGameplay();
    UnloadScreenDialogue();
    UnloadScreenMenu();
    UnloadScreenSplash();

    CloseWindow();

    return 0;
}

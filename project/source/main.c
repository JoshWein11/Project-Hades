#include "raylib.h"
#include "setting.h"
#include "screens.h"

// Include all screen modules
#include "screen_splash.h"
#include "screen_menu.h"
#include "screen_dialogue.h"
#include "screen_gameplay.h"
#include "screen_setting.h"
#include "audio.h"

#include <stdio.h>

int main(void)
{
    // Initialize our settings module
    InitSettings();
    const GameSettings* settings = GetSettings();

    // Use settings for window initialization
    InitWindow(settings->screenWidth, settings->screenHeight, settings->title);
    SetExitKey(0);  // Disable ESC as the close-window key (we use it for settings)

    // Initialize all screens
    InitScreenSplash();
    InitScreenMenu();
    InitScreenDialogue();
    InitScreenGameplay();

    // Initialize Audio
    InitAudioDevice();
    Audio gameAudio;
    InitAudio(&gameAudio);
    
    GameScreen currentScreen = SPLASH;
    GameScreen previousScreen = MAIN_MENU;
    bool exitGame = false;
    bool splashSoundPlayed = false;
    
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose() && !exitGame && currentScreen != QUIT_GAME)
    {
        // Global Input: Toggle settings menu
        if (IsKeyPressed(KEY_ESCAPE)) {
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
                if (!splashSoundPlayed) {
                    PlaySound(gameAudio.sfxSplash);
                    splashSoundPlayed = true;
                }
                currentScreen = UpdateScreenSplash();
                break;
            case MAIN_MENU:
                if (!IsMusicStreamPlaying(gameAudio.bg_menu_music)) {
                    PlayMusicStream(gameAudio.bg_menu_music);
                }
                UpdateMusicStream(gameAudio.bg_menu_music);
                
                currentScreen = UpdateScreenMenu();
                // Special case for settings toggle from within the menu module
                if (currentScreen == SETTINGS) previousScreen = MAIN_MENU;
                break;
            case DIALOGUE:
                currentScreen = UpdateScreenDialogue();
                break;
            case GAMEPLAY:
                currentScreen = UpdateScreenGameplay(&gameAudio);
                break;
            case SETTINGS:
                currentScreen = UpdateScreenSetting(previousScreen);
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
                case SETTINGS:
                    DrawScreenSetting();
                    break;
                default: break;
            }

        EndDrawing();
    }

    // --- DE-INITIALIZATION ---
    UnloadScreenGameplay();
    UnloadScreenDialogue();
    UnloadScreenMenu();
    UnloadScreenSplash();
    UnloadScreenSetting();

    UnloadAudio(&gameAudio);
    CloseAudioDevice();

    CloseWindow();

    return 0;
}

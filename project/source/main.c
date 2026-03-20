#include "raylib.h" //Code written by: Josh and Christopher 沈家豪 
#include "setting.h"
#include "screens.h"

// Include all screen modules
#include "screen_splash.h"
#include "screen_menu.h"
#include "screen_dialogue.h"
#include "screen_gameplay.h"
#include "screen_setting.h"

// Include Audio Module
#include "audio.h"

// Include Save Data Module
#include "savedata.h"

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
    InitScreenDialogue("../assets/dialogue/opening.txt");
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
                // Auto-save when leaving gameplay
                if (currentScreen == GAMEPLAY) {
                    SaveData save = {0};
                    GetGameplaySaveData(&save);
                    save.currentScreen    = (int)GAMEPLAY;
                    save.dialogueFinished = true;
                    SaveGame(&save, SAVE_FILEPATH);
                }
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
                // "Start" was pressed — reset dialogue and gameplay for a fresh run
                if (currentScreen == DIALOGUE) {
                    ResetScreenDialogue();
                    InitScreenGameplay();
                }
                break;
            case DIALOGUE:
                currentScreen = UpdateScreenDialogue();
                break;
            case GAMEPLAY:
                currentScreen = UpdateScreenGameplay(&gameAudio);
                break;
            case SETTINGS: {
                GameScreen settingsResult = UpdateScreenSetting(previousScreen);
                // If settings "Back" goes to main menu and we came from gameplay,
                // auto-save so the Load Game button is available
                if (settingsResult == MAIN_MENU && previousScreen == GAMEPLAY) {
                    SaveData save = {0};
                    GetGameplaySaveData(&save);
                    save.currentScreen    = (int)GAMEPLAY;
                    save.dialogueFinished = true;
                    SaveGame(&save, SAVE_FILEPATH);
                }
                currentScreen = settingsResult;
                break;
            }
            case LOAD_GAME: {
                // Load saved state and jump to the correct screen
                SaveData save;
                if (LoadGame(&save, SAVE_FILEPATH)) {
                    TraceLog(LOG_INFO, "LOAD: Success! Player at (%.0f, %.0f) health=%.0f",
                             save.playerX, save.playerY, save.playerHealth);
                    RestoreGameplayFromSave(&save);
                    currentScreen = (save.dialogueFinished) ? GAMEPLAY : DIALOGUE;
                    TraceLog(LOG_INFO, "LOAD: Jumping to screen %d", currentScreen);
                } else {
                    TraceLog(LOG_WARNING, "LOAD: Failed to read save file!");
                    currentScreen = MAIN_MENU;
                }
                break;
            }
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

    // --- AUTO-SAVE on exit ---
    // Only save if the player has entered gameplay at least once
    if (currentScreen == QUIT_GAME || WindowShouldClose()) {
        SaveData save = {0};
        GetGameplaySaveData(&save);
        save.currentScreen    = (int)GAMEPLAY;
        save.dialogueFinished = true;  // If they reached gameplay, dialogue is done
        SaveGame(&save, SAVE_FILEPATH);
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

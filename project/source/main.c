#include "raylib.h" //Code written by: Josh and Christopher 沈家豪 
#include "setting.h"
#include "screens.h"

// Include all screen modules
#include "screen_splash.h"
#include "screen_menu.h"
#include "screen_dialogue.h"
#include "screen_gameplay.h"
#include "screen_setting.h"
#include "screen_credits.h"
#include "screen_difficulty.h"

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

    InitAudioDevice();
    Audio gameAudio;
    InitAudio(&gameAudio);
    
    // Initialize all screens
    InitScreenSplash();
    InitScreenMenu();
    InitScreenGameplay();
    
    GameScreen currentScreen = SPLASH;
    GameScreen previousScreen = MAIN_MENU;
    bool exitGame = false;
    bool splashSoundPlayed = false;

    // ── Virtual resolution render target ──────────────────────────────────────
    // Everything renders to this texture at a fixed size, then it gets scaled
    // to fit the actual window. This keeps the game looking the same at any res.
    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose() && !exitGame && currentScreen != QUIT_GAME)
    {
        // Global Input: Toggle settings menu
        bool skipUpdateDueToEsc = false;
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (currentScreen == MAIN_MENU) {
                previousScreen = currentScreen;
                currentScreen = SETTINGS;
                skipUpdateDueToEsc = true;
            } else if (currentScreen == SETTINGS) {
                currentScreen = previousScreen;
                skipUpdateDueToEsc = true;
            }
        }

        // --- UPDATE ---
        if (!skipUpdateDueToEsc) {
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
                    StopMusicStream(gameAudio.gameMusic);
                    StopMusicStream(gameAudio.bossMusic);
                    PlayMusicStream(gameAudio.bg_menu_music);
                }
                UpdateMusicStream(gameAudio.bg_menu_music);
                
                currentScreen = UpdateScreenMenu();
                // Special case for settings toggle from within the menu module
                if (currentScreen == SETTINGS) previousScreen = MAIN_MENU;
                break;
            case DIFFICULTY_SELECT:
                currentScreen = UpdateScreenDifficulty();
                if (currentScreen == DIALOGUE) {
                    LoadDialogueFile("../assets/dialogue/opening.txt");
                    ResetScreenDialogue();
                    InitScreenGameplay();
                }
                break;
            case DIALOGUE:
                currentScreen = UpdateScreenDialogue(&gameAudio);
                break;
            case GAMEPLAY: {
                GameScreen next = UpdateScreenGameplay(&gameAudio);
                if (next == SETTINGS) {
                    previousScreen = GAMEPLAY;
                } else if (next == MAIN_MENU) {
                    // Auto-save when manually returning to menu
                    SaveData save = {0};
                    GetGameplaySaveData(&save);
                    save.currentScreen    = (int)GAMEPLAY;
                    save.dialogueFinished = true;
                    SaveGame(&save, SAVE_FILEPATH);
                } else if (next == CREDITS) {
                    InitScreenCredits();
                }
                currentScreen = next;
                break;
            }
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
            case CREDITS:
                currentScreen = UpdateScreenCredits();
                break;
            default: break;
        }
        }

        // --- DRAW ---
        // Draw everything to the virtual-resolution render texture
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            switch (currentScreen) {
                case SPLASH:
                    DrawScreenSplash();
                    break;
                case MAIN_MENU:
                    DrawScreenMenu();
                    break;
                case DIFFICULTY_SELECT:
                    DrawScreenDifficulty();
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
                case CREDITS:
                    DrawScreenCredits();
                    break;
                default: break;
            }

        EndTextureMode();

        // Scale the virtual canvas to fit the actual window (letterboxed)
        BeginDrawing();
            ClearBackground(BLACK);

            float scaleX = (float)settings->screenWidth  / (float)VIRTUAL_WIDTH;
            float scaleY = (float)settings->screenHeight / (float)VIRTUAL_HEIGHT;
            float scale  = (scaleX < scaleY) ? scaleX : scaleY;

            float drawW = VIRTUAL_WIDTH  * scale;
            float drawH = VIRTUAL_HEIGHT * scale;
            float offsetX = (settings->screenWidth  - drawW) * 0.5f;
            float offsetY = (settings->screenHeight - drawH) * 0.5f;

            // RenderTexture Y is flipped, so source height is negative
            Rectangle src  = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
            Rectangle dest = { offsetX, offsetY, drawW, drawH };
            DrawTexturePro(target.texture, src, dest, (Vector2){0,0}, 0.0f, WHITE);
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
    UnloadRenderTexture(target);
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

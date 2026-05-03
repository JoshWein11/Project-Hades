// test_dialogue.c — Preview any dialogue file without playing the full game.
// Build:  gcc test_dialogue.c -o test_dialogue.exe -Wall -Iinclude -Llib -laudio -lchar -lsetting -lweapon -ltiled -lenemy -lsavedata -lpuzzle -lboss -lraylib -lopengl32 -lgdi32 -lwinmm
// Usage:  ./test_dialogue                         (defaults to ending1.txt)
//         ./test_dialogue ending2.txt
//         ./test_dialogue preboss.txt

#include "raylib.h"
#include "setting.h"
#include "screens.h"
#include "screen_dialogue.h"
#include "audio.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    // Default dialogue file — override via command-line argument
    const char* dialogueFilename = "ending1.txt";
    if (argc > 1) dialogueFilename = argv[1];

    // Build the full relative path
    char dialoguePath[256];
    snprintf(dialoguePath, sizeof(dialoguePath), "../assets/dialogue/%s", dialogueFilename);

    printf("=== Dialogue Preview Tool ===\n");
    printf("File: %s\n", dialoguePath);

    // --- INIT ---
    InitSettings();
    const GameSettings* settings = GetSettings();
    InitWindow(settings->screenWidth, settings->screenHeight, "Dialogue Preview");
    SetExitKey(0);

    InitAudioDevice();
    Audio gameAudio;
    InitAudio(&gameAudio);

    // Load and reset the dialogue
    InitScreenDialogue(dialoguePath);
    LoadDialogueFile(dialoguePath);
    ResetScreenDialogue();

    // Virtual resolution render target (same as main game)
    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);

    bool running = true;

    while (!WindowShouldClose() && running)
    {
        // Update dialogue — when it returns something other than DIALOGUE, the scene is done
        GameScreen result = UpdateScreenDialogue(&gameAudio);
        if (result != DIALOGUE) {
            printf("Dialogue finished! (returned screen: %d)\n", result);
            running = false;
        }

        // Draw to virtual render target
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            DrawScreenDialogue();
        EndTextureMode();

        // Scale to window
        BeginDrawing();
            ClearBackground(BLACK);

            float scaleX = (float)settings->screenWidth  / (float)VIRTUAL_WIDTH;
            float scaleY = (float)settings->screenHeight / (float)VIRTUAL_HEIGHT;
            float scale  = (scaleX < scaleY) ? scaleX : scaleY;

            float drawW   = VIRTUAL_WIDTH  * scale;
            float drawH   = VIRTUAL_HEIGHT * scale;
            float offsetX = (settings->screenWidth  - drawW) * 0.5f;
            float offsetY = (settings->screenHeight - drawH) * 0.5f;

            Rectangle src  = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
            Rectangle dest = { offsetX, offsetY, drawW, drawH };
            DrawTexturePro(target.texture, src, dest, (Vector2){0,0}, 0.0f, WHITE);
        EndDrawing();
    }

    // --- CLEANUP ---
    UnloadRenderTexture(target);
    UnloadScreenDialogue();
    UnloadAudio(&gameAudio);
    CloseAudioDevice();
    CloseWindow();

    printf("Preview closed.\n");
    return 0;
}

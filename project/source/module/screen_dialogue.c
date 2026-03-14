#include "raylib.h" //Code written by: Christopher 沈家豪
#include "screen_dialogue.h"
#include "setting.h"
#include <string.h>

static int framesCounter = 0;
static char dialogueCurrent[256] = "\0";
// Dialogue still a prototype, might be deleted or updated in the future
static const char dialogueFull[] = "Welcome to Protocol Hades. Your mission begins now...";
static int dialogueLength = 0;
static int textIndex = 0;
static bool isFinishedTyping = false;

void InitScreenDialogue(void)
{
    framesCounter = 0;
    textIndex = 0;
    isFinishedTyping = false;
    dialogueLength = (int)strlen(dialogueFull);
    memset(dialogueCurrent, 0, sizeof(dialogueCurrent));
}

GameScreen UpdateScreenDialogue(void)
{
    // Typewriter effect logic
    if (!isFinishedTyping) {
        framesCounter++;
        // Reveal a new character every 3 frames
        if (framesCounter % 3 == 0) {
            if (textIndex < dialogueLength) {
                dialogueCurrent[textIndex] = dialogueFull[textIndex];
                dialogueCurrent[textIndex + 1] = '\0';
                textIndex++;
            } else {
                isFinishedTyping = true;
            }
        }
    }
    
    // Press space or click to speed up or advance
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!isFinishedTyping) {
            // Speed up: instantly show full text
            strcpy(dialogueCurrent, dialogueFull);
            isFinishedTyping = true;
        } else {
            // Advance: go to gameplay
            return GAMEPLAY;
        }
    }
    
    return DIALOGUE;
}

void DrawScreenDialogue(void)
{
    const GameSettings* settings = GetSettings();
    
    // Dim background for cinematic feel
    DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.5f));
    
    // Dialogue Box properties
    int boxMargin = 40;
    int boxHeight = 150;
    Rectangle dialogBox = { boxMargin, settings->screenHeight - boxHeight - boxMargin, settings->screenWidth - (boxMargin * 2), boxHeight };
    
    // Name Box properties
    Rectangle nameBox = { boxMargin, dialogBox.y - 40, 200, 40 };
    
    // Draw Name Box (styled like a dark tab)
    DrawRectangleRec(nameBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(nameBox, 3, WHITE);
    DrawText("COMMANDER", nameBox.x + 20, nameBox.y + 10, 20, WHITE);
    
    // Draw Main Dialogue Box
    DrawRectangleRec(dialogBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(dialogBox, 3, WHITE);
    
    // Draw the typing text inside the box
    DrawText(dialogueCurrent, dialogBox.x + 20, dialogBox.y + 20, 24, LIGHTGRAY);
    
    // Blinking prompt when finished
    if (isFinishedTyping) {
        if ((framesCounter / 30) % 2 == 0) { // Blink every half second
            DrawText(">>", dialogBox.x + dialogBox.width - 30, dialogBox.y + dialogBox.height - 30, 20, WHITE);
        }
    }
}

void UnloadScreenDialogue(void)
{
    // Unload textures if any were added
}

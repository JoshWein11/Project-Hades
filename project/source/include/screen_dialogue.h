#ifndef SCREEN_DIALOGUE_H //All Screen Code Written By: Christopher 沈佳豪
#define SCREEN_DIALOGUE_H

#include "screens.h"
#include "raylib.h"
#include "audio.h"
#include <stdbool.h>

#define MAX_DIALOGUES 128

typedef enum {
    EVENT_DIALOGUE,
    EVENT_BG,
    EVENT_BGM,
    EVENT_SFX,
    EVENT_WAIT,
    EVENT_TITLE,
    EVENT_ANIM
} EventType;

typedef struct {
    EventType type;
    char text[256];      // Used for DIALOGUE text, or file paths for BG/BGM/SFX/ANIM
    char speaker[64];    // Used for speaker name
    char image[128];     // Used for portrait image
    bool isRight;        // Used for portrait orientation
    float floatArg;      // Used for WAIT time / ANIM fps
    int   intArg;        // Used for ANIM column count
} SceneEvent;

void InitScreenDialogue(const char* dialogueFile);
void ResetScreenDialogue(void);
GameScreen UpdateScreenDialogue(Audio* audio);
void DrawScreenDialogue(void);
void UnloadScreenDialogue(void);

// Convenience: unloads the current dialogue and loads a new file.
// Call this before transitioning to the DIALOGUE screen.
void LoadDialogueFile(const char* dialogueFile);

#endif // SCREEN_DIALOGUE_H

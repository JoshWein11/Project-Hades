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
    EVENT_WAIT
} EventType;

typedef struct {
    EventType type;
    char text[256];      // Used for DIALOGUE text, or file paths for BG/BGM/SFX
    char speaker[64];    // Used for speaker name
    char image[128];     // Used for portrait image
    bool isRight;        // Used for portrait orientation
    float floatArg;      // Used for WAIT time
} SceneEvent;

void InitScreenDialogue(const char* dialogueFile);
void ResetScreenDialogue(void);
GameScreen UpdateScreenDialogue(Audio* audio);
void DrawScreenDialogue(void);
void UnloadScreenDialogue(void);

#endif // SCREEN_DIALOGUE_H

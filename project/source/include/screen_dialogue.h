#ifndef SCREEN_DIALOGUE_H //All Screen Code Written By: Christopher 沈家豪
#define SCREEN_DIALOGUE_H

#include "screens.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_DIALOGUES 64

typedef struct {
    char speaker[64];
    char text[256];
    char image[128];
    bool isRight;
} DialogueEntry;

void InitScreenDialogue(const char* dialogueFile);
GameScreen UpdateScreenDialogue(void);
void DrawScreenDialogue(void);
void UnloadScreenDialogue(void);

#endif // SCREEN_DIALOGUE_H

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
    EVENT_ANIM,
    EVENT_CAMERA,
    EVENT_CAMERA_TO,
    EVENT_CHOICE,
    EVENT_CINEMATIC
} EventType;

typedef struct {
    EventType type;
    char text[256];      // Used for DIALOGUE text, or file paths for BG/BGM/SFX/ANIM
    char speaker[64];    // Used for speaker name
    char image[128];     // Used for portrait image
    bool isRight;        // Used for portrait orientation
    float floatArg;      // Used for WAIT time / ANIM fps / CAM dx
    float floatArg2;     // Used for CAM dy
    float floatArg3;     // Used for CAM duration
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

// Returns the current camera offset applied by active EVENT_CAMERA dialogues
Vector2 GetDialogueCameraOffset(void);

// Set the player's current world position so CAMTO can compute offsets
void SetDialoguePlayerPos(Vector2 pos);

// Returns the result of the last EVENT_CHOICE interaction:
//   0 = no choice made yet, 1 = first option (Yes), 2 = second option (Wait/No)
int GetDialogueChoiceResult(void);

#endif // SCREEN_DIALOGUE_H

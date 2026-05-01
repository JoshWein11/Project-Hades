#ifndef SETTING_H //Code written by: Christopher 沈佳豪
#define SETTING_H

#include <stdbool.h>
#include "raylib.h"

// Virtual (internal) resolution — the game always renders at this size,
// then scales to fit whatever window/fullscreen resolution is active.
#define VIRTUAL_WIDTH  1280
#define VIRTUAL_HEIGHT 720

typedef enum {
    DIFF_EASY = 0,
    DIFF_NORMAL,
    DIFF_HARD
} GameDifficulty;

extern GameDifficulty currentDifficulty;

typedef struct {
    int screenWidth;
    int screenHeight;
    const char* title;
    float masterVolume;
    bool isFullscreen;
} GameSettings;

// Initializes the settings with default values
void InitSettings(void);

// Retrieves a pointer to the current game settings
const GameSettings* GetSettings(void);

// Applies a new resolution
void ApplyResolution(int width, int height);

// Toggles fullscreen mode
void ToggleFullscreenMode(void);

// Sets the master volume (0.0 to 1.0)
void SetMasterVolumeLevel(float volume);

// Returns mouse position scaled to virtual resolution coordinates.
// Use this instead of GetMousePosition() for UI hit-testing.
Vector2 GetVirtualMouse(void);

#endif

#include "setting.h"  //Code written by: Christopher 沈家豪
#include "raylib.h"

// Global static settings instance
static GameSettings currentSettings = {0};

// Called once at the start of the program to set default values.
// This runs before the window is created, so the window uses these values.
void InitSettings(void)
{
    currentSettings.screenWidth = 1280;      // Default window width
    currentSettings.screenHeight = 720;      // Default window height
    currentSettings.title = "Project Hades Game";  // Window title
    currentSettings.masterVolume = 1.0f;     // Full volume (1.0 = 100%)
}

const GameSettings* GetSettings(void)
{
    return &currentSettings;
}

// Changes the game window resolution.
// Updates our stored settings AND resizes the actual window immediately.
void ApplyResolution(int width, int height)
{
    currentSettings.screenWidth = width;     // Update stored width
    currentSettings.screenHeight = height;   // Update stored height
    SetWindowSize(width, height);            // Raylib function to resize the window
}

// Changes the master volume for all audio in the game.
// Clamps the value between 0.0 (mute) and 1.0 (max volume) to prevent invalid values.
void SetMasterVolumeLevel(float volume)
{
    if (volume < 0.0f) volume = 0.0f;        // Don't allow negative volume
    if (volume > 1.0f) volume = 1.0f;        // Don't allow volume above 100%
    currentSettings.masterVolume = volume;    // Store the new volume level
    SetMasterVolume(volume);                  // Raylib function to apply the volume globally
}

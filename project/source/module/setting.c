#include "setting.h"  //Code written by: Christopher 沈佳豪
#include "raylib.h"

// Global static settings instance
static GameSettings currentSettings = {0};
GameDifficulty currentDifficulty = DIFF_NORMAL;

// Called once at the start of the program to set default values.
// This runs before the window is created, so the window uses these values.
void InitSettings(void)
{
    currentSettings.screenWidth = 1280;      // Default window width
    currentSettings.screenHeight = 720;      // Default window height
    currentSettings.title = "Project Hades Game";  // Window title
    currentSettings.masterVolume = 1.0f;     // Full volume (1.0 = 100%)
    currentSettings.isFullscreen = false;
}

const GameSettings* GetSettings(void)
{
    return &currentSettings;
}

// Changes the game window resolution.
// Updates our stored settings AND resizes the actual window immediately.
void ApplyResolution(int width, int height)
{
    // If currently fullscreen, exit fullscreen first
    if (currentSettings.isFullscreen) {
        ToggleFullscreen();
        currentSettings.isFullscreen = false;
    }
    currentSettings.screenWidth = width;     // Update stored width
    currentSettings.screenHeight = height;   // Update stored height
    SetWindowSize(width, height);            // Raylib function to resize the window
}

// Toggles between windowed and fullscreen mode
void ToggleFullscreenMode(void)
{
    if (!currentSettings.isFullscreen) {
        // Going fullscreen: save current windowed size, then enter fullscreen
        int monitor = GetCurrentMonitor();
        currentSettings.screenWidth = GetMonitorWidth(monitor);
        currentSettings.screenHeight = GetMonitorHeight(monitor);
        SetWindowSize(currentSettings.screenWidth, currentSettings.screenHeight);
        ToggleFullscreen();
        currentSettings.isFullscreen = true;
    } else {
        // Leaving fullscreen: restore to default windowed size
        ToggleFullscreen();
        currentSettings.screenWidth = 1280;
        currentSettings.screenHeight = 720;
        SetWindowSize(1280, 720);
        currentSettings.isFullscreen = false;
    }
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

// Maps window mouse coordinates to virtual resolution coordinates.
// Accounts for letterboxing offset and scale factor.
Vector2 GetVirtualMouse(void)
{
    Vector2 mouse = GetMousePosition();
    float scaleX = (float)currentSettings.screenWidth  / (float)VIRTUAL_WIDTH;
    float scaleY = (float)currentSettings.screenHeight / (float)VIRTUAL_HEIGHT;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    float drawW   = VIRTUAL_WIDTH  * scale;
    float drawH   = VIRTUAL_HEIGHT * scale;
    float offsetX = (currentSettings.screenWidth  - drawW) * 0.5f;
    float offsetY = (currentSettings.screenHeight - drawH) * 0.5f;

    Vector2 vmouse;
    vmouse.x = (mouse.x - offsetX) / scale;
    vmouse.y = (mouse.y - offsetY) / scale;
    return vmouse;
}

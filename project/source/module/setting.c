#include "setting.h"
#include "raylib.h"

// Global static settings instance
static GameSettings currentSettings = {0};

void InitSettings(void)
{
    currentSettings.screenWidth = 1280;
    currentSettings.screenHeight = 720;
    currentSettings.title = "Game With Settings Module";
}

const GameSettings* GetSettings(void)
{
    return &currentSettings;
}

void ApplyResolution(int width, int height)
{
    currentSettings.screenWidth = width;
    currentSettings.screenHeight = height;
    SetWindowSize(width, height);
}

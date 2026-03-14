#include "raylib.h" //Code written by: Christopher 沈家豪
#include "screen_setting.h"
#include "setting.h"
#include <stdio.h>

// --- State ---
static bool isDraggingSlider = false;

void InitScreenSetting(void)
{
    // Nothing to init for now
}

GameScreen UpdateScreenSetting(GameScreen previousScreen)
{
    const GameSettings* settings = GetSettings();
    Vector2 mousePoint = GetMousePosition();
    float centerX = settings->screenWidth / 2.0f;

    // --- Resolution Buttons ---
    Rectangle res1Btn = { centerX - 100, 200, 200, 40 };
    Rectangle res2Btn = { centerX - 100, 260, 200, 40 };
    Rectangle res3Btn = { centerX - 100, 320, 200, 40 };

    // --- Back Button ---
    Rectangle backBtn = { centerX - 100, 490, 200, 40 };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePoint, res1Btn)) {
            ApplyResolution(800, 600);
        } else if (CheckCollisionPointRec(mousePoint, res2Btn)) {
            ApplyResolution(1280, 720);
        } else if (CheckCollisionPointRec(mousePoint, res3Btn)) {
            ApplyResolution(1920, 1080);
        } else if (CheckCollisionPointRec(mousePoint, backBtn)) {
            return MAIN_MENU;
        }
    }

    // --- Volume Slider ---
    float sliderX = centerX - 150;
    float sliderY = 420;
    float sliderWidth = 300;
    float sliderHeight = 10;
    float knobRadius = 12;

    // The clickable area is a bit taller than the track for easier interaction
    Rectangle sliderArea = { sliderX - knobRadius, sliderY - knobRadius, sliderWidth + knobRadius * 2, knobRadius * 2 + sliderHeight };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, sliderArea)) {
        isDraggingSlider = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        isDraggingSlider = false;
    }

    if (isDraggingSlider) {
        float newVolume = (mousePoint.x - sliderX) / sliderWidth;
        SetMasterVolumeLevel(newVolume); // Clamping handled inside
    }

    return SETTINGS;
}

void DrawScreenSetting(void)
{
    const GameSettings* settings = GetSettings();
    int centerX = settings->screenWidth / 2;
    Vector2 mousePoint = GetMousePosition();

    // Dim background overlay
    DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.8f));

    // Title
    DrawText("SETTINGS", centerX - MeasureText("SETTINGS", 40) / 2, 100, 40, RAYWHITE);

    // --- Resolution Label ---
    DrawText("Resolution", centerX - MeasureText("Resolution", 24) / 2, 165, 24, LIGHTGRAY);

    // --- Resolution Buttons ---
    Rectangle res1Btn = { (float)centerX - 100, 200, 200, 40 };
    Rectangle res2Btn = { (float)centerX - 100, 260, 200, 40 };
    Rectangle res3Btn = { (float)centerX - 100, 320, 200, 40 };

    // Button 1: 800x600
    bool hover1 = CheckCollisionPointRec(mousePoint, res1Btn);
    DrawRectangleRec(res1Btn, hover1 ? LIGHTGRAY : Fade(RAYWHITE, 0.15f));
    DrawRectangleLinesEx(res1Btn, 2, RAYWHITE);
    DrawText("800 x 600", centerX - MeasureText("800 x 600", 20) / 2, 210, 20, RAYWHITE);

    // Button 2: 1280x720
    bool hover2 = CheckCollisionPointRec(mousePoint, res2Btn);
    DrawRectangleRec(res2Btn, hover2 ? LIGHTGRAY : Fade(RAYWHITE, 0.15f));
    DrawRectangleLinesEx(res2Btn, 2, RAYWHITE);
    DrawText("1280 x 720", centerX - MeasureText("1280 x 720", 20) / 2, 270, 20, RAYWHITE);

    // Button 3: 1920x1080
    bool hover3 = CheckCollisionPointRec(mousePoint, res3Btn);
    DrawRectangleRec(res3Btn, hover3 ? LIGHTGRAY : Fade(RAYWHITE, 0.15f));
    DrawRectangleLinesEx(res3Btn, 2, RAYWHITE);
    DrawText("1920 x 1080", centerX - MeasureText("1920 x 1080", 20) / 2, 330, 20, RAYWHITE);

    // --- Volume Slider ---
    float sliderX = (float)centerX - 150;
    float sliderY = 420;
    float sliderWidth = 300;
    float sliderHeight = 10;
    float knobRadius = 12;

    DrawText("Volume", centerX - MeasureText("Volume", 24) / 2, 385, 24, LIGHTGRAY);

    // Track background
    DrawRectangle((int)sliderX, (int)(sliderY), (int)sliderWidth, (int)sliderHeight, Fade(RAYWHITE, 0.3f));

    // Filled portion
    float filledWidth = settings->masterVolume * sliderWidth;
    DrawRectangle((int)sliderX, (int)(sliderY), (int)filledWidth, (int)sliderHeight, RAYWHITE);

    // Knob
    float knobX = sliderX + filledWidth;
    float knobY = sliderY + sliderHeight / 2.0f;
    DrawCircle((int)knobX, (int)knobY, knobRadius, RAYWHITE);

    // Volume percentage
    char volText[16];
    int volPercent = (int)(settings->masterVolume * 100);
    sprintf(volText, "%d%%", volPercent);
    DrawText(volText, (int)(sliderX + sliderWidth + 20), (int)(sliderY - 5), 20, RAYWHITE);

    // --- Back Button ---
    Rectangle backBtn = { (float)centerX - 100, 490, 200, 40 };
    bool hoverBack = CheckCollisionPointRec(mousePoint, backBtn);
    DrawRectangleRec(backBtn, hoverBack ? LIGHTGRAY : Fade(RAYWHITE, 0.15f));
    DrawRectangleLinesEx(backBtn, 2, RAYWHITE);
    DrawText("Main Menu", centerX - MeasureText("Main Menu", 20) / 2, 500, 20, RAYWHITE);

    // --- Hint ---
    DrawText("Press ESC to return", centerX - MeasureText("Press ESC to return", 20) / 2, 550, 20, LIGHTGRAY);
}

void UnloadScreenSetting(void)
{
    // Nothing to unload
}

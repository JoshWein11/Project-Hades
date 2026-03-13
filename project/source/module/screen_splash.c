#include "raylib.h"
#include "screen_splash.h"
#include "setting.h"

static int splashFrames = 0;
static Texture2D splashLogo;

void InitScreenSplash(void)
{
    splashFrames = 0;
    splashLogo = LoadTexture("../assets/images/logo.jpg");
}

GameScreen UpdateScreenSplash(void)
{
    splashFrames++;
    if (splashFrames > 300) { // 5 seconds at 60 FPS
        return MAIN_MENU;
    }
    return SPLASH;
}

void DrawScreenSplash(void)
{
    const GameSettings* settings = GetSettings();
    float alpha = 1.0f;
    
    // Calculate alpha for fade in / fade out
    if (splashFrames < 60) alpha = splashFrames / 60.0f;
    else if (splashFrames > 240) alpha = 1.0f - ((splashFrames - 240) / 60.0f);
    
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    
    if (splashLogo.id != 0) {
        DrawTextureEx(splashLogo, 
            (Vector2){ settings->screenWidth / 2.0f - splashLogo.width / 2.0f, 
                       settings->screenHeight / 2.0f - splashLogo.height / 2.0f },
            0.0f, 1.0f, Fade(WHITE, alpha));
    } else {
        int textWidth = MeasureText("YOUR LOGO HERE", 60);
        DrawText("YOUR LOGO HERE", settings->screenWidth / 2 - textWidth / 2, settings->screenHeight / 2 - 30, 60, Fade(BLACK, alpha));
    }
}

void UnloadScreenSplash(void)
{
    UnloadTexture(splashLogo);
}

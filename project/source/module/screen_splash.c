#include "raylib.h" //Code written by: Christopher 沈家豪
#include "screen_splash.h"
#include "setting.h"

static int splashFrames = 0;
static Texture2D splashLogo;

void InitScreenSplash(void)
{
    splashFrames = 0;
    //Load Splash Logo from Assets
    splashLogo = LoadTexture("../assets/images/logo.png");
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

    float scale = 1.5f;
    
    if (splashLogo.id != 0) {
        DrawTextureEx(splashLogo, 
            (Vector2){ settings->screenWidth / 2.0f - (splashLogo.width * scale) / 2.0f, 
                       settings->screenHeight / 2.0f - (splashLogo.height * scale) / 2.0f },
            0.0f, scale, Fade(WHITE, alpha));
    } else {
        //Draw Text if no Logo incase the logo is not loaded
        int textWidth = MeasureText("YOUR LOGO HERE", 60);
        DrawText("YOUR LOGO HERE", settings->screenWidth / 2 - textWidth / 2, settings->screenHeight / 2 - 30, 60, Fade(BLACK, alpha));
    }
}

void UnloadScreenSplash(void)
{
    UnloadTexture(splashLogo);
}

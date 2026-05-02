#include "raylib.h"
#include "screen_credits.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_CREDIT_LINES 100
#define MAX_LINE_LENGTH 128

static char creditsText[MAX_CREDIT_LINES][MAX_LINE_LENGTH];
static int totalLines = 0;
static float scrollY = 0.0f;
static float scrollSpeed = 50.0f; // Pixels per second
static float holdTimer = 0.0f;
static bool skipPrompted = false;
static float skipPromptTimer = 0.0f;
static Music creditMusic = {0};
static bool hasCreditMusic = false;

// Extern declarations for virtual resolution if available, otherwise fallback
extern const int VIRTUAL_WIDTH;
extern const int VIRTUAL_HEIGHT;

void InitScreenCredits(void) {
    totalLines = 0;
    
    // We can just use GetScreenHeight() if virtual height isn't defined, 
    // but typically it's better to use the active render height.
    scrollY = (float)GetScreenHeight(); 
    holdTimer = 5.0f; // Time to hold on screen after finishing
    skipPrompted = false;
    skipPromptTimer = 0.0f;

    // Load and play credit music
    creditMusic = LoadMusicStream("../assets/music/credit.mp3");
    hasCreditMusic = (creditMusic.stream.buffer != NULL);
    if (hasCreditMusic) PlayMusicStream(creditMusic);

    FILE *file = fopen("../assets/dialogue/credits.txt", "r");
    if (file) {
        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), file) && totalLines < MAX_CREDIT_LINES) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            if (len > 1 && line[len - 2] == '\r') {
                line[len - 2] = '\0';
            }
            strncpy(creditsText[totalLines], line, MAX_LINE_LENGTH - 1);
            creditsText[totalLines][MAX_LINE_LENGTH - 1] = '\0';
            totalLines++;
        }
        fclose(file);
    } else {
        strcpy(creditsText[0], "PROJECT HADES");
        strcpy(creditsText[1], "");
        strcpy(creditsText[2], "THANK YOU FOR PLAYING");
        totalLines = 3;
    }
}

GameScreen UpdateScreenCredits(void) {
    float dt = GetFrameTime();

    if (hasCreditMusic) UpdateMusicStream(creditMusic);
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (skipPrompted && skipPromptTimer > 0.0f) {
            // Add 0.5s immunity to prevent accidental double-clicks
            if (skipPromptTimer <= 2.5f) {
                if (hasCreditMusic) StopMusicStream(creditMusic);
                return MAIN_MENU;
            }
        } else {
            skipPrompted = true;
            skipPromptTimer = 3.0f;
        }
    }

    if (skipPromptTimer > 0.0f) {
        skipPromptTimer -= dt;
    }

    // Calculate total height of the text block roughly
    float textHeight = totalLines * 45.0f;

    // Scroll up until the last line is in the middle of the screen
    if (scrollY > -(textHeight - GetScreenHeight() / 2.0f)) {
        scrollY -= scrollSpeed * dt;
    } else {
        holdTimer -= dt;
        if (holdTimer <= 0.0f) {
            if (hasCreditMusic) StopMusicStream(creditMusic);
            return MAIN_MENU;
        }
    }

    return CREDITS;
}

void DrawScreenCredits(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    DrawRectangle(0, 0, screenW, screenH, BLACK);

    float currentY = scrollY;
    int centerX = screenW / 2;

    for (int i = 0; i < totalLines; i++) {
        if (strlen(creditsText[i]) == 0) {
            currentY += 40.0f;
            continue;
        }

        int fontSize = 28;
        Color color = LIGHTGRAY;

        // Check if header (heuristic: no lowercase letters)
        bool isHeader = true;
        for (int j = 0; creditsText[i][j] != '\0'; j++) {
            if (creditsText[i][j] >= 'a' && creditsText[i][j] <= 'z') {
                isHeader = false;
                break;
            }
        }

        if (isHeader) {
            fontSize = 40;
            color = WHITE;
        }

        int textWidth = MeasureText(creditsText[i], fontSize);
        
        // Only draw if on screen
        if (currentY > -100 && currentY < screenH + 100) {
            DrawText(creditsText[i], centerX - textWidth / 2, (int)currentY, fontSize, color);
        }

        currentY += fontSize + 15.0f;
    }
    
    // Draw skip text
    if (skipPrompted && skipPromptTimer > 0.0f) {
        const char* skipText = "Press again to skip";
        int skipW = MeasureText(skipText, 20);
        DrawText(skipText, screenW - skipW - 20, screenH - 40, 20, Fade(DARKGRAY, skipPromptTimer / 3.0f + 0.5f));
    }
}

void UnloadScreenCredits(void) {
    if (hasCreditMusic) {
        StopMusicStream(creditMusic);
        UnloadMusicStream(creditMusic);
        hasCreditMusic = false;
    }
}

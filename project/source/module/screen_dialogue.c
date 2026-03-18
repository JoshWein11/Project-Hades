#include "raylib.h" //Code written by: Christopher 沈家豪
#include "screen_dialogue.h"
#include "setting.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Dialogue entries parsed from file
static DialogueEntry dialogues[MAX_DIALOGUES];
static int dialogueCount = 0;
static int currentDialogue = 0;

// Portrait textures (one per dialogue entry)
static Texture2D portraits[MAX_DIALOGUES];

// Optional background image
static Texture2D bgTexture;
static bool hasBg = false;

// Typewriter effect state
static int framesCounter = 0;
static char dialogueCurrent[256] = "\0";
static int dialogueLength = 0;
static int textIndex = 0;
static bool isFinishedTyping = false;

// Helper: trim leading/trailing whitespace and carriage returns in-place
static void TrimString(char* str)
{
    // Trim leading
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n') start++;
    if (start > 0) {
        int i = 0;
        while (str[start + i] != '\0') {
            str[i] = str[start + i];
            i++;
        }
        str[i] = '\0';
    }
    // Trim trailing
    int len = (int)strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Helper: reset the typewriter effect for the current dialogue entry
static void ResetTypewriter(void)
{
    framesCounter = 0;
    textIndex = 0;
    isFinishedTyping = false;
    dialogueLength = (int)strlen(dialogues[currentDialogue].text);
    memset(dialogueCurrent, 0, sizeof(dialogueCurrent));
}

// Helper: parse a single dialogue block (speaker line + text line)
// Returns true if successfully parsed
static bool ParseDialogueBlock(const char* block, DialogueEntry* entry)
{
    // Find the first newline to split speaker line and text line
    const char* newline = strchr(block, '\n');
    if (newline == NULL) return false;

    // Extract speaker line
    char speakerLine[256];
    int speakerLineLen = (int)(newline - block);
    if (speakerLineLen >= 256) speakerLineLen = 255;
    strncpy(speakerLine, block, speakerLineLen);
    speakerLine[speakerLineLen] = '\0';
    TrimString(speakerLine);

    // Extract text line (everything after the newline)
    char textLine[256];
    strncpy(textLine, newline + 1, 255);
    textLine[255] = '\0';
    TrimString(textLine);

    if (strlen(speakerLine) == 0 || strlen(textLine) == 0) return false;

    // Parse speaker line: SPEAKER|image.png|left_or_right
    char* pipe1 = strchr(speakerLine, '|');
    if (pipe1 == NULL) return false;
    char* pipe2 = strchr(pipe1 + 1, '|');
    if (pipe2 == NULL) return false;

    // Speaker name
    int nameLen = (int)(pipe1 - speakerLine);
    if (nameLen >= 64) nameLen = 63;
    strncpy(entry->speaker, speakerLine, nameLen);
    entry->speaker[nameLen] = '\0';
    TrimString(entry->speaker);

    // Image filename
    int imgLen = (int)(pipe2 - pipe1 - 1);
    if (imgLen >= 128) imgLen = 127;
    strncpy(entry->image, pipe1 + 1, imgLen);
    entry->image[imgLen] = '\0';
    TrimString(entry->image);

    // Position (left or right)
    char position[16];
    strncpy(position, pipe2 + 1, 15);
    position[15] = '\0';
    TrimString(position);
    entry->isRight = (strcmp(position, "right") == 0);

    // Dialogue text
    strncpy(entry->text, textLine, 255);
    entry->text[255] = '\0';

    return true;
}

void InitScreenDialogue(const char* dialogueFile)
{
    // Reset state
    dialogueCount = 0;
    currentDialogue = 0;
    hasBg = false;

    // Load the text file
    char* fileText = LoadFileText(dialogueFile);
    if (fileText == NULL) {
        TraceLog(LOG_WARNING, "DIALOGUE: Failed to load file: %s", dialogueFile);
        return;
    }

    // Split by "---" separator
    // First block = background, remaining blocks = dialogue entries
    char* text = fileText;
    char* separator = NULL;
    int blockIndex = 0;

    while ((separator = strstr(text, "---")) != NULL) {
        // Extract the block between current position and the separator
        int blockLen = (int)(separator - text);
        char block[512];
        if (blockLen >= 512) blockLen = 511;
        strncpy(block, text, blockLen);
        block[blockLen] = '\0';
        TrimString(block);

        if (blockIndex == 0) {
            // First block: background image or "none"
            if (strcmp(block, "none") != 0 && strlen(block) > 0) {
                char bgPath[256];
                snprintf(bgPath, sizeof(bgPath), "../assets/images/%s", block);
                bgTexture = LoadTexture(bgPath);
                hasBg = (bgTexture.id > 0);
            }
        } else {
            // Dialogue entry block
            if (dialogueCount < MAX_DIALOGUES && strlen(block) > 0) {
                if (ParseDialogueBlock(block, &dialogues[dialogueCount])) {
                    // Load portrait texture
                    char portraitPath[256];
                    snprintf(portraitPath, sizeof(portraitPath), "../assets/images/%s", dialogues[dialogueCount].image);
                    portraits[dialogueCount] = LoadTexture(portraitPath);
                    dialogueCount++;
                }
            }
        }

        blockIndex++;
        // Move past the "---" separator
        text = separator + 3;
    }

    UnloadFileText(fileText);

    // Initialize typewriter for the first entry
    if (dialogueCount > 0) {
        ResetTypewriter();
    }

    TraceLog(LOG_INFO, "DIALOGUE: Loaded %d entries (hasBg=%d)", dialogueCount, hasBg);
}

GameScreen UpdateScreenDialogue(void)
{
    if (dialogueCount == 0) return GAMEPLAY;

    // Typewriter effect logic
    if (!isFinishedTyping) {
        framesCounter++;
        // Reveal a new character every 3 frames
        if (framesCounter % 3 == 0) {
            if (textIndex < dialogueLength) {
                dialogueCurrent[textIndex] = dialogues[currentDialogue].text[textIndex];
                dialogueCurrent[textIndex + 1] = '\0';
                textIndex++;
            } else {
                isFinishedTyping = true;
            }
        }
    }
    
    // Press space or click to speed up or advance
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!isFinishedTyping) {
            // Speed up: instantly show full text
            strcpy(dialogueCurrent, dialogues[currentDialogue].text);
            isFinishedTyping = true;
        } else {
            // Advance to next dialogue entry
            currentDialogue++;
            if (currentDialogue >= dialogueCount) {
                // All dialogue finished, go to gameplay
                return GAMEPLAY;
            }
            ResetTypewriter();
        }
    }
    
    return DIALOGUE;
}

void DrawScreenDialogue(void)
{
    const GameSettings* settings = GetSettings();
    
    if (hasBg) {
        // Draw background image scaled to fill screen
        float scaleX = (float)settings->screenWidth / (float)bgTexture.width;
        float scaleY = (float)settings->screenHeight / (float)bgTexture.height;
        float scale = (scaleX > scaleY) ? scaleX : scaleY;
        DrawTextureEx(bgTexture, (Vector2){0, 0}, 0.0f, scale, WHITE);
        // Slight dim on top for readability
        DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.3f));
    } else {
        // No bg image: just dim overlay over whatever is behind (gameplay)
        DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.5f));
    }

    if (dialogueCount == 0) return;

    // Draw character portrait
    Texture2D portrait = portraits[currentDialogue];
    if (portrait.id > 0) {
        // Scale portrait to a reasonable size (e.g., 200px wide, maintain aspect ratio)
        float portraitScale = 200.0f / (float)portrait.width;
        float portraitW = portrait.width * portraitScale;
        float portraitH = portrait.height * portraitScale;
        float portraitY = settings->screenHeight - portraitH - 200;

        if (dialogues[currentDialogue].isRight) {
            // Draw on the right side
            float portraitX = settings->screenWidth - portraitW - 40;
            DrawTextureEx(portrait, (Vector2){portraitX, portraitY}, 0.0f, portraitScale, WHITE);
        } else {
            // Draw on the left side
            float portraitX = 40;
            DrawTextureEx(portrait, (Vector2){portraitX, portraitY}, 0.0f, portraitScale, WHITE);
        }
    }
    
    // Dialogue Box properties
    int boxMargin = 40;
    int boxHeight = 150;
    Rectangle dialogBox = { boxMargin, settings->screenHeight - boxHeight - boxMargin, settings->screenWidth - (boxMargin * 2), boxHeight };
    
    // Name Box properties
    Rectangle nameBox = { boxMargin, dialogBox.y - 40, 200, 40 };
    
    // Draw Name Box (styled like a dark tab)
    DrawRectangleRec(nameBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(nameBox, 3, WHITE);
    DrawText(dialogues[currentDialogue].speaker, nameBox.x + 20, nameBox.y + 10, 20, WHITE);
    
    // Draw Main Dialogue Box
    DrawRectangleRec(dialogBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(dialogBox, 3, WHITE);
    
    // Draw the typing text inside the box
    DrawText(dialogueCurrent, dialogBox.x + 20, dialogBox.y + 20, 24, LIGHTGRAY);
    
    // Blinking prompt when finished
    if (isFinishedTyping) {
        framesCounter++;
        if ((framesCounter / 30) % 2 == 0) { // Blink every half second
            DrawText(">>", dialogBox.x + dialogBox.width - 30, dialogBox.y + dialogBox.height - 30, 20, WHITE);
        }
    }
}

void UnloadScreenDialogue(void)
{
    // Unload portrait textures
    for (int i = 0; i < dialogueCount; i++) {
        if (portraits[i].id > 0) {
            UnloadTexture(portraits[i]);
        }
    }
    // Unload background texture
    if (hasBg && bgTexture.id > 0) {
        UnloadTexture(bgTexture);
    }
    dialogueCount = 0;
    currentDialogue = 0;
    hasBg = false;
}

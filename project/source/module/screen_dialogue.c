#include "raylib.h" //Code written by: Christopher 沈佳豪 
#include "screen_dialogue.h"
#include "setting.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Event entries parsed from file
static SceneEvent events[MAX_DIALOGUES];
static int eventCount = 0;
static int currentEvent = 0;

// Portrait textures (maps 1:1 with dialogue events)
static Texture2D portraits[MAX_DIALOGUES];

// Active background and music state
static Texture2D activeBg;
static bool hasActiveBg = false;
static Music activeBgm;
static bool hasActiveBgm = false;

// Asset caches for dynamic loading
#define MAX_CACHE 32
static Texture2D cachedTextures[MAX_CACHE];
static char cachedTextureNames[MAX_CACHE][128];
static int texCacheCount = 0;

static Music cachedMusic[MAX_CACHE];
static char cachedMusicNames[MAX_CACHE][128];
static int musicCacheCount = 0;

static Sound cachedSounds[MAX_CACHE];
static char cachedSoundNames[MAX_CACHE][128];
static int soundCacheCount = 0;

// State for WAIT event
static int waitTimerFrames = 0;
static int waitTargetFrames = 0;

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

static Texture2D GetCachedTexture(const char* filename) {
    for (int i = 0; i < texCacheCount; i++) {
        if (strcmp(cachedTextureNames[i], filename) == 0) return cachedTextures[i];
    }
    if (texCacheCount < MAX_CACHE) {
        Texture2D t = LoadTexture(filename);
        strcpy(cachedTextureNames[texCacheCount], filename);
        cachedTextures[texCacheCount] = t;
        texCacheCount++;
        return t;
    }
    Texture2D empty = {0};
    return empty;
}

static Music GetCachedMusic(const char* filename) {
    for (int i = 0; i < musicCacheCount; i++) {
        if (strcmp(cachedMusicNames[i], filename) == 0) return cachedMusic[i];
    }
    if (musicCacheCount < MAX_CACHE) {
        Music m = LoadMusicStream(filename);
        strcpy(cachedMusicNames[musicCacheCount], filename);
        cachedMusic[musicCacheCount] = m;
        musicCacheCount++;
        return m;
    }
    Music empty = {0};
    return empty;
}

static Sound GetCachedSound(const char* filename) {
    for (int i = 0; i < soundCacheCount; i++) {
        if (strcmp(cachedSoundNames[i], filename) == 0) return cachedSounds[i];
    }
    if (soundCacheCount < MAX_CACHE) {
        Sound s = LoadSound(filename);
        strcpy(cachedSoundNames[soundCacheCount], filename);
        cachedSounds[soundCacheCount] = s;
        soundCacheCount++;
        return s;
    }
    Sound empty = {0};
    return empty;
}

// Helper: reset the typewriter effect for the current dialogue entry
static void ResetTypewriter(void)
{
    framesCounter = 0;
    textIndex = 0;
    isFinishedTyping = false;
    dialogueLength = (int)strlen(events[currentEvent].text);
    memset(dialogueCurrent, 0, sizeof(dialogueCurrent));
}

// Helper: parse a single dialogue block (speaker line + text line)
static bool ParseDialogueEvent(const char* block, SceneEvent* entry)
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

    entry->type = EVENT_DIALOGUE;
    strncpy(entry->text, textLine, 255);
    entry->text[255] = '\0';

    return true;
}

void InitScreenDialogue(const char* dialogueFile)
{
    // Reset state and caches
    eventCount = 0;
    currentEvent = 0;
    texCacheCount = 0;
    musicCacheCount = 0;
    soundCacheCount = 0;
    hasActiveBg = false;
    hasActiveBgm = false;
    memset(&activeBg, 0, sizeof(Texture2D));
    memset(&activeBgm, 0, sizeof(Music));

    char* fileText = LoadFileText(dialogueFile);
    if (fileText == NULL) {
        TraceLog(LOG_WARNING, "DIALOGUE: Failed to load file: %s", dialogueFile);
        return;
    }

    char* text = fileText;
    char* separator = NULL;
    int blockIndex = 0;

    while ((separator = strstr(text, "---")) != NULL) {
        int blockLen = (int)(separator - text);
        char block[1024];
        if (blockLen >= 1024) blockLen = 1023;
        strncpy(block, text, blockLen);
        block[blockLen] = '\0';
        TrimString(block);

        if (strlen(block) > 0) {
            if (strstr(block, "CMD|") != NULL) {
                // Command block
                char* context;
                char* line = strtok_s(block, "\r\n", &context);
                while (line != NULL && eventCount < MAX_DIALOGUES) {
                    TrimString(line);
                    if (strncmp(line, "CMD|", 4) == 0) {
                        char cmdType[16];
                        char cmdArg[128];
                        char* p1 = strchr(line, '|');
                        if (p1) {
                            char* p2 = strchr(p1 + 1, '|');
                            if (p2) {
                                int typeLen = (int)(p2 - p1 - 1);
                                strncpy(cmdType, p1 + 1, typeLen); 
                                cmdType[typeLen] = '\0';
                                strcpy(cmdArg, p2 + 1);
                                TrimString(cmdType); TrimString(cmdArg);

                                SceneEvent* ev = &events[eventCount];
                                memset(ev, 0, sizeof(SceneEvent));

                                if (strcmp(cmdType, "BG") == 0) {
                                    ev->type = EVENT_BG;
                                    snprintf(ev->text, sizeof(ev->text), "../assets/images/background/%s", cmdArg);
                                    GetCachedTexture(ev->text); // Preload
                                    eventCount++;
                                } else if (strcmp(cmdType, "BGM") == 0) {
                                    ev->type = EVENT_BGM;
                                    snprintf(ev->text, sizeof(ev->text), "../assets/music/%s", cmdArg);
                                    GetCachedMusic(ev->text); // Preload
                                    eventCount++;
                                } else if (strcmp(cmdType, "SFX") == 0) {
                                    ev->type = EVENT_SFX;
                                    snprintf(ev->text, sizeof(ev->text), "../assets/sfx/%s", cmdArg);
                                    GetCachedSound(ev->text); // Preload
                                    eventCount++;
                                } else if (strcmp(cmdType, "WAIT") == 0) {
                                    ev->type = EVENT_WAIT;
                                    ev->floatArg = (float)atof(cmdArg);
                                    eventCount++;
                                }
                            }
                        }
                    }
                    line = strtok_s(NULL, "\r\n", &context);
                }
            } else {
                // Legacy BG or normal dialogue
                if (blockIndex == 0 && strchr(block, '|') == NULL) {
                    if (strcmp(block, "none") != 0 && eventCount < MAX_DIALOGUES) {
                        SceneEvent* ev = &events[eventCount];
                        memset(ev, 0, sizeof(SceneEvent));
                        ev->type = EVENT_BG;
                        snprintf(ev->text, sizeof(ev->text), "../assets/images/background/%.220s", block);
                        GetCachedTexture(ev->text);
                        eventCount++;
                    }
                } else {
                    if (eventCount < MAX_DIALOGUES) {
                        SceneEvent* ev = &events[eventCount];
                        if (ParseDialogueEvent(block, ev)) {
                            char portraitPath[256];
                            snprintf(portraitPath, sizeof(portraitPath), "../assets/images/character/%s", ev->image);
                            portraits[eventCount] = LoadTexture(portraitPath);
                            eventCount++;
                        }
                    }
                }
            }
        }
        blockIndex++;
        text = separator + 3;
    }

    UnloadFileText(fileText);

    if (eventCount > 0 && events[0].type == EVENT_DIALOGUE) {
        ResetTypewriter();
    }

    TraceLog(LOG_INFO, "DIALOGUE: Loaded %d events", eventCount);
}

void ResetScreenDialogue(void)
{
    // Start from beginning with first BG & BGM if applicable
    currentEvent = 0;
    waitTargetFrames = 0;
    waitTimerFrames = 0;
    hasActiveBg = false;
    
    if (hasActiveBgm) {
        StopMusicStream(activeBgm);
        hasActiveBgm = false;
    }

    if (eventCount > 0 && events[0].type == EVENT_DIALOGUE) {
        ResetTypewriter();
    }
}

GameScreen UpdateScreenDialogue(Audio* audio)
{
    // Fast-forward through non-blocking events (BG, BGM, SFX)
    while (currentEvent < eventCount) {
        SceneEvent* ev = &events[currentEvent];
        if (ev->type == EVENT_BG) {
            activeBg = GetCachedTexture(ev->text);
            hasActiveBg = (activeBg.id > 0);
            currentEvent++;
        } else if (ev->type == EVENT_BGM) {
            if (hasActiveBgm) StopMusicStream(activeBgm);
            activeBgm = GetCachedMusic(ev->text);
            hasActiveBgm = (activeBgm.stream.buffer != NULL);
            if (hasActiveBgm) PlayMusicStream(activeBgm);
            currentEvent++;
        } else if (ev->type == EVENT_SFX) {
            Sound s = GetCachedSound(ev->text);
            if (s.stream.buffer != NULL) PlaySound(s);
            currentEvent++;
        } else {
            // Found a blocking event (WAIT or DIALOGUE)
            break;
        }
    }

    // Always update BGM if it's playing
    if (hasActiveBgm) {
        UpdateMusicStream(activeBgm);
    }

    // Sequence end reached
    if (currentEvent >= eventCount) {
        if (hasActiveBgm) StopMusicStream(activeBgm);
        return GAMEPLAY;
    }

    // Handle blocking events
    SceneEvent* current = &events[currentEvent];
    
    if (current->type == EVENT_WAIT) {
        if (waitTargetFrames == 0) {
            waitTargetFrames = (int)(current->floatArg * 60.0f); // Map to 60 FPS
            waitTimerFrames = 0;
        }
        waitTimerFrames++;
        if (waitTimerFrames >= waitTargetFrames) {
            waitTargetFrames = 0;
            currentEvent++;
            if (currentEvent < eventCount && events[currentEvent].type == EVENT_DIALOGUE) {
                ResetTypewriter();
            }
        }
    } 
    else if (current->type == EVENT_DIALOGUE) {
        // Typewriter effect logic
        if (!isFinishedTyping) {
            framesCounter++;
            if (framesCounter % 3 == 0) {
                if (textIndex < dialogueLength) {
                    dialogueCurrent[textIndex] = current->text[textIndex];
                    dialogueCurrent[textIndex + 1] = '\0';
                    textIndex++;
                    PlayRandomSpeaking(audio);
                } else {
                    isFinishedTyping = true;
                }
            }
        }
        
        // Input logic to advance
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (!isFinishedTyping) {
                // Speed up typing
                strcpy(dialogueCurrent, current->text);
                isFinishedTyping = true;
                textIndex = dialogueLength;
            } else {
                // Advance
                currentEvent++;
                if (currentEvent < eventCount && events[currentEvent].type == EVENT_DIALOGUE) {
                    ResetTypewriter();
                }
            }
        }
    }

    return DIALOGUE;
}

void DrawScreenDialogue(void)
{
    if (hasActiveBg) {
        float scaleX = (float)VIRTUAL_WIDTH / (float)activeBg.width;
        float scaleY = (float)VIRTUAL_HEIGHT / (float)activeBg.height;
        float scale = (scaleX > scaleY) ? scaleX : scaleY;
        DrawTextureEx(activeBg, (Vector2){0, 0}, 0.0f, scale, WHITE);
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.3f));
    } else {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.5f));
    }

    if (currentEvent >= eventCount || events[currentEvent].type != EVENT_DIALOGUE) {
        return; // Only draw UI elements if we are on a dialogue prompt
    }

    SceneEvent* current = &events[currentEvent];

    // Draw character portrait
    Texture2D portrait = portraits[currentEvent];
    if (portrait.id > 0) {
        float portraitScale = 367.0f / (float)portrait.width;
        float portraitW = portrait.width * portraitScale;
        float portraitH = portrait.height * portraitScale;
        float portraitY = VIRTUAL_HEIGHT - portraitH - 190;

        if (current->isRight) {
            float portraitX = VIRTUAL_WIDTH - portraitW - 40;
            DrawTextureEx(portrait, (Vector2){portraitX, portraitY}, 0.0f, portraitScale, WHITE);
        } else {
            float portraitX = 40;
            DrawTextureEx(portrait, (Vector2){portraitX, portraitY}, 0.0f, portraitScale, WHITE);
        }
    }
    
    // Dialogue Box properties
    int boxMargin = 40;
    int boxHeight = 150;
    Rectangle dialogBox = { (float)boxMargin, (float)(VIRTUAL_HEIGHT - boxHeight - boxMargin), (float)(VIRTUAL_WIDTH - (boxMargin * 2)), (float)boxHeight };
    
    // Name Box properties
    Rectangle nameBox = { (float)boxMargin, dialogBox.y - 40.0f, 200.0f, 40.0f };
    
    // Draw Name Box
    DrawRectangleRec(nameBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(nameBox, 3.0f, WHITE);
    DrawText(current->speaker, (int)nameBox.x + 20, (int)nameBox.y + 10, 20, WHITE);
    
    // Draw Main Dialogue Box
    DrawRectangleRec(dialogBox, Fade(BLACK, 0.8f));
    DrawRectangleLinesEx(dialogBox, 3.0f, WHITE);
    
    // Draw typing text
    DrawText(dialogueCurrent, (int)dialogBox.x + 20, (int)dialogBox.y + 20, 24, LIGHTGRAY);
    
    // Blinking prompt
    if (isFinishedTyping) {
        framesCounter++;
        if ((framesCounter / 30) % 2 == 0) {
            DrawText(">>", (int)(dialogBox.x + dialogBox.width - 30), (int)(dialogBox.y + dialogBox.height - 30), 20, WHITE);
        }
    }
}

void UnloadScreenDialogue(void)
{
    // Unload portraits
    for (int i = 0; i < eventCount; i++) {
        if (portraits[i].id > 0) {
            UnloadTexture(portraits[i]);
            portraits[i].id = 0;
        }
    }
    // Unload cached textures
    for (int i = 0; i < texCacheCount; i++) {
        if (cachedTextures[i].id > 0) UnloadTexture(cachedTextures[i]);
    }
    // Unload cached sounds
    for (int i = 0; i < soundCacheCount; i++) {
        if (cachedSounds[i].stream.buffer != NULL) UnloadSound(cachedSounds[i]);
    }
    // Unload cached music
    for (int i = 0; i < musicCacheCount; i++) {
        if (cachedMusic[i].stream.buffer != NULL) UnloadMusicStream(cachedMusic[i]);
    }

    eventCount = 0;
    currentEvent = 0;
    texCacheCount = 0;
    soundCacheCount = 0;
    musicCacheCount = 0;
    hasActiveBg = false;
    hasActiveBgm = false;
}

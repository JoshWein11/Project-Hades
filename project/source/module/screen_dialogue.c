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

// Character portrait registry — each character's images loaded once
#define MAX_CHARACTERS 16
typedef struct {
    char name[64];
    Texture2D portraitLeft;
    Texture2D portraitRight;
} CharacterPortrait;

static CharacterPortrait charRegistry[MAX_CHARACTERS];
static int charRegistryCount = 0;

// Helper: look up a character's portrait by name and side
static Texture2D GetCharacterPortrait(const char* name, bool isRight) {
    for (int i = 0; i < charRegistryCount; i++) {
        if (strcmp(charRegistry[i].name, name) == 0) {
            return isRight ? charRegistry[i].portraitRight : charRegistry[i].portraitLeft;
        }
    }
    Texture2D empty = {0};
    return empty;
}

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

// State for ANIM event (sprite sheet playback)
static Texture2D animTexture = {0};
static bool animActive = false;
static int animTotalFrames = 0;
static int animCurrentFrame = 0;
static float animFps = 5.0f;
static int animFrameTimer = 0;

// State for CAMERA event
static Vector2 currentCamOffset = {0};
static Vector2 targetCamOffset = {0};
static Vector2 startCamOffset = {0};
static float camShiftTimer = 0.0f;
static float camShiftDuration = 0.0f;

// Title card fade state
typedef enum { TITLE_FADE_IN, TITLE_HOLD, TITLE_FADE_OUT } TitlePhase;
static TitlePhase titlePhase = TITLE_FADE_IN;
static int titleTimer = 0;
static float titleAlpha = 0.0f;

#define TITLE_FADE_IN_FRAMES  90   // 1.5 seconds at 60 FPS
#define TITLE_HOLD_FRAMES     120  // 2.0 seconds
#define TITLE_FADE_OUT_FRAMES 90   // 1.5 seconds

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
// Format: SPEAKER|left_or_right  (portrait looked up from character registry)
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

    // Parse speaker line: SPEAKER|left_or_right
    char* pipe1 = strchr(speakerLine, '|');
    if (pipe1 == NULL) return false;

    // Speaker name
    int nameLen = (int)(pipe1 - speakerLine);
    if (nameLen >= 64) nameLen = 63;
    strncpy(entry->speaker, speakerLine, nameLen);
    entry->speaker[nameLen] = '\0';
    TrimString(entry->speaker);

    // Position (left or right) — everything after the pipe
    char position[16];
    strncpy(position, pipe1 + 1, 15);
    position[15] = '\0';
    TrimString(position);
    entry->isRight = (strcmp(position, "right") == 0);

    entry->type = EVENT_DIALOGUE;
    strncpy(entry->text, textLine, 255);
    entry->text[255] = '\0';
    entry->image[0] = '\0'; // No longer used

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
    charRegistryCount = 0;
    hasActiveBg = false;
    hasActiveBgm = false;
    titlePhase = TITLE_FADE_IN;
    titleTimer = 0;
    titleAlpha = 0.0f;
    animActive = false;
    animCurrentFrame = 0;
    animFrameTimer = 0;
    animTotalFrames = 0;
    currentCamOffset = (Vector2){0, 0};
    targetCamOffset = (Vector2){0, 0};
    startCamOffset = (Vector2){0, 0};
    camShiftTimer = 0.0f;
    camShiftDuration = 0.0f;
    memset(&animTexture, 0, sizeof(Texture2D));
    memset(&activeBg, 0, sizeof(Texture2D));
    memset(&activeBgm, 0, sizeof(Music));
    memset(charRegistry, 0, sizeof(charRegistry));

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
                                } else if (strcmp(cmdType, "TITLE") == 0) {
                                    ev->type = EVENT_TITLE;
                                    strncpy(ev->text, cmdArg, 255);
                                    ev->text[255] = '\0';
                                    eventCount++;
                                } else if (strcmp(cmdType, "ANIM") == 0) {
                                    // CMD|ANIM|filename.png|columns|fps
                                    // cmdArg = "filename.png|10|5"
                                    char animFile[128] = {0};
                                    int animCols = 1;
                                    float animSpeed = 5.0f;
                                    char* ap1 = strchr(cmdArg, '|');
                                    if (ap1) {
                                        int afLen = (int)(ap1 - cmdArg);
                                        if (afLen >= 128) afLen = 127;
                                        strncpy(animFile, cmdArg, afLen);
                                        animFile[afLen] = '\0';
                                        TrimString(animFile);
                                        char* ap2 = strchr(ap1 + 1, '|');
                                        if (ap2) {
                                            // Has both columns and fps
                                            char colStr[16] = {0};
                                            int colLen = (int)(ap2 - ap1 - 1);
                                            if (colLen >= 16) colLen = 15;
                                            strncpy(colStr, ap1 + 1, colLen);
                                            colStr[colLen] = '\0';
                                            TrimString(colStr);
                                            animCols = atoi(colStr);
                                            animSpeed = (float)atof(ap2 + 1);
                                        } else {
                                            // Only columns, default 5 fps
                                            animCols = atoi(ap1 + 1);
                                        }
                                    } else {
                                        // No pipes — just filename, default 1 col 5 fps
                                        strncpy(animFile, cmdArg, 127);
                                        animFile[127] = '\0';
                                        TrimString(animFile);
                                    }
                                    if (animCols < 1) animCols = 1;
                                    if (animSpeed <= 0.0f) animSpeed = 5.0f;
                                    ev->type = EVENT_ANIM;
                                    snprintf(ev->text, sizeof(ev->text), "../assets/images/background/%s", animFile);
                                    ev->intArg = animCols;
                                    ev->floatArg = animSpeed;
                                    GetCachedTexture(ev->text); // Preload
                                    eventCount++;
                                } else if (strcmp(cmdType, "CAM") == 0) {
                                    // CMD|CAM|dx|dy|duration
                                    char dx[16] = {0}, dy[16] = {0}, dur[16] = {0};
                                    char* p1 = strchr(cmdArg, '|');
                                    if (p1) {
                                        int len1 = (int)(p1 - cmdArg);
                                        if (len1 >= 16) len1 = 15;
                                        strncpy(dx, cmdArg, len1);
                                        char* p2 = strchr(p1 + 1, '|');
                                        if (p2) {
                                            int len2 = (int)(p2 - p1 - 1);
                                            if (len2 >= 16) len2 = 15;
                                            strncpy(dy, p1 + 1, len2);
                                            strncpy(dur, p2 + 1, 15);
                                        } else {
                                            strncpy(dy, p1 + 1, 15);
                                        }
                                    } else {
                                        strncpy(dx, cmdArg, 15);
                                    }
                                    ev->type = EVENT_CAMERA;
                                    ev->floatArg = (float)atof(dx);
                                    ev->floatArg2 = (float)atof(dy);
                                    ev->floatArg3 = (float)atof(dur);
                                    eventCount++;
                                } else if (strcmp(cmdType, "CHAR") == 0 && charRegistryCount < MAX_CHARACTERS) {
                                    // CMD|CHAR|NAME|left_img.png|right_img.png
                                    // cmdArg = "NAME|left_img.png|right_img.png"
                                    char charName[64] = {0};
                                    char leftImg[128] = {0};
                                    char rightImg[128] = {0};
                                    char* cp1 = strchr(cmdArg, '|');
                                    if (cp1) {
                                        int cnLen = (int)(cp1 - cmdArg);
                                        if (cnLen >= 64) cnLen = 63;
                                        strncpy(charName, cmdArg, cnLen);
                                        charName[cnLen] = '\0';
                                        TrimString(charName);
                                        char* cp2 = strchr(cp1 + 1, '|');
                                        if (cp2) {
                                            // Has both left and right images
                                            int lLen = (int)(cp2 - cp1 - 1);
                                            if (lLen >= 128) lLen = 127;
                                            strncpy(leftImg, cp1 + 1, lLen);
                                            leftImg[lLen] = '\0';
                                            TrimString(leftImg);
                                            strncpy(rightImg, cp2 + 1, 127);
                                            rightImg[127] = '\0';
                                            TrimString(rightImg);
                                        } else {
                                            // Only one image — used for both sides
                                            strncpy(leftImg, cp1 + 1, 127);
                                            leftImg[127] = '\0';
                                            TrimString(leftImg);
                                            strcpy(rightImg, leftImg);
                                        }
                                        // Load textures into registry
                                        char pathBuf[256];
                                        CharacterPortrait* cp = &charRegistry[charRegistryCount];
                                        strcpy(cp->name, charName);
                                        snprintf(pathBuf, sizeof(pathBuf), "../assets/images/character/%s", leftImg);
                                        cp->portraitLeft = LoadTexture(pathBuf);
                                        snprintf(pathBuf, sizeof(pathBuf), "../assets/images/character/%s", rightImg);
                                        cp->portraitRight = LoadTexture(pathBuf);
                                        charRegistryCount++;
                                        TraceLog(LOG_INFO, "DIALOGUE: Registered character '%s' (L:%s R:%s)", charName, leftImg, rightImg);
                                    }
                                    // CHAR is not a playback event — don't increment eventCount
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
    titlePhase = TITLE_FADE_IN;
    titleTimer = 0;
    titleAlpha = 0.0f;
    animActive = false;
    animCurrentFrame = 0;
    animFrameTimer = 0;
    currentCamOffset = (Vector2){0, 0};
    targetCamOffset = (Vector2){0, 0};
    startCamOffset = (Vector2){0, 0};
    camShiftTimer = 0.0f;
    camShiftDuration = 0.0f;
    
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
        } else if (ev->type == EVENT_CAMERA) {
            targetCamOffset = (Vector2){ ev->floatArg, ev->floatArg2 };
            startCamOffset = currentCamOffset;
            camShiftDuration = ev->floatArg3;
            camShiftTimer = 0.0f;
            if (camShiftDuration <= 0.0f) {
                currentCamOffset = targetCamOffset;
            }
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
    
    // Smooth camera interpolation
    if (camShiftDuration > 0.0f && camShiftTimer < camShiftDuration) {
        camShiftTimer += GetFrameTime();
        if (camShiftTimer > camShiftDuration) camShiftTimer = camShiftDuration;
        float t = camShiftTimer / camShiftDuration;
        float ease = t * t * (3.0f - 2.0f * t); // Smooth step
        currentCamOffset.x = startCamOffset.x + (targetCamOffset.x - startCamOffset.x) * ease;
        currentCamOffset.y = startCamOffset.y + (targetCamOffset.y - startCamOffset.y) * ease;
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
    else if (current->type == EVENT_TITLE) {
        titleTimer++;
        switch (titlePhase) {
            case TITLE_FADE_IN:
                titleAlpha = (float)titleTimer / (float)TITLE_FADE_IN_FRAMES;
                if (titleAlpha >= 1.0f) {
                    titleAlpha = 1.0f;
                    titlePhase = TITLE_HOLD;
                    titleTimer = 0;
                }
                break;
            case TITLE_HOLD:
                titleAlpha = 1.0f;
                if (titleTimer >= TITLE_HOLD_FRAMES) {
                    titlePhase = TITLE_FADE_OUT;
                    titleTimer = 0;
                }
                break;
            case TITLE_FADE_OUT:
                titleAlpha = 1.0f - (float)titleTimer / (float)TITLE_FADE_OUT_FRAMES;
                if (titleAlpha <= 0.0f) {
                    titleAlpha = 0.0f;
                    titlePhase = TITLE_FADE_IN;
                    titleTimer = 0;
                    currentEvent++;
                    if (currentEvent < eventCount && events[currentEvent].type == EVENT_DIALOGUE) {
                        ResetTypewriter();
                    }
                }
                break;
        }
        // Allow skipping with click or space
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (titlePhase != TITLE_FADE_OUT) {
                // Jump to fade-out
                titlePhase = TITLE_FADE_OUT;
                titleTimer = 0;
                titleAlpha = 1.0f;
            } else {
                // Already fading out, skip entirely
                titleAlpha = 0.0f;
                titlePhase = TITLE_FADE_IN;
                titleTimer = 0;
                currentEvent++;
                if (currentEvent < eventCount && events[currentEvent].type == EVENT_DIALOGUE) {
                    ResetTypewriter();
                }
            }
        }
    }
    else if (current->type == EVENT_ANIM) {
        // Initialize animation on first frame
        if (!animActive) {
            animTexture = GetCachedTexture(current->text);
            animActive = (animTexture.id > 0);
            animTotalFrames = current->intArg;
            animFps = current->floatArg;
            animCurrentFrame = 0;
            animFrameTimer = 0;
            TraceLog(LOG_INFO, "ANIM: file=%s id=%d size=%dx%d frames=%d fps=%.1f active=%d",
                     current->text, animTexture.id, animTexture.width, animTexture.height,
                     animTotalFrames, animFps, animActive);
        }

        if (animActive) {
            animFrameTimer++;
            int framesPerTick = (int)(60.0f / animFps);
            if (framesPerTick < 1) framesPerTick = 1;
            if (animFrameTimer >= framesPerTick) {
                animFrameTimer = 0;
                animCurrentFrame++;
                if (animCurrentFrame >= animTotalFrames) {
                    // Animation finished
                    animActive = false;
                    currentEvent++;
                    if (currentEvent < eventCount && events[currentEvent].type == EVENT_DIALOGUE) {
                        ResetTypewriter();
                    }
                }
            }
        } else {
            // Texture failed to load, skip
            currentEvent++;
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
    // Title card: full black screen with centered fading text
    if (currentEvent < eventCount && events[currentEvent].type == EVENT_TITLE) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACK);
        SceneEvent* titleEv = &events[currentEvent];
        int fontSize = 40;
        int textWidth = MeasureText(titleEv->text, fontSize);
        int x = (VIRTUAL_WIDTH - textWidth) / 2;
        int y = VIRTUAL_HEIGHT / 2 - fontSize / 2;
        DrawText(titleEv->text, x, y, fontSize, Fade(WHITE, titleAlpha));
        return;
    }

    // Sprite sheet animation: draw current frame (rows = stacked vertically)
    if (currentEvent < eventCount && events[currentEvent].type == EVENT_ANIM && animActive) {
        int frameHeight = animTexture.height / animTotalFrames;
        Rectangle src = { 0, (float)(animCurrentFrame * frameHeight),
                          (float)animTexture.width, (float)frameHeight };
        Rectangle dest = { 0, 0, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT };
        DrawTexturePro(animTexture, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
        return;
    }

    if (hasActiveBg) {
        float scaleX = (float)VIRTUAL_WIDTH / (float)activeBg.width;
        float scaleY = (float)VIRTUAL_HEIGHT / (float)activeBg.height;
        float scale = (scaleX > scaleY) ? scaleX : scaleY;
        DrawTextureEx(activeBg, (Vector2){0, 0}, 0.0f, scale, WHITE);
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.3f));
    } else {
        // Draw nothing — background is perfectly transparent over the active game scene!
    }

    if (currentEvent >= eventCount || events[currentEvent].type != EVENT_DIALOGUE) {
        return; // Only draw UI elements if we are on a dialogue prompt
    }

    SceneEvent* current = &events[currentEvent];

    // Draw character portrait
    Texture2D portrait = GetCharacterPortrait(current->speaker, current->isRight);
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
    // Unload character registry portraits
    for (int i = 0; i < charRegistryCount; i++) {
        if (charRegistry[i].portraitLeft.id > 0) UnloadTexture(charRegistry[i].portraitLeft);
        if (charRegistry[i].portraitRight.id > 0) UnloadTexture(charRegistry[i].portraitRight);
    }
    charRegistryCount = 0;
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
    
    currentCamOffset = (Vector2){0,0};
    targetCamOffset = (Vector2){0,0};
    startCamOffset = (Vector2){0,0};
    camShiftTimer = 0.0f;
    camShiftDuration = 0.0f;
}

void LoadDialogueFile(const char* dialogueFile)
{
    UnloadScreenDialogue();
    InitScreenDialogue(dialogueFile);
}

Vector2 GetDialogueCameraOffset(void)
{
    return currentCamOffset;
}

#include "raylib.h" //Code written by: Christopher 沈佳豪
#include "raymath.h"
#include "screen_gameplay.h"
#include "character.h"
#include "audio.h"
#include "weapon.h"
#include "tiled_map.h"
#include "setting.h"
#include "enemy.h"
#include "savedata.h"
#include "screen_dialogue.h"
#include "puzzle.h"
#include <string.h>
#include <stdio.h>

static Character player;
static MapData   map;
static Weapon    playerWeapon;
static Camera2D  camera = { 0 };

// Shared enemy sprite textures — loaded once, used by all enemies
static Texture2D enemySpriteR;
static Texture2D enemySpriteL;

static bool isPaused = false;

// ─────────────────────────────────────────────────────────────────────────────
// Objective System
// ─────────────────────────────────────────────────────────────────────────────
static bool objMoveDone = false;
static bool objDashDone = false;
static bool objShootDone = false;
static bool objEvacuateDone = false;
static bool displayObjectives = false;

// ─────────────────────────────────────────────────────────────────────────────
// Puzzle / Generator System
// ─────────────────────────────────────────────────────────────────────────────
#define MAX_GENS 4
static PuzzleState genPuzzles[MAX_GENS];   // One puzzle per generator
static int         genMapIndex[MAX_GENS];  // Maps gen index → puzzleObjects[] index
static bool        genSolved[MAX_GENS];    // Tracks which gens are activated
static int         genCount = 0;           // How many "Gens" objects were found
static int         gensSolvedCount = 0;    // Total solved
static int         nearbyGenIndex = -1;    // Which gen the player is currently near (-1 = none)
static int         activeGenIndex = -1;    // Which gen's puzzle is open (-1 = none)
static bool        nearOrionTrigger = false; // Player is inside Next ACT zone (after all gens done)

// ─────────────────────────────────────────────────────────────────────────────
// Interactive Image Viewer (Act 3)
// ─────────────────────────────────────────────────────────────────────────────
static Texture2D imgCode;
static Texture2D imgRule;
static int       imageMapIndex[2] = {-1, -1}; // Maps [0]=code, [1]=rules to puzzleObjects[] index (-1 if none)
static int       nearbyImageIndex = -1;       // 0=code, 1=rules, -1=none
static int       activeImageIndex = -1;       // 0=code, 1=rules, -1=none

// ─────────────────────────────────────────────────────────────────────────────
// Puzzle Systems (Act 3)
// ─────────────────────────────────────────────────────────────────────────────
static CodePuzzle codePuzzle;
static int        codePuzzleMapIndex = -1;
static bool       codePuzzleNearby = false;

// ─────────────────────────────────────────────────────────────────────────────
// RGB Puzzle System (Act 3)
// ─────────────────────────────────────────────────────────────────────────────
static RGBPuzzle rgbPuzzle;             // State for tempo rhythm minigame
static int       rgbMapIndex = -1;      // Tiled object index
static bool      rgbNearby = false;     // Player near rgb puzzle
static Sound     sfxButton;
static Sound     sfxFail;

// ─────────────────────────────────────────────────────────────────────────────
// Keycard & Popup System (Act 3)
// ─────────────────────────────────────────────────────────────────────────────
static bool hasKeycardA = false;
static bool hasKeycardB = false;
static float pickupTimer = 0.0f;
static const char* pickupText = "";
static bool nearAct3Exit = false;

// ─────────────────────────────────────────────────────────────────────────────
// In-Game Dialogue System
// ─────────────────────────────────────────────────────────────────────────────
static bool isInGameDialogue = false;
static int nearbyCharacterIndex = -1;

// ─────────────────────────────────────────────────────────────────────────────
// Enemy list — increase capacity and call InitEnemy multiple times to add more.
//
//   Placing an enemy anywhere:
//     Edit 'patrolPoints' and pass them to InitEnemy with the desired spawnPos.
//
//   Giving an enemy a sprite:
//     Replace NULL with a path string, e.g. "../assets/character/enemy.png",
//     and set the correct frameWidth / frameHeight for your sprite sheet.
// ─────────────────────────────────────────────────────────────────────────────
#define MAX_ENEMIES 30
static Enemy enemies[MAX_ENEMIES];
static int   enemyCount = 0;

// ── Screen shake state ────────────────────────────────────────────────────────
static float shakeTimer     = 0.0f;
static float shakeMagnitude = 0.0f;

// ── Lives & death state ───────────────────────────────────────────────────────
#define STARTING_LIVES 5
static int   playerLives    = STARTING_LIVES;
static bool  isPlayerDead   = false;
static float deathTimer     = 0.0f;
static bool  isGameOver     = false;
static float deathFadeAlpha = 0.0f;

// Spawn point for the current chapter (used for respawn)
static Vector2 chapterSpawnPos = { 600, 867 };

// Act/chapter tracking
static int currentAct     = 0;
static int currentChapter  = 0;
static int pendingAct     = -1;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: trigger a camera shake
// ─────────────────────────────────────────────────────────────────────────────
static void TriggerShake(float magnitude, float duration) {
    shakeMagnitude = magnitude;
    shakeTimer     = duration;
}

// ─────────────────────────────────────────────────────────────────────────────
// SpawnEnemiesFromMap — reads map.enemySpawns[] (parsed from the Tiled
//   "Enemy" object layer) and calls InitEnemy for each enemy.
//
//   Unnamed points → stationary enemy (1 waypoint).
//   Points with the same name → one enemy patrolling between those waypoints.
// ─────────────────────────────────────────────────────────────────────────────
static void SpawnEnemiesFromMap(void)
{
    bool processed[MAX_ENEMY_SPAWNS] = {0};

    printf("[Enemy] Total spawn points from map: %d\n", map.enemySpawnCount);
    for (int d = 0; d < map.enemySpawnCount; d++) {
        printf("[Enemy]   spawn[%d] pos=(%.0f, %.0f) group='%s'\n",
               d, map.enemySpawns[d].position.x, map.enemySpawns[d].position.y,
               map.enemySpawns[d].group);
    }

    for (int i = 0; i < map.enemySpawnCount; i++) {
        if (processed[i]) continue;

        EnemySpawnPoint* sp = &map.enemySpawns[i];

        if (sp->group[0] == '\0') {
            // ── Stationary enemy: single waypoint ──────────────────────────
            if (enemyCount < MAX_ENEMIES) {
                Vector2 pts[] = { sp->position };
                InitEnemy(&enemies[enemyCount++], sp->position, pts, 1,
                    enemySpriteR, enemySpriteL, 32, 32, 0.67f);
                printf("[Enemy] Spawned STATIONARY enemy #%d at (%.0f, %.0f)\n",
                       enemyCount, sp->position.x, sp->position.y);
            } else {
                printf("[Enemy] SKIPPED stationary at (%.0f, %.0f) — MAX_ENEMIES reached\n",
                       sp->position.x, sp->position.y);
            }
            processed[i] = true;
        } else {
            // ── Patrol group: gather all points with the same name ─────────
            Vector2 waypoints[ENEMY_MAX_WAYPOINTS];
            int wpCount = 0;

            for (int j = i; j < map.enemySpawnCount; j++) {
                if (!processed[j] && strcmp(map.enemySpawns[j].group, sp->group) == 0) {
                    if (wpCount < ENEMY_MAX_WAYPOINTS) {
                        waypoints[wpCount++] = map.enemySpawns[j].position;
                    }
                    processed[j] = true;
                }
            }

            if (enemyCount < MAX_ENEMIES && wpCount > 0) {
                InitEnemy(&enemies[enemyCount++], waypoints[0], waypoints, wpCount,
                    enemySpriteR, enemySpriteL, 32, 32, 0.67f);
                printf("[Enemy] Spawned PATROL enemy #%d group='%s' with %d waypoints\n",
                       enemyCount, sp->group, wpCount);
            } else {
                printf("[Enemy] SKIPPED patrol group '%s' — MAX_ENEMIES reached or no waypoints\n",
                       sp->group);
            }
        }
    }
    printf("[Enemy] Total enemies spawned: %d\n", enemyCount);
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupAct — configure map, enemies, and spawn point for a given act.
//   Call this when initialising or transitioning acts.
// ─────────────────────────────────────────────────────────────────────────────
static void SetupAct(int act)
{
    // Clean up previous act resources
    UnloadTiledMap(&map);
    for (int i = 0; i < enemyCount; i++) UnloadEnemy(&enemies[i]);
    enemyCount = 0;

    // Unload previous shared enemy textures
    if (enemySpriteR.id != 0) { UnloadTexture(enemySpriteR); enemySpriteR = (Texture2D){0}; }
    if (enemySpriteL.id != 0) { UnloadTexture(enemySpriteL); enemySpriteL = (Texture2D){0}; }

    currentAct = act;

    switch (act) {
        case 0: // ACT 1 — Prologue (stun-only bullets)
            LoadTiledMap(&map, "../assets/map/trial.json");
            chapterSpawnPos = (Vector2){ 600, 867 };
            enemySpriteR = LoadTexture("../assets/character/slime_abberant_r.png");
            enemySpriteL = LoadTexture("../assets/character/slime_abberant_l.png");
            SpawnEnemiesFromMap();
            objMoveDone = false;
            objDashDone = false;
            objShootDone = false;
            objEvacuateDone = false;
            displayObjectives = true;
            break;

        case 1: // ACT 2 — Level 1 (checkpoint map, stun-only)
            LoadTiledMap(&map, "../assets/map/level 1.json");
            chapterSpawnPos = map.hasSpawnPoint ? map.spawnPoint : (Vector2){ 600, 867 };
            enemySpriteR = LoadTexture("../assets/character/slime_abberant_r.png");
            enemySpriteL = LoadTexture("../assets/character/slime_abberant_l.png");
            SpawnEnemiesFromMap();
            displayObjectives = true;

            // ── Scan Puzzle layer for "Gens" objects ──────────────────────────
            genCount = 0;
            gensSolvedCount = 0;
            nearbyGenIndex = -1;
            activeGenIndex = -1;
            nearOrionTrigger = false;
            for (int i = 0; i < map.puzzleObjectCount && genCount < MAX_GENS; i++) {
                if (strcmp(map.puzzleObjects[i].name, "Gens") == 0) {
                    InitPuzzle(&genPuzzles[genCount]);
                    genMapIndex[genCount] = i;
                    genSolved[genCount] = false;
                    genCount++;
                }
            }
            printf("[Puzzle] Found %d generators on Puzzle layer\n", genCount);
            break;

        case 2: // ACT 3 — Level 2 (full damage)
            LoadTiledMap(&map, "../assets/map/level 2.json");
            chapterSpawnPos = map.hasSpawnPoint ? map.spawnPoint : (Vector2){ 600, 867 };
            enemySpriteR = LoadTexture("../assets/character/slime_abberant_r.png");
            enemySpriteL = LoadTexture("../assets/character/slime_abberant_l.png");
            SpawnEnemiesFromMap();
            displayObjectives = true;
            
            // ── Scan Puzzle layer for image viewers, code, and rgb ───────────────────
            imageMapIndex[0] = -1;
            imageMapIndex[1] = -1;
            nearbyImageIndex = -1;
            activeImageIndex = -1;

            rgbMapIndex = -1;
            rgbNearby = false;
            rgbPuzzle.solved = false; // Reset solved state for new map session
            rgbPuzzle.active = false;
            
            // Setup CodePuzzle
            Texture2D codeBg = LoadTexture("../assets/images/puzzle/code_puzzle.png");
            InitCodePuzzle(&codePuzzle, codeBg);
            codePuzzleMapIndex = -1;
            codePuzzleNearby = false;

            for (int i = 0; i < map.puzzleObjectCount; i++) {
                if (strcmp(map.puzzleObjects[i].name, "note1") == 0) {
                    imageMapIndex[0] = i; 
                } else if (strcmp(map.puzzleObjects[i].name, "rules") == 0) {
                    imageMapIndex[1] = i; 
                } else if (strcmp(map.puzzleObjects[i].name, "rgb") == 0) {
                    rgbMapIndex = i;
                } else if (strcmp(map.puzzleObjects[i].name, "code") == 0 || strcmp(map.puzzleObjects[i].name, "code_puzzle") == 0) {
                    codePuzzleMapIndex = i;
                }
            }
            break;

        default:
            break;
    }

    // Place player at the act's spawn point
    player.position = chapterSpawnPos;
    player.health   = player.maxHealth;
}

// ─────────────────────────────────────────────────────────────────────────────
// InitScreenGameplay
// ─────────────────────────────────────────────────────────────────────────────
void InitScreenGameplay(void)
{
    // ── Player ────────────────────────────────────────────────────────────────
    InitCharacter(&player,
                  600,
                  867,
                  "../assets/character/Adrian_Walk.png", 6, 2);

    // ── Weapon ────────────────────────────────────────────────────────────────
    InitWeapon(&playerWeapon);

    // ── Camera ────────────────────────────────────────────────────────────────
    camera.target   = player.position;
    camera.offset   = (Vector2){ VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 2.0f;

    // ── State reset ───────────────────────────────────────────────────────────
    isPaused       = false;
    playerLives    = STARTING_LIVES;
    isPlayerDead   = false;
    deathTimer     = 0.0f;
    isGameOver     = false;
    deathFadeAlpha = 0.0f;
    currentChapter = 0;

    // ── Load Act 3 specific puzzle assets ─────────────────────────────────────
    imgCode = LoadTexture("../assets/images/code.png");
    imgRule = LoadTexture("../assets/images/rule.png");
    Texture2D bg1 = LoadTexture("../assets/images/puzzle/rgb_1.png");
    Texture2D bg2 = LoadTexture("../assets/images/puzzle/rgb_2.png");
    Texture2D bg3 = LoadTexture("../assets/images/puzzle/rgb_3.png");
    InitRGBPuzzle(&rgbPuzzle, bg1, bg2, bg3);
    sfxButton = LoadSound("../assets/sfx/button.wav");
    sfxFail = LoadSound("../assets/sfx/button_fail.wav");

    // ── Load first act ────────────────────────────────────────────────────────
    SetupAct(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateScreenGameplay
// ─────────────────────────────────────────────────────────────────────────────
GameScreen UpdateScreenGameplay(Audio* gameAudio)
{
    float dt = GetFrameTime();

    // ── Pending Map Transition ────────────────────────────────────────────────
    if (pendingAct != -1) {
        SetupAct(pendingAct);
        pendingAct = -1;
    }

    // ── Pause Handle ──────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_ESCAPE)) {
        isPaused = !isPaused;
    }

    if (isPaused) {
        Vector2 mousePoint = GetVirtualMouse();
        float centerX = VIRTUAL_WIDTH / 2.0f;
        float centerY = VIRTUAL_HEIGHT / 2.0f;
        
        Rectangle saveBtn     = { centerX - 100, centerY - 80, 200, 50 };
        Rectangle settingsBtn = { centerX - 100, centerY - 10, 200, 50 };
        Rectangle exitBtn     = { centerX - 100, centerY + 60, 200, 50 };

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mousePoint, saveBtn)) {
                SaveData save = {0};
                GetGameplaySaveData(&save);
                save.currentScreen    = (int)GAMEPLAY;
                save.dialogueFinished = true;
                SaveGame(&save, SAVE_FILEPATH);
                isPaused = false; // Unpause after saving
            } else if (CheckCollisionPointRec(mousePoint, settingsBtn)) {
                return SETTINGS;
            } else if (CheckCollisionPointRec(mousePoint, exitBtn)) {
                isPaused = false; // Reset pause state when exiting
                return MAIN_MENU;
            }
        }
        return GAMEPLAY; // Skip logic updates
    }

    // ── Camera offset (shake applied later in Draw) ───────────────────────────
    camera.offset = (Vector2){ VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
    camera.target = (Vector2){
        player.position.x + (player.frameRec.width  * player.scale) / 2.0f,
        player.position.y + (player.frameRec.height * player.scale) / 2.0f
    };

    Vector2 mouseWorldPos = GetScreenToWorld2D(GetVirtualMouse(), camera);

    // ── Core Gameplay Logic (Freeze during death) ─────────────────────────────
    if (!isPlayerDead && !isGameOver) {

        // ── In-Game Dialogue Active: freeze gameplay, process dialogue ────────
        if (isInGameDialogue) {
            GameScreen result = UpdateScreenDialogue(gameAudio);
            if (result == GAMEPLAY) { // Finished playing sequence
                isInGameDialogue = false;
                UnloadScreenDialogue();
            }
            return GAMEPLAY; // Skip all other gameplay updates while discussing
        }

        // ── Code Puzzle Active: freeze gameplay ─────────────────────────────
        if (codePuzzle.active) {
            if (UpdateCodePuzzle(&codePuzzle, GetVirtualMouse(), sfxButton, sfxFail)) {
                // Puzzle solved!
                printf("[Puzzle] Code Memory Match solved!\n");
                hasKeycardB = true;
                pickupTimer = 4.0f;
                pickupText = "- KEYCARD B ACQUIRED -";
            }
            return GAMEPLAY;
        }

        // ── Image Viewer Active: freeze gameplay, wait for 'E' to close ───────
        if (activeImageIndex >= 0) { // Only triggered for rules image now
            if (IsKeyPressed(KEY_E)) {
                activeImageIndex = -1; // Close image view
            }
            return GAMEPLAY; // Skip all other gameplay updates while image is open
        }

        // ── RGB Puzzle Active: freeze gameplay, process rhythms ───────────────
        if (rgbPuzzle.active) {
            if (UpdateRGBPuzzle(&rgbPuzzle, GetVirtualMouse(), sfxButton, sfxFail)) {
                // Puzzle just solved!
                printf("[Puzzle] RGB Timing Game solved!\n");
                hasKeycardA = true;
                pickupTimer = 4.0f;
                pickupText = "- KEYCARD A ACQUIRED -";
            }
            return GAMEPLAY; // Skip all other gameplay updates while puzzle is open
        }

        // ── Puzzle UI Active: freeze gameplay, only process puzzle input ─────
        if (activeGenIndex >= 0) {
            if (IsKeyPressed(KEY_E)) {
                ClosePuzzle(&genPuzzles[activeGenIndex]); // Reset connections
                activeGenIndex = -1;
            } else {
                Vector2 puzzleMouse = GetVirtualMouse();
                if (UpdatePuzzle(&genPuzzles[activeGenIndex], puzzleMouse)) {
                    // Puzzle just solved!
                    genSolved[activeGenIndex] = true;
                    gensSolvedCount++;
                    printf("[Puzzle] Generator %d solved! (%d/%d)\n", activeGenIndex, gensSolvedCount, genCount);
                    activeGenIndex = -1;
                }
            }
            return GAMEPLAY; // Skip all other gameplay updates while puzzle is open
        }

        // ── Player ────────────────────────────────────────────────────────────
        UpdateCharacter(&player, map.collisionRecs, map.collisionCount, mouseWorldPos, gameAudio);

        // ── Character (NPC) Interaction (All acts) ────────────────────────────
        nearbyCharacterIndex = -1;
        {
            Rectangle playerHitbox = {
                player.position.x, player.position.y,
                player.frameRec.width * player.scale, player.frameRec.height * player.scale
            };
            
            for (int i = 0; i < map.characterObjectCount; i++) {
                if (CheckCollisionRecs(playerHitbox, map.characterObjects[i].bounds)) {
                    nearbyCharacterIndex = i;
                    if (IsKeyPressed(KEY_E)) {
                        char filepath[128];
                        snprintf(filepath, sizeof(filepath), "../assets/dialogue/character/%s.txt", map.characterObjects[i].name);
                        LoadDialogueFile(filepath);
                        isInGameDialogue = true;
                    }
                    break;
                }
            }
        }

        // ── Objective Tracking ────────────────────────────────────────────────
        if (displayObjectives) {
            if (currentAct == 0) {
                if (!objMoveDone && Vector2Length(player.velocity) > 0.1f) objMoveDone = true;
                if (!objDashDone && player.isDashing) objDashDone = true;
                if (!objShootDone && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) objShootDone = true;
                
                // Objective: Evacuate the laboratory
                if (!objEvacuateDone && map.hasNextActTrigger) {
                    Rectangle playerHitbox = {
                        player.position.x, player.position.y,
                        player.frameRec.width * player.scale, player.frameRec.height * player.scale
                    };
                    if (CheckCollisionRecs(playerHitbox, map.nextActTrigger)) {
                        objEvacuateDone = true;
                        SetupAct(1); // Load Act 2 map NOW so player is already there
                        LoadDialogueFile("../assets/dialogue/act2.txt");
                        return DIALOGUE;
                    }
                }
            } else if (currentAct == 1) {
                // ── Generator proximity + interaction ────────────────────────
                nearbyGenIndex = -1;
                nearOrionTrigger = false;
                {
                    Rectangle playerHitbox = {
                        player.position.x, player.position.y,
                        player.frameRec.width * player.scale, player.frameRec.height * player.scale
                    };

                    // Check generator proximity
                    for (int i = 0; i < genCount; i++) {
                        if (genSolved[i]) continue; // Already solved, skip
                        int mi = genMapIndex[i];
                        if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[mi].bounds)) {
                            nearbyGenIndex = i;
                            if (IsKeyPressed(KEY_E)) {
                                OpenPuzzle(&genPuzzles[i]);
                                activeGenIndex = i;
                            }
                            break;
                        }
                    }

                    // ── ORION activation gate (requires all gens solved) ─────
                    if (map.hasNextActTrigger && gensSolvedCount >= genCount && genCount > 0) {
                        if (CheckCollisionRecs(playerHitbox, map.nextActTrigger)) {
                            nearOrionTrigger = true;
                            if (IsKeyPressed(KEY_E)) {
                                nearOrionTrigger = false;
                                SetupAct(2); // Load Act 3 map NOW so player is already there
                                LoadDialogueFile("../assets/dialogue/act3.txt");
                                return DIALOGUE;
                            }
                        }
                    }
                }
            } else if (currentAct == 2) {
                // ── Act 3 Interactables (Image viewers & RGB Puzzle) ─────────
                nearbyImageIndex = -1;
                rgbNearby = false;
                
                Rectangle playerHitbox = {
                    player.position.x, player.position.y,
                    player.frameRec.width * player.scale, player.frameRec.height * player.scale
                };

                // Check image viewers proximity
                for (int i = 0; i < 2; i++) {
                    if (imageMapIndex[i] >= 0) {
                        if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[imageMapIndex[i]].bounds)) {
                            nearbyImageIndex = i;
                            if (IsKeyPressed(KEY_E)) {
                                activeImageIndex = i;
                            }
                            break;
                        }
                    }
                }
                
                // Check CodePuzzle proximity
                if (codePuzzleMapIndex >= 0 && !codePuzzle.solved && nearbyImageIndex < 0) {
                    if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[codePuzzleMapIndex].bounds)) {
                        codePuzzleNearby = true;
                        if (IsKeyPressed(KEY_E)) {
                            OpenCodePuzzle(&codePuzzle);
                        }
                    }
                }
                
                // Check rgb puzzle proximity
                if (rgbMapIndex >= 0 && !rgbPuzzle.solved && nearbyImageIndex < 0) {
                    if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[rgbMapIndex].bounds)) {
                        rgbNearby = true;
                        if (IsKeyPressed(KEY_E)) {
                            OpenRGBPuzzle(&rgbPuzzle);
                        }
                    }
                }
                
                // ── Act 3 Exit Gate (requires both keycards) ─────
                nearAct3Exit = false;
                if (map.hasNextActTrigger) {
                    if (CheckCollisionRecs(playerHitbox, map.nextActTrigger)) {
                        nearAct3Exit = true;
                        if (IsKeyPressed(KEY_E)) {
                            if (hasKeycardA && hasKeycardB) {
                                nearAct3Exit = false;
                                // Wait for act 4 or transition logic
                                printf("Both keycards acquired. Ready to transition!\n");
                            } else {
                                // Missing keycards
                                pickupTimer = 3.0f;
                                pickupText = "Requires Keycard A and Keycard B";
                            }
                        }
                    }
                }
            }
        }

    Vector2 playerCentre = {
        player.position.x + (player.frameRec.width  * player.scale) * 0.5f,
        player.position.y + (player.frameRec.height * player.scale) * 0.5f,
    };

    // ── Enemies ───────────────────────────────────────────────────────────────
    for (int i = 0; i < enemyCount; i++) {
        bool shakeThisFrame = false;
        UpdateEnemy(&enemies[i], playerCentre, dt, &shakeThisFrame,
                    map.collisionRecs, map.collisionCount,
                    map.navNodes, map.navNodeCount);
        if (shakeThisFrame) TriggerShake(5.0f, 0.15f);

        if (enemies[i].state != ENEMY_DEAD) {
            Rectangle enemyBounds = {
                enemies[i].position.x,
                enemies[i].position.y,
                (float)enemies[i].frameWidth * enemies[i].scale,
                (float)enemies[i].frameHeight * enemies[i].scale
            };

            // ── Bullet hits Enemy ─────────────────────────────────────────────
            for (int b = 0; b < MAX_BULLETS; b++) {
                if (playerWeapon.bullets[b].active) {
                    Vector2 bPos = playerWeapon.bullets[b].position;
                    if (CheckCollisionCircleRec(bPos, playerWeapon.bullets[b].radius, enemyBounds)) {
                        // Bullet hits enemy
                        playerWeapon.bullets[b].active = false;
                        if (currentAct <= 2) {
                            // Act 1 to 3: stun only — no damage until Level 3 map!
                            StunEnemy(&enemies[i]);
                            TriggerShake(2.0f, 0.08f);
                        } else {
                            // Normal combat: full damage
                            DamageEnemy(&enemies[i], 34.0f);
                            TriggerShake(4.0f, 0.10f);
                        }
                    }
                }
            }

            // ── Enemy hits Player ─────────────────────────────────────────────
            if (player.hitInvincibleTimer <= 0.0f && !player.playerInvincible) {
                Rectangle playerBounds = {
                    player.position.x,
                    player.position.y,
                    player.frameRec.width * player.scale,
                    player.frameRec.height * player.scale
                };

                if (CheckCollisionRecs(playerBounds, enemyBounds)) {
                    // Deal damage to player
                    player.health -= 50.0f;
                    if (player.health < 0.0f) player.health = 0.0f;
                    player.hitInvincibleTimer = 1.0f; // 1 second of i-frames
                    TriggerShake(8.0f, 0.20f);
                }
            }
        }
    }

    // ── Weapons / bullets ─────────────────────────────────────────────────────
    UpdateWeapon(&playerWeapon, dt, map.collisionRecs, map.collisionCount);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 startPos = { playerCentre.x, playerCentre.y };
            ShootWeapon(&playerWeapon, startPos, mouseWorldPos);
        }
    } // End Core Gameplay Logic

    // ── Player death check ───────────────────────────────────────────────────────
    if (!isPlayerDead && !isGameOver && player.health <= 0.0f) {
        isPlayerDead   = true;
        deathTimer     = 3.0f;
        deathFadeAlpha = 0.0f;
        playerLives--;
    }

    // ── Death overlay countdown ───────────────────────────────────────────────────
    if (isPlayerDead) {
        deathFadeAlpha += dt * 1.5f;
        if (deathFadeAlpha > 1.0f) deathFadeAlpha = 1.0f;

        deathTimer -= dt;
        if (deathTimer <= 0.0f) {
            if (playerLives <= 0) {
                isGameOver   = true;
                isPlayerDead = false;
            } else {
                // Respawn at chapter start
                isPlayerDead = false;
                player.position = chapterSpawnPos;
                player.health   = player.maxHealth;
                player.hitInvincibleTimer = 1.5f;

                // Reset all enemies
                for (int i = 0; i < enemyCount; i++) {
                    enemies[i].health = enemies[i].maxHealth;
                    enemies[i].state  = ENEMY_PATROL;
                    enemies[i].position = enemies[i].waypoints[0];
                    enemies[i].waypointIndex = 0;
                    enemies[i].navPathActive = false;
                }

                // Reset generator puzzle progress
                for (int i = 0; i < genCount; i++) {
                    genSolved[i] = false;
                    InitPuzzle(&genPuzzles[i]); // Re-shuffle wires
                }
                gensSolvedCount = 0;
                nearbyGenIndex = -1;
                activeGenIndex = -1;
                nearOrionTrigger = false;
            }
        }
        return GAMEPLAY;
    }

    // ── Game Over screen — wait for RESTART button ───────────────────────────
    if (isGameOver) {
        Vector2 mousePoint = GetVirtualMouse();
        float centerX = VIRTUAL_WIDTH / 2.0f;
        float centerY = VIRTUAL_HEIGHT / 2.0f;
        Rectangle restartBtn = { centerX - 120, centerY + 40, 240, 55 };

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(mousePoint, restartBtn)) {
            isGameOver     = false;
            playerLives    = STARTING_LIVES;
            deathFadeAlpha = 0.0f;

            if (currentAct >= 1) {
                // Always respawn at Act 1 (level 1.json) checkpoint
                pendingAct = 1;
            } else {
                // Act 0 restart (stays in lab)
                player.position = chapterSpawnPos;
                player.health   = player.maxHealth;
                player.hitInvincibleTimer = 1.5f;

                for (int i = 0; i < enemyCount; i++) {
                    enemies[i].health = enemies[i].maxHealth;
                    enemies[i].state  = ENEMY_PATROL;
                    enemies[i].position = enemies[i].waypoints[0];
                    enemies[i].waypointIndex = 0;
                    enemies[i].navPathActive = false;
                }
            }
        }
        return GAMEPLAY;
    }

    // ── Timers ────────────────────────────────────────────────────────────────
    if (shakeTimer > 0.0f) shakeTimer -= dt;
    if (pickupTimer > 0.0f) pickupTimer -= dt;

    return GAMEPLAY;
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawScreenGameplay
// ─────────────────────────────────────────────────────────────────────────────
void DrawScreenGameplay(void)
{
    // Apply camera shake as a random offset that decays over time
    Camera2D shakeCam = camera;
    if (shakeTimer > 0.0f) {
        float strength = shakeMagnitude * (shakeTimer / 0.15f); // Ease out
        if (strength > shakeMagnitude) strength = shakeMagnitude;
        shakeCam.offset.x += (float)GetRandomValue(-(int)strength, (int)strength);
        shakeCam.offset.y += (float)GetRandomValue(-(int)strength, (int)strength);
    }

    BeginMode2D(shakeCam);
        DrawTiledMap(&map);
        DrawWeapon(&playerWeapon);
        for (int i = 0; i < enemyCount; i++) DrawEnemy(&enemies[i]);
        DrawCharacter(&player);

        // ── World-space green checkmark on solved generators ──────────────────
        if (currentAct == 1) {
            for (int i = 0; i < genCount; i++) {
                if (genSolved[i]) {
                    int mi = genMapIndex[i];
                    Rectangle b = map.puzzleObjects[mi].bounds;
                    float cx = b.x + b.width / 2.0f;
                    float cy = b.y + b.height / 2.0f;
                    // Green checkmark
                    DrawLineEx((Vector2){cx - 8, cy},     (Vector2){cx - 2, cy + 8}, 3, GREEN);
                    DrawLineEx((Vector2){cx - 2, cy + 8}, (Vector2){cx + 10, cy - 6}, 3, GREEN);
                }
            }
        }
        // ── World-space "Press E" prompts above targets ───────────────────────────
        {
            float targetCenterX = 0.0f;
            float targetTopY = 0.0f;
            const char* prompt = NULL;

            // Character prompt
            if (nearbyCharacterIndex >= 0 && !isInGameDialogue) {
                prompt = "Press \"E\" to talk";
                targetCenterX = map.characterObjects[nearbyCharacterIndex].bounds.x + map.characterObjects[nearbyCharacterIndex].bounds.width / 2.0f;
                targetTopY = map.characterObjects[nearbyCharacterIndex].bounds.y - 5.0f;
            }
            // Generator prompt
            else if (nearbyGenIndex >= 0 && activeGenIndex < 0 && !genSolved[nearbyGenIndex]) {
                prompt = "Press \"E\" to interact";
                int mi = genMapIndex[nearbyGenIndex];
                targetCenterX = map.puzzleObjects[mi].bounds.x + map.puzzleObjects[mi].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[mi].bounds.y - 5.0f;
            }
            // ORION activation prompt
            else if (nearOrionTrigger && nearbyGenIndex < 0 && activeGenIndex < 0) {
                prompt = "Press \"E\" to activate ORION";
                targetCenterX = map.nextActTrigger.x + map.nextActTrigger.width / 2.0f;
                targetTopY = map.nextActTrigger.y - 5.0f;
            }
            // Act 3 Image viewer prompt
            else if (nearbyImageIndex >= 0 && activeImageIndex < 0) {
                prompt = "Press \"E\" to read";
                int mi = imageMapIndex[nearbyImageIndex];
                targetCenterX = map.puzzleObjects[mi].bounds.x + map.puzzleObjects[mi].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[mi].bounds.y - 5.0f;
            }
            // Act 3 RGB Puzzle prompt
            else if (rgbNearby && !rgbPuzzle.active && !rgbPuzzle.solved) {
                prompt = "Press \"E\" to interact";
                targetCenterX = map.puzzleObjects[rgbMapIndex].bounds.x + map.puzzleObjects[rgbMapIndex].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[rgbMapIndex].bounds.y - 5.0f;
            }
            // Act 3 Code Puzzle prompt
            else if (codePuzzleNearby && !codePuzzle.active && !codePuzzle.solved) {
                prompt = "Press \"E\" to interact";
                targetCenterX = map.puzzleObjects[codePuzzleMapIndex].bounds.x + map.puzzleObjects[codePuzzleMapIndex].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[codePuzzleMapIndex].bounds.y - 5.0f;
            }
            // Act 3 Exit prompt
            else if (nearAct3Exit && nearbyImageIndex < 0 && !rgbNearby) {
                prompt = "Press \"E\" to proceed";
                targetCenterX = map.nextActTrigger.x + map.nextActTrigger.width / 2.0f;
                targetTopY = map.nextActTrigger.y - 5.0f;
            }

            if (prompt) {
                int pw = MeasureText(prompt, 10);
                int tx = (int)(targetCenterX - pw / 2);
                int ty = (int)(targetTopY - 12);
                
                // Draw textbox background
                int paddingX = 4;
                int paddingY = 2;
                DrawRectangle(tx - paddingX, ty - paddingY, pw + paddingX * 2, 10 + paddingY * 2, Fade(BLACK, 0.75f));
                DrawRectangleLines(tx - paddingX, ty - paddingY, pw + paddingX * 2, 10 + paddingY * 2, DARKGRAY);

                // Text fill on top
                DrawText(prompt, tx, ty, 10, YELLOW);
            }
        }
    EndMode2D();

    // ── HUD (screen-space, no camera transform) ───────────────────────────────
    DrawCharacterHUD(&player, player.sprite);
    
    int textYpos = 20;

    // Objective Bar
    if (displayObjectives) {
        const char* objTexts[4] = {0};
        bool objStates[4] = {0};
        int numObjectives = 0;

        if (currentAct == 0) {
            objTexts[0] = "Move (WASD / Arrows)";
            objTexts[1] = "Dash (SHIFT)";
            objTexts[2] = "Shoot (LMB)";
            objTexts[3] = "Evacuate the laboratory";
            objStates[0] = objMoveDone; objStates[1] = objDashDone; objStates[2] = objShootDone; objStates[3] = objEvacuateDone;
            numObjectives = 4;
        } else if (currentAct == 1) {
            objTexts[0] = TextFormat("Activate generators (%d/%d)", gensSolvedCount, genCount);
            objStates[0] = (gensSolvedCount >= genCount && genCount > 0);
            numObjectives = 1;
        } else if (currentAct == 2) {
            int cards = (hasKeycardA ? 1 : 0) + (hasKeycardB ? 1 : 0);
            objTexts[0] = TextFormat("Collect keycards (%d/2)", cards);
            objStates[0] = (cards == 2);
            numObjectives = 1;
        }

        if (numObjectives > 0) {
            int maxTextW = MeasureText("Objectives:", 22);
            for (int i = 0; i < numObjectives; i++) {
                int w = MeasureText(objTexts[i], 20);
                if (w > maxTextW) maxTextW = w;
            }

            Rectangle barRec = { 20, 20, (float)(maxTextW + 70), 40 + (numObjectives * 30) };
            
            DrawRectangleRounded(barRec, 0.2f, 10, Fade(BLACK, 0.7f));
            DrawRectangleRoundedLines(barRec, 0.2f, 10, DARKGRAY);

            DrawText("Objectives:", (int)barRec.x + 15, (int)barRec.y + 10, 22, LIGHTGRAY);

            for (int i = 0; i < numObjectives; i++) {
                int y = (int)barRec.y + 40 + (i * 30);
                Color textColor = objStates[i] ? GRAY : RAYWHITE;
                DrawText(objTexts[i], (int)barRec.x + 15, y, 20, textColor);
                
                Rectangle box = { barRec.x + maxTextW + 30, y, 20, 20 };
                DrawRectangleLinesEx(box, 2, objStates[i] ? DARKGRAY : GRAY);
                
                if (objStates[i]) {
                    DrawLineEx((Vector2){box.x + 4, box.y + 10}, (Vector2){box.x + 8, box.y + 16}, 3, GREEN);
                    DrawLineEx((Vector2){box.x + 8, box.y + 16}, (Vector2){box.x + 18, box.y + 4}, 3, GREEN);
                }
            }
            
            textYpos = 20 + 40 + (numObjectives * 30) + 15;
        }
    }

    DrawText("Ignore enemy visual cone, they can't see through walls!!", 20, textYpos, 20, RAYWHITE);

    // ("Press E" prompts are now drawn in world-space above the player head)

    // ── In-Game Dialogue Overlay ──────────────────────────────────────────────
    if (isInGameDialogue) {
        DrawScreenDialogue(); // Fills screen perfectly due to native transparent backgrounds
    }

    // ── Puzzle overlay (on top of everything) ─────────────────────────────────
    if (activeGenIndex >= 0) {
        DrawPuzzle(&genPuzzles[activeGenIndex]);
    }
    
    if (rgbPuzzle.active) {
        DrawRGBPuzzle(&rgbPuzzle);
    }
    
    if (activeImageIndex >= 0) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.85f));
        Texture2D tex = (activeImageIndex == 0) ? imgCode : imgRule;
        if (tex.id != 0) {
            float scale = fminf((float)VIRTUAL_WIDTH * 0.9f / tex.width, (float)VIRTUAL_HEIGHT * 0.9f / tex.height);
            Vector2 pos = { (VIRTUAL_WIDTH - tex.width * scale) / 2.0f, (VIRTUAL_HEIGHT - tex.height * scale) / 2.0f };
            DrawTextureEx(tex, pos, 0.0f, scale, WHITE);
            
            const char* hint = "Press 'E' to close";
            int hw = MeasureText(hint, 20);
            DrawText(hint, (VIRTUAL_WIDTH - hw)/2, VIRTUAL_HEIGHT - 40, 20, GRAY);
        }
    }

    if (codePuzzle.active) {
        DrawCodePuzzle(&codePuzzle);
    }

    // Draw pickup notification popups
    if (pickupTimer > 0.0f && pickupText[0] != '\0') {
        int pw = MeasureText(pickupText, 30);
        int px = (VIRTUAL_WIDTH - pw) / 2;
        int py = 80;
        
        float alpha = (pickupTimer < 1.0f) ? pickupTimer : 1.0f;
        Color bg = Fade(BLACK, 0.7f * alpha);
        Color fg = Fade(YELLOW, alpha);
        
        DrawRectangle(px - 20, py - 10, pw + 40, 50, bg);
        DrawRectangleLines(px - 20, py - 10, pw + 40, 50, Fade(DARKGRAY, alpha));
        DrawText(pickupText, px, py, 30, fg);
    }

    // Lives counter in the top right
    const char* hudLivesText = TextFormat("Lives: %d", playerLives);
    int hudLivesW = MeasureText(hudLivesText, 20);
    DrawText(hudLivesText, VIRTUAL_WIDTH - hudLivesW - 20, 15, 20, MAROON);

    if (isPaused) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.6f));
        DrawText("PAUSED", VIRTUAL_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, VIRTUAL_HEIGHT / 2 - 150, 40, RAYWHITE);
        
        float centerX = VIRTUAL_WIDTH / 2.0f;
        float centerY = VIRTUAL_HEIGHT / 2.0f;
        Vector2 mousePoint = GetVirtualMouse();

        Rectangle saveBtn     = { centerX - 100, centerY - 80, 200, 50 };
        Rectangle settingsBtn = { centerX - 100, centerY - 10, 200, 50 };
        Rectangle exitBtn     = { centerX - 100, centerY + 60, 200, 50 };

        // Save Button
        DrawRectangleRec(saveBtn, CheckCollisionPointRec(mousePoint, saveBtn) ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(saveBtn, 2, DARKGRAY);
        DrawText("SAVE GAME", (int)centerX - MeasureText("SAVE GAME", 20) / 2, (int)(saveBtn.y + 15), 20, DARKGRAY);

        // Settings Button
        DrawRectangleRec(settingsBtn, CheckCollisionPointRec(mousePoint, settingsBtn) ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(settingsBtn, 2, DARKGRAY);
        DrawText("SETTINGS", (int)centerX - MeasureText("SETTINGS", 20) / 2, (int)(settingsBtn.y + 15), 20, DARKGRAY);

        // Exit Button
        DrawRectangleRec(exitBtn, CheckCollisionPointRec(mousePoint, exitBtn) ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(exitBtn, 2, DARKGRAY);
        DrawText("EXIT TO MENU", (int)centerX - MeasureText("EXIT TO MENU", 20) / 2, (int)(exitBtn.y + 15), 20, DARKGRAY);
    }

    // ── Death overlay ─────────────────────────────────────────────────────────────
    if (isPlayerDead) {
        // Dark red fade-in overlay
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                      Fade((Color){40, 0, 0, 255}, deathFadeAlpha * 0.85f));

        // "YOU DIED" text
        const char* deathText = "YOU DIED";
        int deathFontSize = 60;
        int deathTextW = MeasureText(deathText, deathFontSize);
        Color textColor = Fade(RED, deathFadeAlpha);
        DrawText(deathText, VIRTUAL_WIDTH / 2 - deathTextW / 2,
                 VIRTUAL_HEIGHT / 2 - 60, deathFontSize, textColor);

        // Lives remaining
        if (playerLives > 0) {
            const char* livesText = TextFormat("Lives remaining: %d", playerLives);
            int livesW = MeasureText(livesText, 24);
            DrawText(livesText, VIRTUAL_WIDTH / 2 - livesW / 2,
                     VIRTUAL_HEIGHT / 2 + 20, 24, Fade(RAYWHITE, deathFadeAlpha));
        } else {
            const char* noLives = "No lives remaining...";
            int nlW = MeasureText(noLives, 24);
            DrawText(noLives, VIRTUAL_WIDTH / 2 - nlW / 2,
                     VIRTUAL_HEIGHT / 2 + 20, 24, Fade(GRAY, deathFadeAlpha));
        }
    }

    // ── Game Over screen ─────────────────────────────────────────────────────────
    if (isGameOver) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.9f));

        float centerX = VIRTUAL_WIDTH / 2.0f;
        float centerY = VIRTUAL_HEIGHT / 2.0f;

        // "GAME OVER" heading
        const char* goText = "GAME OVER";
        int goFontSize = 60;
        int goW = MeasureText(goText, goFontSize);
        DrawText(goText, (int)(centerX) - goW / 2, (int)(centerY) - 80, goFontSize, RED);

        // Sub-text
        const char* subText = "All lives lost";
        int subW = MeasureText(subText, 20);
        DrawText(subText, (int)(centerX) - subW / 2, (int)(centerY) - 10, 20, GRAY);

        // RESTART button
        Vector2 mousePoint = GetVirtualMouse();
        Rectangle restartBtn = { centerX - 120, centerY + 40, 240, 55 };
        bool hovered = CheckCollisionPointRec(mousePoint, restartBtn);
        DrawRectangleRec(restartBtn, hovered ? (Color){200, 50, 50, 255} : (Color){150, 30, 30, 255});
        DrawRectangleLinesEx(restartBtn, 2, RED);
        const char* btnText = "RESTART";
        int btnW = MeasureText(btnText, 28);
        DrawText(btnText, (int)(centerX) - btnW / 2, (int)(restartBtn.y + 14), 28, RAYWHITE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UnloadScreenGameplay
// ─────────────────────────────────────────────────────────────────────────────
void UnloadScreenGameplay(void)
{
    UnloadTiledMap(&map);
    UnloadCharacter(&player);
    for (int i = 0; i < enemyCount; i++) UnloadEnemy(&enemies[i]);

    // Unload shared enemy textures
    if (enemySpriteR.id != 0) UnloadTexture(enemySpriteR);
    if (enemySpriteL.id != 0) UnloadTexture(enemySpriteL);
    
    // Unload puzzle assets
    if (imgCode.id != 0) UnloadTexture(imgCode);
    if (codePuzzle.bgTexture.id != 0) {
        UnloadTexture(codePuzzle.bgTexture);
        codePuzzle.bgTexture = (Texture2D){0};
    }
    if (imgRule.id != 0) UnloadTexture(imgRule);
    if (rgbPuzzle.textures[0].id != 0) UnloadTexture(rgbPuzzle.textures[0]);
    if (rgbPuzzle.textures[1].id != 0) UnloadTexture(rgbPuzzle.textures[1]);
    if (rgbPuzzle.textures[2].id != 0) UnloadTexture(rgbPuzzle.textures[2]);
    if (sfxButton.frameCount > 0) UnloadSound(sfxButton);
    if (sfxFail.frameCount > 0) UnloadSound(sfxFail);
}

// ─────────────────────────────────────────────────────────────────────────────
// GetGameplaySaveData — snapshot the live game state into a SaveData struct
// ─────────────────────────────────────────────────────────────────────────────
void GetGameplaySaveData(SaveData* data)
{
    data->playerX       = player.position.x;
    data->playerY       = player.position.y;
    data->playerHealth  = player.health;
    data->playerStamina = player.stamina;

    // Progression
    data->lives          = playerLives;
    data->currentAct     = currentAct;
    data->currentChapter = currentChapter;
    
    // Inventory
    data->hasKeycardA    = hasKeycardA;
    data->hasKeycardB    = hasKeycardB;

    data->enemyCount = enemyCount;
    for (int i = 0; i < enemyCount && i < SAVE_MAX_ENEMIES; i++) {
        data->enemyX[i]      = enemies[i].position.x;
        data->enemyY[i]      = enemies[i].position.y;
        data->enemyHealth[i] = enemies[i].health;
        data->enemyDead[i]   = (enemies[i].state == ENEMY_DEAD);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// RestoreGameplayFromSave — write SaveData values back into the live structs
// ─────────────────────────────────────────────────────────────────────────────
void RestoreGameplayFromSave(const SaveData* data)
{
    // Progression
    playerLives    = data->lives;
    if (playerLives <= 0) playerLives = STARTING_LIVES; // Fallback for older saves
    
    currentChapter = data->currentChapter;

    // If the saved act differs from the currently loaded one,
    // reload the correct map, enemies, and textures.
    if (data->currentAct != currentAct) {
        SetupAct(data->currentAct);
    }

    // Override the default spawn position with the saved position
    player.position.x = data->playerX;
    player.position.y = data->playerY;
    player.health     = data->playerHealth;
    player.stamina    = data->playerStamina;
    
    // Inventory
    hasKeycardA = data->hasKeycardA;
    hasKeycardB = data->hasKeycardB;

    // Reset death states on load
    isPlayerDead   = false;
    isGameOver     = false;
    deathTimer     = 0.0f;
    deathFadeAlpha = 0.0f;

    // Restore enemy states from save
    for (int i = 0; i < data->enemyCount && i < enemyCount; i++) {
        enemies[i].position.x = data->enemyX[i];
        enemies[i].position.y = data->enemyY[i];
        enemies[i].health     = data->enemyHealth[i];
        if (data->enemyDead[i]) {
            enemies[i].state  = ENEMY_DEAD;
            enemies[i].health = 0.0f;
        }
    }
}

#include "raylib.h" //Code written by: Christopher 沈佳豪
#include "raymath.h"
#include "screen_gameplay.h"
#include "character.h"
#include "audio.h"
#include "weapon.h"
#include "tiled_map.h"
#include "setting.h"
#include "enemy.h"
#include "boss.h"
#include "savedata.h"
#include "screen_dialogue.h"
#include "puzzle.h"
#include <string.h>
#include <stdio.h>

static Character player;
static MapData   map;
static Weapon    playerWeapon;
static Camera2D  camera = { 0 };

// Shared enemy sprite textures — one pair per EnemyType, loaded once per act
static Texture2D enemySpriteR[ENEMY_TYPE_COUNT];
static Texture2D enemySpriteL[ENEMY_TYPE_COUNT];

static bool isPaused = false;

static Boss currentBoss;
static Texture2D bossSprite;
static Texture2D meteorSprite;

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
// Level 3 Puzzle System (Act 4)
// ─────────────────────────────────────────────────────────────────────────────
static Texture2D imgNote2;
static int       note2MapIndex = -1;
static bool      note2Active = false;

static int       sequenceBtnIndices[4] = {-1, -1, -1, -1}; // 0=RED, 1=YELLOW, 2=BLUE, 3=GREEN
static int       nearbySequenceBtn = -1;
static int       sequenceProgress = 0;
static bool      act4DoorUnlocked = false;
static bool      nearAct4Exit = false;

// ─────────────────────────────────────────────────────────────────────────────
// Keycard & Popup System (Act 3)
// ─────────────────────────────────────────────────────────────────────────────
static bool hasKeycardA = false;
static bool hasKeycardB = false;
static float pickupTimer = 0.0f;
static const char* pickupText = "";
static bool nearAct3Exit = false;

// ─────────────────────────────────────────────────────────────────────────────
// Hazard Password Puzzle & Hazmat Suit (Act 5 / Level 4)
// ─────────────────────────────────────────────────────────────────────────────
static HazardPuzzle hazardPuzzle;
static int          hazardMapIndex = -1;
static bool         hazardNearby = false;
static bool         hasHazmatSuit = false;

// ─────────────────────────────────────────────────────────────────────────────
// In-Game Dialogue System
// ─────────────────────────────────────────────────────────────────────────────
static bool isInGameDialogue = false;
static int nearbyCharacterIndex = -1;
static bool forceDialogueTriggered = false;

// ─────────────────────────────────────────────────────────────────────────────
// Enemy list — increase capacity and call InitEnemy multiple times to add more.
//
//   Placing an enemy anywhere:
//     Edit 'patrolPoints' and pass them to InitEnemy with the desired spawnPos.
//
//   Giving an enemy a sprite:
//     Replace NULL with a path string, e.g. "../assets/enemy/slime_abberant_r.png",
//     and set the correct frameWidth / frameHeight for your sprite sheet.
// ─────────────────────────────────────────────────────────────────────────────
#define MAX_ENEMIES 50
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
//   The first character of the name determines the sprite: s/r/w/m.
// ─────────────────────────────────────────────────────────────────────────────
static void SpawnEnemiesFromMap(void)
{
    bool processed[MAX_ENEMY_SPAWNS] = {0};

    printf("[Enemy] Total spawn points from map: %d\n", map.enemySpawnCount);
    for (int d = 0; d < map.enemySpawnCount; d++) {
        printf("[Enemy]   spawn[%d] pos=(%.0f, %.0f) group='%s' type=%d\n",
               d, map.enemySpawns[d].position.x, map.enemySpawns[d].position.y,
               map.enemySpawns[d].group, map.enemySpawns[d].type);
    }

    for (int i = 0; i < map.enemySpawnCount; i++) {
        if (processed[i]) continue;

        EnemySpawnPoint* sp = &map.enemySpawns[i];

        // Stationary = unnamed OR type-only name (no underscore, e.g. "s", "r", "w", "m")
        // Patrol     = name contains underscore (e.g. "s_1", "r_1")
        bool isStationary = (sp->group[0] == '\0') || (strchr(sp->group, '_') == NULL);

        if (isStationary) {
            // ── Stationary enemy: single waypoint ──────────────────────────
            if (enemyCount < MAX_ENEMIES) {
                EnemyType t = sp->type;
                EnemyBehavior beh = (t == ENEMY_TYPE_MUSHROOM) ? BEHAVIOR_MUSHROOM : BEHAVIOR_PATROL;
                int fw = 32, fh = 32;
                float sc = 0.67f;
                if (t == ENEMY_TYPE_MUSHROOM) {
                    // Use full texture size for mushroom (bigger sprite)
                    fw = enemySpriteR[t].width;
                    fh = enemySpriteR[t].height;
                    sc = 1.0f;
                }
                Vector2 pts[] = { sp->position };
                InitEnemy(&enemies[enemyCount++], sp->position, pts, 1,
                    enemySpriteR[t], enemySpriteL[t], fw, fh, sc, beh);
                printf("[Enemy] Spawned STATIONARY enemy #%d type=%d at (%.0f, %.0f)\n",
                       enemyCount, t, sp->position.x, sp->position.y);
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
                EnemyType t = sp->type;
                EnemyBehavior beh = (t == ENEMY_TYPE_MUSHROOM) ? BEHAVIOR_MUSHROOM : BEHAVIOR_PATROL;
                int fw = 32, fh = 32;
                float sc = 0.67f;
                if (t == ENEMY_TYPE_MUSHROOM) {
                    fw = enemySpriteR[t].width;
                    fh = enemySpriteR[t].height;
                    sc = 1.0f;
                }
                InitEnemy(&enemies[enemyCount++], waypoints[0], waypoints, wpCount,
                    enemySpriteR[t], enemySpriteL[t], fw, fh, sc, beh);
                printf("[Enemy] Spawned PATROL enemy #%d type=%d group='%s' with %d waypoints\n",
                       enemyCount, t, sp->group, wpCount);
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

    // Unload previous shared enemy textures (all types)
    for (int t = 0; t < ENEMY_TYPE_COUNT; t++) {
        if (enemySpriteR[t].id != 0) { UnloadTexture(enemySpriteR[t]); enemySpriteR[t] = (Texture2D){0}; }
        if (enemySpriteL[t].id != 0) { UnloadTexture(enemySpriteL[t]); enemySpriteL[t] = (Texture2D){0}; }
    }

    currentAct = act;

    switch (act) {
        case 0: // ACT 1 — Prologue (stun-only bullets)
            LoadTiledMap(&map, "../assets/map/trial.json");
            chapterSpawnPos = (Vector2){ 600, 867 };
            enemySpriteR[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_r.png");
            enemySpriteL[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_l.png");
            enemySpriteR[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_r.png");
            enemySpriteL[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_l.png");
            enemySpriteR[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_r.png");
            enemySpriteL[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_l.png");
            enemySpriteR[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_r.png");
            enemySpriteL[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_l.png");
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
            enemySpriteR[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_r.png");
            enemySpriteL[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_l.png");
            enemySpriteR[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_r.png");
            enemySpriteL[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_l.png");
            enemySpriteR[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_r.png");
            enemySpriteL[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_l.png");
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
            enemySpriteR[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_r.png");
            enemySpriteL[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_l.png");
            enemySpriteR[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_r.png");
            enemySpriteL[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_l.png");
            enemySpriteR[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_r.png");
            enemySpriteL[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_l.png");
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

        case 3: // ACT 4 — Level 3 (full damage)
            LoadTiledMap(&map, "../assets/map/level 3.json");
            chapterSpawnPos = map.hasSpawnPoint ? map.spawnPoint : (Vector2){ 600, 867 };
            enemySpriteR[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_r.png");
            enemySpriteL[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_l.png");
            enemySpriteR[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_r.png");
            enemySpriteL[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_l.png");
            enemySpriteR[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_r.png");
            enemySpriteL[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_l.png");
            enemySpriteR[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_r.png");
            enemySpriteL[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_l.png");
            SpawnEnemiesFromMap();
            displayObjectives = true;

            // Setup Level 3 puzzle state
            note2MapIndex = -1;
            note2Active = false;
            for (int i = 0; i < 4; i++) sequenceBtnIndices[i] = -1;
            nearbySequenceBtn = -1;
            sequenceProgress = 0;
            act4DoorUnlocked = false;
            nearAct4Exit = false;

            for (int i = 0; i < map.puzzleObjectCount; i++) {
                if (strcmp(map.puzzleObjects[i].name, "note2") == 0) {
                    note2MapIndex = i;
                } else if (strcmp(map.puzzleObjects[i].name, "red") == 0) {
                    sequenceBtnIndices[0] = i;
                } else if (strcmp(map.puzzleObjects[i].name, "yellow") == 0) {
                    sequenceBtnIndices[1] = i;
                } else if (strcmp(map.puzzleObjects[i].name, "blue") == 0) {
                    sequenceBtnIndices[2] = i;
                } else if (strcmp(map.puzzleObjects[i].name, "green") == 0) {
                    sequenceBtnIndices[3] = i;
                }
            }
            break;

        case 4: // ACT 5 — Level 4
            LoadTiledMap(&map, "../assets/map/level 4.json");
            chapterSpawnPos = map.hasSpawnPoint ? map.spawnPoint : (Vector2){ 600, 867 };
            enemySpriteR[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_r.png");
            enemySpriteL[ENEMY_TYPE_SLIME]    = LoadTexture("../assets/enemy/slime_abberant_l.png");
            enemySpriteR[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_r.png");
            enemySpriteL[ENEMY_TYPE_WORKER]   = LoadTexture("../assets/enemy/worker_l.png");
            enemySpriteR[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_r.png");
            enemySpriteL[ENEMY_TYPE_RAT]      = LoadTexture("../assets/enemy/rat_l.png");
            enemySpriteR[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_r.png");
            enemySpriteL[ENEMY_TYPE_MUSHROOM] = LoadTexture("../assets/enemy/mushroom_l.png");
            SpawnEnemiesFromMap();
            displayObjectives = true;

            // Reset stale state from previous acts
            note2MapIndex = -1;
            note2Active = false;
            nearbyImageIndex = -1;
            activeImageIndex = -1;
            rgbNearby = false;
            codePuzzleNearby = false;
            nearbySequenceBtn = -1;
            nearAct3Exit = false;
            nearAct4Exit = false;

            // Scan for hazard puzzle object
            hazardMapIndex = -1;
            hazardNearby = false;
            for (int i = 0; i < map.puzzleObjectCount; i++) {
                if (strcmp(map.puzzleObjects[i].name, "hazmat") == 0) {
                    hazardMapIndex = i;
                }
            }
            break;

        case 5: // Boss map
            LoadTiledMap(&map, "../assets/map/boss.json");
            chapterSpawnPos = map.hasSpawnPoint ? map.spawnPoint : (Vector2){ 600, 867 };
            Vector2 bossPos = map.hasBossSpawnPoint ? map.bossSpawnPoint : (Vector2){ 1546, 1248 };
            SpawnEnemiesFromMap();
            displayObjectives = true;
            InitBoss(&currentBoss, bossPos, bossSprite, meteorSprite, 500.0f, 1000.0f);
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
    forceDialogueTriggered = false;
    hasHazmatSuit = false;

    // Inventory
    hasKeycardA = false;
    hasKeycardB = false;

    // Puzzle / interaction states
    isInGameDialogue = false;
    nearbyCharacterIndex = -1;
    nearbyImageIndex = -1;
    activeImageIndex = -1;
    note2MapIndex = -1;
    note2Active = false;
    rgbNearby = false;
    codePuzzleNearby = false;
    nearbySequenceBtn = -1;
    nearbyGenIndex = -1;
    activeGenIndex = -1;
    nearOrionTrigger = false;
    nearAct3Exit = false;
    nearAct4Exit = false;
    hazardMapIndex = -1;
    hazardNearby = false;
    pickupTimer = 0.0f;
    pickupText = "";

    // ── Load puzzle assets ────────────────────────────────────────────────────
    imgCode = LoadTexture("../assets/images/code.png");
    imgRule = LoadTexture("../assets/images/rule.png");
    imgNote2 = LoadTexture("../assets/images/code_2.png");
    Texture2D bg1 = LoadTexture("../assets/images/puzzle/rgb_1.png");
    Texture2D bg2 = LoadTexture("../assets/images/puzzle/rgb_2.png");
    Texture2D bg3 = LoadTexture("../assets/images/puzzle/rgb_3.png");
    InitRGBPuzzle(&rgbPuzzle, bg1, bg2, bg3);
    InitHazardPuzzle(&hazardPuzzle);
    sfxButton = LoadSound("../assets/sfx/button.wav");
    sfxFail = LoadSound("../assets/sfx/button_fail.wav");

    bossSprite = LoadTexture("../assets/enemy/boss.png");
    meteorSprite = LoadTexture("../assets/images/meteor.png");

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
    
    // Apply dynamic dialogue-driven camera shift
    Vector2 camShift = GetDialogueCameraOffset();
    camera.target.x += camShift.x;
    camera.target.y += camShift.y;

    Vector2 mouseWorldPos = GetScreenToWorld2D(GetVirtualMouse(), camera);

    // ── Core Gameplay Logic (Freeze during death) ─────────────────────────────
    if (!isPlayerDead && !isGameOver) {

        // ── In-Game Dialogue Active: freeze gameplay, process dialogue ────────
        if (isInGameDialogue) {
            GameScreen result = UpdateScreenDialogue(gameAudio);
            if (result == GAMEPLAY) { // Finished playing sequence
                isInGameDialogue = false;
                int choice = GetDialogueChoiceResult();
                if (currentAct == 4 && choice == 1) {
                    // Player chose "Yes" → transition to boss map
                    UnloadScreenDialogue();
                    SetupAct(5);
                    return GAMEPLAY;
                }
                UnloadScreenDialogue();
            }
            return GAMEPLAY; // Skip all other gameplay updates while discussing
        }

        // ── Code Puzzle Active: freeze gameplay ─────────────────────────────
        if (codePuzzle.active) {
            if (IsKeyPressed(KEY_E)) {
                codePuzzle.active = false;
            } else if (UpdateCodePuzzle(&codePuzzle, GetVirtualMouse(), sfxButton, sfxFail)) {
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

        // ── Note2 Viewer Active: freeze gameplay, wait for 'E' to close ───────
        if (note2Active) {
            if (IsKeyPressed(KEY_E)) {
                note2Active = false; // Close image view
            }
            return GAMEPLAY; // Skip all other gameplay updates while note is open
        }

        // ── RGB Puzzle Active: freeze gameplay, process rhythms ───────────────
        if (rgbPuzzle.active) {
            if (IsKeyPressed(KEY_E)) {
                rgbPuzzle.active = false;
            } else if (UpdateRGBPuzzle(&rgbPuzzle, GetVirtualMouse(), sfxButton, sfxFail)) {
                // Puzzle just solved!
                printf("[Puzzle] RGB Timing Game solved!\n");
                hasKeycardA = true;
                pickupTimer = 4.0f;
                pickupText = "- KEYCARD A ACQUIRED -";
            }
            return GAMEPLAY; // Skip all other gameplay updates while puzzle is open
        }

        // ── Hazard Puzzle Active: freeze gameplay, process keypad ─────────────
        if (hazardPuzzle.active) {
            if (IsKeyPressed(KEY_E)) {
                CloseHazardPuzzle(&hazardPuzzle);
            } else if (UpdateHazardPuzzle(&hazardPuzzle, GetVirtualMouse(), sfxButton, sfxFail)) {
                // Puzzle solved!
                printf("[Puzzle] Hazard password solved!\n");
                hasHazmatSuit = true;
                pickupTimer = 4.0f;
                pickupText = "- HAZMAT SUIT ACQUIRED -";
                // Trigger success dialogue
                LoadDialogueFile("../assets/dialogue/hazard_room.txt");
                isInGameDialogue = true;
            }
            return GAMEPLAY;
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
            
            // Force Dialogue trigger
            if (!forceDialogueTriggered) {
                for (int i = 0; i < map.forceDialogueObjectCount; i++) {
                    if (CheckCollisionRecs(playerHitbox, map.forceDialogueObjects[i].bounds)) {
                        forceDialogueTriggered = true;
                        char filepath[128];
                        snprintf(filepath, sizeof(filepath), "../assets/dialogue/character/%s.txt", map.forceDialogueObjects[i].name);
                        LoadDialogueFile(filepath);
                        isInGameDialogue = true;
                        break;
                    }
                }
            }
            
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
                                SetupAct(3); // Load Act 4 map NOW
                                LoadDialogueFile("../assets/dialogue/act4.txt");
                                return DIALOGUE;
                            } else {
                                // Missing keycards
                                pickupTimer = 3.0f;
                                pickupText = "Requires Keycard A and Keycard B";
                            }
                        }
                    }
                }
            } else if (currentAct == 3) {
                // ── Act 4 (Level 3) Interactables ─────────
                nearbySequenceBtn = -1;
                nearAct4Exit = false;

                Rectangle playerHitbox = {
                    player.position.x, player.position.y,
                    player.frameRec.width * player.scale, player.frameRec.height * player.scale
                };

                // Check note2 proximity
                if (note2MapIndex >= 0 && note2Active == false) {
                    if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[note2MapIndex].bounds)) {
                        if (IsKeyPressed(KEY_E)) {
                            note2Active = true;
                        }
                    }
                }

                // Check sequence buttons proximity
                if (!act4DoorUnlocked) {
                    for (int i = 0; i < 4; i++) {
                        if (sequenceBtnIndices[i] >= 0) {
                            if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[sequenceBtnIndices[i]].bounds)) {
                                nearbySequenceBtn = i;
                                if (IsKeyPressed(KEY_E)) {
                                    if (i == sequenceProgress) {
                                        sequenceProgress++;
                                        PlaySound(sfxButton);
                                        if (sequenceProgress >= 4) {
                                            act4DoorUnlocked = true;
                                            pickupTimer = 4.0f;
                                            pickupText = "DOOR UNLOCKED";
                                        }
                                    } else {
                                        sequenceProgress = 0;
                                        PlaySound(sfxFail);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }

                // ── Act 4 Exit Gate ─────
                if (map.hasNextActTrigger) {
                    if (CheckCollisionRecs(playerHitbox, map.nextActTrigger)) {
                        nearAct4Exit = true;
                        if (IsKeyPressed(KEY_E)) {
                            if (act4DoorUnlocked) {
                                nearAct4Exit = false;
                                SetupAct(4); // Load next map
                                LoadDialogueFile("../assets/dialogue/act5.txt");
                                return DIALOGUE;
                            } else {
                                pickupTimer = 3.0f;
                                pickupText = "Sequence Incomplete";
                            }
                        }
                    }
                }
            }
        } // <-- End of if (displayObjectives)

        // ── Act 5 (Level 4) Interactables (runs independently) ─────
        if (currentAct == 4) {
            hazardNearby = false;

            Rectangle playerHitbox = {
                player.position.x, player.position.y,
                player.frameRec.width * player.scale, player.frameRec.height * player.scale
            };

            // Check hazard puzzle proximity
            if (hazardMapIndex >= 0 && !hazardPuzzle.solved) {
                if (CheckCollisionRecs(playerHitbox, map.puzzleObjects[hazardMapIndex].bounds)) {
                    hazardNearby = true;
                    if (IsKeyPressed(KEY_E)) {
                        OpenHazardPuzzle(&hazardPuzzle);
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

            // ── Enemy hits Player (contact — skip for mushrooms) ─────────────
            if (enemies[i].behavior != BEHAVIOR_MUSHROOM &&
                player.hitInvincibleTimer <= 0.0f && !player.playerInvincible) {
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

            // ── Mushroom Poison Projectile Collision ──────────────────────────
            if (enemies[i].behavior == BEHAVIOR_MUSHROOM) {
                for (int p = 0; p < ENEMY_MAX_PROJECTILES; p++) {
                    EnemyProjectile* proj = &enemies[i].projectiles[p];
                    if (!proj->active || !proj->isSplattered) continue;
                    Rectangle playerBounds = {
                        player.position.x, player.position.y,
                        player.frameRec.width * player.scale,
                        player.frameRec.height * player.scale
                    };
                    if (CheckCollisionCircleRec(proj->position, proj->radius, playerBounds)) {
                        // Continuous DPS while overlapping
                        if (player.hitInvincibleTimer <= 0.0f) {
                            player.health -= proj->damagePerSec * dt;
                            if (player.health < 0.0f) player.health = 0.0f;
                        }
                    }
                }
            }
        }
    }

    // ── Boss Collision & Update ───────────────────────────────────────────────
    if (currentAct == 5) {
        bool playerHitByBoss = false;
        float bossDamage = 0.0f;
        UpdateBoss(&currentBoss, playerCentre, dt, map.collisionRecs, map.collisionCount, &playerHitByBoss, &bossDamage);
        
        if (playerHitByBoss && player.hitInvincibleTimer <= 0.0f && !player.playerInvincible) {
            player.health -= bossDamage;
            if (player.health < 0.0f) player.health = 0.0f;
            player.hitInvincibleTimer = 1.0f;
            TriggerShake(8.0f, 0.20f);
        }
    }

    // ── Weapons / bullets ─────────────────────────────────────────────────────
    UpdateWeapon(&playerWeapon, dt, map.collisionRecs, map.collisionCount);

    if (currentAct == 5 && currentBoss.state != BOSS_STATE_DEAD) {
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (playerWeapon.bullets[b].active) {
                Vector2 bPos = playerWeapon.bullets[b].position;
                float bRad = playerWeapon.bullets[b].radius;
                
                // Check boss body
                Rectangle bossBounds = {currentBoss.position.x, currentBoss.position.y, currentBoss.frameWidth * currentBoss.scale, currentBoss.frameHeight * currentBoss.scale};
                if (CheckCollisionCircleRec(bPos, bRad, bossBounds)) {
                    playerWeapon.bullets[b].active = false;
                    DamageBoss(&currentBoss, 34.0f); // Same as enemy damage
                    TriggerShake(4.0f, 0.10f);
                    continue;
                }
                
                // Check cores (only if in invincible state)
                if (currentBoss.state == BOSS_STATE_STAGE1_INVINCIBLE) {
                    for (int c = 0; c < BOSS_MAX_CORES; c++) {
                        if (currentBoss.cores[c].active) {
                            // Simple circle vs circle
                            float dist = Vector2Distance(bPos, currentBoss.cores[c].position);
                            if (dist < (bRad + 16.0f)) {
                                playerWeapon.bullets[b].active = false;
                                DamageBossCore(&currentBoss, c, 34.0f);
                                TriggerShake(2.0f, 0.08f);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

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

                if (currentAct == 5) {
                    Vector2 bossPos = map.hasBossSpawnPoint ? map.bossSpawnPoint : (Vector2){ 1546, 1248 };
                    InitBoss(&currentBoss, bossPos, bossSprite, meteorSprite, 500.0f, 1000.0f);
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
            forceDialogueTriggered = false;

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
        if (currentAct == 5) DrawBoss(&currentBoss);
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
            // Act 4 Note2 prompt
            else if (note2MapIndex >= 0 && !note2Active && CheckCollisionRecs((Rectangle){player.position.x, player.position.y, player.frameRec.width * player.scale, player.frameRec.height * player.scale}, map.puzzleObjects[note2MapIndex].bounds)) {
                prompt = "Press \"E\" to read";
                targetCenterX = map.puzzleObjects[note2MapIndex].bounds.x + map.puzzleObjects[note2MapIndex].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[note2MapIndex].bounds.y - 5.0f;
            }
            // Act 4 Sequence buttons prompt
            else if (nearbySequenceBtn >= 0 && !act4DoorUnlocked) {
                prompt = "Press \"E\" to interact";
                targetCenterX = map.puzzleObjects[sequenceBtnIndices[nearbySequenceBtn]].bounds.x + map.puzzleObjects[sequenceBtnIndices[nearbySequenceBtn]].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[sequenceBtnIndices[nearbySequenceBtn]].bounds.y - 5.0f;
            }
            // Act 4 Exit prompt
            else if (nearAct4Exit) {
                prompt = act4DoorUnlocked ? "Press \"E\" to proceed" : "Locked";
                targetCenterX = map.nextActTrigger.x + map.nextActTrigger.width / 2.0f;
                targetTopY = map.nextActTrigger.y - 5.0f;
            }
            // Hazard Puzzle prompt (Act 5 / Level 4)
            else if (hazardNearby && !hazardPuzzle.solved && !hazardPuzzle.active) {
                prompt = "Press \"E\" to interact";
                targetCenterX = map.puzzleObjects[hazardMapIndex].bounds.x + map.puzzleObjects[hazardMapIndex].bounds.width / 2.0f;
                targetTopY = map.puzzleObjects[hazardMapIndex].bounds.y - 5.0f;
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
    if (currentAct == 5) DrawBossUI(&currentBoss);
    
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
        } else if (currentAct == 3) {
            objTexts[0] = TextFormat("Activate the door (%d/4)", sequenceProgress);
            objStates[0] = act4DoorUnlocked;
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

    if (note2Active) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.85f));
        if (imgNote2.id != 0) {
            float scale = fminf((float)VIRTUAL_WIDTH * 0.9f / imgNote2.width, (float)VIRTUAL_HEIGHT * 0.9f / imgNote2.height);
            Vector2 pos = { (VIRTUAL_WIDTH - imgNote2.width * scale) / 2.0f, (VIRTUAL_HEIGHT - imgNote2.height * scale) / 2.0f };
            DrawTextureEx(imgNote2, pos, 0.0f, scale, WHITE);
            
            const char* hint = "Press 'E' to close";
            int hw = MeasureText(hint, 20);
            DrawText(hint, (VIRTUAL_WIDTH - hw)/2, VIRTUAL_HEIGHT - 40, 20, GRAY);
        }
    }

    if (codePuzzle.active) {
        DrawCodePuzzle(&codePuzzle);
    }

    if (hazardPuzzle.active) {
        DrawHazardPuzzle(&hazardPuzzle);
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

    // Unload shared enemy textures (all types)
    for (int t = 0; t < ENEMY_TYPE_COUNT; t++) {
        if (enemySpriteR[t].id != 0) UnloadTexture(enemySpriteR[t]);
        if (enemySpriteL[t].id != 0) UnloadTexture(enemySpriteL[t]);
    }
    
    if (bossSprite.id != 0) UnloadTexture(bossSprite);
    if (meteorSprite.id != 0) UnloadTexture(meteorSprite);
    
    // Unload puzzle assets
    if (imgCode.id != 0) UnloadTexture(imgCode);
    if (imgNote2.id != 0) UnloadTexture(imgNote2);
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
    data->hasHazmatSuit  = hasHazmatSuit;

    // Events
    data->forceDialogueTriggered = forceDialogueTriggered;

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
    hasHazmatSuit = data->hasHazmatSuit;

    // Events
    forceDialogueTriggered = data->forceDialogueTriggered;

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

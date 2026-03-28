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

static Character player;
static MapData   map;
static Weapon    playerWeapon;
static Camera2D  camera = { 0 };

static bool isPaused = false;

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
#define MAX_ENEMIES 10
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

// ─────────────────────────────────────────────────────────────────────────────
// Helper: trigger a camera shake
// ─────────────────────────────────────────────────────────────────────────────
static void TriggerShake(float magnitude, float duration) {
    shakeMagnitude = magnitude;
    shakeTimer     = duration;
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

    // ── Map ───────────────────────────────────────────────────────────────────
    LoadTiledMap(&map, "../assets/map/trial.json");

    // ── Weapon ────────────────────────────────────────────────────────────────
    InitWeapon(&playerWeapon);

    // ── Camera ────────────────────────────────────────────────────────────────
    camera.target   = player.position;
    camera.offset   = (Vector2){ VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 2.0f;

    // ── Enemies ───────────────────────────────────────────────────────────────
    // Each enemy has its own patrol route (up to ENEMY_MAX_WAYPOINTS stops).
    // To add a new enemy: copy the block below, change the waypoints and spawn
    // position, and increment enemyCount.
    //
    // To give it a sprite replace NULL with the path, e.g.:
    //   "../assets/character/goblin.png"
    // and supply the correct frameWidth / frameHeight / scale.

    enemyCount = 0;
    isPaused = false;

    // Reset lives & death state
    playerLives    = STARTING_LIVES;
    isPlayerDead   = false;
    deathTimer     = 0.0f;
    isGameOver     = false;
    deathFadeAlpha = 0.0f;
    chapterSpawnPos = (Vector2){ 600, 867 }; // Current chapter spawn
    currentAct     = 0;
    currentChapter = 0;

    // Helper macro to easily spawn an enemy that patrols between two points
    #define SPAWN_BASIC_ENEMY(x1, y1, x2, y2) \
        do { \
            if (enemyCount < MAX_ENEMIES) { \
                Vector2 pts[] = { { x1, y1 }, { x2, y2 } }; \
                InitEnemy(&enemies[enemyCount++], pts[0], pts, 2, "../assets/character/slime_abberant_r.png", "../assets/character/slime_abberant_l.png", 32, 32, 0.67f); \
            } \
        } while(0)
        SPAWN_BASIC_ENEMY(790,1480,790,1650);
        SPAWN_BASIC_ENEMY(1070,1480,1070,1650);

    #undef SPAWN_BASIC_ENEMY
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateScreenGameplay
// ─────────────────────────────────────────────────────────────────────────────
GameScreen UpdateScreenGameplay(Audio* gameAudio)
{
    float dt = GetFrameTime();

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
        
        // ── Player ────────────────────────────────────────────────────────────
        UpdateCharacter(&player, map.collisionRecs, map.collisionCount, mouseWorldPos, gameAudio);

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
                        Vector2 kbDir = Vector2Normalize(playerWeapon.bullets[b].velocity);
                        DamageEnemy(&enemies[i], 34.0f, kbDir, 400.0f);
                        TriggerShake(4.0f, 0.10f);
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
                    player.health -= 25.0f;
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
            player.position = chapterSpawnPos; // Back to act start logic would go here
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
        return GAMEPLAY;
    }

    // ── Screen shake decay ────────────────────────────────────────────────────
    if (shakeTimer > 0.0f) shakeTimer -= dt;

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
    EndMode2D();

    // ── HUD (screen-space, no camera transform) ───────────────────────────────
    DrawCharacterHUD(&player, player.sprite);
    DrawText("WASD / Arrows: Move  |  LMB: Shoot  |  ESC: Pause",
             10, 10, 14, DARKGRAY);

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
    player.position.x = data->playerX;
    player.position.y = data->playerY;
    player.health     = data->playerHealth;
    player.stamina    = data->playerStamina;

    // Progression
    playerLives    = data->lives;
    if (playerLives <= 0) playerLives = STARTING_LIVES; // Fallback for older saves
    
    currentAct     = data->currentAct;
    currentChapter = data->currentChapter;

    // Reset death states on load
    isPlayerDead   = false;
    isGameOver     = false;
    deathTimer     = 0.0f;
    deathFadeAlpha = 0.0f;

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

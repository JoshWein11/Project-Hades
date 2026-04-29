#include "boss.h"
#include "raymath.h"
#include <stdlib.h>

static void BossShoot(Boss* boss, Vector2 start, Vector2 dir, float speed) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!boss->weapon.bullets[i].active) {
            boss->weapon.bullets[i].active = true;
            boss->weapon.bullets[i].position = start;
            boss->weapon.bullets[i].velocity = dir;
            boss->weapon.bullets[i].speed = speed;
            boss->weapon.bullets[i].lifeTimer = 5.0f;
            break;
        }
    }
}

void InitBoss(Boss* boss, Vector2 spawnPos, Texture2D sprite, Texture2D meteorSprite, float baseMaxHp, float stage2MaxHp) {
    boss->position = spawnPos;
    boss->spawnPosition = spawnPos;
    boss->sprite = sprite;
    boss->meteorSprite = meteorSprite;
    
    // Force logical and collision size to exactly 64x64 (2x2 tiles)
    boss->frameWidth = 64;
    boss->frameHeight = 64;
    boss->scale = 1.0f;
    
    // Recenter position so spawnPos is the center of the boss
    boss->position.x = spawnPos.x - boss->frameWidth * 0.5f;
    boss->position.y = spawnPos.y - boss->frameHeight * 0.5f;

    boss->maxHealth = baseMaxHp;
    boss->stage2MaxHealth = stage2MaxHp;
    boss->health = boss->maxHealth;
    boss->stage = 1;
    boss->state = BOSS_STATE_STAGE1_INVINCIBLE;
    boss->currentAttack = BOSS_ATTACK_NONE;
    boss->attackTimer = 3.0f; // Initial delay before first attack
    boss->vulnerableTimer = 0.0f;
    boss->flashTimer = 0.0f;
    
    InitWeapon(&boss->weapon);
    
    // Initialize Cores
    for (int i = 0; i < BOSS_MAX_CORES; i++) {
        boss->cores[i].active = true;
        boss->cores[i].health = BOSS_CORE_HP;
        boss->cores[i].angleOffset = i * (360.0f / BOSS_MAX_CORES);
        boss->cores[i].position = boss->spawnPosition;
    }
    
    // Initialize Meteors
    for (int i = 0; i < BOSS_MAX_METEORS; i++) {
        boss->meteors[i].state = METEOR_DONE;
    }
}

static void PickRandomAttack(Boss* boss) {
    int randChoice;
    if (boss->stage == 1) {
        randChoice = GetRandomValue(1, 3); // Equal chance: SMG, Meteor, Radial
    } else {
        // Stage 2: 50% Charge, ~17% each for SMG/Meteor/Radial
        int roll = GetRandomValue(1, 6);
        if (roll <= 3) randChoice = 4;      // 3/6 = 50% → Charge
        else if (roll == 4) randChoice = 1;  // 1/6 ≈ 17% → SMG
        else if (roll == 5) randChoice = 2;  // 1/6 ≈ 17% → Meteor
        else randChoice = 3;                 // 1/6 ≈ 17% → Radial
    }
    
    if (randChoice == 1) {
        boss->currentAttack = BOSS_ATTACK_SMG;
        boss->attackStep = 0;
        boss->attackTimer = 0.0f;
    } else if (randChoice == 2) {
        boss->currentAttack = BOSS_ATTACK_METEOR;
        boss->attackStep = 0;
        boss->attackTimer = 0.0f;
        // Spawn meteors
        int numMeteors = GetRandomValue(10, 15);
        int spawned = 0;
        for (int i = 0; i < BOSS_MAX_METEORS && spawned < numMeteors; i++) {
            boss->meteors[i].state = METEOR_WARNING;
            boss->meteors[i].stateTimer = 3.0f; // Warning duration
            boss->meteors[i].radius = BOSS_METEOR_RADIUS;
            boss->meteors[i].currentYOffset = -2400.0f; // Drop from higher sky for faster fall
            
            // Random position in 500px radius
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float dist = GetRandomValue(0, 500);
            boss->meteors[i].targetPos.x = boss->spawnPosition.x + cosf(angle) * dist;
            boss->meteors[i].targetPos.y = boss->spawnPosition.y + sinf(angle) * dist;
            spawned++;
        }
    } else if (randChoice == 3) {
        boss->currentAttack = BOSS_ATTACK_RADIAL;
        boss->attackStep = 0;
        boss->attackTimer = 0.0f;
    } else if (randChoice == 4) {
        boss->currentAttack = BOSS_ATTACK_CHARGE;
        boss->attackStep = 0;
        boss->attackTimer = 1.5f; // Telegraph time
        boss->targetPosition = (Vector2){0, 0}; // Set below in Update
    }
}

void UpdateBoss(Boss* boss, Vector2 playerPos, float dt, Rectangle* colliders, int colliderCount, bool* playerHit, float* outDamage) {
    if (boss->state == BOSS_STATE_DEAD) return;

    if (boss->flashTimer > 0) boss->flashTimer -= dt;

    // Update weapon (wall collisions)
    UpdateWeapon(&boss->weapon, dt, colliders, colliderCount);
    
    // Player collision against Boss Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (boss->weapon.bullets[i].active) {
            Vector2 bPos = boss->weapon.bullets[i].position;
            float bRad = boss->weapon.bullets[i].radius;
            if (CheckCollisionCircleRec(bPos, bRad, (Rectangle){playerPos.x, playerPos.y, 32, 32})) {
                *playerHit = true;
                *outDamage = 5.0f; // Base bullet damage
                boss->weapon.bullets[i].active = false;
            }
        }
    }

    // Update Cores visually rotating
    for (int i = 0; i < BOSS_MAX_CORES; i++) {
        if (boss->cores[i].active) {
            boss->cores[i].angleOffset += 30.0f * dt;
            float rad = boss->cores[i].angleOffset * DEG2RAD;
            boss->cores[i].position.x = boss->spawnPosition.x + cosf(rad) * 120.0f;
            boss->cores[i].position.y = boss->spawnPosition.y + sinf(rad) * 120.0f;
        }
    }

    // Update Meteors
    for (int i = 0; i < BOSS_MAX_METEORS; i++) {
        if (boss->meteors[i].state == METEOR_WARNING) {
            boss->meteors[i].stateTimer -= dt;
            // Animate falling faster
            boss->meteors[i].currentYOffset += (2400.0f / 3.0f) * dt;
            
            if (boss->meteors[i].stateTimer <= 0) {
                boss->meteors[i].state = METEOR_IMPACT;
                boss->meteors[i].currentYOffset = 0;
                boss->meteors[i].stateTimer = 0.5f; // Impact lingering
                
                // Deal Damage instantly on impact
                if (CheckCollisionCircleRec(boss->meteors[i].targetPos, boss->meteors[i].radius, (Rectangle){playerPos.x, playerPos.y, 32, 32})) {
                    *playerHit = true;
                    *outDamage = 15.0f;
                }
            }
        } else if (boss->meteors[i].state == METEOR_IMPACT) {
            boss->meteors[i].stateTimer -= dt;
            if (boss->meteors[i].stateTimer <= 0) {
                boss->meteors[i].state = METEOR_DONE;
            }
        }
    }

    // FSM
    if (boss->state == BOSS_STATE_STAGE1_INVINCIBLE) {
        // Check if all cores are destroyed
        bool allDead = true;
        for (int i = 0; i < BOSS_MAX_CORES; i++) {
            if (boss->cores[i].active) {
                allDead = false;
                break;
            }
        }
        
        if (allDead) {
            boss->state = BOSS_STATE_STAGE1_VULNERABLE;
            boss->vulnerableTimer = BOSS_VULNERABLE_TIME;
            boss->currentAttack = BOSS_ATTACK_NONE; // Stop attacking
        }
    } else if (boss->state == BOSS_STATE_STAGE1_VULNERABLE) {
        boss->vulnerableTimer -= dt;
        if (boss->vulnerableTimer <= 0) {
            boss->state = BOSS_STATE_STAGE1_INVINCIBLE;
            for (int i = 0; i < BOSS_MAX_CORES; i++) {
                boss->cores[i].active = true;
                boss->cores[i].health = BOSS_CORE_HP;
            }
            boss->attackTimer = 3.0f; // Brief pause before resuming attacks
        }
        if (boss->health <= 0) {
            boss->stage = 2;
            boss->state = BOSS_STATE_STAGE2;
            boss->health = boss->stage2MaxHealth;
            boss->maxHealth = boss->stage2MaxHealth;
            boss->currentAttack = BOSS_ATTACK_NONE;
            boss->attackTimer = 3.0f;
        }
    } else if (boss->state == BOSS_STATE_STAGE2) {
        if (boss->health <= 0) {
            boss->state = BOSS_STATE_DEAD;
            boss->currentAttack = BOSS_ATTACK_NONE;
        }
    }

    // Attack Logic
    if (boss->state == BOSS_STATE_STAGE1_INVINCIBLE || boss->state == BOSS_STATE_STAGE2) {
        if (boss->currentAttack == BOSS_ATTACK_NONE) {
            boss->attackTimer -= dt;
            if (boss->attackTimer <= 0) {
                PickRandomAttack(boss);
            }
        } else {
            Vector2 bCenter = BossCentre(boss);
            
            if (boss->currentAttack == BOSS_ATTACK_SMG) {
                int totalShots = 5;
                float delay = 0.4f; // 2.5 rps = 0.4s
                
                boss->attackTimer -= dt;
                if (boss->attackTimer <= 0) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract((Vector2){playerPos.x+16, playerPos.y+16}, bCenter));
                    BossShoot(boss, bCenter, dir, 200.0f); // Half speed
                    boss->attackStep++;
                    boss->attackTimer = delay;
                    
                    if (boss->attackStep >= totalShots) {
                        boss->currentAttack = BOSS_ATTACK_NONE;
                        boss->attackTimer = 3.0f; // Cooldown
                    }
                }
            } else if (boss->currentAttack == BOSS_ATTACK_RADIAL) {
                boss->attackTimer -= dt;
                if (boss->attackTimer <= 0) {
                    float baseOffset = 0.0f;
                    if (boss->attackStep == 1) baseOffset = 12.0f;
                    if (boss->attackStep == 2) baseOffset = 24.0f;
                    
                    for (int i = 0; i < 10; i++) {
                        float angle = (i * 36.0f + baseOffset) * DEG2RAD;
                        Vector2 dir = (Vector2){cosf(angle), sinf(angle)};
                        BossShoot(boss, bCenter, dir, 200.0f); // Half speed
                    }
                    
                    boss->attackStep++;
                    boss->attackTimer = 0.5f;
                    if (boss->attackStep >= 3) {
                        boss->currentAttack = BOSS_ATTACK_NONE;
                        boss->attackTimer = 3.0f; // Cooldown
                    }
                }
            } else if (boss->currentAttack == BOSS_ATTACK_METEOR) {
                // Meteors act independently once spawned, just wait for them to finish
                bool meteorsActive = false;
                for (int i = 0; i < BOSS_MAX_METEORS; i++) {
                    if (boss->meteors[i].state != METEOR_DONE) {
                        meteorsActive = true;
                        break;
                    }
                }
                if (!meteorsActive) {
                    boss->currentAttack = BOSS_ATTACK_NONE;
                    boss->attackTimer = 3.0f; // Cooldown
                }
            } else if (boss->currentAttack == BOSS_ATTACK_CHARGE) {
                if (boss->attackStep == 0) {
                    // Telegraphing
                    boss->targetPosition = (Vector2){playerPos.x+16, playerPos.y+16}; // Keep tracking or lock? User said: "stop if there's wall, didn't change path when charging"
                    boss->attackTimer -= dt;
                    if (boss->attackTimer <= 0) {
                        boss->attackStep = 1; // Charging
                        Vector2 dir = Vector2Normalize(Vector2Subtract(boss->targetPosition, bCenter));
                        boss->velocity = Vector2Scale(dir, 800.0f); // Fast charge
                    }
                } else if (boss->attackStep == 1) {
                    // Moving
                    Vector2 nextPos = Vector2Add(boss->position, Vector2Scale(boss->velocity, dt));
                    Rectangle bRect = {nextPos.x, nextPos.y, boss->frameWidth, boss->frameHeight};
                    
                    bool hitWall = false;
                    for (int i = 0; i < colliderCount; i++) {
                        if (CheckCollisionRecs(bRect, colliders[i])) {
                            hitWall = true; break;
                        }
                    }
                    
                    if (hitWall || Vector2Distance(bCenter, boss->targetPosition) < 20.0f) {
                        // Smash!
                        boss->attackStep = 2;
                        boss->attackTimer = 1.0f; // Recovery time
                        
                        // Deal smash damage
                        if (CheckCollisionCircleRec(BossCentre(boss), BOSS_SMASH_RADIUS, (Rectangle){playerPos.x, playerPos.y, 32, 32})) {
                            *playerHit = true;
                            *outDamage = 20.0f;
                        }
                    } else {
                        boss->position = nextPos;
                    }
                } else if (boss->attackStep == 2) {
                    boss->attackTimer -= dt;
                    if (boss->attackTimer <= 0) {
                        boss->currentAttack = BOSS_ATTACK_NONE;
                        boss->attackTimer = 3.0f; // Cooldown
                    }
                }
            }
        }
    }
}

void DrawBoss(Boss* boss) {
    if (boss->state == BOSS_STATE_DEAD) return;

    // Draw telegraph for charge
    if (boss->currentAttack == BOSS_ATTACK_CHARGE && boss->attackStep == 0) {
        DrawLineEx(BossCentre(boss), boss->targetPosition, (float)boss->frameWidth, ColorAlpha(RED, 0.3f));
    }

    // Draw Meteors
    for (int i = 0; i < BOSS_MAX_METEORS; i++) {
        if (boss->meteors[i].state == METEOR_WARNING) {
            DrawCircleLines(boss->meteors[i].targetPos.x, boss->meteors[i].targetPos.y, boss->meteors[i].radius, RED);
            
            if (boss->meteors[i].stateTimer <= 1.0f) {
                float xOffset = boss->meteors[i].currentYOffset * 0.577f; // 30 degrees to the right (starts left, moves right)
                if (boss->meteorSprite.id != 0) {
                    float sWidth = boss->meteorSprite.width * 0.67f;
                    float sHeight = boss->meteorSprite.height * 0.67f;
                    Vector2 pos = {
                        boss->meteors[i].targetPos.x + xOffset - sWidth/2.0f,
                        boss->meteors[i].targetPos.y + boss->meteors[i].currentYOffset - sHeight/2.0f
                    };
                    DrawTextureEx(boss->meteorSprite, pos, 0.0f, 0.67f, WHITE);
                } else {
                    DrawCircle(boss->meteors[i].targetPos.x + xOffset, boss->meteors[i].targetPos.y + boss->meteors[i].currentYOffset, boss->meteors[i].radius, ORANGE);
                }
            }
        } else if (boss->meteors[i].state == METEOR_IMPACT) {
            if (boss->meteorSprite.id != 0) {
                float sWidth = boss->meteorSprite.width * 0.67f;
                float sHeight = boss->meteorSprite.height * 0.67f;
                Vector2 pos = {boss->meteors[i].targetPos.x - sWidth/2.0f, boss->meteors[i].targetPos.y - sHeight/2.0f};
                DrawTextureEx(boss->meteorSprite, pos, 0.0f, 0.67f, WHITE);
            } else {
                DrawCircle(boss->meteors[i].targetPos.x, boss->meteors[i].targetPos.y, boss->meteors[i].radius, ORANGE);
            }
            DrawCircleLines(boss->meteors[i].targetPos.x, boss->meteors[i].targetPos.y, boss->meteors[i].radius, RED);
        }
    }

    // Draw Boss
    Color tint = (boss->flashTimer > 0) ? RED : WHITE;
    if (boss->state == BOSS_STATE_STAGE1_VULNERABLE) {
        tint = (boss->flashTimer > 0) ? RED : LIGHTGRAY; // Show weakness
    }
    
    if (boss->sprite.id != 0) {
        Rectangle src = {0, 0, boss->sprite.width, boss->sprite.height};
        Rectangle dest = {boss->position.x, boss->position.y, boss->frameWidth, boss->frameHeight};
        DrawTexturePro(boss->sprite, src, dest, (Vector2){0,0}, 0.0f, tint);
    } else {
        DrawRectangle(boss->position.x, boss->position.y, boss->frameWidth, boss->frameHeight, tint);
    }
    
    // Draw Smash AOE
    if (boss->currentAttack == BOSS_ATTACK_CHARGE && boss->attackStep == 2 && boss->attackTimer > 0.8f) {
        DrawCircle(BossCentre(boss).x, BossCentre(boss).y, BOSS_SMASH_RADIUS, ColorAlpha(MAROON, 0.6f));
        DrawCircleLines(BossCentre(boss).x, BossCentre(boss).y, BOSS_SMASH_RADIUS, RED);
    }

    // Draw Cores
    for (int i = 0; i < BOSS_MAX_CORES; i++) {
        if (boss->cores[i].active) {
            DrawCircle(boss->cores[i].position.x, boss->cores[i].position.y, 16.0f, PURPLE);
            DrawCircleLines(boss->cores[i].position.x, boss->cores[i].position.y, 16.0f, MAGENTA);
            // Core HP Bar
            float hpPct = boss->cores[i].health / BOSS_CORE_HP;
            DrawRectangle(boss->cores[i].position.x - 16, boss->cores[i].position.y - 24, 32, 6, DARKGRAY);
            DrawRectangle(boss->cores[i].position.x - 16, boss->cores[i].position.y - 24, 32 * hpPct, 6, RED);
        }
    }

    // Draw Bullets
    DrawWeapon(&boss->weapon);
}

void DrawBossUI(Boss* boss) {
    if (boss->state == BOSS_STATE_DEAD) return;

    int screenWidth = GetScreenWidth();
    
    // Mega Health Bar
    int barWidth = screenWidth - 200;
    int barHeight = 24;
    int barX = 100;
    int barY = 60;

    float hpPct = boss->health / boss->maxHealth;
    if (hpPct < 0) hpPct = 0;

    DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
    DrawRectangle(barX, barY, barWidth * hpPct, barHeight, RED);
    DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);

    const char* title = (boss->stage == 1) ? "STAGE 1: PROTOCOL GUARDIAN" : "STAGE 2: PROTOCOL ANOMALY";
    int textWidth = MeasureText(title, 20);
    DrawText(title, barX + barWidth/2 - textWidth/2, barY + 4, 20, WHITE);
    
    if (boss->state == BOSS_STATE_STAGE1_VULNERABLE) {
        DrawText("VULNERABLE!", barX + barWidth/2 - MeasureText("VULNERABLE!", 20)/2, barY + 30, 20, YELLOW);
    } else if (boss->state == BOSS_STATE_STAGE1_INVINCIBLE) {
        DrawText("DESTROY THE CORES!", barX + barWidth/2 - MeasureText("DESTROY THE CORES!", 20)/2, barY + 30, 20, LIGHTGRAY);
    }
}

void DamageBoss(Boss* boss, float amount) {
    if (boss->state == BOSS_STATE_STAGE1_INVINCIBLE) return; // Immune
    
    boss->health -= amount;
    boss->flashTimer = 0.1f;
}

void DamageBossCore(Boss* boss, int coreIndex, float amount) {
    if (boss->state != BOSS_STATE_STAGE1_INVINCIBLE) return;
    if (!boss->cores[coreIndex].active) return;
    
    boss->cores[coreIndex].health -= amount;
    if (boss->cores[coreIndex].health <= 0) {
        boss->cores[coreIndex].health = 0;
        boss->cores[coreIndex].active = false;
    }
}

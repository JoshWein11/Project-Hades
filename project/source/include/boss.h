#ifndef BOSS_H
#define BOSS_H

#include "raylib.h"
#include "weapon.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────
#define BOSS_MAX_CORES 4
#define BOSS_MAX_METEORS 15
#define BOSS_CORE_HP 68.0f
#define BOSS_VULNERABLE_TIME 15.0f
#define BOSS_METEOR_RADIUS 40.0f
#define BOSS_SMASH_RADIUS 64.0f

// ─────────────────────────────────────────────────────────────────────────────
// Boss Structs
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    BOSS_STATE_STAGE1_INVINCIBLE,
    BOSS_STATE_STAGE1_VULNERABLE,
    BOSS_STATE_STAGE2,
    BOSS_STATE_DEAD
} BossState;

typedef enum {
    BOSS_ATTACK_NONE,
    BOSS_ATTACK_SMG,
    BOSS_ATTACK_METEOR,
    BOSS_ATTACK_RADIAL,
    BOSS_ATTACK_CHARGE
} BossAttackType;

typedef enum {
    METEOR_WARNING,
    METEOR_FALLING,
    METEOR_IMPACT,
    METEOR_DONE
} MeteorState;

typedef struct {
    Vector2 position;
    float health;
    bool active;
    float angleOffset; // For drawing in a circle around the boss
} BossCore;

typedef struct {
    Vector2 targetPos;
    float currentYOffset; // Used to simulate falling
    float radius;
    MeteorState state;
    float stateTimer;
} BossMeteor;

typedef struct {
    Vector2 position;
    Vector2 spawnPosition; // Center point for meteor bounds and cores
    Vector2 targetPosition; // Used for charging
    Vector2 velocity;
    
    float health;
    float maxHealth;
    float stage2MaxHealth;
    int stage; // 1 or 2
    
    BossState state;
    float vulnerableTimer;
    
    BossAttackType currentAttack;
    float attackTimer;
    int attackStep; // Tracks SMG shot count or Radial waves
    
    Weapon weapon; // Reusing weapon struct for boss projectiles
    
    BossCore cores[BOSS_MAX_CORES];
    BossMeteor meteors[BOSS_MAX_METEORS];
    
    Texture2D sprite; // Can be 0 if not loaded, fallback to rect
    Texture2D meteorSprite;
    int frameWidth;
    int frameHeight;
    float scale;
    
    float flashTimer; // For hit flash
} Boss;

// ─────────────────────────────────────────────────────────────────────────────
// API
// ─────────────────────────────────────────────────────────────────────────────

void InitBoss(Boss* boss, Vector2 spawnPos, Texture2D sprite, Texture2D meteorSprite, float baseMaxHp, float stage2MaxHp);
void UpdateBoss(Boss* boss, Vector2 playerPos, float dt, Rectangle* colliders, int colliderCount, bool* playerHit, float* outDamage);
void DrawBoss(Boss* boss);
void DrawBossUI(Boss* boss);
void DamageBoss(Boss* boss, float amount);
void DamageBossCore(Boss* boss, int coreIndex, float amount);

// Helper for external checks
static inline Vector2 BossCentre(const Boss* boss) {
    if (boss->sprite.id == 0) return boss->position; // Fallback
    return (Vector2){
        boss->position.x + (boss->frameWidth * boss->scale) * 0.5f,
        boss->position.y + (boss->frameHeight * boss->scale) * 0.5f
    };
}

#endif // BOSS_H

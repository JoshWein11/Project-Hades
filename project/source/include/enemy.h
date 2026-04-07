#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>

// ─────────────────────────────────────────────────────────────────────────────
// Constants — tweak these to tune the global feel of all enemies
// ─────────────────────────────────────────────────────────────────────────────
#define ENEMY_MAX_WAYPOINTS   8    // Maximum patrol stops per enemy
#define ENEMY_MAX_PARTICLES   24   // Hit-burst particles per enemy

// ─────────────────────────────────────────────────────────────────────────────
// EnemyState — the four states in the finite state machine
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    ENEMY_PATROL,   // Walking between waypoints
    ENEMY_CHASE,    // Pursuing the player
    ENEMY_SEARCH,   // Lost the player — looking around before returning
    ENEMY_HIT,      // Brief stun after taking damage
    ENEMY_DEAD      // Playing death animation, no longer moves
} EnemyState;

// ─────────────────────────────────────────────────────────────────────────────
// EnemyAnimClip — one animation clip defined by a row of frames in a sprite sheet.
//   All clips share the same Texture2D atlas stored on the Enemy.
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    int   startRow;      // Which row in the sheet this clip lives on
    int   frameCount;    // Number of columns (frames) in the clip
    float frameDuration; // Seconds per frame  (e.g. 0.1 = 10 fps)
    bool  loops;         // If false the clip freezes on the last frame when done
} EnemyAnimClip;

// ─────────────────────────────────────────────────────────────────────────────
// EnemyAnimState — runtime playback cursor for one clip
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    int   currentFrame;  // Which column we are on right now
    float timer;         // Accumulated seconds since last frame advance
    bool  finished;      // True once a non-looping clip reaches its last frame
} EnemyAnimState;

// ─────────────────────────────────────────────────────────────────────────────
// EnemyParticle — one small debris square spawned on hit
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float   life;        // Starts at 1.0, fades toward 0
    float   size;
    Color   color;
} EnemyParticle;

// ─────────────────────────────────────────────────────────────────────────────
// Enemy — complete state for one patrolling, reactive enemy.
//
//   Quick-start:
//     Call InitEnemy() with position, patrol waypoints, and a sprite sheet path.
//     Then call UpdateEnemy() and DrawEnemy() each frame.
//
//   Placing enemies:
//     Pass different Vector2 waypoints to InitEnemy() to patrol any route.
//
//   Custom sprites:
//     Set frameWidth/Height and per-clip row/frameCount in the EnemyAnimClip
//     fields before calling InitEnemy, OR edit the defaults inside InitEnemy.
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {

    // ── Position / Physics ────────────────────────────────────────────────────
    Vector2    position;         // Top-left of the sprite (world space)
    float      patrolSpeed;      // px/s while patrolling
    float      chaseSpeed;       // px/s while chasing

    // ── Patrol waypoints (up to ENEMY_MAX_WAYPOINTS) ──────────────────────────
    Vector2    waypoints[ENEMY_MAX_WAYPOINTS];
    int        waypointCount;    // How many waypoints are actually set
    int        waypointIndex;    // Which waypoint we are heading toward now
    int        patrolDir;        // +1 = forward through list, -1 = backward (ping-pong)

    // ── Vision cone ───────────────────────────────────────────────────────────
    float      visionRange;      // Max detection distance (px)
    float      visionAngle;      // Half-angle of the cone in degrees (e.g. 60 = 120° total)
    float      facingAngleDeg;   // Current heading in degrees (0=right, 90=down etc.)
    float      loseRange;        // Beyond this distance the enemy gives up the chase

    // ── Health / combat ───────────────────────────────────────────────────────
    float      health;
    float      maxHealth;
    float      hitInvincibleTimer; // Brief window after a hit during which more hits are ignored

    // ── FSM ───────────────────────────────────────────────────────────────────
    EnemyState state;
    float      hitStateTimer;    // How long to stay in HIT state
    float      waitTimer;        // Counts down at waypoint before resuming patrol

    // ── Patrol look-around ────────────────────────────────────────────────────
    float      lookAroundTimer;  // Counts down during waypoint idle sweep
    float      lookBaseAngle;    // The angle the enemy arrived at the waypoint
    int        lookDir;          // +1 or -1, alternates sweep direction

    // ── Search state (lost-player) ────────────────────────────────────────────
    float      searchTimer;      // Counts down while searching (~2 s)
    float      searchBaseAngle;  // Heading when player was last seen
    Vector2    lastKnownPlayerPos; // Where the player was last spotted

    // ── Nav-node return path (SEARCH → PATROL) ────────────────────────────────
    Vector2    navPath[16];       // Sequence of nav nodes to follow back
    int        navPathCount;      // How many nodes in the path
    int        navPathCurrent;    // Which node we are walking toward now
    bool       navPathActive;     // True while following a nav path

    // ── Sprite / animation ────────────────────────────────────────────────────
    Texture2D      spriteRight;  // Texture for facing right
    Texture2D      spriteLeft;   // Texture for facing left
    Texture2D*     currentSprite;// Points to the active texture
    bool           hasSprite;    // False → draw placeholder rectangle
    int            frameWidth;   // Pixel width  of ONE frame
    int            frameHeight;  // Pixel height of ONE frame
    float          scale;        // Render scale (1.0 = native size)
    Rectangle      frameRec;     // Current sub-rectangle of the sheet

    // Animation clip definitions — edit rows/frameCounts to match your sheet
    EnemyAnimClip  clipIdle;
    EnemyAnimClip  clipWalk;
    EnemyAnimClip  clipChase;
    EnemyAnimClip  clipHit;
    EnemyAnimClip  clipDeath;

    // Runtime playback state (one per clip so they don't share a cursor)
    EnemyAnimState animIdle;
    EnemyAnimState animWalk;
    EnemyAnimState animChase;
    EnemyAnimState animHit;
    EnemyAnimState animDeath;

    // ── Flash / hit feedback ──────────────────────────────────────────────────
    float      flashTimer;       // Counts down; enemy renders tinted RED while > 0
    float      hitPauseTimer;    // Counts down; all update logic is frozen while > 0

    // ── Particles ─────────────────────────────────────────────────────────────
    EnemyParticle particles[ENEMY_MAX_PARTICLES];

    // ── Placeholder visuals (no sprite) ───────────────────────────────────────
    float      width;            // Rectangle width  to draw
    float      height;           // Rectangle height to draw

} Enemy;

// ─────────────────────────────────────────────────────────────────────────────
// API
// ─────────────────────────────────────────────────────────────────────────────

// Initializes one enemy.
//
//   spawnPos       : Starting position (world space, top-left of sprite).
//   waypoints      : Array of patrol waypoints; first entry should equal spawnPos.
//   waypointCount  : Number of entries in the waypoints array (max ENEMY_MAX_WAYPOINTS).
//   spriteR/L      : Pre-loaded textures for right/left facing. Pass (Texture2D){0} for placeholder.
//   frameWidth/H   : Pixel size of one animation frame.
//   scale          : Render scale (1.5 = 1.5× native size).
void InitEnemy(Enemy*      e,
               Vector2     spawnPos,
               Vector2*    waypoints,
               int         waypointCount,
               Texture2D   spriteR,
               Texture2D   spriteL,
               int         frameWidth,
               int         frameHeight,
               float       scale);

// Updates FSM, movement, vision cone, animations, particles, and timers.
//   playerPos      : Centre of the player character in world space.
//   dt             : Delta time (use GetFrameTime()).
//   outShake       : Written to true for one frame when the player lands a hit
//                    that warrants a screen shake — read this in screen_gameplay.c.
//   colliders      : Array of wall collision rectangles (from MapData).
//   colliderCount  : Number of entries in colliders.
void UpdateEnemy(Enemy* e, Vector2 playerPos, float dt, bool* outShake,
                 Rectangle* colliders, int colliderCount,
                 Vector2* navNodes, int navNodeCount);

// Draws the enemy sprite (or placeholder), health bar, vision cone (debug), and particles.
void DrawEnemy(Enemy* e);

// Call when the player damages this enemy.
//   damage         : Hit-points to subtract.
void DamageEnemy(Enemy* e, float damage);

// Stun the enemy without dealing damage.
// Puts the enemy into ENEMY_HIT state for the standard hit duration.
void StunEnemy(Enemy* e);

// Frees the GPU sprite texture (if one was loaded).
// NOTE: Only call this if the enemy owns its textures (not shared).
//       When using shared textures from SpawnEnemiesFromMap, the caller
//       unloads the shared textures separately.
void UnloadEnemy(Enemy* e);

// Returns the world-space centre of the enemy's sprite rectangle.
// Useful in screen_gameplay.c for distance/knockback calculations.
static inline Vector2 EnemyCentre(const Enemy* e) {
    return (Vector2){
        e->position.x + (e->frameWidth  * e->scale) * 0.5f,
        e->position.y + (e->frameHeight * e->scale) * 0.5f
    };
}

#endif // ENEMY_H

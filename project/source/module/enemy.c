#include "enemy.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────


// Moves the enemy toward 'target' at 'speed' px/s with wall collision.
// Returns true when it arrives (within 4 px).
// Also updates facingAngleDeg from the movement direction.
static bool MoveToward(Enemy* e, Vector2 target, float speed, float dt,
                       Rectangle* colliders, int colliderCount) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);
    if (dist < 4.0f) {
        e->position = target;
        return true;
    }
    Vector2 dir = Vector2Normalize(diff);
    float dx = dir.x * speed * dt;
    float dy = dir.y * speed * dt;

    // Enemy hitbox (scaled sprite size)
    float w = (float)e->frameWidth  * e->scale;
    float h = (float)e->frameHeight * e->scale;

    // Axis-separated collision (same approach as the player in character.c)
    // Test X axis
    Rectangle testX = { e->position.x + dx, e->position.y, w, h };
    bool blockedX = false;
    for (int i = 0; i < colliderCount; i++) {
        if (CheckCollisionRecs(testX, colliders[i])) { blockedX = true; break; }
    }
    if (!blockedX) e->position.x += dx;

    // Test Y axis
    Rectangle testY = { e->position.x, e->position.y + dy, w, h };
    bool blockedY = false;
    for (int i = 0; i < colliderCount; i++) {
        if (CheckCollisionRecs(testY, colliders[i])) { blockedY = true; break; }
    }
    if (!blockedY) e->position.y += dy;

    // Track heading so the vision cone faces the direction of travel
    e->facingAngleDeg = atan2f(dir.y, dir.x) * RAD2DEG;
    return false;
}

// Advances one animation clip's playback cursor by dt.
// Writes the resolved row+column into e->frameRec, using 'dirRow' to
// pick the correct facing-direction row within that clip's block.
static void TickClip(Enemy*          e,
                     EnemyAnimClip*  clip,
                     EnemyAnimState* anim,
                     int             dirRow,
                     float           dt)
{
    if (!e->hasSprite) return;
    if (anim->finished && !clip->loops) {
        // Freeze on last frame
        e->frameRec.x = (float)((clip->frameCount - 1) * e->frameWidth);
        e->frameRec.y = (float)((clip->startRow + dirRow) * e->frameHeight);
        return;
    }

    anim->timer += dt;
    if (anim->timer >= clip->frameDuration) {
        anim->timer -= clip->frameDuration;
        anim->currentFrame++;
        if (anim->currentFrame >= clip->frameCount) {
            if (clip->loops) {
                anim->currentFrame = 0;
            } else {
                anim->currentFrame = clip->frameCount - 1;
                anim->finished = true;
            }
        }
    }

    // Clamp dirRow to the available sheet rows
    int maxRows = (e->currentSprite && e->currentSprite->height > 0) ? (e->currentSprite->height / e->frameHeight) : 1;
    int row = clip->startRow + dirRow;
    if (row >= maxRows) row = clip->startRow; // Fallback if sheet is short

    e->frameRec.x = (float)(anim->currentFrame * e->frameWidth);
    e->frameRec.y = (float)(row * e->frameHeight);
}

// Returns the direction row index (0=Right, 1=Left, 2=Up, 3=Down)
// based on the enemy's current facing angle.
static int FacingRow(const Enemy* e) {
    float a = e->facingAngleDeg;
    // Normalise to 0-360
    while (a < 0.0f)   a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;

    if (a < 45.0f || a >= 315.0f) return 0; // Right
    if (a < 135.0f)               return 3; // Down
    if (a < 225.0f)               return 1; // Left
    return 2;                               // Up
}

// Spawns a burst of particles at the enemy centre, coloured 'c'.
static void SpawnParticles(Enemy* e, Color c) {
    for (int i = 0; i < ENEMY_MAX_PARTICLES; i++) {
        float angle = ((float)i / ENEMY_MAX_PARTICLES) * 2.0f * PI
                      + ((float)GetRandomValue(-30, 30) * DEG2RAD);
        float spd   = (float)GetRandomValue(60, 180);
        e->particles[i].position = EnemyCentre(e);
        e->particles[i].velocity = (Vector2){ cosf(angle) * spd, sinf(angle) * spd };
        e->particles[i].life     = 1.0f;
        e->particles[i].size     = (float)GetRandomValue(3, 7);
        e->particles[i].color    = c;
    }
}

// Resets one animation state's cursor and finished flag
static void ResetAnim(EnemyAnimState* a) {
    a->currentFrame = 0;
    a->timer        = 0.0f;
    a->finished     = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Vision cone — returns true when the player is within range, inside the
// angle cone, AND no wall blocks the line of sight.
// ─────────────────────────────────────────────────────────────────────────────
static bool CanSeePlayer(const Enemy* e, Vector2 playerPos,
                         Rectangle* colliders, int colliderCount) {
    Vector2 centre = EnemyCentre((Enemy*)e);
    float dist = Vector2Distance(centre, playerPos);
    if (dist > e->visionRange) return false;

    // Angle from enemy to player
    Vector2 toPlayer = Vector2Subtract(playerPos, centre);
    float angleToPlayer = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;

    // Difference from facing heading (absolute), wrapped to ±180
    float diff = angleToPlayer - e->facingAngleDeg;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    if (fabsf(diff) > e->visionAngle) return false;

    // Raycast: step along the line from enemy to player, checking for walls
    float stepSize = 8.0f; // Check every 8 pixels
    int steps = (int)(dist / stepSize);
    Vector2 stepDir = Vector2Normalize(toPlayer);
    for (int i = 1; i <= steps; i++) {
        Vector2 point = Vector2Add(centre, Vector2Scale(stepDir, stepSize * (float)i));
        for (int c = 0; c < colliderCount; c++) {
            if (CheckCollisionPointRec(point, colliders[c])) {
                return false; // Wall blocks line of sight
            }
        }
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// InitEnemy
// ─────────────────────────────────────────────────────────────────────────────
void InitEnemy(Enemy*       e,
               Vector2      spawnPos,
               Vector2*     waypoints,
               int          waypointCount,
               const char*  spriteRightPath,
               const char*  spriteLeftPath,
               int          frameWidth,
               int          frameHeight,
               float        scale)
{
    memset(e, 0, sizeof(Enemy));

    // ── Position / physics ────────────────────────────────────────────────────
    e->position     = spawnPos;
    e->patrolSpeed  = 80.0f;
    e->chaseSpeed   = 167.0f;

    // ── Waypoints ─────────────────────────────────────────────────────────────
    waypointCount = (waypointCount < 1) ? 1 : 
                    (waypointCount > ENEMY_MAX_WAYPOINTS) ? ENEMY_MAX_WAYPOINTS : waypointCount;
    for (int i = 0; i < waypointCount; i++) e->waypoints[i] = waypoints[i];
    e->waypointCount = waypointCount;
    e->waypointIndex = 0;
    e->patrolDir     = 1;

    // ── Vision cone ───────────────────────────────────────────────────────────
    e->visionRange    = 167.0f; 
    e->visionAngle    = 36.7f;
    e->loseRange      = 150.0f;
    e->facingAngleDeg = 0.0f;

    // ── Health ────────────────────────────────────────────────────────────────
    e->maxHealth = 100.0f;
    e->health    = e->maxHealth;

    // ── FSM ───────────────────────────────────────────────────────────────────
    e->state         = ENEMY_PATROL;
    e->hitStateTimer = 0.4f; // HIT state lasts 0.4 seconds by default

    // ── Sprite & animation ────────────────────────────────────────────────────
    e->frameWidth  = (frameWidth  > 0) ? frameWidth  : 32;
    e->frameHeight = (frameHeight > 0) ? frameHeight : 32;
    e->scale       = (scale       > 0) ? scale       : 0.5f;
    e->frameRec    = (Rectangle){ 0, 0, (float)e->frameWidth, (float)e->frameHeight };

    if (spriteRightPath != NULL && spriteLeftPath != NULL) {
        e->spriteRight = LoadTexture(spriteRightPath);
        e->spriteLeft  = LoadTexture(spriteLeftPath);
        e->currentSprite = &e->spriteRight;
        e->hasSprite = true;
    }

    // ── Animation clip definitions ────────────────────────────────────────────
    // Each clip references one or more rows in the sheet.
    // Row layout assumed (change startRow to match YOUR sheet):
    //   Row 0 = walk/idle row 0 (right)     }  idle & walk
    //   Row 1 = walk/idle row 1 (left)      }  share the same rows
    //   Row 2 = walk/idle row 2 (up)        }
    //   Row 3 = walk/idle row 3 (down)      }
    //   Row 4 = chase rows (same layout)    — set startRow=4 if different
    //   Row 8 = hit  (single-row flash)
    //   Row 9 = death
    e->clipIdle  = (EnemyAnimClip){ .startRow = 0, .frameCount = 1,  .frameDuration = 0.15f, .loops = true  };
    e->clipWalk  = (EnemyAnimClip){ .startRow = 0, .frameCount = 1,  .frameDuration = 0.10f, .loops = true  };
    e->clipChase = (EnemyAnimClip){ .startRow = 0, .frameCount = 1,  .frameDuration = 0.07f, .loops = true  };
    e->clipHit   = (EnemyAnimClip){ .startRow = 0, .frameCount = 1,  .frameDuration = 0.08f, .loops = false };
    e->clipDeath = (EnemyAnimClip){ .startRow = 0, .frameCount = 1,  .frameDuration = 0.12f, .loops = false };

    // ── Placeholder rectangle ─────────────────────────────────────────────────
    e->width  = (float)e->frameWidth  * e->scale;
    e->height = (float)e->frameHeight * e->scale;
}

// ─────────────────────────────────────────────────────────────────────────────
// DamageEnemy — called by the player when a hit lands
// ─────────────────────────────────────────────────────────────────────────────
void DamageEnemy(Enemy* e, float damage, Vector2 knockbackDir, float knockbackForce) {
    if (e->state == ENEMY_DEAD) return;         // Already dead, ignore
    if (e->hitInvincibleTimer > 0.0f) return;   // In invincibility window

    e->health -= damage;
    e->hitInvincibleTimer = 0.5f; // 0.5 s invincibility after each hit

    if (e->health <= 0.0f) {
        e->health = 0.0f;
        e->state  = ENEMY_DEAD;
        ResetAnim(&e->animDeath);
        SpawnParticles(e, RED);
    } else {
        e->state         = ENEMY_HIT;
        e->hitStateTimer = 0.35f;
        e->flashTimer    = 0.35f;
        e->hitPauseTimer = 0.06f; // 60 ms freeze
        e->knockbackVel  = Vector2Scale(knockbackDir, knockbackForce);
        ResetAnim(&e->animHit);
        SpawnParticles(e, ORANGE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateEnemy
// ─────────────────────────────────────────────────────────────────────────────
void UpdateEnemy(Enemy* e, Vector2 playerPos, float dt, bool* outShake,
                 Rectangle* colliders, int colliderCount) {
    if (outShake) *outShake = false;

    // ── Tick-down timers that run even while paused ────────────────────────────
    if (e->hitInvincibleTimer > 0.0f) e->hitInvincibleTimer -= dt;
    if (e->flashTimer         > 0.0f) e->flashTimer         -= dt;

    // ── Particles (always update, even when dead / paused) ────────────────────
    for (int i = 0; i < ENEMY_MAX_PARTICLES; i++) {
        EnemyParticle* p = &e->particles[i];
        if (p->life <= 0.0f) continue;
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        p->velocity = Vector2Scale(p->velocity, 0.85f); // Drag
        p->life    -= dt * 2.5f;
    }

    // ── Hit pause — freeze all game logic for a few ms ────────────────────────
    if (e->hitPauseTimer > 0.0f) {
        e->hitPauseTimer -= dt;
        return; // Skip everything else this frame
    }

    // ── Dead — only play death animation, no movement ─────────────────────────
    if (e->state == ENEMY_DEAD) {
        TickClip(e, &e->clipDeath, &e->animDeath, 0, dt);
        return;
    }

    // ── Knockback decay ───────────────────────────────────────────────────────
    if (Vector2Length(e->knockbackVel) > 1.0f) {
        e->position     = Vector2Add(e->position, Vector2Scale(e->knockbackVel, dt));
        e->knockbackVel = Vector2Scale(e->knockbackVel, 0.80f); // Strong drag
    } else {
        e->knockbackVel = (Vector2){0};
    }

    // ── HIT state countdown ───────────────────────────────────────────────────
    if (e->state == ENEMY_HIT) {
        e->hitStateTimer -= dt;
        TickClip(e, &e->clipHit, &e->animHit, FacingRow(e), dt);
        if (e->hitStateTimer <= 0.0f) {
            // Recover: if player still visible, resume chase; otherwise patrol
            e->state = CanSeePlayer(e, playerPos, colliders, colliderCount) ? ENEMY_CHASE : ENEMY_PATROL;
        }
        return;
    }

    Vector2 centre = EnemyCentre(e);
    float distToPlayer = Vector2Distance(centre, playerPos);
    bool playerVisible = CanSeePlayer(e, playerPos, colliders, colliderCount);

    // ── FSM Transitions ───────────────────────────────────────────────────────
    switch (e->state) {
        case ENEMY_PATROL:
            if (playerVisible) e->state = ENEMY_CHASE;
            break;
        case ENEMY_CHASE:
            if (distToPlayer > e->loseRange) e->state = ENEMY_PATROL;
            break;
        default: break;
    }

    // ── Movement + animation ──────────────────────────────────────────────────
    int dirRow = FacingRow(e);

    if (e->hasSprite) {
        if (dirRow == 1) e->currentSprite = &e->spriteLeft;
        else e->currentSprite = &e->spriteRight; // Right, Up, Down default to right
    }

    switch (e->state) {

        case ENEMY_PATROL: {
            // If waiting at waypoint, count down before resuming
            if (e->waitTimer > 0.0f) {
                e->waitTimer -= dt;
                TickClip(e, &e->clipIdle, &e->animIdle, dirRow, dt);
                break;
            }

            // Ping-pong between waypoints
            Vector2 target = e->waypoints[e->waypointIndex];
            if (MoveToward(e, target, e->patrolSpeed, dt, colliders, colliderCount)) {
                // Arrived at waypoint — pause 2 seconds before turning
                e->waitTimer = 2.0f;
                e->waypointIndex += e->patrolDir;
                if (e->waypointIndex >= e->waypointCount) {
                    e->waypointIndex = e->waypointCount - 2; // Bounce back
                    e->patrolDir     = -1;
                } else if (e->waypointIndex < 0) {
                    e->waypointIndex = 1;
                    e->patrolDir     = 1;
                }
            }
            bool moving = Vector2Length(Vector2Subtract(target, e->position)) > 4.0f;
            if (moving) TickClip(e, &e->clipWalk, &e->animWalk, dirRow, dt);
            else        TickClip(e, &e->clipIdle, &e->animIdle, dirRow, dt);
            break;
        }

        case ENEMY_CHASE:
            MoveToward(e, playerPos, e->chaseSpeed, dt, colliders, colliderCount);
            TickClip(e, &e->clipChase, &e->animChase, dirRow, dt);
            break;

        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawEnemy
// ─────────────────────────────────────────────────────────────────────────────
void DrawEnemy(Enemy* e) {

    // ── Particles (draw beneath the sprite) ───────────────────────────────────
    for (int i = 0; i < ENEMY_MAX_PARTICLES; i++) {
        EnemyParticle* p = &e->particles[i];
        if (p->life <= 0.0f) continue;
        Color c = Fade(p->color, p->life);
        DrawRectangleV(p->position, (Vector2){ p->size, p->size }, c);
    }

    // ── Sprite or placeholder rectangle ───────────────────────────────────────
    float w = (float)e->frameWidth  * e->scale;
    float h = (float)e->frameHeight * e->scale;

    // Flash tint: RED while hit, WHITE otherwise; dead = dark grey
    Color tint = WHITE;
    if (e->state == ENEMY_DEAD)           tint = DARKGRAY;
    else if (e->flashTimer > 0.0f)        tint = RED;

    if (e->hasSprite) {
        Rectangle dest = { e->position.x, e->position.y, w, h };
        DrawTexturePro(*e->currentSprite, e->frameRec, dest, (Vector2){0,0}, 0.0f, tint);
    } else {
        // Placeholder: colour encodes state
        Color base = (e->state == ENEMY_PATROL) ? (Color){60, 120, 220, 255}  :  // Blue
                     (e->state == ENEMY_CHASE)  ? (Color){220, 80, 30, 255}   :  // Orange
                     (e->state == ENEMY_HIT)    ? (Color){255, 50, 50, 255}   :  // Red
                                                   (Color){80, 80, 80, 255};       // Grey dead
        // Flash override
        if (e->flashTimer > 0.0f && e->state != ENEMY_DEAD) base = RED;
        DrawRectangleV(e->position, (Vector2){ e->width, e->height }, base);
    }

    // ── Vision cone (helpful during development, comment out for release) ──────
    if (e->state != ENEMY_DEAD) {
        Vector2 centre = EnemyCentre(e);
        float startAngle = (e->facingAngleDeg - e->visionAngle) * DEG2RAD;
        float endAngle   = (e->facingAngleDeg + e->visionAngle) * DEG2RAD;
        Color coneColor  = (e->state == ENEMY_CHASE) ? Fade(ORANGE, 0.20f)
                                                      : Fade(YELLOW, 0.12f);
        Color edgeColor  = Fade(YELLOW, 0.4f);

        // Draw filled arc as a triangle fan (20 slices for a smooth curve)
        int segments = 20;
        float step = (endAngle - startAngle) / (float)segments;
        for (int i = 0; i < segments; i++) {
            float a1 = startAngle + step * (float)i;
            float a2 = startAngle + step * (float)(i + 1);
            Vector2 p1 = { centre.x + cosf(a1) * e->visionRange,
                           centre.y + sinf(a1) * e->visionRange };
            Vector2 p2 = { centre.x + cosf(a2) * e->visionRange,
                           centre.y + sinf(a2) * e->visionRange };
            DrawTriangle(centre, p2, p1, coneColor);
        }

        // Edge lines along the two sides of the cone
        Vector2 v1 = { centre.x + cosf(startAngle) * e->visionRange,
                       centre.y + sinf(startAngle) * e->visionRange };
        Vector2 v2 = { centre.x + cosf(endAngle)   * e->visionRange,
                       centre.y + sinf(endAngle)   * e->visionRange };
        DrawLineV(centre, v1, edgeColor);
        DrawLineV(centre, v2, edgeColor);
    }

    // ── Health bar (hidden when full or dead) ─────────────────────────────────
    if (e->state != ENEMY_DEAD && e->health < e->maxHealth) {
        float barW = (float)e->frameWidth * e->scale;
        float barH = 4.0f;
        float pct  = e->health / e->maxHealth;
        DrawRectangle((int)e->position.x, (int)(e->position.y - 8), (int)barW, (int)barH, Fade(DARKGRAY, 0.8f));
        DrawRectangle((int)e->position.x, (int)(e->position.y - 8), (int)(barW * pct), (int)barH,
                      (pct > 0.5f) ? GREEN : (pct > 0.25f) ? YELLOW : RED);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UnloadEnemy
// ─────────────────────────────────────────────────────────────────────────────
void UnloadEnemy(Enemy* e) {
    if (e->hasSprite) {
        UnloadTexture(e->spriteRight);
        UnloadTexture(e->spriteLeft);
        e->hasSprite = false;
    }
}

#include "enemy.h" //Code Written by: Josh Weinrich
#include "raymath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────


// Moves the enemy toward 'target' at 'speed' px/s with wall collision.
// Uses a "feeler" steering approach: it tests angles deviating from the direct
// line to the target, picking the first angle where its hitbox isn't blocked.
// Returns true when it arrives (within 4 px).
static bool MoveToward(Enemy* e, Vector2 target, float speed, float dt,
                       Rectangle* colliders, int colliderCount) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);
    // Increased arrival radius to 16.0f to prevent stuttering when whiskers push away from walls
    if (dist < 16.0f) {
        e->position = target; // Removed snapping to prevent jerky movement
        return true;
    }

    float desiredAngle = atan2f(diff.y, diff.x) * RAD2DEG;
    float bestAngle = desiredAngle;

    // Enemy hitbox size
    float w = (float)e->frameWidth  * e->scale;
    float h = (float)e->frameHeight * e->scale;

    // Context Steering: test angles outward from the ideal path.
    // We project the enemy's hitbox forward by a short distance.
    float deviations[] = { 0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 85.0f, -85.0f };
    float lookAhead = 12.0f; // pixel distance to project hitbox for testing

    for (int i = 0; i < 7; i++) {
        float testAngle = desiredAngle + deviations[i];
        Vector2 testDir = { cosf(testAngle * DEG2RAD), sinf(testAngle * DEG2RAD) };
        
        // Project the hitbox forward, but shrink it slightly by 1px on all sides
        // to prevent getting falsely "blocked" by a wall we are already sliding against.
        Rectangle testRec = { 
            (e->position.x + testDir.x * lookAhead) + 1.0f, 
            (e->position.y + testDir.y * lookAhead) + 1.0f, 
            w - 2.0f, 
            h - 2.0f 
        };
        
        bool blocked = false;
        for (int c = 0; c < colliderCount; c++) {
            if (CheckCollisionRecs(testRec, colliders[c])) {
                blocked = true;
                break;
            }
        }
        
        if (!blocked) {
            bestAngle = testAngle;
            break;
        }
    }

    // Move along the chosen unblocked angle
    Vector2 moveDir = { cosf(bestAngle * DEG2RAD), sinf(bestAngle * DEG2RAD) };
    float dx = moveDir.x * speed * dt;
    float dy = moveDir.y * speed * dt;

    // Apply strict axis-separated collision sliding as a final safety net
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

    // Track heading so the vision cone and sprites face the direction of travel
    e->facingAngleDeg = bestAngle;
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
// BFS Pathfinding over NavNodes — guarantees the enemy can walk around walls.
// ─────────────────────────────────────────────────────────────────────────────

// Helper: True if a line intersects a rectangle cleanly
static bool LineIntersectsRect(Vector2 start, Vector2 end, Rectangle rec) {
    if (CheckCollisionPointRec(start, rec) || CheckCollisionPointRec(end, rec)) return true;
    Vector2 point;
    Vector2 r1 = {rec.x, rec.y};
    Vector2 r2 = {rec.x + rec.width, rec.y};
    Vector2 r3 = {rec.x + rec.width, rec.y + rec.height};
    Vector2 r4 = {rec.x, rec.y + rec.height};
    if (CheckCollisionLines(start, end, r1, r2, &point)) return true;
    if (CheckCollisionLines(start, end, r2, r3, &point)) return true;
    if (CheckCollisionLines(start, end, r3, r4, &point)) return true;
    if (CheckCollisionLines(start, end, r4, r1, &point)) return true;
    return false;
}

// Helper: True if a clear line of sight exists between A and B
static bool HasLOS(Vector2 a, Vector2 b, Rectangle* colliders, int colliderCount) {
    for (int i = 0; i < colliderCount; i++) {
        if (LineIntersectsRect(a, b, colliders[i])) return false; // Blocked by wall
    }
    return true; // Clear path
}

// Build a guaranteed path across the NavNodes using Breadth-First-Search
static void BuildNavPath(Enemy* e, Vector2 goal,
                         Vector2* navNodes, int navNodeCount,
                         Rectangle* colliders, int colliderCount) {
    e->navPathActive  = false;
    e->navPathCount   = 0;
    e->navPathCurrent = 0;

    if (navNodeCount <= 0) return;

    Vector2 current = EnemyCentre(e);

    // 1. If we can see the goal directly, just walk there!
    if (HasLOS(current, goal, colliders, colliderCount)) {
        e->navPath[0] = goal;
        e->navPathCount = 1;
        e->navPathActive = true;
        return;
    }

    // 2. Find the nav node closest to the enemy (start node)
    int startNode = -1;
    float bestStartDist = 1e9f;
    for (int i = 0; i < navNodeCount; i++) {
        float d = Vector2Distance(current, navNodes[i]);
        if (d < bestStartDist && HasLOS(current, navNodes[i], colliders, colliderCount)) {
            bestStartDist = d;
            startNode = i;
        }
    }
    if (startNode == -1) startNode = 0; // Fallback if stuck

    // 3. Find the nav node closest to the goal (end node)
    int goalNode = -1;
    float bestGoalDist = 1e9f;
    for (int i = 0; i < navNodeCount; i++) {
        float d = Vector2Distance(goal, navNodes[i]);
        if (d < bestGoalDist && HasLOS(goal, navNodes[i], colliders, colliderCount)) {
            bestGoalDist = d;
            goalNode = i;
        }
    }
    if (goalNode == -1) goalNode = 0;

    // 4. Breadth-First-Search across the graph of visible nodes
    int queue[128];     // MAX_NAV_NODES is 128
    int parent[128];
    bool visited[128] = {0};
    int head = 0, tail = 0;

    for (int i=0; i<128; i++) parent[i] = -1;

    queue[tail++] = startNode;
    visited[startNode] = true;

    bool found = false;
    while (head < tail) {
        int u = queue[head++];
        if (u == goalNode) {
            found = true;
            break;
        }
        
        // Explore all siblings
        for (int v = 0; v < navNodeCount; v++) {
            if (!visited[v] && HasLOS(navNodes[u], navNodes[v], colliders, colliderCount)) {
                visited[v] = true;
                parent[v] = u;
                queue[tail++] = v;
            }
        }
    }

    // 5. If we found a path, reconstruct it backwards, then reverse it
    if (found) {
        int tempPath[128];
        int tempCount = 0;
        int curr = goalNode;
        
        while (curr != -1) {
            tempPath[tempCount++] = curr;
            curr = parent[curr];
        }

        // Reverse into the enemy's path array (cap at 16 to fit in memory)
        for (int i = 0; i < tempCount && i < 15; i++) {
            e->navPath[i] = navNodes[tempPath[tempCount - 1 - i]];
            e->navPathCount++;
        }
        // Finally, add the actual patrol waypoint as the absolute last step
        e->navPath[e->navPathCount++] = goal;
        e->navPathActive = true;
    } else {
        // Fallback: just walk to the goal directly if maze is broken
        e->navPath[0] = goal;
        e->navPathCount = 1;
        e->navPathActive = true;
    }
}
// Find the index of the nearest waypoint to the enemy's current position
static int NearestWaypoint(const Enemy* e) {
    Vector2 pos = EnemyCentre(e);
    int best = 0;
    float bestDist = Vector2Distance(pos, e->waypoints[0]);
    for (int i = 1; i < e->waypointCount; i++) {
        float d = Vector2Distance(pos, e->waypoints[i]);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

// ─────────────────────────────────────────────────────────────────────────────
// InitEnemy
// ─────────────────────────────────────────────────────────────────────────────
void InitEnemy(Enemy*       e,
               Vector2      spawnPos,
               Vector2*     waypoints,
               int          waypointCount,
               Texture2D    spriteR,
               Texture2D    spriteL,
               int          frameWidth,
               int          frameHeight,
               float        scale)
{
    memset(e, 0, sizeof(Enemy));

    // ── Position / physics ────────────────────────────────────────────────────
    e->position     = spawnPos;
    e->patrolSpeed  = 80.0f;
    e->chaseSpeed   = 130.0f;

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

    // ── Look-around defaults ──────────────────────────────────────────────────
    e->lookAroundTimer = 0.0f;
    e->lookBaseAngle   = 0.0f;
    e->lookDir         = 1;

    e->searchTimer     = 0.0f;
    e->searchBaseAngle = 0.0f;

    // ── Sprite & animation ────────────────────────────────────────────────────
    e->frameWidth  = (frameWidth  > 0) ? frameWidth  : 32;
    e->frameHeight = (frameHeight > 0) ? frameHeight : 32;
    e->scale       = (scale       > 0) ? scale       : 0.5f;
    e->frameRec    = (Rectangle){ 0, 0, (float)e->frameWidth, (float)e->frameHeight };

    if (spriteR.id != 0 && spriteL.id != 0) {
        e->spriteRight   = spriteR;
        e->spriteLeft    = spriteL;
        e->currentSprite = &e->spriteRight;
        e->hasSprite     = true;
    }

    // ── Animation clip definitions ────────────────────────────────────────────
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
void DamageEnemy(Enemy* e, float damage) {
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
        ResetAnim(&e->animHit);
        SpawnParticles(e, ORANGE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// StunEnemy — stun without damage (used in Act 1)
// ─────────────────────────────────────────────────────────────────────────────
void StunEnemy(Enemy* e) {
    if (e->state == ENEMY_DEAD) return;
    if (e->hitInvincibleTimer > 0.0f) return;

    e->hitInvincibleTimer = 0.5f;
    e->state         = ENEMY_HIT;
    e->hitStateTimer = 0.35f;
    e->flashTimer    = 0.35f;
    e->hitPauseTimer = 0.06f;
    ResetAnim(&e->animHit);
    SpawnParticles(e, YELLOW);  // Yellow particles to distinguish stun from damage
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateEnemy
// ─────────────────────────────────────────────────────────────────────────────
void UpdateEnemy(Enemy* e, Vector2 playerPos, float dt, bool* outShake,
                 Rectangle* colliders, int colliderCount,
                 Vector2* navNodes, int navNodeCount) {
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
            if (playerVisible) {
                e->state = ENEMY_CHASE;
                e->navPathActive = false; // cancel any active nav path
            }
            break;

        case ENEMY_CHASE:
            // Track last known player position every frame while chasing
            e->lastKnownPlayerPos = playerPos;
            if (!playerVisible || distToPlayer > e->loseRange) {
                // Lost the player → enter SEARCH state
                e->state = ENEMY_SEARCH;
                e->searchTimer = 2.0f; // look around for 2 seconds
                e->searchBaseAngle = e->facingAngleDeg;
                e->lookDir = 1;
            }
            break;

        case ENEMY_SEARCH:
            // If we spot the player again during search, go back to chasing
            if (playerVisible) {
                e->state = ENEMY_CHASE;
            }
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

            // --- Following a nav-node path (returning from search) ---
            if (e->navPathActive && e->navPathCount > 0) {
                Vector2 navTarget = e->navPath[e->navPathCurrent];
                if (MoveToward(e, navTarget, e->patrolSpeed, dt, colliders, colliderCount)) {
                    // Arrived at this nav node — advance to next
                    e->navPathCurrent++;
                    if (e->navPathCurrent >= e->navPathCount) {
                        // Reached the end of the nav path
                        e->navPathActive = false;
                        e->waypointIndex = NearestWaypoint(e);
                    }
                }
                TickClip(e, &e->clipWalk, &e->animWalk, FacingRow(e), dt);
                break;
            }

            // --- Normal waypoint patrol ---
            // If waiting at waypoint, count down AND look around
            if (e->waitTimer > 0.0f) {
                e->waitTimer -= dt;

                // ── Look-around sweep: rotate ±90° from arrival heading ──
                float sweepSpeed = 67.0f; // degrees per second (slow, natural)
                float sweepRange = 90.0f; // degrees from base in each direction

                e->facingAngleDeg += (float)e->lookDir * sweepSpeed * dt;

                // Compute angular difference from the base angle
                float angleDiff = e->facingAngleDeg - e->lookBaseAngle;
                while (angleDiff >  180.0f) angleDiff -= 360.0f;
                while (angleDiff < -180.0f) angleDiff += 360.0f;

                if (angleDiff >= sweepRange) {
                    e->facingAngleDeg = e->lookBaseAngle + sweepRange;
                    e->lookDir = -1; // reverse toward left
                } else if (angleDiff <= -sweepRange) {
                    e->facingAngleDeg = e->lookBaseAngle - sweepRange;
                    e->lookDir = 1;  // reverse toward right
                }

                TickClip(e, &e->clipIdle, &e->animIdle, FacingRow(e), dt);
                break;
            }

            // Ping-pong between waypoints
            Vector2 target = e->waypoints[e->waypointIndex];
            if (MoveToward(e, target, e->patrolSpeed, dt, colliders, colliderCount)) {
                // Arrived at waypoint — store arrival heading and pause
                e->lookBaseAngle = e->facingAngleDeg;
                e->lookDir = 1;  // start by sweeping right from arrival heading
                e->waitTimer = 6.0f;  // 5 seconds of looking around

                // Only advance waypoint if there are multiple stops
                if (e->waypointCount > 1) {
                    e->waypointIndex += e->patrolDir;
                    if (e->waypointIndex >= e->waypointCount) {
                        e->waypointIndex = e->waypointCount - 2; // Bounce back
                        e->patrolDir     = -1;
                    } else if (e->waypointIndex < 0) {
                        e->waypointIndex = 1;
                        e->patrolDir     = 1;
                    }
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

        case ENEMY_SEARCH: {
            // ── Look-around sweep (~2 seconds), then go back to patrol ──
            e->searchTimer -= dt;

            // Sweep the vision cone ±60° around the last known heading
            float sweepSpeed = 120.0f; // degrees per second (faster than patrol)
            float sweepRange = 60.0f;

            e->facingAngleDeg += (float)e->lookDir * sweepSpeed * dt;

            float angleDiff = e->facingAngleDeg - e->searchBaseAngle;
            while (angleDiff >  180.0f) angleDiff -= 360.0f;
            while (angleDiff < -180.0f) angleDiff += 360.0f;

            if (angleDiff > sweepRange) {
                e->facingAngleDeg = e->searchBaseAngle + sweepRange;
                e->lookDir = -1;
            } else if (angleDiff < -sweepRange) {
                e->facingAngleDeg = e->searchBaseAngle - sweepRange;
                e->lookDir = 1;
            }

            TickClip(e, &e->clipIdle, &e->animIdle, FacingRow(e), dt);

            if (e->searchTimer <= 0.0f) {
                // Search is over — build a nav-node path back to nearest waypoint
                e->state = ENEMY_PATROL;
                e->waypointIndex = NearestWaypoint(e);
                BuildNavPath(e, e->waypoints[e->waypointIndex], navNodes, navNodeCount, colliders, colliderCount);
            }
            break;
        }

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
        Color base = (e->state == ENEMY_PATROL)  ? (Color){60, 120, 220, 255}  :  // Blue
                     (e->state == ENEMY_CHASE)   ? (Color){220, 80, 30, 255}   :  // Orange
                     (e->state == ENEMY_SEARCH)  ? (Color){220, 200, 30, 255}  :  // Yellow (searching)
                     (e->state == ENEMY_HIT)     ? (Color){255, 50, 50, 255}   :  // Red
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
        Color coneColor  = (e->state == ENEMY_CHASE)  ? Fade(ORANGE, 0.20f)
                         : (e->state == ENEMY_SEARCH) ? Fade(YELLOW, 0.25f)
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
    // Textures are shared (loaded once, passed in) — do NOT unload them here.
    // The caller (e.g. SetupAct) is responsible for unloading shared textures.
    e->hasSprite = false;
}

#include "character.h" //Code Written by: Josh Weinrich
#include "setting.h"
#include <stdbool.h>
#include <math.h>     // For powf, sqrtf
#include "raymath.h"  // For Vector2 math
#include <stddef.h>

// ─────────────────────────────────────────────────────────────────────────────
// InitCharacter
//   Sets all player fields to their starting values, loads the sprite sheet,
//   and zeroes out dash / stamina / health / trail state.
// ─────────────────────────────────────────────────────────────────────────────
void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows){
    // Place the player at the requested spawn point
    player->position = (Vector2){ (float)startX, (float)startY };
    player->velocity = (Vector2){ 0.0f, 0.0f };  // Start with no movement

    // Movement tuning values
    player->maxSpeed     = 175.0f;   // Pixels per second cap
    player->acceleration = 500.0f;  // How snappily the player reaches max speed
    player->friction     = 500.0f;  // How quickly the player stops when no key is held
    player->scale        = 1.2f;     // Scale sprite up 1.5× from its native 32×32 size

    // Load sprite sheet from disk and store the column / row counts
    player->sprite = LoadTexture(spritePath);
    player->frames = cols;
    player->rows   = rows;

    // Start the frame rectangle at the top-left cell of the sprite sheet (frame 0, row 0)
    player->frameRec.x      = 0.0f;
    player->frameRec.y      = 0.0f;
    player->frameRec.width  = 32.0f; // Each frame is 32 px wide
    player->frameRec.height = 32.0f; // Each frame is 32 px tall

    player->currentFrame   = 0;  // Begin on the first animation frame
    player->framesCounter  = 0;  // No elapsed ticks yet
    player->framesSpeed    = 6;  // Advance one animation frame every 60/6 = 10 game frames
    player->movingDirection = 0; // Default facing: right (row 0)

    // Dash Variables Init
    player->isDashing      = false;           // Not dashing at start
    player->dashDir        = (Vector2){0.0f, 0.0f}; // No dash direction yet
    player->dashSpeed      = 500.0f;         // Pixels per second at the peak of a dash
    player->dashDuration   = 0.25f;           // Dash lasts 0.25 seconds
    player->dashTime       = 0.0f;            // No time remaining on a dash
    player->dashProgress   = 0.0f;            // 0 = start of dash, 1 = end of dash
    player->playerInvincible = false;         // Not invincible at start

    // Stamina Variables Init
    player->maxStamina      = 100.0f;         // Full stamina pool
    player->stamina         = player->maxStamina; // Start full
    player->dashCost        = 34.0f;          // Each dash costs ~1/3 of max stamina
    player->staminaRegenRate = 30.0f;         // Recovers 30 stamina per second

    // Health Init
    player->maxHealth = 100.0f;
    player->health    = player->maxHealth; // Start at full health

    // Trail Init
    player->trailCount = 0; // No ghost frames exist yet

    // Damage recovery
    player->hitInvincibleTimer = 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdateCharacter
//   Called every frame. Handles (in order):
//     1. Stamina regeneration
//     2. Dash input & activation
//     3. Dash movement (with eased speed falloff)
//     4. Normal movement (acceleration + friction physics)
//     5. Axis-separated collision detection
//     6. Facing direction from mouse position
//     7. Sprite animation
// ─────────────────────────────────────────────────────────────────────────────
void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount, Vector2 mouseWorldPos, Audio* gameAudio){
    bool isMoving = false;        // Will be set true if the player is moving or dashing
    float dt = GetFrameTime();    // Delta time in seconds for frame-rate-independent math

    // ── 1. Stamina Regeneration ──────────────────────────────────────────────
    // Stamina only regenerates when the player is NOT actively dashing
    if (!player->isDashing) {
        player->stamina += player->staminaRegenRate * dt;
        // Clamp stamina to max so it never overflows
        if (player->stamina > player->maxStamina) {
            player->stamina = player->maxStamina;
        }
    }

    // ── Damage Recovery ──────────────────────────────────────────────────────
    if (player->hitInvincibleTimer > 0.0f) {
        player->hitInvincibleTimer -= dt;
    }

    // ── 2. Dash Activation ──────────────────────────────────────────────────
    // Trigger a dash when: Left Shift pressed AND enough stamina AND not already dashing
    if (IsKeyPressed(KEY_LEFT_SHIFT) && player->stamina >= player->dashCost && !player->isDashing) {
        // Read the movement keys to determine which direction to dash
        float inputDx = 0.0f;
        float inputDy = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputDx += 1.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) inputDx -= 1.0f;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) inputDy -= 1.0f;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) inputDy += 1.0f;

        // Only dash if the player is actually pressing a movement key
        if (inputDx != 0.0f || inputDy != 0.0f) {
            // Normalize the input vector so diagonal dashes aren't faster
            float length = sqrtf(inputDx*inputDx + inputDy*inputDy);
            player->dashDir = (Vector2){ inputDx / length, inputDy / length };

            player->isDashing       = true;
            player->dashTime        = player->dashDuration; // Start countdown
            player->dashProgress    = 0.0f;
            player->stamina        -= player->dashCost;     // Consume stamina
            player->playerInvincible = true;                // Grant invincibility frames
            player->trailCount      = 0;                    // Reset trail so old ghosts don't bleed into the new dash
        }
    }

    // ── 3 & 4. Movement (Dash OR Normal Physics) ────────────────────────────
    float dx = 0.0f; // Horizontal displacement this frame
    float dy = 0.0f; // Vertical   displacement this frame

    if (player->isDashing) {
        // Map elapsed dash time to a 0→1 progress value
        player->dashProgress = (player->dashDuration - player->dashTime) / player->dashDuration;

        // Ease the speed out: starts fast, slows to a stop using a quadratic curve
        float speedMultiplier = powf(1.0f - player->dashProgress, 2.0f);

        dx = player->dashDir.x * player->dashSpeed * speedMultiplier * dt;
        dy = player->dashDir.y * player->dashSpeed * speedMultiplier * dt;

        // Record a ghost snapshot for the dash trail every frame
        if (player->trailCount < MAX_DASH_TRAIL) {
            player->trail[player->trailCount].position = player->position;
            player->trail[player->trailCount].frameRec = player->frameRec;
            player->trail[player->trailCount].alpha     = 0.6f; // Start semi-transparent
            player->trailCount++;
        }

        // Tick the dash timer down; end the dash when time runs out
        player->dashTime -= dt;
        if (player->dashTime <= 0.0f) {
            player->isDashing        = false;
            player->playerInvincible = false; // Invincibility ends with the dash
        }
    } else {
        // ── Normal movement physics (Acceleration and Friction) ──────────────
        float inputDx = 0.0f;
        float inputDy = 0.0f;

        // Gather directional input (supports both arrow keys and WASD)
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputDx += 1.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) inputDx -= 1.0f;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) inputDy -= 1.0f;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) inputDy += 1.0f;

        if (inputDx != 0.0f || inputDy != 0.0f) {
            // Normalize so diagonal movement isn't sqrt(2)× faster
            float inputLen = sqrtf(inputDx*inputDx + inputDy*inputDy);
            inputDx /= inputLen;
            inputDy /= inputLen;

            // Apply acceleration in the input direction
            player->velocity.x += inputDx * player->acceleration * dt;
            player->velocity.y += inputDy * player->acceleration * dt;

            // Cap velocity magnitude at maxSpeed
            float currentSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
            if (currentSpeed > player->maxSpeed) {
                // Scale velocity down to maxSpeed while preserving direction
                player->velocity.x = (player->velocity.x / currentSpeed) * player->maxSpeed;
                player->velocity.y = (player->velocity.y / currentSpeed) * player->maxSpeed;
            }
        } else {
            // No input: Apply friction to bleed off velocity until stopped
            float currentSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
            if (currentSpeed > 0.0f) {
                float drop     = player->friction * dt; // How much speed to remove this frame
                float newSpeed = currentSpeed - drop;
                if (newSpeed < 0.0f) newSpeed = 0.0f;  // Don't reverse direction
                // Scale velocity down to newSpeed while preserving direction
                player->velocity.x = (player->velocity.x / currentSpeed) * newSpeed;
                player->velocity.y = (player->velocity.y / currentSpeed) * newSpeed;
            }
        }

        // Convert velocity (px/s) to a per-frame displacement
        dx = player->velocity.x * dt;
        dy = player->velocity.y * dt;
    }

    // Candidate new position before collision resolution
    float newX = player->position.x + dx;
    float newY = player->position.y + dy;

    // ── Determine animation trigger ──────────────────────────────────────────
    // Use velocity magnitude (not input) so the walk animation stops cleanly
    float velSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
    if (velSpeed > 5.0f || player->isDashing) {
        isMoving = true; // Threshold avoids flickering animation at near-zero speed
    }

    // ── 6. Facing Direction (mouse-driven) ───────────────────────────────────
    // Calculate vector from the player's centre to the mouse cursor in world space
    float dxMouse = mouseWorldPos.x - (player->position.x + (player->frameRec.width  * player->scale) / 2.0f);
    float dyMouse = mouseWorldPos.y - (player->position.y + (player->frameRec.height * player->scale) / 2.0f);

    // Pick the dominant axis to decide which sprite row (direction) to use
    if (fabs(dxMouse) > fabs(dyMouse)) {
        if (dxMouse > 0) player->movingDirection = 0; // Look Right
        else             player->movingDirection = 1; // Look Left
    } else {
        if (dyMouse < 0) player->movingDirection = 2; // Look Up
        else             player->movingDirection = 3; // Look Down
    }

    int renderRow = player->movingDirection;
    if (renderRow >= player->rows) {
        // Fallback: if the sprite sheet doesn't have 4 direction rows yet, use Left/Right only
        renderRow = (dxMouse > 0) ? 0 : 1;
    }

    // ── 5. Axis-Separated Collision Detection ────────────────────────────────
    // Recalculate frame size (in case it was changed elsewhere)
    player->frameRec.width  = 32.0f;
    player->frameRec.height = 32.0f;

    float scaledWidth  = player->frameRec.width  * player->scale;
    float scaledHeight = player->frameRec.height * player->scale;

    // Use a slightly smaller hitbox (67% of sprite size) so the player can
    // squeeze past corners more comfortably; anchored to the bottom-centre of the sprite
    float hitboxWidth   = scaledWidth  * 0.67f;
    float hitboxHeight  = scaledHeight * 0.67f;
    float hitboxOffsetX = (scaledWidth  - hitboxWidth)  / 2.0f; // Centre horizontally
    float hitboxOffsetY =  scaledHeight - hitboxHeight;          // Anchor to bottom half

    // Test X axis first: only apply horizontal movement if no collision
    Rectangle playerXRec = { newX + hitboxOffsetX, player->position.y + hitboxOffsetY, hitboxWidth, hitboxHeight };
    bool collisionX = false;
    for (int i=0; i < colliderCount; i++) {
        if (CheckCollisionRecs(playerXRec, colliders[i])) { collisionX = true; break; }
    }
    if (!collisionX) player->position.x = newX; // Safe to move horizontally

    // Test Y axis separately: only apply vertical movement if no collision
    Rectangle playerYRec = { player->position.x + hitboxOffsetX, newY + hitboxOffsetY, hitboxWidth, hitboxHeight };
    bool collisionY = false;
    for (int i=0; i < colliderCount; i++) {
        if (CheckCollisionRecs(playerYRec, colliders[i])) { collisionY = true; break; }
    }
    if (!collisionY) player->position.y = newY; // Safe to move vertically

    // ── 7. Sprite Animation ──────────────────────────────────────────────────
    if (isMoving) {
        player->framesCounter++; // Count game ticks

        // Advance to the next sprite column once enough ticks have accumulated
        if (player->framesCounter >= (60 / player->framesSpeed)) {
            player->framesCounter = 0;
            player->currentFrame++;

            // Loop animation back to the first frame after the last column
            if (player->currentFrame >= player->frames) player->currentFrame = 0;

            // Play a footstep sound every 2th animation frame to sync with the walk cycle
            if (gameAudio != NULL && (player->currentFrame % 3 == 0)) {
                PlayRandomFootstep(gameAudio);
            }

            // Advance the source rect X to the next column in the sprite sheet
            player->frameRec.x = (float)player->currentFrame * player->frameRec.width;
        }
    }
    else {
        // Standing still: reset to the first (idle) frame
        player->framesCounter = 0;
        player->currentFrame  = 0;
        player->frameRec.x    = 0; // First column = idle pose
    }

    // Set the sprite sheet row based on the resolved facing direction
    player->frameRec.y = (float)renderRow * player->frameRec.height;
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawCharacter
//   Renders (in order):
//     1. Dash ghost trail beneath the player
//     2. The player sprite at the current animation frame
//     3. The stamina bar above the player (only visible when stamina is not full)
// ─────────────────────────────────────────────────────────────────────────────
void DrawCharacter(Character* player){
    // ── 1. Render Ghost Dash Trails beneath player ───────────────────────────
    for (int i = 0; i < player->trailCount; i++) {
        // Build a destination rect matching the ghost's world position and scaled size
        Rectangle dest = {
            player->trail[i].position.x,
            player->trail[i].position.y,
            player->trail[i].frameRec.width  * player->scale,
            player->trail[i].frameRec.height * player->scale
        };
        // Draw the ghost tinted SKYBLUE with its current alpha
        DrawTexturePro(player->sprite, player->trail[i].frameRec, dest, (Vector2){0,0}, 0.0f, Fade(SKYBLUE, player->trail[i].alpha));

        // Fade each ghost out over time (1.5 alpha units per second)
        player->trail[i].alpha -= GetFrameTime() * 1.5f;
    }

    // Remove the oldest ghost (index 0) once it has fully faded out.
    // Shift all remaining ghosts one position earlier to keep the array tightly packed.
    if (player->trailCount > 0 && player->trail[0].alpha <= 0.0f) {
        for (int i=1; i < player->trailCount; i++) {
            player->trail[i-1] = player->trail[i]; // Shift them over
        }
        player->trailCount--; // One fewer valid ghost now
    }

    // ── 2. Render actual Player ──────────────────────────────────────────────
    // Destination rect scales the 32×32 source frame up by player->scale
    Rectangle destRec = {
        player->position.x,
        player->position.y,
        player->frameRec.width  * player->scale,
        player->frameRec.height * player->scale
    };
    // Draw with no rotation, no origin offset, and full WHITE tint (unmodified colors)
    DrawTexturePro(player->sprite, player->frameRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

    // ── 3. Draw Stamina bar ONLY when not full ───────────────────────────────
    // Hiding it at max stamina keeps the screen clean during normal play
    if (player->stamina < player->maxStamina) {
        float centerX       = player->position.x + (player->frameRec.width * player->scale) / 2.0f;
        float bottomY       = player->position.y + (player->frameRec.height * player->scale);
        float barWidth      = 40.0f;
        float barHeight     = 4.0f;
        float fillPercentage = player->stamina / player->maxStamina; // 0.0 = empty, 1.0 = full

        // Dark background track
        DrawRectangle(centerX - barWidth/2.0f, bottomY + 5.0f, barWidth, barHeight, Fade(DARKGRAY, 0.6f));
        // Green fill proportional to remaining stamina
        DrawRectangle(centerX - barWidth/2.0f, bottomY + 5.0f, barWidth * fillPercentage, barHeight, Fade(LIME, 0.8f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawCharacterHUD
//   Draws the HUD overlay in screen-space (call outside any camera transform):
//     - Portrait: small sprite thumbnail in the bottom-left corner
//     - Health bar: colour-coded bar next to the portrait
// ─────────────────────────────────────────────────────────────────────────────
void DrawCharacterHUD(Character* player, Texture2D portrait) {
    int screenH = VIRTUAL_HEIGHT; // Anchor HUD to the virtual canvas height

    // ── Portrait Layout ──────────────────────────────────────────────────────
    int portraitSize = 60;                                  // Portrait box is 60×60 px
    int padding      = 16;                                  // Margin from screen edges
    int portraitX    = padding;
    int portraitY    = screenH - portraitSize - padding - 30; // Anchored near the bottom-left

    // Semi-transparent dark background makes the portrait readable on any backdrop
    DrawRectangle(portraitX - 4, portraitY - 4, portraitSize + 8, portraitSize + 8, Fade(BLACK, 0.7f));

    // Crop out only the first frame (top-left cell) of the portrait sheet to display
    Rectangle srcRec = { 0.0f, 0.0f, (float)portrait.width / player->frames, (float)portrait.height / player->rows };
    Rectangle dstRec = { (float)portraitX, (float)portraitY, (float)portraitSize, (float)portraitSize };
    DrawTexturePro(portrait, srcRec, dstRec, (Vector2){0, 0}, 0.0f, WHITE);

    // White border around the portrait for a clean, framed look
    DrawRectangleLinesEx((Rectangle){portraitX - 4, portraitY - 4, portraitSize + 8, portraitSize + 8}, 2, WHITE);

    // ── Health Bar ───────────────────────────────────────────────────────────
    int   barX = portraitX + portraitSize + 8 + 4; // Positioned just right of the portrait
    int   barY = portraitY + portraitSize / 2 - 6; // Vertically centred on the portrait
    int   barW = 150;
    int   barH = 14;
    float healthPct = player->health / player->maxHealth; // 0.0 = dead, 1.0 = full

    // Dark background track for the health bar
    DrawRectangle(barX, barY, barW, barH, Fade(DARKGRAY, 0.8f));

    // Colour-code the fill: green above 50%, yellow 25-50%, red below 25%
    Color healthColor = (healthPct > 0.5f)  ? (Color){34, 197, 94, 220}  :  // Green
                        (healthPct > 0.25f) ? (Color){234, 179, 8, 220}  :  // Yellow
                                              (Color){220, 38, 38, 220};     // Red
    DrawRectangle(barX, barY, (int)(barW * healthPct), barH, healthColor);

    // Subtle white border around the health bar
    DrawRectangleLinesEx((Rectangle){barX, barY, barW, barH}, 1, Fade(WHITE, 0.5f));

    // Numeric HP text ("current / max") drawn inside the bar
    DrawText(TextFormat("%d / %d", (int)player->health, (int)player->maxHealth), barX + 4, barY + 1, 10, WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
// UnloadCharacter
//   Frees GPU memory held by the sprite texture.
//   Call this when the game closes or the character is no longer needed.
// ─────────────────────────────────────────────────────────────────────────────
void UnloadCharacter(Character* player){
    UnloadTexture(player->sprite);
}

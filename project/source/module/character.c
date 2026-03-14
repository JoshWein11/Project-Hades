#include "character.h"
#include <stdbool.h>
#include <math.h>     // For powf, sqrtf
#include "raymath.h"  // For Vector2 math
#include <stddef.h>

void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows){
    player->position = (Vector2){ (float)startX, (float)startY };
    player->velocity = (Vector2){ 0.0f, 0.0f };
    player->maxSpeed = 300.0f;
    player->acceleration = 2000.0f;
    player->friction = 1400.0f;
    player->scale = 1.5f;

    // Load sprite
    player->sprite = LoadTexture(spritePath);
    player->frames = cols;
    player->rows = rows;

    // Setup initial frame slice
    player->frameRec.x = 0.0f;
    player->frameRec.y = 0.0f;
    player->frameRec.width = 32.0f;
    player->frameRec.height = 32.0f;

    player->currentFrame = 0;
    player->framesCounter = 0;
    player->framesSpeed = 8;
    player->movingDirection = 0; 

    // Dash Variables Init
    player->isDashing = false;
    player->dashDir = (Vector2){0.0f, 0.0f};
    player->dashSpeed = 1600.0f; 
    player->dashDuration = 0.25f;
    player->dashTime = 0.0f;
    player->dashProgress = 0.0f;
    player->playerInvincible = false;

    // Stamina Variables Init
    player->maxStamina = 100.0f;
    player->stamina = player->maxStamina;
    player->dashCost = 34.0f;
    player->staminaRegenRate = 30.0f;
    
    // Health Init
    player->maxHealth = 100.0f;
    player->health = player->maxHealth;
    
    // Trail Init
    player->trailCount = 0;
}

void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount, Vector2 mouseWorldPos, Audio* gameAudio){
    bool isMoving = false;
    float dt = GetFrameTime();

    // Stamina Regeneration (Only when not dashing)
    if (!player->isDashing) {
        player->stamina += player->staminaRegenRate * dt;
        if (player->stamina > player->maxStamina) {
            player->stamina = player->maxStamina;
        }
    }

    // Dash start logic - now checks and consumes Stamina instead of waiting for cooldown
    if (IsKeyPressed(KEY_LEFT_SHIFT) && player->stamina >= player->dashCost && !player->isDashing) {
        float inputDx = 0.0f;
        float inputDy = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputDx += 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) inputDx -= 1.0f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) inputDy -= 1.0f;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) inputDy += 1.0f;

        // Only dash if moving
        if (inputDx != 0.0f || inputDy != 0.0f) {
            float length = sqrtf(inputDx*inputDx + inputDy*inputDy);
            player->dashDir = (Vector2){ inputDx / length, inputDy / length };
            
            player->isDashing = true;
            player->dashTime = player->dashDuration;
            player->dashProgress = 0.0f;
            player->stamina -= player->dashCost; // Consume stamina
            player->playerInvincible = true;
            player->trailCount = 0; // Reset trail length for new dash
        }
    }

    // Movement checks
    float dx = 0.0f;
    float dy = 0.0f;

    if (player->isDashing) {
        player->dashProgress = (player->dashDuration - player->dashTime) / player->dashDuration;
        float speedMultiplier = powf(1.0f - player->dashProgress, 2.0f);
        
        dx = player->dashDir.x * player->dashSpeed * speedMultiplier * dt;
        dy = player->dashDir.y * player->dashSpeed * speedMultiplier * dt;

        // Record dash trail ghost effect every frame
        if (player->trailCount < MAX_DASH_TRAIL) {
            player->trail[player->trailCount].position = player->position;
            player->trail[player->trailCount].frameRec = player->frameRec;
            player->trail[player->trailCount].alpha = 0.6f;
            player->trailCount++;
        }

        player->dashTime -= dt;
        if (player->dashTime <= 0.0f) {
            player->isDashing = false;
            player->playerInvincible = false;
        }
    } else {
        // Normal movement physics (Acceleration and Friction)
        float inputDx = 0.0f;
        float inputDy = 0.0f;

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputDx += 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) inputDx -= 1.0f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) inputDy -= 1.0f;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) inputDy += 1.0f;

        if (inputDx != 0.0f || inputDy != 0.0f) {
            // Normalize input
            float inputLen = sqrtf(inputDx*inputDx + inputDy*inputDy);
            inputDx /= inputLen;
            inputDy /= inputLen;

            // Apply Acceleration vector
            player->velocity.x += inputDx * player->acceleration * dt;
            player->velocity.y += inputDy * player->acceleration * dt;

            // Cap velocity at maxSpeed
            float currentSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
            if (currentSpeed > player->maxSpeed) {
                player->velocity.x = (player->velocity.x / currentSpeed) * player->maxSpeed;
                player->velocity.y = (player->velocity.y / currentSpeed) * player->maxSpeed;
            }
        } else {
            // No input: Apply Friction
            float currentSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
            if (currentSpeed > 0.0f) {
                float drop = player->friction * dt;
                float newSpeed = currentSpeed - drop;
                if (newSpeed < 0.0f) newSpeed = 0.0f;
                player->velocity.x = (player->velocity.x / currentSpeed) * newSpeed;
                player->velocity.y = (player->velocity.y / currentSpeed) * newSpeed;
            }
        }

        dx = player->velocity.x * dt;
        dy = player->velocity.y * dt;
    }

    float newX = player->position.x + dx;
    float newY = player->position.y + dy;

    // Check if player has velocity for animation trigger
    float velSpeed = sqrtf(player->velocity.x*player->velocity.x + player->velocity.y*player->velocity.y);
    if (velSpeed > 5.0f || player->isDashing) {
        isMoving = true;
    }

    // Determine aiming/facing direction via mouse position completely independent of movement
    float dxMouse = mouseWorldPos.x - (player->position.x + (player->frameRec.width * player->scale) / 2.0f);
    float dyMouse = mouseWorldPos.y - (player->position.y + (player->frameRec.height * player->scale) / 2.0f);

    if (fabs(dxMouse) > fabs(dyMouse)) {
        if (dxMouse > 0) player->movingDirection = 0; // Look Right
        else player->movingDirection = 1; // Look Left
    } else {
        if (dyMouse < 0) player->movingDirection = 2; // Look Up
        else player->movingDirection = 3; // Look Down
    }

    int renderRow = player->movingDirection;
    if (renderRow >= player->rows) {
        // Fallback to Left/Right if the SpriteSheet doesn't have 4 rows yet
        renderRow = (dxMouse > 0) ? 0 : 1;
    }

    // Recalculate frame dimension
    player->frameRec.width = 32.0f;
    player->frameRec.height = 32.0f;
    
    float scaledWidth = player->frameRec.width * player->scale;
    float scaledHeight = player->frameRec.height * player->scale;
    
    // Create a smaller hitbox (e.g. 50% width, 50% height anchored at the bottom center of the sprite)
    float hitboxWidth = scaledWidth * 0.85f;
    float hitboxHeight = scaledHeight * 0.85f;
    float hitboxOffsetX = (scaledWidth - hitboxWidth) / 2.0f;
    float hitboxOffsetY = scaledHeight - hitboxHeight; // Anchored to bottom half of character sprite

    // Test X collision
    Rectangle playerXRec = { newX + hitboxOffsetX, player->position.y + hitboxOffsetY, hitboxWidth, hitboxHeight };
    bool collisionX = false;
    for (int i=0; i < colliderCount; i++) {
        if (CheckCollisionRecs(playerXRec, colliders[i])) { collisionX = true; break; }
    }
    if (!collisionX) player->position.x = newX;

    // Test Y collision
    Rectangle playerYRec = { player->position.x + hitboxOffsetX, newY + hitboxOffsetY, hitboxWidth, hitboxHeight };
    bool collisionY = false;
    for (int i=0; i < colliderCount; i++) {
        if (CheckCollisionRecs(playerYRec, colliders[i])) { collisionY = true; break; }
    }
    if (!collisionY) player->position.y = newY;

    // ANIMATION LOGIC
    if (isMoving) {
        player->framesCounter++;

        if (player->framesCounter >= (60 / player->framesSpeed)) {
            player->framesCounter = 0;
            player->currentFrame++;

            // Loop back to start if we exceed the texture's column count
            if (player->currentFrame >= player->frames) player->currentFrame = 0;

            // Trigger footstep every 3 frame
            if (gameAudio != NULL && (player->currentFrame % 4 == 0)) {
                PlayRandomFootstep(gameAudio);
            }

            player->frameRec.x = (float)player->currentFrame * player->frameRec.width;
        }
    } 
    else {
        // Standing still
        player->framesCounter = 0;
        // Optionally rest to a specific idle frame, e.g., currentFrame = 0
        player->currentFrame = 0;
        player->frameRec.x = 0; 
    }

    player->frameRec.y = (float)renderRow * player->frameRec.height;
}

void DrawCharacter(Character* player){
    // 1. Render Ghost Dash Trails beneath player
    for (int i = 0; i < player->trailCount; i++) {
        Rectangle dest = {
            player->trail[i].position.x,
            player->trail[i].position.y,
            player->trail[i].frameRec.width * player->scale,
            player->trail[i].frameRec.height * player->scale
        };
        DrawTexturePro(player->sprite, player->trail[i].frameRec, dest, (Vector2){0,0}, 0.0f, Fade(SKYBLUE, player->trail[i].alpha));
        
        // Decay the alpha so the trail fades over time
        player->trail[i].alpha -= GetFrameTime() * 1.5f; 
    }
    
    // Cleanup invisible trail pieces from array by keeping track of the count
    if (player->trailCount > 0 && player->trail[0].alpha <= 0.0f) {
        for (int i=1; i < player->trailCount; i++) {
            player->trail[i-1] = player->trail[i]; // Shift them over
        }
        player->trailCount--;
    }

    // 2. Render actual Player
    Rectangle destRec = {
        player->position.x,
        player->position.y,
        player->frameRec.width * player->scale,
        player->frameRec.height * player->scale
    };
    
    DrawTexturePro(player->sprite, player->frameRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

    // 3. Draw Stamina bar ONLY when not full (so it's hidden at max stamina)
    if (player->stamina < player->maxStamina) {
        float centerX = player->position.x + (player->frameRec.width * player->scale) / 2.0f;
        float bottomY = player->position.y + (player->frameRec.height * player->scale);
        float barWidth = 40.0f;
        float barHeight = 4.0f;
        float fillPercentage = player->stamina / player->maxStamina;
        DrawRectangle(centerX - barWidth/2.0f, bottomY + 5.0f, barWidth, barHeight, Fade(DARKGRAY, 0.6f));
        DrawRectangle(centerX - barWidth/2.0f, bottomY + 5.0f, barWidth * fillPercentage, barHeight, Fade(LIME, 0.8f));
    }
}

void DrawCharacterHUD(Character* player, Texture2D portrait) {
    int screenH = GetScreenHeight();

    // --- Portrait Frame ---
    int portraitSize = 60;
    int padding = 16;
    int portraitX = padding;
    int portraitY = screenH - portraitSize - padding - 30;

    // Dark background for portrait
    DrawRectangle(portraitX - 4, portraitY - 4, portraitSize + 8, portraitSize + 8, Fade(BLACK, 0.7f));
    // Draw the portrait sprite (first frame of sprite sheet, facing right)
    Rectangle srcRec = { 0.0f, 0.0f, (float)portrait.width / player->frames, (float)portrait.height / player->rows };
    Rectangle dstRec = { (float)portraitX, (float)portraitY, (float)portraitSize, (float)portraitSize };
    DrawTexturePro(portrait, srcRec, dstRec, (Vector2){0, 0}, 0.0f, WHITE);
    // White border around portrait
    DrawRectangleLinesEx((Rectangle){portraitX - 4, portraitY - 4, portraitSize + 8, portraitSize + 8}, 2, WHITE);

    // --- Health Bar ---
    int barX = portraitX + portraitSize + 8 + 4;
    int barY = portraitY + portraitSize / 2 - 6;
    int barW = 150;
    int barH = 14;
    float healthPct = player->health / player->maxHealth;

    // Background
    DrawRectangle(barX, barY, barW, barH, Fade(DARKGRAY, 0.8f));
    // Health fill (red grad: full = green-ish red, low = full red)
    Color healthColor = (healthPct > 0.5f) ? (Color){34, 197, 94, 220} : (healthPct > 0.25f) ? (Color){234, 179, 8, 220} : (Color){220, 38, 38, 220};
    DrawRectangle(barX, barY, (int)(barW * healthPct), barH, healthColor);
    // Border
    DrawRectangleLinesEx((Rectangle){barX, barY, barW, barH}, 1, Fade(WHITE, 0.5f));
    // HP text
    DrawText(TextFormat("%d / %d", (int)player->health, (int)player->maxHealth), barX + 4, barY + 1, 10, WHITE);
}

void UnloadCharacter(Character* player){
    UnloadTexture(player->sprite);
}

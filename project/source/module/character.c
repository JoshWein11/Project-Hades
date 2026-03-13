#include "character.h"
#include <stdbool.h>
#include <math.h>     // For powf, sqrtf
#include "raymath.h"  // For Vector2 math

void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows){
    player->position = (Vector2){ (float)startX, (float)startY };
    player->speed = (Vector2){ 3.0f, 3.0f };
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
    player->dashCooldown = 1.0f; 
    player->dashCooldownTimer = 0.0f;
    player->dashProgress = 0.0f;
    player->playerInvincible = false;
}

void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount){
    bool isMoving = false;
    float dt = GetFrameTime();

    // Dash cooldown
    if (player->dashCooldownTimer > 0.0f) {
        player->dashCooldownTimer -= dt;
    }

    // Dash start logic
    if (IsKeyPressed(KEY_LEFT_SHIFT) && player->dashCooldownTimer <= 0.0f && !player->isDashing) {
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
            player->dashCooldownTimer = player->dashCooldown;
            player->playerInvincible = true;
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

        player->dashTime -= dt;
        if (player->dashTime <= 0.0f) {
            player->isDashing = false;
            player->playerInvincible = false;
        }
    } else {
        // Normal movement
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += player->speed.x;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dx -= player->speed.x;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dy -= player->speed.y;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dy += player->speed.y;
    }

    float newX = player->position.x + dx;
    float newY = player->position.y + dy;

    if (dx > 0) player->movingDirection = 0; // Right
    else if (dx < 0) player->movingDirection = 1; // Left
    else if (dy < 0) player->movingDirection = 0; // Up
    else if (dy > 0) player->movingDirection = 1; // Down

    if (dx != 0.0f || dy != 0.0f) {
        isMoving = true;
    }

    int renderRow = player->movingDirection;
    if (renderRow >= player->rows) {
        renderRow = 0; // Fallback to first row
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

            player->frameRec.x = (float)player->currentFrame * player->frameRec.width;
        }
    } 
    else {
        // Standing still
        player->framesCounter = 0;
        // Optionally rest to a specific idle frame, e.g., currentFrame = 0
        player->currentFrame = 0;
        player->frameRec.x = 0; // Or leave it alone to stop on current frame: `player->frameRec.x = (float)player->currentFrame * player->frameRec.width;`
    }

    player->frameRec.y = (float)renderRow * player->frameRec.height;
}

void DrawCharacter(Character* player){
    Rectangle destRec = {
        player->position.x,
        player->position.y,
        player->frameRec.width * player->scale,
        player->frameRec.height * player->scale
    };
    
    Vector2 origin = {0.0f, 0.0f};
    DrawTexturePro(player->sprite, player->frameRec, destRec, origin, 0.0f, WHITE);
}

void UnloadCharacter(Character* player){
    UnloadTexture(player->sprite);
}
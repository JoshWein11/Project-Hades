#include "character.h"
#include <stdbool.h>

void InitCharacter(Character* player, int startX, int startY, const char* walkPath, int walkCols, const char* runPath, int runCols){
    player->position = (Vector2){ (float)startX, (float)startY };
    player->speed = (Vector2){ 2.0f, 2.0f };
    player->scale = 1.5f;

    // Load both sprites
    player->walkSprite = LoadTexture(walkPath);
    player->runSprite = LoadTexture(runPath);

    player->walkFrames = walkCols;
    player->runFrames = runCols;
    player->isRunning = false;

    // Setup initial frame slice (defaulting to walk)
    player->frameRec.x = 0.0f;
    player->frameRec.y = 0.0f;
    player->frameRec.width = (float)(player->walkSprite.width / player->walkFrames);
    player->frameRec.height = (float)(player->walkSprite.height / 4);

    player->currentFrame = 0;
    player->framesCounter = 0;
    player->framesSpeed = 8;
    player->movingDirection = 0; 
}

void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount){
    bool isMoving = false;

    // Detect if Shift is held
    player->isRunning = IsKeyDown(KEY_LEFT_SHIFT);

    // Adjust speed depending on state
    player->speed.x = player->isRunning ? 4.0f : 2.0f;
    player->speed.y = player->isRunning ? 4.0f : 2.0f;

    // Adjust animation playback speed
    player->framesSpeed = player->isRunning ? 12 : 8;

    // Movement checks
    float dx = 0.0f;
    float dy = 0.0f;

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += player->speed.x;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dx -= player->speed.x;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dy -= player->speed.y;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dy += player->speed.y;

    float newX = player->position.x + dx;
    float newY = player->position.y + dy;

    if (dx > 0) player->movingDirection = 2;
    else if (dx < 0) player->movingDirection = 1;
    else if (dy < 0) player->movingDirection = 3;
    else if (dy > 0) player->movingDirection = 0;

    if (dx != 0.0f || dy != 0.0f) {
        isMoving = true;
    }

    // Determine the active texture and frame count so we slice it correctly
    Texture2D activeTex = player->isRunning ? player->runSprite : player->walkSprite;
    int maxCols = player->isRunning ? player->runFrames : player->walkFrames;

    // Recalculate frame dimension, as run and walk images might be different sizes
    player->frameRec.width = (float)(activeTex.width / maxCols);
    player->frameRec.height = (float)(activeTex.height / 4);
    
    float scaledWidth = player->frameRec.width * player->scale;
    float scaledHeight = player->frameRec.height * player->scale;
    
    // Create a smaller hitbox (e.g. 50% width, 50% height anchored at the bottom center of the sprite)
    float hitboxWidth = scaledWidth * 0.35f;
    float hitboxHeight = scaledHeight * 0.35f;
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
            if (player->currentFrame >= maxCols) player->currentFrame = 0;

            player->frameRec.x = (float)player->currentFrame * player->frameRec.width;
        }
    } 
    else {
        // Standing still
        player->currentFrame = 0;
        player->framesCounter = 0;
        player->frameRec.x = 0;
    }

    player->frameRec.y = (float)player->movingDirection * player->frameRec.height;
}

void DrawCharacter(Character* player){
    Texture2D activeTex = player->isRunning ? player->runSprite : player->walkSprite;

    Rectangle destRec = {
        player->position.x,
        player->position.y,
        player->frameRec.width * player->scale,
        player->frameRec.height * player->scale
    };
    
    Vector2 origin = {0.0f, 0.0f};
    DrawTexturePro(activeTex, player->frameRec, destRec, origin, 0.0f, WHITE);
}

void UnloadCharacter(Character* player){
    UnloadTexture(player->walkSprite);
    UnloadTexture(player->runSprite);
}
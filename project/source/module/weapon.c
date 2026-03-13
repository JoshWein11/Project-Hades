#include "weapon.h"
#include "raymath.h"

void InitWeapon(Weapon* weapon) {
    weapon->shootCooldown = 0.2f; // Time between shots in seconds
    weapon->cooldownTimer = 0.0f;
    for (int i = 0; i < MAX_BULLETS; i++) {
        weapon->bullets[i].active = false;
        weapon->bullets[i].radius = 4.0f;
        weapon->bullets[i].speed = 500.0f; 
    }
}

void ShootWeapon(Weapon* weapon, Vector2 startPos, Vector2 targetPos) {
    if (weapon->cooldownTimer > 0.0f) {
        return; // Still cooling down
    }

    // Find first inactive bullet
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!weapon->bullets[i].active) {
            weapon->bullets[i].active = true;
            weapon->bullets[i].position = startPos;
            
            // Calculate bullet direction
            Vector2 direction = Vector2Subtract(targetPos, startPos);
            direction = Vector2Normalize(direction);
            
            weapon->bullets[i].velocity = direction;
            weapon->bullets[i].lifeTimer = 2.0f; // Bullet lives for 2 seconds before disappearing
            
            // Reset cooldown
            weapon->cooldownTimer = weapon->shootCooldown;
            break; // Only shoot one bullet
        }
    }
}

void UpdateWeapon(Weapon* weapon, float deltaTime, Rectangle* colliders, int colliderCount) {
    if (weapon->cooldownTimer > 0.0f) {
        weapon->cooldownTimer -= deltaTime;
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (weapon->bullets[i].active) {
            // Update life timer
            weapon->bullets[i].lifeTimer -= deltaTime;
            if (weapon->bullets[i].lifeTimer <= 0.0f) {
                weapon->bullets[i].active = false;
                continue;
            }

            // Move bullet
            Vector2 frameVelocity = Vector2Scale(weapon->bullets[i].velocity, weapon->bullets[i].speed * deltaTime);
            weapon->bullets[i].position = Vector2Add(weapon->bullets[i].position, frameVelocity);

            // Create a small collision rectangle for the bullet
            Rectangle bulletRec = {
                weapon->bullets[i].position.x - weapon->bullets[i].radius,
                weapon->bullets[i].position.y - weapon->bullets[i].radius,
                weapon->bullets[i].radius * 2,
                weapon->bullets[i].radius * 2
            };

            // Check wall collisions so bullets don't fly through buildings
            for (int r = 0; r < colliderCount; r++) {
                if (CheckCollisionRecs(bulletRec, colliders[r])) {
                    weapon->bullets[i].active = false; // Destroy bullet on hit
                    break;
                }
            }
        }
    }
}

void DrawWeapon(Weapon* weapon) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (weapon->bullets[i].active) {
            DrawRectangle(weapon->bullets[i].position.x, weapon->bullets[i].position.y, weapon->bullets[i].radius, weapon->bullets[i].radius, BLACK);
        }
    }
}

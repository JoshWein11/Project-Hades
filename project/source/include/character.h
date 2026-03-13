#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_DASH_TRAIL 15

typedef struct {
    Vector2 position;
    Rectangle frameRec;
    float alpha;
} DashFrame;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float maxSpeed;
    float acceleration;
    float friction;
    float scale;

    // Animation Fields
    Texture2D sprite;       // The walking sprite sheet
    int frames;             // Number of columns in sprite
    int rows;               // Number of rows in sprite
    
    Rectangle frameRec;     // The exact part of the sprite sheet to draw
    
    int currentFrame;       // Which column we are on
    int framesCounter;      // Counts elapsed frames to slow down animation
    int framesSpeed;        // How fast the animation plays
    int movingDirection;    // Which row to draw (e.g. 0=Right, 1=Left, 2=Up, 3=Down)

    // Dash Variables
    bool isDashing;
    Vector2 dashDir;
    float dashSpeed;
    float dashDuration;
    float dashTime;
    float dashProgress;

    // Stamina Variables
    float stamina;
    float maxStamina;
    float staminaRegenRate;
    float dashCost;

    // Health
    float health;
    float maxHealth;

    // Dash Trail
    DashFrame trail[MAX_DASH_TRAIL];
    int trailCount;

    // Dash Optional Invincibility
    bool playerInvincible;

} Character;

// Initialize character
void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows);

// Update logic and animations
void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount, Vector2 mouseWorldPos);

// Draw the correct frame from the active sprite sheet
void DrawCharacter(Character* player);

// Draw the HUD (health bar, portrait, stamina) to screen-space
void DrawCharacterHUD(Character* player, Texture2D portrait);

// Free up the RAM when the game closes
void UnloadCharacter(Character* player);

#endif

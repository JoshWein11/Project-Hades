#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Vector2 position;
    Vector2 speed;
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
    float dashCooldown;
    float dashCooldownTimer;
    float dashProgress;

    // Dash Optional Invincibility
    bool playerInvincible;

} Character;

// Initialize character
void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows);

// Update logic and animations
void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount);

// Draw the correct frame from the active sprite sheet
void DrawCharacter(Character* player);

// Free up the RAM when the game closes
void UnloadCharacter(Character* player);

#endif
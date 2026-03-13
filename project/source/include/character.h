#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Vector2 position;
    Vector2 speed;
    float scale;

    // Animation Fields
    Texture2D walkSprite;   // The walking sprite sheet
    Texture2D runSprite;    // The running sprite sheet
    int walkFrames;         // Number of columns in walk sprite
    int runFrames;          // Number of columns in run sprite
    
    Rectangle frameRec;     // The exact part of the sprite sheet to draw
    
    int currentFrame;       // Which column we are on
    int framesCounter;      // Counts elapsed frames to slow down animation
    int framesSpeed;        // How fast the animation plays
    int movingDirection;    // Which row to draw (e.g. 0=Down, 1=Up, 2=Right, 3=Left)
    bool isRunning;         // True if the player is holding SHIFT

} Character;

// Initialize character and load both sprites
void InitCharacter(Character* player, int startX, int startY, const char* walkPath, int walkCols, const char* runPath, int runCols);

// Update logic and animations
void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount);

// Draw the correct frame from the active sprite sheet
void DrawCharacter(Character* player);

// Free up the RAM when the game closes
void UnloadCharacter(Character* player);

#endif
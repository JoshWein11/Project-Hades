#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include <stdbool.h>
#include "audio.h"

// Maximum number of ghost frames stored for the dash trail effect
#define MAX_DASH_TRAIL 15

// Stores a single snapshot of the player's position and sprite frame for a dash trail ghost
typedef struct {
    Vector2 position;   // World position of this ghost frame
    Rectangle frameRec; // Sprite frame region to draw for this ghost
    float alpha;        // Transparency; fades to 0 over time so the trail disappears
} DashFrame;

// Holds all state for the player character
typedef struct {
    Vector2 position;       // World position (top-left of the sprite)
    Vector2 velocity;       // Current movement velocity (pixels per second)
    float maxSpeed;         // Maximum movement speed cap
    float acceleration;     // How quickly the player speeds up when a key is held
    float friction;         // How quickly the player slows down when no key is held
    float scale;            // Render scale multiplier applied to the sprite

    // Animation Fields
    Texture2D sprite;       // The walking sprite sheet
    int frames;             // Number of columns in sprite (animation frames per row)
    int rows;               // Number of rows in sprite (one row per facing direction)
    
    Rectangle frameRec;     // The exact sub-rectangle of the sprite sheet to draw this tick
    
    int currentFrame;       // Which column (animation frame) we are currently on
    int framesCounter;      // Counts elapsed game frames to throttle animation speed
    int framesSpeed;        // Animation playback speed (frames per second equivalent)
    int movingDirection;    // Which sprite row to use: 0=Right, 1=Left, 2=Up, 3=Down

    // Dash Variables
    bool isDashing;         // True while a dash is actively in progress
    Vector2 dashDir;        // Normalized direction the dash is travelling
    float dashSpeed;        // Peak speed at the start of a dash (pixels per second)
    float dashDuration;     // Total time (seconds) a dash lasts
    float dashTime;         // Remaining dash time; counts down to 0
    float dashProgress;     // 0→1 progress of the dash, used to ease speed out

    // Stamina Variables
    float stamina;          // Current stamina points
    float maxStamina;       // Maximum stamina the player can hold
    float staminaRegenRate; // Stamina recovered per second (when not dashing)
    float dashCost;         // Stamina consumed per dash

    // Health
    float health;           // Current health points
    float maxHealth;        // Maximum health the player can have

    // Dash Trail
    DashFrame trail[MAX_DASH_TRAIL]; // Ring buffer of ghost frames drawn behind the player during a dash
    int trailCount;                  // How many valid ghost frames are currently in the trail array

    // Dash Optional Invincibility
    bool playerInvincible;  // When true, the player cannot take damage (active during a dash)

} Character;

// Initializes all Character fields, loads the sprite sheet, and sets default stats
void InitCharacter(Character* player, int startX, int startY, const char* spritePath, int cols, int rows);

// Runs every frame: handles input, dash, movement physics, collision, facing direction, and animation
void UpdateCharacter(Character* player, Rectangle* colliders, int colliderCount, Vector2 mouseWorldPos, Audio* gameAudio);

// Draws the dash trail ghosts and the player sprite at the correct animation frame
void DrawCharacter(Character* player);

// Draws the HUD elements (portrait, health bar) in screen-space (called after BeginDrawing, outside any camera)
void DrawCharacterHUD(Character* player, Texture2D portrait);

// Unloads GPU resources (sprite texture) when the game closes to prevent memory leaks
void UnloadCharacter(Character* player);

#endif

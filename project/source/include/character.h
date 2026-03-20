#ifndef CHARACTER_H //Code Written By: Josh
#define CHARACTER_H

#include "raylib.h"
#include <stdbool.h>
#include "audio.h"

// Maximum number of ghost frames stored for the dash trail effect
#define MAX_DASH_TRAIL 15

// Stores the player's position and sprite frame for a dash trail
typedef struct {
    Vector2 position;   // World position of this trail
    Rectangle frameRec; // Sprite frame region to draw for this trail
    float alpha;        // Transparency; fades to 0 over time so the trail disappears
} DashFrame;

// Holds all state for the player character
typedef struct {
    Vector2 position;       
    Vector2 velocity;      
    float maxSpeed;         
    float acceleration;     
    float friction;         
    float scale;            

    // Character Animation Fields defining sprite sheet layout
    Texture2D sprite;      
    int frames;             
    int rows;               
    
    Rectangle frameRec;    
    
    int currentFrame;      
    int framesCounter;     
    int framesSpeed;        
    int movingDirection;    

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

    // Dash Invincibility
    bool playerInvincible;  

    // Damage recovery
    float hitInvincibleTimer;

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

#include "raylib.h"
#include "screen_gameplay.h"
#include "character.h"
#include "weapon.h"
#include "tiled_map.h"
#include "setting.h"

static Character player;
static MapData map;
static Weapon playerWeapon;
static Camera2D camera = { 0 };

void InitScreenGameplay(void)
{
    const GameSettings* settings = GetSettings();
    
    InitCharacter(&player, settings->screenWidth / 2, settings->screenHeight / 2, 
                  "../assets/character/Adrian_Walk.png", 16, 2);

    LoadTiledMap(&map, "../assets/map/level1.json");
    InitWeapon(&playerWeapon);

    camera.target = (Vector2){ player.position.x, player.position.y };
    camera.offset = (Vector2){ settings->screenWidth / 2.0f, settings->screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;
}

GameScreen UpdateScreenGameplay(void)
{
    const GameSettings* settings = GetSettings();
    float dt = GetFrameTime();
    
    // Update character logic 
    UpdateCharacter(&player, map.collisionRecs, map.collisionCount);
    
    // Update bullets
    UpdateWeapon(&playerWeapon, dt, map.collisionRecs, map.collisionCount);
    
    // Update camera to follow player
    camera.offset = (Vector2){ settings->screenWidth / 2.0f, settings->screenHeight / 2.0f };
    camera.target = (Vector2){ player.position.x + (player.frameRec.width * player.scale) / 2.0f, 
                               player.position.y + (player.frameRec.height * player.scale) / 2.0f };
                               
    // Check for shooting input
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        // Shoot from the center of the player
        Vector2 startPos = { player.position.x + (player.frameRec.width * player.scale) / 2.0f, 
                             player.position.y + (player.frameRec.height * player.scale) / 2.0f };
        ShootWeapon(&playerWeapon, startPos, mouseWorldPos);
    }
    
    // We could return MAIN_MENU if the player hits Esc, but WindowShouldClose handles that.
    return GAMEPLAY; 
}

void DrawScreenGameplay(void)
{
    BeginMode2D(camera);
        // Draw the map
        DrawTiledMap(&map);
        
        // Draw active bullets
        DrawWeapon(&playerWeapon);
        
        // Let the character module draw its sprite
        DrawCharacter(&player);
    EndMode2D();
    
    DrawText("Move with Arrow Keys or WASD!", 10, 10, 20, DARKGRAY);
    DrawText("Click to Shoot! Press 'M' for Settings | ESC to exit", 10, 40, 20, LIGHTGRAY);
}

void UnloadScreenGameplay(void)
{
    UnloadTiledMap(&map);
    UnloadCharacter(&player);
}

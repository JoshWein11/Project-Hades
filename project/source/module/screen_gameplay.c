#include "raylib.h" //Code written by: Christopher 沈家豪
#include "screen_gameplay.h"
#include "character.h"
#include "audio.h"
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
    //Initialize Player
    InitCharacter(&player, settings->screenWidth / 2, settings->screenHeight / 2, 
                  "../assets/character/Adrian_Walk.png", 16, 2);

    //Initialize Map
    LoadTiledMap(&map, "../assets/map/level1.json");
    
    //Initialize Weapon
    InitWeapon(&playerWeapon);

    //Initialize Camera
    camera.target = (Vector2){ player.position.x, player.position.y };
    camera.offset = (Vector2){ settings->screenWidth / 2.0f, settings->screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;
}

GameScreen UpdateScreenGameplay(Audio* gameAudio)
{
    const GameSettings* settings = GetSettings();
    float dt = GetFrameTime();
    
    // Update camera first so mouseWorldPos is calculated correctly
    camera.offset = (Vector2){ settings->screenWidth / 2.0f, settings->screenHeight / 2.0f };
    camera.target = (Vector2){ player.position.x + (player.frameRec.width * player.scale) / 2.0f, 
                               player.position.y + (player.frameRec.height * player.scale) / 2.0f };

    // Calculate mouse world position (needed for aiming direction)
    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

    // Update character logic 
    UpdateCharacter(&player, map.collisionRecs, map.collisionCount, mouseWorldPos, gameAudio);
    
    // Update bullets
    UpdateWeapon(&playerWeapon, dt, map.collisionRecs, map.collisionCount);
                               
    // Check for shooting input
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Shoot from the center of the player
        Vector2 startPos = { player.position.x + (player.frameRec.width * player.scale) / 2.0f, 
                             player.position.y + (player.frameRec.height * player.scale) / 2.0f };
        ShootWeapon(&playerWeapon, startPos, mouseWorldPos);
    }
    
    return GAMEPLAY; 
}

void DrawScreenGameplay(void)
{
    BeginMode2D(camera); // 2D Camera for Gameplay
        DrawTiledMap(&map);
        DrawWeapon(&playerWeapon);
        DrawCharacter(&player);
    EndMode2D();

    // HUD drawn in screen-space (outside the camera so it stays pinned)
    DrawCharacterHUD(&player, player.sprite);

    DrawText("Move with Arrow Keys or WASD!", 10, 10, 20, DARKGRAY);
    DrawText("Click to Shoot! Press 'M' for Settings | ESC to open Setting", 10, 40, 20, LIGHTGRAY);
}

void UnloadScreenGameplay(void)
{
    UnloadTiledMap(&map);
    UnloadCharacter(&player);
}

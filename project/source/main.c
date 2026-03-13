#include "raylib.h"
#include "raymath.h"
#include "setting.h"
#include "character.h"
#include "tiled_map.h"
#include "weapon.h"

typedef enum GameScreen { GAMEPLAY, SETTINGS } GameScreen;
int main(void)
{
    // Initialize our settings module
    InitSettings();
    const GameSettings* settings = GetSettings();

    // Use settings for window initialization
    InitWindow(settings->screenWidth, settings->screenHeight, settings->title);


    // Initialize our player character
    Character player;
    // THIS EXTREMELY IMPORTANT: We MUST call this after InitWindow!
    InitCharacter(&player, settings->screenWidth / 2, settings->screenHeight / 2, 
                  "../assets/character/Unarmed_Walk_with_shadow.png", 6, 
                  "../assets/character/Unarmed_Run_with_shadow.png", 8);

    MapData map;
    LoadTiledMap(&map, "../assets/map/level1.json");

    Weapon playerWeapon;
    InitWeapon(&playerWeapon);

    Camera2D camera = { 0 };
    camera.target = (Vector2){ player.position.x, player.position.y };
    camera.offset = (Vector2){ settings->screenWidth / 2.0f, settings->screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;

    GameScreen currentScreen = GAMEPLAY;

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Toggle settings menu
        if (IsKeyPressed(KEY_M)) {
            if (currentScreen == GAMEPLAY) currentScreen = SETTINGS;
            else currentScreen = GAMEPLAY;
        }

        if (currentScreen == GAMEPLAY) {
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
        } else if (currentScreen == SETTINGS) {
            // Settings logic
            if (IsKeyPressed(KEY_ONE)) ApplyResolution(800, 600);
            else if (IsKeyPressed(KEY_TWO)) ApplyResolution(1280, 720);
            else if (IsKeyPressed(KEY_THREE)) ApplyResolution(1920, 1080);
        }

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            if (currentScreen == GAMEPLAY) {
                BeginMode2D(camera);
                    DrawTiledMap(&map);
                    
                    // Draw active bullets
                    DrawWeapon(&playerWeapon);
                    
                    // Let the character module draw its sprite
                    DrawCharacter(&player);
                EndMode2D();
                
                DrawText("Move with Arrow Keys or WASD!", 10, 10, 20, DARKGRAY);
                DrawText("Press 'M' for Settings | ESC to exit", 10, 40, 20, LIGHTGRAY);
            } else if (currentScreen == SETTINGS) {
                // Dim background
                DrawRectangle(0, 0, settings->screenWidth, settings->screenHeight, Fade(BLACK, 0.8f));
                
                int centerX = settings->screenWidth / 2;
                DrawText("SETTINGS MENU", centerX - MeasureText("SETTINGS MENU", 40) / 2, 100, 40, RAYWHITE);
                DrawText("1. 800x600", centerX - MeasureText("1. 800x600", 20) / 2, 200, 20, RAYWHITE);
                DrawText("2. 1280x790", centerX - MeasureText("2. 1280x720", 20) / 2, 250, 20, RAYWHITE);
                DrawText("3. 1920x1080", centerX - MeasureText("3. 1920x1080", 20) / 2, 300, 20, RAYWHITE);
                DrawText("Press 'M' to return to game", centerX - MeasureText("Press 'M' to return to game", 20) / 2, 400, 20, LIGHTGRAY);
            }

        EndDrawing();
    }

    // De-Initialization
    UnloadTiledMap(&map);
    UnloadCharacter(&player); // VERY IMPORTANT: Free the sprite Texture RAM
    CloseWindow();        // Close window and OpenGL context

    return 0;
}

#ifndef TILED_MAP_H //Code Written By: Christopher 沈佳豪
#define TILED_MAP_H

#include "raylib.h"
#include "cute_tiled.h"

#define MAX_COLLISION_RECS 256
#define MAX_NAV_NODES      128
#define MAX_ENEMY_SPAWNS   32

// A single point from the Tiled "Enemy" object layer.
// 'group' holds the object's name — empty string means stationary.
typedef struct {
    Vector2 position;    // World position of this point
    char    group[16];   // Name from Tiled — empty = stationary, same name = shared patrol
} EnemySpawnPoint;

typedef struct {
    cute_tiled_map_t* map;
    Texture2D tilesetx1;
    Texture2D labtileset;
    Texture2D tube;
    Texture2D spritesheet;
    Texture2D SciFi;
    Texture2D stage2tileset;
    
    Rectangle collisionRecs[MAX_COLLISION_RECS];
    int collisionCount;

    Vector2 navNodes[MAX_NAV_NODES];   // NavNode points from Tiled
    int     navNodeCount;

    EnemySpawnPoint enemySpawns[MAX_ENEMY_SPAWNS]; // Enemy spawn points from Tiled
    int             enemySpawnCount;

    Rectangle nextActTrigger;
    bool      hasNextActTrigger;

    Vector2   spawnPoint;
    bool      hasSpawnPoint;
} MapData;

void LoadTiledMap(MapData* mapData, const char* jsonPath); //Load Tiled Map From Assets JSON File
void DrawTiledMap(MapData* mapData); //Draw Tiled Map
void UnloadTiledMap(MapData* mapData); //Unload Tiled Map

#endif

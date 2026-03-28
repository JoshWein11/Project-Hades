#ifndef TILED_MAP_H //Code Written By: Christopher 沈佳豪
#define TILED_MAP_H

#include "raylib.h"
#include "cute_tiled.h"

#define MAX_COLLISION_RECS 256
#define MAX_NAV_NODES      128

typedef struct {
    cute_tiled_map_t* map;
    Texture2D tilesetx1;
    Texture2D labtileset;
    Texture2D tube;
    Texture2D spritesheet;
    Texture2D SciFi;
    
    Rectangle collisionRecs[MAX_COLLISION_RECS];
    int collisionCount;

    Vector2 navNodes[MAX_NAV_NODES];   // NavNode points from Tiled
    int     navNodeCount;
} MapData;

void LoadTiledMap(MapData* mapData, const char* jsonPath); //Load Tiled Map From Assets JSON File
void DrawTiledMap(MapData* mapData); //Draw Tiled Map
void UnloadTiledMap(MapData* mapData); //Unload Tiled Map

#endif

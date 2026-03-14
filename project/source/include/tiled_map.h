#ifndef TILED_MAP_H //Code Written By: Christopher 沈家豪
#define TILED_MAP_H

#include "raylib.h"
#include "cute_tiled.h"

#define MAX_COLLISION_RECS 256

typedef struct {
    cute_tiled_map_t* map;
    Texture2D tileset2;
    Texture2D fieldsTileset;
    
    Rectangle collisionRecs[MAX_COLLISION_RECS];
    int collisionCount;
} MapData;

void LoadTiledMap(MapData* mapData, const char* jsonPath); //Load Tiled Map From Assets JSON File
void DrawTiledMap(MapData* mapData); //Draw Tiled Map
void UnloadTiledMap(MapData* mapData); //Unload Tiled Map

#endif

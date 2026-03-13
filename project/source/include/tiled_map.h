#ifndef TILED_MAP_H
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

void LoadTiledMap(MapData* mapData, const char* jsonPath);
void DrawTiledMap(MapData* mapData);
void UnloadTiledMap(MapData* mapData);

#endif

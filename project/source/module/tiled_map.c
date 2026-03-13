#define CUTE_TILED_IMPLEMENTATION
#include "tiled_map.h"
#include <stdio.h>
#include <string.h>

void LoadTiledMap(MapData* mapData, const char* jsonPath) {
    mapData->map = cute_tiled_load_map_from_file(jsonPath, NULL);
    mapData->fieldsTileset = LoadTexture("../assets/images/FieldsTileset.png");
    mapData->tileset2 = LoadTexture("../assets/images/Tileset2.png");
    mapData->collisionCount = 0;

    if (mapData->map) {
        cute_tiled_layer_t* layer = mapData->map->layers;
        while (layer) {
            if (layer->type.ptr && strcmp(layer->type.ptr, "objectgroup") == 0) {
                cute_tiled_object_t* obj = layer->objects;
                while (obj && mapData->collisionCount < MAX_COLLISION_RECS) {
                    mapData->collisionRecs[mapData->collisionCount].x = obj->x;
                    mapData->collisionRecs[mapData->collisionCount].y = obj->y;
                    mapData->collisionRecs[mapData->collisionCount].width = obj->width;
                    mapData->collisionRecs[mapData->collisionCount].height = obj->height;
                    mapData->collisionCount++;
                    obj = obj->next;
                }
            }
            layer = layer->next;
        }
    } else {
        printf("Failed to load map %s. Reason: %s\n", jsonPath, cute_tiled_error_reason);
    }
}

void DrawTiledMap(MapData* mapData) {
    if (!mapData->map) return;
    int tw = mapData->map->tilewidth;
    int th = mapData->map->tileheight;

    // To respect Z-ordering, layers are drawn in order they appear
    cute_tiled_layer_t* layer = mapData->map->layers;
    while (layer) {
        if (layer->type.ptr && strcmp(layer->type.ptr, "tilelayer") == 0) {
            for (int y = 0; y < layer->height; ++y) {
                for (int x = 0; x < layer->width; ++x) {
                    int gid = layer->data[y * layer->width + x];
                    if (gid == 0) continue;
                    
                    int hflip, vflip, dflip;
                    cute_tiled_get_flags(gid, &hflip, &vflip, &dflip);
                    gid = cute_tiled_unset_flags(gid);
                    
                    Texture2D tex = {0};
                    int firstgid = 0;
                    if (gid >= 107) {
                        tex = mapData->tileset2;
                        firstgid = 107;
                    } else if (gid >= 43) {
                        tex = mapData->fieldsTileset;
                        firstgid = 43;
                    } else {
                        // Ground.tsx missing texture? We just ignore for now.
                        continue;
                    }
                    
                    if (tex.id == 0) continue; // Safety check
                    
                    int local_id = gid - firstgid;
                    int cols = tex.width / tw;
                    if (cols == 0) cols = 1;
                    int src_x = (local_id % cols) * tw;
                    int src_y = (local_id / cols) * th;
                    
                    Rectangle srcRec = { (float)src_x, (float)src_y, (float)tw, (float)th };
                    if (hflip) srcRec.width *= -1;
                    if (vflip) srcRec.height *= -1;
                    
                    Rectangle destRec = { (float)(x * tw + layer->offsetx), (float)(y * th + layer->offsety), (float)tw, (float)th };
                    DrawTexturePro(tex, srcRec, destRec, (Vector2){0,0}, 0.0f, WHITE);
                }
            }
        }
        layer = layer->next;
    }
}

void UnloadTiledMap(MapData* mapData) {
    if (mapData->map) {
        cute_tiled_free_map(mapData->map);
        mapData->map = NULL;
    }
    UnloadTexture(mapData->fieldsTileset);
    UnloadTexture(mapData->tileset2);
}

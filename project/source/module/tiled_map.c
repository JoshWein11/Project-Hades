#define CUTE_TILED_IMPLEMENTATION //Code written by: Christopher 沈佳豪
#include "tiled_map.h"
#include <stdio.h>
#include <string.h>

//Load Tiled Map from JSON file and Texture from Assets
void LoadTiledMap(MapData* mapData, const char* jsonPath) {
    mapData->map = cute_tiled_load_map_from_file(jsonPath, NULL);
    mapData->tilesetx1 = LoadTexture("../assets/images/tileset/tileset x1.png");
    mapData->labtileset = LoadTexture("../assets/images/tileset/labtileset.png");
    mapData->tube = LoadTexture("../assets/images/tileset/tube.png");
    mapData->spritesheet = LoadTexture("../assets/images/tileset/spritesheet.png");
    mapData->SciFi = LoadTexture("../assets/images/tileset/SciFi.png");
    mapData->stage2tileset = LoadTexture("../assets/images/tileset/stage 2 tileset.png");
    mapData->collisionCount = 0;
    mapData->navNodeCount   = 0;
    mapData->enemySpawnCount = 0;
    mapData->hasNextActTrigger = false;
    mapData->hasSpawnPoint = false;

    if (mapData->map) {
        cute_tiled_layer_t* layer = mapData->map->layers;
        while (layer) {
            if (layer->type.ptr && strcmp(layer->type.ptr, "objectgroup") == 0) {

                // ── NavNodes layer → load as navigation waypoints ─────────
                if (layer->name.ptr && strcmp(layer->name.ptr, "NavNodes") == 0) {
                    cute_tiled_object_t* obj = layer->objects;
                    while (obj && mapData->navNodeCount < MAX_NAV_NODES) {
                        mapData->navNodes[mapData->navNodeCount] = (Vector2){ obj->x, obj->y };
                        mapData->navNodeCount++;
                        obj = obj->next;
                    }
                }
                // ── Enemy layer → load as spawn points with patrol groups ──
                else if (layer->name.ptr && strcmp(layer->name.ptr, "Enemy") == 0) {
                    cute_tiled_object_t* obj = layer->objects;
                    while (obj && mapData->enemySpawnCount < MAX_ENEMY_SPAWNS) {
                        EnemySpawnPoint* sp = &mapData->enemySpawns[mapData->enemySpawnCount];
                        sp->position = (Vector2){ obj->x, obj->y };
                        // Copy name — empty string if unnamed
                        if (obj->name.ptr && strlen(obj->name.ptr) > 0) {
                            strncpy(sp->group, obj->name.ptr, 15);
                            sp->group[15] = '\0';
                        } else {
                            sp->group[0] = '\0';
                        }
                        mapData->enemySpawnCount++;
                        obj = obj->next;
                    }
                }
                // ── Next ACT Trigger layer ──
                else if (layer->name.ptr && strcmp(layer->name.ptr, "Next ACT") == 0) {
                    cute_tiled_object_t* obj = layer->objects;
                    if (obj) { // Just take the first object in the layer
                        mapData->nextActTrigger.x = obj->x;
                        mapData->nextActTrigger.y = obj->y;
                        mapData->nextActTrigger.width = obj->width;
                        mapData->nextActTrigger.height = obj->height;
                        mapData->hasNextActTrigger = true;
                    }
                }
                // ── Spawn layer ──
                else if (layer->name.ptr && strcmp(layer->name.ptr, "Spawn") == 0) {
                    cute_tiled_object_t* obj = layer->objects;
                    if (obj) { // Just take the first object in the layer
                        mapData->spawnPoint.x = obj->x;
                        mapData->spawnPoint.y = obj->y;
                        mapData->hasSpawnPoint = true;
                    }
                } else {
                    // ── All other object layers → load as collision recs ───
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
            }
            layer = layer->next;
        }
        printf("[Map] Loaded %d collision recs, %d nav nodes, %d enemy spawns\n", mapData->collisionCount, mapData->navNodeCount, mapData->enemySpawnCount);
    } else {
        printf("Failed to load map %s. Reason: %s\n", jsonPath, cute_tiled_error_reason);
    }
}

void DrawTiledMap(MapData* mapData) {
    if (!mapData->map) return;
    int tw = mapData->map->tilewidth; //Tile Width
    int th = mapData->map->tileheight; //Tile Height

    // To respect Z-ordering, layers are drawn in order they appear
    cute_tiled_layer_t* layer = mapData->map->layers;
    while (layer) {
        if (layer->type.ptr && strcmp(layer->type.ptr, "tilelayer") == 0) {
            for (int y = 0; y < layer->height; ++y) {
                for (int x = 0; x < layer->width; ++x) {
                    int gid = layer->data[y * layer->width + x];
                    if (gid == 0) continue; //Skip empty tiles
                    
                    int hflip, vflip, dflip; //Flip Flags
                    cute_tiled_get_flags(gid, &hflip, &vflip, &dflip);
                    gid = cute_tiled_unset_flags(gid); //Remove flags from GID
                    
                    Texture2D tex = {0};
                    int firstgid = 0;
                    cute_tiled_tileset_t* best_ts = NULL;
                    cute_tiled_tileset_t* ts = mapData->map->tilesets;
                    
                    while (ts) {
                        // Skip completely ghost/empty embedded tilesets (tilecount == 0)
                        if (ts->image.ptr != NULL && ts->tilecount <= 0) {
                            ts = ts->next;
                            continue;
                        }
                        if (gid >= ts->firstgid) {
                            if (!best_ts || ts->firstgid >= best_ts->firstgid) {
                                best_ts = ts;
                            }
                        }
                        ts = ts->next;
                    }
                    
                    if (best_ts) {
                        firstgid = best_ts->firstgid;
                        const char* ts_ident = best_ts->image.ptr ? best_ts->image.ptr : 
                                              (best_ts->source.ptr ? best_ts->source.ptr : 
                                              (best_ts->name.ptr ? best_ts->name.ptr : ""));
                                              
                        if (strstr(ts_ident, "tileset x1") || strstr(ts_ident, "tilesetx1")) {
                            tex = mapData->tilesetx1;
                        } else if (strstr(ts_ident, "labtileset")) {
                            tex = mapData->labtileset;
                        } else if (strstr(ts_ident, "tube")) {
                            tex = mapData->tube;
                        } else if (strstr(ts_ident, "spritesheet")) {
                            tex = mapData->spritesheet;
                        } else if (strstr(ts_ident, "SciFi")) {
                            tex = mapData->SciFi;
                        } else if (strstr(ts_ident, "stage 2 tileset")) {
                            tex = mapData->stage2tileset;
                        }
                    }
                    
                    if (tex.id == 0) continue; // Safety check
                    
                    int local_id = gid - firstgid;    // Convert global ID to local (0-based)
                    int cols = tex.width / tw;         // How many tiles fit in one row of the image
                    int src_x = (local_id % cols) * tw; // Column in tileset × tile width
                    int src_y = (local_id / cols) * th; // Row in tileset × tile height
                    
                    Rectangle srcRec = { (float)src_x, (float)src_y, (float)tw, (float)th };
                    if (hflip) srcRec.width *= -1; //Horizontal Flip
                    if (vflip) srcRec.height *= -1; //Vertical Flip
                    
                    Rectangle destRec = { (float)(x * tw + layer->offsetx), (float)(y * th + layer->offsety), (float)tw, (float)th };
                    DrawTexturePro(tex, srcRec, destRec, (Vector2){0,0}, 0.0f, WHITE); //Draw Tile
                }
            }
        }
        layer = layer->next;
    }
}

void UnloadTiledMap(MapData* mapData) { //Unload Tiled Map
    if (mapData->map) { 
        cute_tiled_free_map(mapData->map); 
        mapData->map = NULL; 
    }
    UnloadTexture(mapData->tilesetx1);
    UnloadTexture(mapData->labtileset);
    UnloadTexture(mapData->tube);
    UnloadTexture(mapData->spritesheet);
    UnloadTexture(mapData->SciFi);
    UnloadTexture(mapData->stage2tileset);
}

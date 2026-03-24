#define CUTE_TILED_IMPLEMENTATION //Code written by: Christopher 沈佳豪
#include "tiled_map.h"
#include <stdio.h>
#include <string.h>

//Load Tiled Map from JSON file and Texture from Assets
void LoadTiledMap(MapData* mapData, const char* jsonPath) {
    mapData->map = cute_tiled_load_map_from_file(jsonPath, NULL);
    mapData->tilesetx1 = LoadTexture("../assets/images/tileset/tilesetx1.png");
    mapData->labtileset = LoadTexture("../assets/images/tileset/labtileset.png");
    mapData->tube = LoadTexture("../assets/images/tileset/tube.png");
    mapData->spritesheet = LoadTexture("../assets/images/tileset/spritesheet.png");
    mapData->SciFi = LoadTexture("../assets/images/tileset/SciFi.png");
    mapData->collisionCount = 0;

    if (mapData->map) {
        cute_tiled_layer_t* layer = mapData->map->layers;
        while (layer) {
            if (layer->type.ptr && strcmp(layer->type.ptr, "objectgroup") == 0) {
                cute_tiled_object_t* obj = layer->objects;
                //Load Collision Recs
                while (obj && mapData->collisionCount < MAX_COLLISION_RECS) {
                    mapData->collisionRecs[mapData->collisionCount].x = obj->x; //Collision Recs X
                    mapData->collisionRecs[mapData->collisionCount].y = obj->y; //Collision Recs Y
                    mapData->collisionRecs[mapData->collisionCount].width = obj->width; //Collision Recs Width
                    mapData->collisionRecs[mapData->collisionCount].height = obj->height; //Collision Recs Height
                    mapData->collisionCount++;
                    obj = obj->next;
                }
            }
            layer = layer->next; //Next Layer
        }
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
                    if (gid >= 2362) { //Determine which tileset to use
                        tex = mapData->spritesheet;
                        firstgid = 2362;
                    } else if (gid >= 1786) {
                        tex = mapData->SciFi;
                        firstgid = 1786;
                    } else if (gid >= 1784) {
                        tex = mapData->tube;
                        firstgid = 1784;
                    } else if (gid >= 1703) {
                        tex = mapData->labtileset;
                        firstgid = 1703;
                    } else if (gid >= 1) {
                        tex = mapData->tilesetx1;
                        firstgid = 1;
                    } else {
                        //Ignore missing texture
                        continue;
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
}

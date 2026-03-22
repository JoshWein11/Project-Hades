#include "savedata.h" //Code written by: Christopher 沈佳豪
#include "raylib.h"
#include <stdio.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// SaveGame — writes the entire SaveData struct to a binary file
// ─────────────────────────────────────────────────────────────────────────────
bool SaveGame(const SaveData* data, const char* filepath) {
    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        TraceLog(LOG_WARNING, "SAVE: Failed to open file for writing: %s", filepath);
        return false;
    }

    size_t written = fwrite(data, sizeof(SaveData), 1, f);
    fclose(f);

    if (written == 1) {
        TraceLog(LOG_INFO, "SAVE: Game saved successfully to %s", filepath);
    } else {
        TraceLog(LOG_WARNING, "SAVE: Failed to write data to %s", filepath);
    }

    return (written == 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadGame — reads a SaveData struct from a binary file
// ─────────────────────────────────────────────────────────────────────────────
bool LoadGame(SaveData* data, const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (f == NULL) return false;

    // Zero out the struct first so partial reads don't leave garbage
    memset(data, 0, sizeof(SaveData));

    size_t read = fread(data, sizeof(SaveData), 1, f);
    fclose(f);

    return (read == 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// SaveFileExists — simple check: try to open for reading
// ─────────────────────────────────────────────────────────────────────────────
bool SaveFileExists(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (f != NULL) {
        fclose(f);
        return true;
    }
    return false;
}

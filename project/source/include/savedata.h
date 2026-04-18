#ifndef SAVEDATA_H //Code written by: Christopher 沈佳豪
#define SAVEDATA_H

#include <stdbool.h>

#define SAVE_MAX_ENEMIES 10
#define SAVE_FILEPATH    "../assets/save/savegame.dat"

// All persistent game state packed into one struct for easy file I/O
typedef struct {
    // Player
    float playerX, playerY;
    float playerHealth;
    float playerStamina;

    // Enemies
    int   enemyCount;
    float enemyX[SAVE_MAX_ENEMIES];
    float enemyY[SAVE_MAX_ENEMIES];
    float enemyHealth[SAVE_MAX_ENEMIES];
    bool  enemyDead[SAVE_MAX_ENEMIES];

    // Progress
    int   currentScreen;    // Which GameScreen to restore to
    int   dialogueIndex;    // Which dialogue entry the player reached
    bool  dialogueFinished; // True if dialogue was fully completed

    // Lives & Act/Chapter progression
    int   lives;            // Remaining lives (starts at 5)
    int   currentAct;       // Which Act the player is in (0-based)
    int   currentChapter;   // Which Chapter within the Act (0-based)
    
    // Inventory
    bool  hasKeycardA;
    bool  hasKeycardB;

    // Events
    bool  forceDialogueTriggered;
} SaveData;

// Writes the SaveData struct to a binary file. Returns true on success.
bool SaveGame(const SaveData* data, const char* filepath);

// Reads SaveData from a binary file. Returns true on success (file exists + valid).
bool LoadGame(SaveData* data, const char* filepath);

// Returns true if a save file exists at the given path.
bool SaveFileExists(const char* filepath);

#endif // SAVEDATA_H

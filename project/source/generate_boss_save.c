#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "savedata.h"

int main(void) {
    SaveData save = {0};
    
    // Spawn player near the boss arena center
    save.playerX = 1546.0f;
    save.playerY = 1400.0f;
    save.playerHealth = 100.0f;
    save.playerStamina = 100.0f;
    
    save.enemyCount = 0;
    
    save.currentScreen = 4; // 4 is GAMEPLAY
    save.dialogueFinished = true;
    
    save.lives = 5;
    save.currentAct = 4; // Act 5 is the Boss Act
    save.currentChapter = 0;
    
    // All items collected
    save.hasKeycardA = true;
    save.hasKeycardB = true;
    save.hasHazmatSuit = true;
    
    if (SaveGame(&save, "../assets/save/savegame.dat")) {
        printf("Boss save generated successfully! Load the game from the main menu.\n");
    } else {
        printf("Failed to generate save.\n");
    }
    
    return 0;
}

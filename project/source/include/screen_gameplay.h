#ifndef SCREEN_GAMEPLAY_H //All Screen Code Written By: Christopher 沈佳豪
#define SCREEN_GAMEPLAY_H

#include "screens.h"
#include "audio.h"
#include "savedata.h"

void InitScreenGameplay(void);
GameScreen UpdateScreenGameplay(Audio* gameAudio);
void DrawScreenGameplay(void);
void UnloadScreenGameplay(void);

// Save / Load helpers
void GetGameplaySaveData(SaveData* data);
void RestoreGameplayFromSave(const SaveData* data);

#endif // SCREEN_GAMEPLAY_H

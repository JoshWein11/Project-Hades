#ifndef SCREEN_GAMEPLAY_H //All Screen Code Written By: Christopher 沈家豪
#define SCREEN_GAMEPLAY_H

#include "screens.h"
#include "audio.h"

void InitScreenGameplay(void);
GameScreen UpdateScreenGameplay(Audio* gameAudio);
void DrawScreenGameplay(void);
void UnloadScreenGameplay(void);

#endif // SCREEN_GAMEPLAY_H

#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

typedef struct audio{
    Music bg_menu_music;
    Sound sfxSplash;
}Audio;

void InitAudio(Audio* audio);
void UnloadAudio(Audio* audio);

#endif
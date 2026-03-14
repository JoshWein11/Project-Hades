#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

#define MAX_FOOTSTEP_SOUNDS 4 // Using 4 sounds for natural variation

typedef struct audio{
    Music bg_menu_music;
    Sound sfxSplash;
    Sound sfxFootsteps[MAX_FOOTSTEP_SOUNDS];
}Audio;

void InitAudio(Audio* audio);
void UnloadAudio(Audio* audio);
void PlayRandomFootstep(Audio* audio);

#endif
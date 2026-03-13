#include "audio.h"

void InitAudio(Audio* audio) {
    audio->bg_menu_music = LoadMusicStream("../assets/music/menubg.wav");
    audio->sfxSplash = LoadSound("../assets/music/opening.wav");
}

void UnloadAudio(Audio* audio) {
    UnloadMusicStream(audio->bg_menu_music);
    UnloadSound(audio->sfxSplash);
}
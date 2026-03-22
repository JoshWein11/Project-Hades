#include "audio.h" //Code written by: Christopher 沈佳豪

void InitAudio(Audio* audio) {
    audio->bg_menu_music = LoadMusicStream("../assets/music/menubg.wav");
    audio->sfxSplash = LoadSound("../assets/music/opening.wav");
    
    // Load footstep sounds
    audio->sfxFootsteps[0] = LoadSound("../assets/sfx/footstep/1.ogg");
    audio->sfxFootsteps[1] = LoadSound("../assets/sfx/footstep/2.ogg");
    audio->sfxFootsteps[2] = LoadSound("../assets/sfx/footstep/3.ogg");
    audio->sfxFootsteps[3] = LoadSound("../assets/sfx/footstep/4.ogg");
}

void UnloadAudio(Audio* audio) {
    UnloadMusicStream(audio->bg_menu_music);
    UnloadSound(audio->sfxSplash);
    
    // Unload footstep sounds
    for (int i = 0; i < MAX_FOOTSTEP_SOUNDS; i++) {
        UnloadSound(audio->sfxFootsteps[i]);
    }
}

void PlayRandomFootstep(Audio* audio) {
    // Pick a random index between 0 and MAX_FOOTSTEP_SOUNDS - 1 (0 to 3)
    int stepIndex = GetRandomValue(0, MAX_FOOTSTEP_SOUNDS - 1);
    
    // Pitch shift slightly for natural variation (e.g., 90% to 110%)
    float pitchShift = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(audio->sfxFootsteps[stepIndex], pitchShift);
    
    // Volume shift slightly for natural variation (e.g., 80% to 100%)
    float volShift = GetRandomValue(80, 100) / 100.0f;
    SetSoundVolume(audio->sfxFootsteps[stepIndex], volShift);
    
    PlaySound(audio->sfxFootsteps[stepIndex]);
}
#ifndef SETTING_H
#define SETTING_H

typedef struct {
    int screenWidth;
    int screenHeight;
    const char* title;
    float masterVolume;
} GameSettings;

// Initializes the settings with default values
void InitSettings(void);

// Retrieves a pointer to the current game settings
const GameSettings* GetSettings(void);

// Applies a new resolution
void ApplyResolution(int width, int height);

// Sets the master volume (0.0 to 1.0)
void SetMasterVolumeLevel(float volume);

#endif // SETTING_H

#ifndef SETTING_H
#define SETTING_H

typedef struct {
    int screenWidth;
    int screenHeight;
    const char* title;
} GameSettings;

// Initializes the settings with default values
void InitSettings(void);

// Retrieves a pointer to the current game settings
const GameSettings* GetSettings(void);

// Applies a new resolution
void ApplyResolution(int width, int height);

#endif // SETTING_H

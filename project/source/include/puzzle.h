#ifndef PUZZLE_H //Code written by: Christopher 沈佳豪
#define PUZZLE_H

#include "raylib.h"
#include <stdbool.h>

#define PUZZLE_WIRE_COUNT 4 // Number of wires to connect

// ─────────────────────────────────────────────────────────────────────────────
// PuzzleState — generic wire-connecting minigame state.
//   This module is UNIVERSAL: it only manages the puzzle UI and logic.
//   The gameplay screen maps game objects to PuzzleState instances.
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    bool   active;       // Is the puzzle UI currently open?
    bool   solved;       // Has this puzzle been completed?

    // Wire colours: leftOrder[i] = colour index of i-th left node
    // rightOrder[i] = colour index of i-th right node (shuffled)
    int    leftOrder[PUZZLE_WIRE_COUNT];
    int    rightOrder[PUZZLE_WIRE_COUNT];

    // connections[i] = right-side node index that left node i is connected to (-1 = none)
    int    connections[PUZZLE_WIRE_COUNT];

    // Drag state
    int    draggingFrom;  // Left node index being dragged (-1 = none)
    Vector2 dragEnd;      // Current mouse position while dragging
} PuzzleState;

// Initialise and randomise wire layout
void InitPuzzle(PuzzleState* puzzle);

// Reset all connections (called when player walks away)
void ResetPuzzle(PuzzleState* puzzle);

// Open the puzzle UI (sets active = true)
void OpenPuzzle(PuzzleState* puzzle);

// Close the puzzle UI and reset connections (sets active = false)
void ClosePuzzle(PuzzleState* puzzle);

// Update drag/connect logic; returns true the frame the puzzle is solved
bool UpdatePuzzle(PuzzleState* puzzle, Vector2 mousePos);

// Draw the puzzle overlay panel (call in screen-space, after EndMode2D)
void DrawPuzzle(PuzzleState* puzzle);

// ─────────────────────────────────────────────────────────────────────────────
// RGBPuzzle — timing/rhythm minigame state.
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    float x;
    float y;
    float radius;
    Color color;
} RGBButton;

typedef struct {
    bool   active;
    bool   solved;
    
    int    currentStage;   // 0=Stage 1, 1=Stage 2, 2=Stage 3
    int    currentBeat;    // 0, 1, 2 (the specific color hit sequence in current stage)
    
    float  stageTimer;     // Counter starting from 0.0f
    float  tempo;          // e.g. 2.0s, 1.0s, 0.5s
    float  tolerance;      // e.g. 0.1s (so +/- 0.1s is the tolerance window)
    
    // Ordered target colours the user MUST press (Always Red, Blue, Green)
    Color  targetOrder[3];
    
    // Visual buttons on screen
    RGBButton circles[3];
    
    Texture2D textures[3]; // bg1, bg2, bg3
} RGBPuzzle;

void InitRGBPuzzle(RGBPuzzle* puzzle, Texture2D bg1, Texture2D bg2, Texture2D bg3);
void OpenRGBPuzzle(RGBPuzzle* puzzle);
void CloseRGBPuzzle(RGBPuzzle* puzzle);
bool UpdateRGBPuzzle(RGBPuzzle* puzzle, Vector2 mousePos, Sound btnSfx, Sound failSfx);
void DrawRGBPuzzle(RGBPuzzle* puzzle);
// ─────────────────────────────────────────────────────────────────────────────
// CodePuzzle — memory sequence minigame state.
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    CODE_MEMORIZE,
    CODE_INPUT,
    CODE_FAIL,
    CODE_SUCCESS
} CodePuzzleState;

typedef struct {
    Rectangle bounds;
    int number;
    float pressTimer; // For visual feedback when clicked
} CodeButton;

typedef struct {
    bool active;
    bool solved;
    CodePuzzleState state;

    int currentRound;   // 0 to 4 (representing lengths 4 to 8)
    int sequence[8];    // The sequence the player must memorize
    int seqLength;      // 4 + currentRound
    int inputIndex;     // Correct inputs this round

    float stateTimer;   // General state timer (used for MEMORIZE and FAIL screens)
    
    CodeButton buttons[8]; // UI boxes the user clicks

    Texture2D bgTexture; // Base overlay asset (code.png)
} CodePuzzle;

void InitCodePuzzle(CodePuzzle* puzzle, Texture2D bg);
void OpenCodePuzzle(CodePuzzle* puzzle);
void CloseCodePuzzle(CodePuzzle* puzzle);
bool UpdateCodePuzzle(CodePuzzle* puzzle, Vector2 mousePos, Sound btnSfx, Sound failSfx);
void DrawCodePuzzle(CodePuzzle* puzzle);

// ─────────────────────────────────────────────────────────────────────────────
// HazardPuzzle — 8-digit password keypad minigame (code: 5961-6769).
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    Rectangle bounds;
    int number;        // 0-9
    float pressTimer;  // Visual feedback when clicked
} HazardButton;

typedef enum {
    HAZARD_INPUT,
    HAZARD_FAIL,
    HAZARD_SUCCESS
} HazardPuzzleState;

typedef struct {
    bool active;
    bool solved;
    HazardPuzzleState state;

    char code[9];       // Correct code: "59616769"
    char input[9];      // Player's typed digits so far
    int  inputLength;   // How many digits entered (0-8)

    HazardButton buttons[10]; // 0-9 digit buttons
    float failTimer;          // Brief flash on wrong code
} HazardPuzzle;

void InitHazardPuzzle(HazardPuzzle* puzzle);
void OpenHazardPuzzle(HazardPuzzle* puzzle);
void CloseHazardPuzzle(HazardPuzzle* puzzle);
bool UpdateHazardPuzzle(HazardPuzzle* puzzle, Vector2 mousePos, Sound btnSfx, Sound failSfx);
void DrawHazardPuzzle(HazardPuzzle* puzzle);

#endif

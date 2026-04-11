#include "puzzle.h" //Code written by: Christopher 沈佳豪
#include "raymath.h"
#include "setting.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// Wire colours — Red, Blue, Yellow, Green (matching Among Us style)
// ─────────────────────────────────────────────────────────────────────────────
static const Color wireColors[PUZZLE_WIRE_COUNT] = {
    { 220,  50,  50, 255 },  // Red
    {  50, 100, 220, 255 },  // Blue
    { 230, 210,  50, 255 },  // Yellow
    {  50, 200,  80, 255 },  // Green
};

// ─────────────────────────────────────────────────────────────────────────────
// Panel layout constants
// ─────────────────────────────────────────────────────────────────────────────
#define PANEL_WIDTH  420
#define PANEL_HEIGHT 360
#define NODE_RADIUS  18
#define NODE_SPACING 70
#define LEFT_COL_X   80    // X offset from panel left edge for left nodes
#define RIGHT_COL_X  340   // X offset from panel left edge for right nodes
#define FIRST_NODE_Y 80    // Y offset from panel top for first node
#define WIRE_THICK   5.0f

// ─────────────────────────────────────────────────────────────────────────────
// Fisher-Yates shuffle for an int array of size n
// ─────────────────────────────────────────────────────────────────────────────
static void ShuffleArray(int* arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// InitPuzzle — set up a fresh puzzle with randomised right-side wire order
// ─────────────────────────────────────────────────────────────────────────────
void InitPuzzle(PuzzleState* puzzle) {
    puzzle->active = false;
    puzzle->solved = false;
    puzzle->draggingFrom = -1;
    puzzle->dragEnd = (Vector2){ 0, 0 };

    // Left side: always ordered 0,1,2,3 (Red, Blue, Yellow, Green top-to-bottom)
    for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
        puzzle->leftOrder[i] = i;
        puzzle->rightOrder[i] = i;
        puzzle->connections[i] = -1;
    }

    // Shuffle the right side so wires aren't trivially aligned
    ShuffleArray(puzzle->rightOrder, PUZZLE_WIRE_COUNT);
}

// ─────────────────────────────────────────────────────────────────────────────
// ResetPuzzle — clear all connections but keep wire order intact
// ─────────────────────────────────────────────────────────────────────────────
void ResetPuzzle(PuzzleState* puzzle) {
    for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
        puzzle->connections[i] = -1;
    }
    puzzle->draggingFrom = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// OpenPuzzle — show the puzzle UI
// ─────────────────────────────────────────────────────────────────────────────
void OpenPuzzle(PuzzleState* puzzle) {
    if (!puzzle->solved) {
        puzzle->active = true;
        ResetPuzzle(puzzle); // Always start fresh
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ClosePuzzle — hide the puzzle UI and discard partial progress
// ─────────────────────────────────────────────────────────────────────────────
void ClosePuzzle(PuzzleState* puzzle) {
    puzzle->active = false;
    ResetPuzzle(puzzle); // Discard connections on close
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: compute screen-space centre of a left-side node
// ─────────────────────────────────────────────────────────────────────────────
static Vector2 GetLeftNodePos(int index) {
    float panelX = (VIRTUAL_WIDTH  - PANEL_WIDTH)  / 2.0f;
    float panelY = (VIRTUAL_HEIGHT - PANEL_HEIGHT) / 2.0f;
    return (Vector2){
        panelX + LEFT_COL_X,
        panelY + FIRST_NODE_Y + index * NODE_SPACING
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: compute screen-space centre of a right-side node
// ─────────────────────────────────────────────────────────────────────────────
static Vector2 GetRightNodePos(int index) {
    float panelX = (VIRTUAL_WIDTH  - PANEL_WIDTH)  / 2.0f;
    float panelY = (VIRTUAL_HEIGHT - PANEL_HEIGHT) / 2.0f;
    return (Vector2){
        panelX + RIGHT_COL_X,
        panelY + FIRST_NODE_Y + index * NODE_SPACING
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// UpdatePuzzle — handle mouse drag logic; returns true when just solved
// ─────────────────────────────────────────────────────────────────────────────
bool UpdatePuzzle(PuzzleState* puzzle, Vector2 mousePos) {
    if (!puzzle->active || puzzle->solved) return false;

    // ── Begin drag: click on a left node ─────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
            Vector2 nodePos = GetLeftNodePos(i);
            float dx = mousePos.x - nodePos.x;
            float dy = mousePos.y - nodePos.y;
            if (dx*dx + dy*dy <= (NODE_RADIUS + 6) * (NODE_RADIUS + 6)) {
                // Clear any existing connection from this left node
                puzzle->connections[i] = -1;
                puzzle->draggingFrom = i;
                puzzle->dragEnd = mousePos;
                break;
            }
        }
    }

    // ── Continue drag: update endpoint ───────────────────────────────────────
    if (puzzle->draggingFrom >= 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        puzzle->dragEnd = mousePos;
    }

    // ── End drag: release on a right node ────────────────────────────────────
    if (puzzle->draggingFrom >= 0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        int from = puzzle->draggingFrom;
        int leftColorIdx = puzzle->leftOrder[from];

        for (int j = 0; j < PUZZLE_WIRE_COUNT; j++) {
            Vector2 nodePos = GetRightNodePos(j);
            float dx = mousePos.x - nodePos.x;
            float dy = mousePos.y - nodePos.y;
            if (dx*dx + dy*dy <= (NODE_RADIUS + 6) * (NODE_RADIUS + 6)) {
                int rightColorIdx = puzzle->rightOrder[j];

                // Only connect if colours match
                if (leftColorIdx == rightColorIdx) {
                    // Remove any other left node already connected to this right node
                    for (int k = 0; k < PUZZLE_WIRE_COUNT; k++) {
                        if (puzzle->connections[k] == j) {
                            puzzle->connections[k] = -1;
                        }
                    }
                    puzzle->connections[from] = j;
                }
                // If colours don't match, silently do nothing
                break;
            }
        }

        puzzle->draggingFrom = -1;

        // ── Check if all wires are connected (puzzle solved) ─────────────────
        bool allConnected = true;
        for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
            if (puzzle->connections[i] < 0) {
                allConnected = false;
                break;
            }
        }

        if (allConnected) {
            puzzle->solved = true;
            puzzle->active = false;
            return true; // Signal: puzzle just solved this frame
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawPuzzle — render the puzzle overlay in screen-space
// ─────────────────────────────────────────────────────────────────────────────
void DrawPuzzle(PuzzleState* puzzle) {
    if (!puzzle->active) return;

    float panelX = (VIRTUAL_WIDTH  - PANEL_WIDTH)  / 2.0f;
    float panelY = (VIRTUAL_HEIGHT - PANEL_HEIGHT) / 2.0f;

    // ── Dim background ───────────────────────────────────────────────────────
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.6f));

    // ── Panel background ─────────────────────────────────────────────────────
    Rectangle panelRec = { panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT };
    DrawRectangleRounded(panelRec, 0.05f, 10, (Color){ 30, 30, 40, 240 });
    DrawRectangleRoundedLines(panelRec, 0.05f, 10, (Color){ 80, 80, 100, 255 });

    // ── Title ────────────────────────────────────────────────────────────────
    const char* title = "Connect the Wires";
    int titleW = MeasureText(title, 24);
    DrawText(title, (int)(panelX + PANEL_WIDTH / 2 - titleW / 2), (int)(panelY + 20), 24, RAYWHITE);

    // ── Subtitle hint ────────────────────────────────────────────────────────
    const char* hint = "E to close";
    int hintW = MeasureText(hint, 14);
    DrawText(hint, (int)(panelX + PANEL_WIDTH / 2 - hintW / 2), (int)(panelY + PANEL_HEIGHT - 30), 14, GRAY);

    // ── Draw connected wires (behind nodes) ──────────────────────────────────
    for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
        if (puzzle->connections[i] >= 0) {
            Vector2 leftPos  = GetLeftNodePos(i);
            Vector2 rightPos = GetRightNodePos(puzzle->connections[i]);
            Color wireCol = wireColors[puzzle->leftOrder[i]];
            DrawLineEx(leftPos, rightPos, WIRE_THICK, wireCol);
        }
    }

    // ── Draw dragging wire ───────────────────────────────────────────────────
    if (puzzle->draggingFrom >= 0) {
        Vector2 leftPos = GetLeftNodePos(puzzle->draggingFrom);
        Color wireCol = Fade(wireColors[puzzle->leftOrder[puzzle->draggingFrom]], 0.7f);
        DrawLineEx(leftPos, puzzle->dragEnd, WIRE_THICK, wireCol);
    }

    // ── Draw left-side nodes ─────────────────────────────────────────────────
    for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
        Vector2 pos = GetLeftNodePos(i);
        Color col = wireColors[puzzle->leftOrder[i]];

        // Outer ring
        DrawCircleV(pos, NODE_RADIUS + 3, Fade(WHITE, 0.3f));
        // Filled node
        DrawCircleV(pos, NODE_RADIUS, col);
        // Inner highlight
        DrawCircleV((Vector2){ pos.x - 4, pos.y - 4 }, NODE_RADIUS * 0.35f, Fade(WHITE, 0.3f));
    }

    // ── Draw right-side nodes ────────────────────────────────────────────────
    for (int i = 0; i < PUZZLE_WIRE_COUNT; i++) {
        Vector2 pos = GetRightNodePos(i);
        Color col = wireColors[puzzle->rightOrder[i]];

        // Outer ring
        DrawCircleV(pos, NODE_RADIUS + 3, Fade(WHITE, 0.3f));
        // Filled node
        DrawCircleV(pos, NODE_RADIUS, col);
        // Inner highlight
        DrawCircleV((Vector2){ pos.x - 4, pos.y - 4 }, NODE_RADIUS * 0.35f, Fade(WHITE, 0.3f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// RGB Puzzle Logic
// ─────────────────────────────────────────────────────────────────────────────
void InitRGBPuzzle(RGBPuzzle* puzzle, Texture2D bg1, Texture2D bg2, Texture2D bg3) {
    puzzle->active = false;
    puzzle->solved = false;
    puzzle->textures[0] = bg1;
    puzzle->textures[1] = bg2;
    puzzle->textures[2] = bg3;
    puzzle->currentStage = 0;
    puzzle->currentBeat = 0;
    puzzle->stageTimer = 0.0f;
    puzzle->tempo = 2.0f;
    puzzle->tolerance = 0.1f; // +/- 0.1 means 200ms total window
    
    // Ordered targets: ALWAYS Red, Blue, Green
    puzzle->targetOrder[0] = RED;
    puzzle->targetOrder[1] = BLUE;
    puzzle->targetOrder[2] = GREEN;
}

void OpenRGBPuzzle(RGBPuzzle* puzzle) {
    if (!puzzle->solved) {
        puzzle->active = true;
        puzzle->currentStage = 0;
        puzzle->currentBeat = 0;
        puzzle->stageTimer = 0.0f;
    }
}

void CloseRGBPuzzle(RGBPuzzle* puzzle) {
    puzzle->active = false;
}

static void FormatRGBStage(RGBPuzzle* puzzle) {
    puzzle->currentBeat = 0;
    puzzle->stageTimer = 0.0f;
    
    float cx = VIRTUAL_WIDTH / 2.0f;
    float cy = VIRTUAL_HEIGHT / 2.0f;
    
    // Shift circles down a bit so they don't cover the image center
    // Shift circles down relative to center point - lower number moves them UP
    float circleY = cy + 50.0f; 
    
    if (puzzle->currentStage == 0) {
        puzzle->tempo = 4.0f;
        puzzle->tolerance = 0.2f; // 400ms window (+/- 0.2s)
        // Stage 1 visuals (Green, Red, Blue) from left to right
        puzzle->circles[0] = (RGBButton){ cx - 190, circleY, 60, GREEN };
        puzzle->circles[1] = (RGBButton){ cx,       circleY, 60, RED };
        puzzle->circles[2] = (RGBButton){ cx + 190, circleY, 60, BLUE };
    }
    else if (puzzle->currentStage == 1) {
        puzzle->tempo = 3.0f;
        puzzle->tolerance = 0.15f; // 300ms window (+/- 0.15s)
        // Stage 2 visuals (Red, Green, Blue) from left to right
        puzzle->circles[0] = (RGBButton){ cx - 190, circleY, 60, RED };
        puzzle->circles[1] = (RGBButton){ cx,       circleY, 60, GREEN };
        puzzle->circles[2] = (RGBButton){ cx + 190, circleY, 60, BLUE };
    }
    else if (puzzle->currentStage == 2) {
        puzzle->tempo = 2.0f;
        puzzle->tolerance = 0.15f; // 200ms window (+/- 0.1s)
        // Stage 3 visuals (Blue, Green, Red) from left to right
        puzzle->circles[0] = (RGBButton){ cx - 190, circleY, 60, BLUE };
        puzzle->circles[1] = (RGBButton){ cx,       circleY, 60, GREEN };
        puzzle->circles[2] = (RGBButton){ cx + 190, circleY, 60, RED };
    }
}

bool UpdateRGBPuzzle(RGBPuzzle* puzzle, Vector2 mousePos, Sound btnSfx, Sound failSfx) {
    if (!puzzle->active || puzzle->solved) return false;
    
    // First frame initialization or transitioning from OpenRGBPuzzle
    if (puzzle->currentBeat == 0 && puzzle->stageTimer == 0.0f) {
        FormatRGBStage(puzzle);
    }
    
    // Update timer
    puzzle->stageTimer += GetFrameTime();
    
    // Check if missed the current beat target window completely
    float targetTime = puzzle->tempo * (puzzle->currentBeat + 1);
    if (puzzle->stageTimer > targetTime + puzzle->tolerance) {
        // Missed!
        PlaySound(failSfx);
        puzzle->currentStage = 0; // reset
        FormatRGBStage(puzzle);
        return false;
    }
    
    // Handle click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool hitVisualCircle = false;
        Color clickedColor = BLANK;
        
        for (int i = 0; i < 3; i++) {
            float dx = mousePos.x - puzzle->circles[i].x;
            float dy = mousePos.y - puzzle->circles[i].y;
            if (dx*dx + dy*dy <= puzzle->circles[i].radius * puzzle->circles[i].radius) {
                hitVisualCircle = true;
                clickedColor = puzzle->circles[i].color;
                break;
            }
        }
        
        if (hitVisualCircle) {
            bool correctTiming = (puzzle->stageTimer >= targetTime - puzzle->tolerance && 
                                  puzzle->stageTimer <= targetTime + puzzle->tolerance);
            
            // Check if correct colour is pressed
            bool correctColor = false;
            Color tgtCol = puzzle->targetOrder[puzzle->currentBeat];
            if (clickedColor.r == tgtCol.r && clickedColor.g == tgtCol.g && clickedColor.b == tgtCol.b) {
                correctColor = true;
            }
            
            if (correctTiming && correctColor) {
                // Success!
                PlaySound(btnSfx);
                puzzle->currentBeat++;
                
                if (puzzle->currentBeat >= 3) {
                    // Stage cleared
                    puzzle->currentStage++;
                    if (puzzle->currentStage >= 3) {
                        puzzle->solved = true;
                        puzzle->active = false;
                        return true;
                    } else {
                        FormatRGBStage(puzzle);
                    }
                }
            } else {
                // Wrong timing or wrong colour!
                PlaySound(failSfx);
                puzzle->currentStage = 0; // reset
                FormatRGBStage(puzzle);
            }
        }
    }
    
    return false;
}

void DrawRGBPuzzle(RGBPuzzle* puzzle) {
    if (!puzzle->active) return;
    
    // Draw background
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.85f));
    
    // Draw scaled stage texture
    Texture2D tex = puzzle->textures[puzzle->currentStage];
    if (tex.id != 0) {
        // Scale to fit screen
        float scale = fminf((float)VIRTUAL_WIDTH * 0.6f / tex.width, (float)VIRTUAL_HEIGHT * 0.6f / tex.height);
        Vector2 pos = { (VIRTUAL_WIDTH - tex.width * scale) / 2.0f, (VIRTUAL_HEIGHT - tex.height * scale) / 2.0f };
        
        // Offset a bit to the right and down
        pos.x += 40.0f;
        pos.y += 60.0f;
        
        DrawTextureEx(tex, pos, 0.0f, scale, WHITE);
    }
    
    // Draw circles
    for(int i=0; i<3; i++) {
        bool isPressed = false;
        for(int b=0; b < puzzle->currentBeat; b++) {
            if (puzzle->circles[i].color.r == puzzle->targetOrder[b].r &&
                puzzle->circles[i].color.g == puzzle->targetOrder[b].g &&
                puzzle->circles[i].color.b == puzzle->targetOrder[b].b) {
                isPressed = true;
                break;
            }
        }
        
        Vector2 mousePos = GetVirtualMouse();
        float dx = mousePos.x - puzzle->circles[i].x;
        float dy = mousePos.y - puzzle->circles[i].y;
        float r = puzzle->circles[i].radius;
        bool isHovered = (dx*dx + dy*dy <= r*r);
        bool isBeingClicked = isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        
        float currentRadius = puzzle->circles[i].radius;
        Color renderColor = puzzle->circles[i].color;
        
        if (isPressed || isBeingClicked) {
            // Make the button noticeably darker and appear pressed down
            renderColor.r = (unsigned char)(renderColor.r * 0.4f);
            renderColor.g = (unsigned char)(renderColor.g * 0.4f);
            renderColor.b = (unsigned char)(renderColor.b * 0.4f);
            currentRadius -= 3.0f; 
        }
        
        DrawCircle(puzzle->circles[i].x, puzzle->circles[i].y, currentRadius + 3, WHITE);
        DrawCircle(puzzle->circles[i].x, puzzle->circles[i].y, currentRadius, renderColor);
        
        // Extra dimming for buttons that have already been locked in
        if (isPressed) {
            DrawCircle(puzzle->circles[i].x, puzzle->circles[i].y, currentRadius, Fade(BLACK, 0.5f));
        }
    }
    
    // Draw "NOW!" if in tolerance window
    float targetTime = puzzle->tempo * (puzzle->currentBeat + 1);
    if (puzzle->stageTimer >= targetTime - puzzle->tolerance && 
        puzzle->stageTimer <= targetTime + puzzle->tolerance) {
        const char* nowTxt = "NOW!";
        int tw = MeasureText(nowTxt, 60);
        DrawText(nowTxt, (VIRTUAL_WIDTH - tw)/2, 90, 60, YELLOW);
    }
    
    // Stage info
    const char* stgTxt = TextFormat("STAGE %d", puzzle->currentStage + 1);
    int sw = MeasureText(stgTxt, 30);
    DrawText(stgTxt, (VIRTUAL_WIDTH - sw)/2, 10, 30, RAYWHITE);
    
    // Timer display
    const char* timerTxt = TextFormat("Timer: %.2fs", puzzle->stageTimer);
    int tw2 = MeasureText(timerTxt, 30);
    DrawText(timerTxt, (VIRTUAL_WIDTH - tw2)/2, 50, 30, LIGHTGRAY);
}

// ─────────────────────────────────────────────────────────────────────────────
// CodePuzzle — memory sequence minigame state.
// ─────────────────────────────────────────────────────────────────────────────
void InitCodePuzzle(CodePuzzle* puzzle, Texture2D bg) {
    puzzle->active = false;
    puzzle->solved = false;
    puzzle->bgTexture = bg;
    
    // Layout 8 buttons: 4 on top, 4 on bottom, centered
    // With spacingX=100 and btnW=70, total width = 3*100+70 = 370. Centered left is -185.
    float startX = VIRTUAL_WIDTH / 2.0f - 210.0f;
    float startY = VIRTUAL_HEIGHT / 2.0f - 25.0f;
    float btnW = 70.0f;
    float btnH = 50.0f;
    float spacingX = 118.0f;
    float spacingY = 110.0f;
    
    for (int i = 0; i < 8; i++) {
        puzzle->buttons[i].number = i + 1;
        puzzle->buttons[i].pressTimer = 0.0f;
        
        float bx = startX + (i % 4) * spacingX;
        float by = startY + (i / 4) * spacingY;
        puzzle->buttons[i].bounds = (Rectangle){ bx, by, btnW, btnH };
    }
}

void OpenCodePuzzle(CodePuzzle* puzzle) {
    if (puzzle->solved) return;
    puzzle->active = true;
    puzzle->currentRound = 0; // Starts at round 0 (length 4)
    
    // Setup first round
    puzzle->state = CODE_MEMORIZE;
    puzzle->seqLength = 4;
    puzzle->inputIndex = 0;
    // Base 2.0 sec for 4 digits
    puzzle->stateTimer = 2.0f; 
    
    for (int i = 0; i < puzzle->seqLength; i++) {
        puzzle->sequence[i] = GetRandomValue(1, 8);
    }
}

void CloseCodePuzzle(CodePuzzle* puzzle) {
    puzzle->active = false;
}

bool UpdateCodePuzzle(CodePuzzle* puzzle, Vector2 mousePos, Sound btnSfx, Sound failSfx) {
    if (!puzzle->active || puzzle->solved) return false;
    
    float dt = GetFrameTime();
    
    // Decrement visual press timers
    for (int i = 0; i < 8; i++) {
        if (puzzle->buttons[i].pressTimer > 0.0f) {
            puzzle->buttons[i].pressTimer -= dt;
        }
    }
    
    if (puzzle->state == CODE_MEMORIZE) {
        puzzle->stateTimer -= dt;
        if (puzzle->stateTimer <= 0.0f) {
            puzzle->state = CODE_INPUT;
        }
    } 
    else if (puzzle->state == CODE_FAIL) {
        puzzle->stateTimer -= dt;
        if (puzzle->stateTimer <= 0.0f) {
            OpenCodePuzzle(puzzle); // Restart cleanly from round 1
        }
    }
    else if (puzzle->state == CODE_INPUT) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < 8; i++) {
                if (CheckCollisionPointRec(mousePos, puzzle->buttons[i].bounds)) {
                    puzzle->buttons[i].pressTimer = 0.2f;
                    
                    if (puzzle->buttons[i].number == puzzle->sequence[puzzle->inputIndex]) {
                        // Correct button pressed
                        PlaySound(btnSfx);
                        puzzle->inputIndex++;
                        
                        if (puzzle->inputIndex >= puzzle->seqLength) {
                            // Round complete
                            puzzle->currentRound++;
                            if (puzzle->currentRound >= 5) {
                                puzzle->solved = true;
                                puzzle->active = false;
                                return true; // Handled dynamically in gameplay loop
                            } else {
                                // Transition to next round dynamically
                                puzzle->state = CODE_MEMORIZE;
                                puzzle->seqLength = 4 + puzzle->currentRound;
                                puzzle->inputIndex = 0;
                                
                                // Increase timer strictly by 0.5s per round!
                                puzzle->stateTimer = 2.0f + (puzzle->currentRound * 1.0f);
                                
                                for (int j = 0; j < puzzle->seqLength; j++) {
                                    puzzle->sequence[j] = GetRandomValue(1, 8);
                                }
                            }
                        }
                    } else {
                        // Wrong sequence input!
                        PlaySound(failSfx);
                        puzzle->state = CODE_FAIL;
                        puzzle->stateTimer = 1.5f; // Short burst error screen
                    }
                    break;
                }
            }
        }
    }
    return false;
}

void DrawCodePuzzle(CodePuzzle* puzzle) {
    if (!puzzle->active) return;
    
    // Draw Dark overlay
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, 0.85f));
    
    // Draw bg terminal
    if (puzzle->bgTexture.id != 0) {
        float scale = fminf((float)VIRTUAL_WIDTH * 0.95f / puzzle->bgTexture.width, (float)VIRTUAL_HEIGHT * 0.95f / puzzle->bgTexture.height);
        Vector2 pos = { (VIRTUAL_WIDTH - puzzle->bgTexture.width * scale) / 2.0f, (VIRTUAL_HEIGHT - puzzle->bgTexture.height * scale) / 2.0f };
        pos.y -= 70.0f; // Shift background up
        DrawTextureEx(puzzle->bgTexture, pos, 0.0f, scale, WHITE);
    }
    
    // Draw 8 interactive buttons
    for (int i = 0; i < 8; i++) {
        Rectangle r = puzzle->buttons[i].bounds;
        Color btnCol = GRAY;
        
        if (puzzle->buttons[i].pressTimer > 0.0f) {
            btnCol = DARKGRAY;
        } else if (puzzle->state == CODE_INPUT && CheckCollisionPointRec(GetVirtualMouse(), r)) {
            btnCol = LIGHTGRAY;
        }
        
        // Draw Button Fill
        DrawRectangleRounded(r, 0.2f, 10, btnCol);
        
        // Draw Button Outline thick if input state
        float lineThick = (puzzle->state == CODE_INPUT) ? 3.0f : 1.0f;
        DrawRectangleRoundedLines(r, 0.2f, 10, DARKGRAY); // Raylib thick lines aren't directly supported by RoundedLines, so standard it is
        DrawRectangleLinesEx(r, lineThick, DARKGRAY);
        
        // Draw explicit button numbers
        const char* numStr = TextFormat("%d", puzzle->buttons[i].number);
        int tw = MeasureText(numStr, 24);
        DrawText(numStr, (int)(r.x + r.width/2 - tw/2), (int)(r.y + 13), 24, RAYWHITE);
    }
    
    // ── State rendering ──
    if (puzzle->state == CODE_MEMORIZE) {
        // Draw sequence string nicely spaced
        char buf[128] = {0};
        for (int i = 0; i < puzzle->seqLength; i++) {
            char t[8];
            if (i == puzzle->seqLength - 1) {
                snprintf(t, sizeof(t), "%d", puzzle->sequence[i]);
            } else {
                snprintf(t, sizeof(t), "%d ", puzzle->sequence[i]);
            }
            strcat(buf, t);
        }
        
        int w = MeasureText(buf, 40);
        int cx = (VIRTUAL_WIDTH - w) / 2;
        int cy = VIRTUAL_HEIGHT / 2 - 190;
        
        DrawRectangle(cx - 30, cy - 15, w + 60, 70, Fade(BLACK, 0.8f));
        DrawRectangleLines(cx - 30, cy - 15, w + 60, 70, WHITE);
        DrawText(buf, cx, cy, 40, WHITE);
        
        // Subtle countdown timer
        const char* timerStr = TextFormat("%.1fs", puzzle->stateTimer);
        int tw2 = MeasureText(timerStr, 20);
        DrawText(timerStr, (VIRTUAL_WIDTH - tw2)/2, cy + 65, 20, LIGHTGRAY);
    } 
    else if (puzzle->state == CODE_FAIL) {
        const char* msg = "ERROR! FAILED";
        int w = MeasureText(msg, 40);
        int cx = (VIRTUAL_WIDTH - w) / 2;
        int cy = VIRTUAL_HEIGHT / 2 - 190;
        
        DrawRectangle(cx - 30, cy - 15, w + 60, 70, Fade(BLACK, 0.8f));
        DrawRectangleLines(cx - 30, cy - 15, w + 60, 70, RED);
        DrawText(msg, cx, cy, 40, RED);
    }
    else if (puzzle->state == CODE_INPUT) {
        const char* msg = TextFormat("INPUT SEQUENCE (%d/%d)", puzzle->inputIndex, puzzle->seqLength);
        int w = MeasureText(msg, 30);
        DrawText(msg, (VIRTUAL_WIDTH - w) / 2, VIRTUAL_HEIGHT / 2 - 190, 30, GRAY);
    }
    
    // Close instructions (bottom screen)
    const char* hint = "Press 'E' to leave console";
    int hw = MeasureText(hint, 20);
    DrawText(hint, (VIRTUAL_WIDTH - hw)/2, VIRTUAL_HEIGHT - 40, 20, GRAY);
}

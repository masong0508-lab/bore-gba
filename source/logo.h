#pragma once
// logo_play(): plays the ~6 s logo (blocking). Call once at boot. Leaves a black screen, all layers/DMA off.
// Uses mode 0, BG0-3, OBJ, windows, BLDY, HBlank DMA1-3. Build: `make` (needs devkitARM).
void logo_play(void* scratch);   // scratch: 6.3 KB of RAM the logo may use while it plays (the game lends its frame buffer)
#define LOGO_FRAMES (359 + 120)   /* +120 frames (2 s) hold before the fade-out */

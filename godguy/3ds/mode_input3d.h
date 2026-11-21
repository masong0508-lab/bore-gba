// 3DS game-mode input edge guard. Prevent held SELECT+R/SELECT+Y,
// START, or touchscreen press from leaking into the following mode.
// Masks ONLY keys held during the transition; new keys respond at once.
// Allocation-free and independent from libctru, suitable for host tests.
#ifndef GG_MODE_INPUT3D_H
#define GG_MODE_INPUT3D_H
typedef struct { int suppressUntilUp; } GgModeInputGuard;
static void ggModeInputTransition(GgModeInputGuard *g,int keys){
    g->suppressUntilUp|=keys;
}
static int ggModeInputFilter(GgModeInputGuard *g,int keys){
    // Retain a key in the mask only while it is STILL physically held.
    // A newly pressed key that wasn't part of the mode transition is
    // immediately delivered. Individual releases unlock the old keys.
    g->suppressUntilUp&=keys;
    return keys&~g->suppressUntilUp;
}
#endif

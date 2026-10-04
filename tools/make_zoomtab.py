#!/usr/bin/env python3
"""Writes source/zoomtab.h: the per-scanline BG2 scaling tables of the in-game ZOOM (OPTIONS > VIDEO > ZOOM, SELECT + UP / DOWN).

Zoomed, the game draws only a window in the middle of the room (zoomWin) and the GBA's own BG2 scaling stretches it over the room
rows of the screen; the HUD rows above and below stay 1:1. One table line per scanline (and one more: the DMA reads a line ahead):
BG2PA, PB, PC, PD, BG2X, BG2Y, written by an HBlank DMA (main.c, zoomVbl). Tables for the two HUD layouts (panels on: room rows
10..129; INFO ON SCREEN OFF: 10..159) and the two zooms (1.5X, 2X). Run it again if HUD_TOPH / HUD_BOTY (hud.h) change."""
import os
SW, SH, TOP, BOT = 240, 160, 10, 130
def table(b0, b1, z):
    hb = b1 - b0
    ww, hw, pa = (120, hb // 2, 128) if z == 2 else (160, hb * 2 // 3, 170)   # 2X: half as many pixels each way; 1.5X: two thirds
    x0 = (SW - ww) // 2; y0 = b0 + (hb - hw) // 2
    rows = []
    for L in range(SH + 1):
        if b0 <= L < b1:
            y = y0 * 256 + ((L - b0) * 128 if z == 2 else (L - b0) * 512 // 3)
            rows.append((pa, 0, 0, pa, x0 * 256, y))
        else:   # (the extra line after the last one is line 0's: BG2X / BG2Y are latched from it at vblank)
            rows.append((256, 0, 0, 256, 0, (L % SH) * 256))
    return (x0, x0 + ww, y0, y0 + hw), rows
out = ['// zoomtab.h - made by tools/make_zoomtab.py (do not edit): the in-game ZOOM\'s per-scanline BG2 scaling. See that script.',
       '#define ZT_N %d' % (SH + 1),
       'typedef struct { u16 pa, pb, pc, pd; s32 x, y; } ZLn;   // one scanline: BG2PA..PD, BG2X, BG2Y (the order of the registers)',
       '_Static_assert(HUD_TOPH==%d&&HUD_BOTY==%d,"the HUD moved: run tools/make_zoomtab.py again");' % (TOP, BOT)]
wins = []; tabs = []
for hudoff in (0, 1):
    for z in (1, 2):
        w, rows = table(TOP, SH if hudoff else BOT, 2 if z == 2 else 1)
        wins.append('{%d,%d,%d,%d}' % w)
        tabs.append('{' + ','.join('{%d,%d,%d,%d,%d,%d}' % r for r in rows) + '}')
out.append('static const u8 zoomWin[2][2][4]={{%s,%s},{%s,%s}};   // [INFO ON SCREEN OFF][1.5X, 2X]: the room window drawn (x0, x1, y0, y1)' % tuple(wins))
out.append('static const ZLn zoomTab[2][2][ZT_N]={{%s,\n%s},\n{%s,\n%s}};' % tuple(tabs))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'source', 'zoomtab.h'), 'w').write('\n'.join(out) + '\n')
print('wrote source/zoomtab.h:', wins)

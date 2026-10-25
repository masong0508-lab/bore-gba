"""bld.py - the buildings: 32 x 32 pixel (2 x 2 cells), each with its own 16 colour palette.
Palette roles (indices): 0 outline  1 grass shadow  2 grass  3 grass light  4 wall dark  5 wall  6 wall light  7 roof dark  8 roof  9 roof light
                         10 wood / door  11 window  12 window glint  13 accent A  14 accent B  15 highlight"""
import random
from pix import Cv, lit, rgb15 as C
O, GS, G, GL, WD, WM, WL, RD, RM, RL, WO, WIN, WG, A1, A2, HI = range(16)
def pal(**k):
    base = dict(o=C(2,3,8), gs=C(3,9,5), g=C(8,20,7), gl=C(12,24,9))
    base.update(k); r = [base[n] for n in ('o','gs','g','gl','wd','wm','wl','rd','rm','rl','wo','win','wg','a1','a2','hi')]; return r
PAL = {
 'hut':    pal(wd=C(15,9,5), wm=C(21,14,8), wl=C(26,19,11), rd=C(18,12,4), rm=C(25,19,7), rl=C(30,25,11), wo=C(9,5,3), win=C(4,5,10), wg=C(26,26,8), a1=C(28,28,26), a2=C(20,20,20), hi=C(31,31,29)),
 'house':  pal(wd=C(18,16,13), wm=C(26,24,20), wl=C(30,29,25), rd=C(18,5,5), rm=C(26,9,7), rl=C(30,15,10), wo=C(10,6,3), win=C(5,10,20), wg=C(22,28,31), a1=C(14,9,5), a2=C(21,19,18), hi=C(31,31,31)),
 'wcut':   pal(wd=C(9,5,3), wm=C(15,9,5), wl=C(21,14,7), rd=C(4,12,5), rm=C(7,18,7), rl=C(12,24,9), wo=C(7,4,2), win=C(4,5,9), wg=C(24,24,8), a1=C(22,22,23), a2=C(27,27,28), hi=C(28,20,10)),
 'quarry': pal(wd=C(8,8,11), wm=C(14,14,17), wl=C(21,21,24), rd=C(15,9,5), rm=C(21,14,8), rl=C(26,19,11), wo=C(9,5,3), win=C(3,3,7), wg=C(25,25,26), a1=C(24,24,27), a2=C(29,29,30), hi=C(31,31,31)),
 'saw':    pal(wd=C(11,6,3), wm=C(18,11,5), wl=C(25,17,8), rd=C(10,6,3), rm=C(16,10,5), rl=C(22,15,8), wo=C(7,4,2), win=C(3,3,7), wg=C(26,26,28), a1=C(24,24,27), a2=C(30,25,15), hi=C(31,29,22)),
 'mason':  pal(wd=C(10,10,14), wm=C(17,17,21), wl=C(24,24,27), rd=C(5,8,18), rm=C(9,13,26), rl=C(15,20,30), wo=C(9,5,3), win=C(4,5,10), wg=C(24,28,31), a1=C(26,26,28), a2=C(29,22,8), hi=C(31,31,31)),
 'store':  pal(wd=C(11,6,3), wm=C(19,12,6), wl=C(26,18,9), rd=C(10,12,15), rm=C(15,17,21), rl=C(21,23,27), wo=C(6,3,2), win=C(3,3,6), wg=C(27,23,6), a1=C(27,20,8), a2=C(21,13,6), hi=C(30,27,14)),
 'sheep':  pal(wd=C(11,6,3), wm=C(18,11,5), wl=C(25,17,8), rd=C(10,6,3), rm=C(16,10,5), rl=C(22,15,8), wo=C(7,4,2), win=C(3,3,6), wg=C(24,24,8), a1=C(30,30,29), a2=C(22,22,23), hi=C(31,31,31)),
 'mine':   pal(wd=C(7,7,10), wm=C(12,12,15), wl=C(18,18,21), rd=C(10,6,3), rm=C(16,10,5), rl=C(22,15,8), wo=C(5,3,2), win=C(2,2,5), wg=C(14,27,31), a1=C(4,17,23), a2=C(11,26,31), hi=C(31,31,31)),
 'weaver': pal(wd=C(15,10,6), wm=C(23,17,11), wl=C(28,23,16), rd=C(14,5,18), rm=C(21,9,26), rl=C(28,16,30), wo=C(9,5,3), win=C(4,5,10), wg=C(26,28,31), a1=C(30,12,18), a2=C(10,22,27), hi=C(31,31,31)),
 'cutter': pal(wd=C(14,14,18), wm=C(21,21,25), wl=C(27,27,30), rd=C(3,10,20), rm=C(6,17,27), rl=C(13,24,31), wo=C(9,5,3), win=C(3,4,9), wg=C(24,31,31), a1=C(5,22,28), a2=C(13,28,31), hi=C(31,31,31)),
 'castle': pal(wd=C(13,13,17), wm=C(20,20,24), wl=C(27,27,29), rd=C(18,4,5), rm=C(26,7,8), rl=C(30,13,12), wo=C(9,5,3), win=C(2,3,7), wg=C(30,26,6), a1=C(30,24,3), a2=C(31,29,12), hi=C(31,31,31)),
 'tavern': pal(wd=C(14,9,5), wm=C(21,14,8), wl=C(27,20,12), rd=C(13,3,8), rm=C(21,6,12), rl=C(28,12,17), wo=C(8,4,2), win=C(26,20,6), wg=C(31,29,14), a1=C(28,24,10), a2=C(20,20,22), hi=C(31,31,28)),
}
PAL['mc'] = pal(wd=C(10,10,14), wm=C(17,17,21), wl=C(24,24,27), rd=C(4,9,20), rm=C(8,15,27), rl=C(14,22,31), wo=C(9,5,3), win=C(4,5,10), wg=C(24,29,31), a1=C(4,18,25), a2=C(13,28,31), hi=C(31,31,31))
PAL['well'] = pal(wd=C(11,11,15), wm=C(18,18,22), wl=C(25,25,28), rd=C(13,3,8), rm=C(20,6,10), rl=C(27,12,15), wo=C(9,5,3), win=C(3,9,21), wg=C(14,24,31), a1=C(20,13,6), a2=C(26,19,9), hi=C(31,31,31))
PAL['site'] = pal(wd=C(16,11,6), wm=C(21,16,9), wl=C(26,22,14), rd=C(9,5,3), rm=C(15,9,5), rl=C(22,15,8), wo=C(7,4,2), win=C(3,3,6), wg=C(24,24,8), a1=C(26,22,14), a2=C(30,26,18), hi=C(31,31,29))
def ground(seed, w=32, h=32):
    r = random.Random(seed); c = Cv(w, h, G)
    for y in range(h):
        for x in range(w):
            v = r.random(); c.p[y][x] = GS if v < .06 else GL if v > .93 else G
    return c
def shadow(c, cx, cy, rx, ry):
    c.ell(cx, cy, rx, ry, GS, only={G, GL, GS})
def wall(c, x0, y0, x1, y1, tex='plank', side=False):
    w, h = x1 - x0 + 1, y1 - y0 + 1; c.rect(x0, y0, w, h, WM)
    if tex == 'plank':
        for y in range(y0 + 2, y1, 3): c.hline(x0, x1, y, WD)
        for y in range(y0 + 1, y1, 3): c.hline(x0 + 1, x1 - 1, y, WL) if False else None
    elif tex == 'stone':
        for k, y in enumerate(range(y0, y1 + 1, 4)):
            c.hline(x0, x1, y, WD)
            for x in range(x0 + (2 if k % 2 else 0), x1 + 1, 5): c.vline(x, y, min(y + 3, y1), WD)
            for x in range(x0 + 1, x1, 5): c.px(x + (2 if k % 2 else 0), y + 1, WL)
    elif tex == 'log':
        for k, y in enumerate(range(y0, y1 + 1, 3)):
            c.hline(x0, x1, y, WD); c.hline(x0, x1, y + 1, WL) if y + 1 <= y1 else None
            c.px(x0, y, WL); c.px(x1, y, WD)
    elif tex == 'plaster':
        c.rect(x0, y0, w, h, WL)
        for y in range(y0, y1 + 1, 6):
            c.hline(x0, x1, y, WO)
        c.vline(x0, y0, y1, WO); c.vline(x1, y0, y1, WO); c.vline((x0 + x1) // 2, y0, y1, WO)
    c.vline(x0, y0, y1, WL if tex != 'plaster' else WO); c.vline(x1, y0, y1, WD if tex != 'plaster' else WO)
    c.hline(x0, x1, y1, WD)
def roof(c, x0, x1, ybase, h, shingle=True, ridge=True, over=2):   # a trapezoid roof seen from the front and above
    for j in range(h):
        y = ybase - j; t = j / max(1, h - 1)
        xl = x0 - over + int(t * (x1 - x0 - 2 * over) / 2 * .55); xr = x1 + over - int(t * (x1 - x0 - 2 * over) / 2 * .55)
        for x in range(xl, xr + 1):
            col = RM
            if shingle and (j % 3 == 2): col = RD
            if shingle and (j % 3 == 0) and ((x + j) % 4 == 0): col = RD
            if x == xl or x == xl + 1: col = RL
            if x == xr: col = RD
            c.px(x, y, col)
        if j == 0: c.hline(xl, xr, y, RD)
    if ridge:
        t = (h - 1) / max(1, h - 1); xl = x0 - over + int(t * (x1 - x0 - 2 * over) / 2 * .55); xr = x1 + over - int(t * (x1 - x0 - 2 * over) / 2 * .55)
        c.hline(xl, xr, ybase - h + 1, RL)
    c.hline(x0 - over, x1 + over, ybase + 1, WD)   # the eave's shadow on the wall
def door(c, x, y, w, h, arch=True):
    c.rect(x, y, w, h, WO); c.vline(x, y, y + h - 1, WD); c.hline(x, x + w - 1, y, WD)
    if arch: c.px(x, y, WM); c.px(x + w - 1, y, WM)
    c.px(x + w - 2, y + h // 2, WG)
def window(c, x, y, w=3, h=3, shut=True):
    c.rect(x, y, w, h, WIN); c.px(x, y, WG)
    if shut: c.vline(x - 1, y, y + h - 1, WO); c.vline(x + w, y, y + h - 1, WO)
    c.hline(x - 1, x + w, y + h, WD)
def chimney(c, x, y, h):
    c.rect(x, y, 3, h, WD); c.vline(x, y, y + h - 1, WM); c.hline(x - 1, x + 3, y, WL)
def finish(c): # a thin dark outline around the non-ground art so it reads on any terrain
    return c
def hut():
    c = ground(1); shadow(c, 16, 27, 11, 3)
    wall(c, 8, 19, 23, 27, 'plank'); 
    c.ell(16, 21, 11, 4, lambda x, y, nx, ny: WM if ny > 0 else None, only={WM, WD, WL})   # clay wall belly
    for y in range(19, 28):
        for x in range(8, 24):
            if c.get(x, y) == WM: c.px(x, y, WM if (x + y) % 5 else WL if x < 12 else WD if x > 20 else WM)
    c.rect(8, 27, 16, 1, WD)
    # the thatch: a tall dome with straw stripes
    c.ell(16, 15, 12.5, 9.5, lambda x, y, nx, ny: lit(nx, ny, [RD, RM, RM, RL]) if True else None, only={G, GL, GS, WM, WD, WL})
    for y in range(6, 24):
        for x in range(3, 30):
            if c.get(x, y) in (RD, RM, RL) and ((x * 3 + y) % 5 == 0): c.px(x, y, RD)
    for x in range(4, 29):
        if c.get(x, 22) in (RD, RM, RL): c.px(x, 23, WD)
    c.px(15, 5, RL); c.px(16, 5, RL); c.px(14, 6, RL)
    door(c, 13, 20, 6, 8); window(c, 22, 18, 2, 2, False)
    for (x, y) in ((6, 22), (26, 22), (9, 26)): c.px(x, y, GL)
    return c
def house():
    c = ground(2); shadow(c, 16, 28, 13, 3)
    wall(c, 5, 17, 26, 28, 'plaster'); wall(c, 5, 24, 26, 28, 'stone')
    roof(c, 5, 26, 18, 11, True, True, 3)
    chimney(c, 21, 3, 8); c.px(22, 2, WG)
    door(c, 13, 20, 6, 9); window(c, 7, 20, 3, 3); window(c, 21, 20, 3, 3); window(c, 15, 12, 3, 3, False)
    c.hline(11, 20, 19, WO)
    for (x, y) in ((3, 29), (28, 29), (9, 30), (23, 30)): c.px(x, y, GL)
    return c
def woodcutter():
    c = ground(3); shadow(c, 14, 28, 12, 3)
    wall(c, 4, 15, 21, 27, 'log'); roof(c, 4, 21, 16, 9, True, True, 2)
    door(c, 10, 19, 5, 9, False); window(c, 16, 19, 3, 3, False)
    # log pile
    for k, (x, y) in enumerate(((23, 26), (27, 26), (25, 23), (23, 30), (27, 30), (25, 27))):
        c.ell(x + 1.5, y, 2.6, 2.1, lambda a, b, nx, ny: WO if nx * nx + ny * ny > .5 else (A1 if nx * nx + ny * ny > .17 else WL), only={G, GL, GS})
        c.px(x + 1, y, A1)
    # stump and axe
    c.ell(7.5, 29, 3, 1.6, WM, only={G, GL, GS}); c.hline(5, 9, 29, WL); c.rect(5, 29, 5, 2, WD)
    c.line(8, 27, 10, 24, WO); c.px(10, 23, A1); c.px(11, 24, A2); c.px(11, 23, A1)
    return c
def quarry():
    c = ground(4)
    c.ell(16, 20, 14, 10, lambda x, y, nx, ny: WD if nx * nx + ny * ny > .62 else WM, only={G, GL, GS})
    for y in range(8, 32):
        for x in range(2, 30):
            if c.get(x, y) == WM and (x * 7 + y * 3) % 9 == 0: c.px(x, y, WD)
            if c.get(x, y) == WM and (x * 5 + y * 11) % 13 == 0: c.px(x, y, WL)
    # cut blocks stacked
    for (x, y) in ((6, 23), (11, 23), (8, 19), (21, 25), (17, 21)):
        c.rect(x, y, 5, 4, WL); c.hline(x, x + 4, y, HI); c.vline(x + 4, y, y + 3, WD); c.hline(x, x + 4, y + 3, WD)
    # a little wooden hoist
    c.rect(22, 9, 2, 14, WO); c.rect(14, 9, 10, 2, WO); c.line(14, 11, 18, 9, WO); c.vline(16, 11, 16, A1); c.rect(15, 16, 3, 2, A2)
    c.line(24, 23, 27, 22, WO); c.px(27, 21, A1); c.px(28, 22, A2); c.px(26, 21, A2)
    return c
def sawmill():
    c = ground(5); shadow(c, 16, 28, 13, 3)
    c.rect(4, 12, 2, 16, WO); c.rect(23, 12, 2, 16, WO)
    roof(c, 3, 25, 13, 8, True, True, 1)
    c.rect(6, 17, 17, 9, WD); c.rect(6, 16, 17, 1, WO)
    c.ell(13, 22, 5.6, 5.6, lambda x, y, nx, ny: A1 if nx * nx + ny * ny > .5 else A2, only={WD})
    for a in range(0, 360, 30):
        import math
        c.px(int(13 + 6.3 * math.cos(math.radians(a))), int(22 + 6.3 * math.sin(math.radians(a))), HI)
    c.px(13, 22, WO); c.px(12, 22, WO); c.px(14, 22, WO)
    for k in range(4): c.rect(8, 28 - 0, 14, 1, WL) if False else None
    for y, w in ((25, 6), (27, 7), (29, 8)):
        c.rect(24, y, w, 1, A2); c.hline(24, 24 + w - 1, y + 1, WL)
    c.rect(22, 26, 8, 4, WL); 
    for y in (26, 28): c.hline(22, 29, y, WD)
    return c
def mason():
    c = ground(6); shadow(c, 14, 28, 12, 3)
    wall(c, 4, 15, 21, 27, 'stone'); roof(c, 4, 21, 16, 9, True, True, 2)
    door(c, 9, 19, 5, 9); window(c, 16, 19, 3, 3, False)
    for (x, y) in ((24, 24), (24, 28), (28, 26)):
        c.rect(x, y, 5, 3, WL); c.hline(x, x + 4, y, HI); c.vline(x + 4, y, y + 2, WD)
    # anvil and hammer
    c.rect(24, 20, 6, 2, A1); c.rect(26, 22, 2, 2, WD); c.px(24, 19, A1)
    c.line(28, 18, 30, 16, WO); c.px(30, 15, A1); c.px(31, 16, A1)
    return c
def storehouse():
    c = ground(7); shadow(c, 16, 28, 14, 3)
    wall(c, 3, 14, 28, 28, 'plank'); roof(c, 3, 28, 15, 10, True, True, 2)
    c.rect(11, 18, 10, 10, WD); c.rect(12, 19, 4, 9, WO); c.rect(16, 19, 4, 9, WO); c.line(12, 19, 15, 27, WD); c.line(15, 19, 12, 27, WD); c.line(16, 19, 19, 27, WD); c.line(19, 19, 16, 27, WD)
    c.rect(14, 8, 4, 4, WD); c.rect(15, 9, 2, 2, WIN); c.px(15, 9, WG)
    for (x, y) in ((4, 26), (7, 26), (5, 23)):
        c.rect(x, y, 3, 3, A1); c.hline(x, x + 2, y, A2); c.px(x + 1, y + 1, WD)
    for (x, y) in ((22, 24), (25, 25)):
        c.ell(x + 1.5, y + 1.5, 2.2, 2.6, lambda a, b, nx, ny: A2 if nx > .2 else A1, only={WM, WD, WL, G, GL, GS, WO}); c.hline(x, x + 3, y + 1, WD)
    c.rect(22, 18, 4, 4, WL) ; c.rect(23, 19, 2, 2, WD)
    return c
def sheep_pen():
    c = ground(8)
    for x in range(2, 30, 4):
        for y in (6, 27): c.rect(x, y - 2, 2, 5, WO); c.px(x, y - 2, WL)
    for x in range(2, 30): c.px(x, 5, WM); c.px(x, 26, WM)
    for y in range(4, 28, 4):
        for x in (2, 28): c.rect(x, y - 2, 2, 5, WO)
    for y in range(5, 27): c.px(2, y, WM) if y % 2 else None; c.px(29, y, WM) if y % 2 else None
    def sheep(x, y, face):
        c.ell(x, y, 4.3, 3.3, lambda a, b, nx, ny: lit(nx, ny, [A2, A2, A1, HI]), only=None)
        c.ell(x + (4.4 if face else -4.4), y + 0.6, 1.8, 1.7, WD); c.px(int(x + (5 if face else -5)), y, O)
        c.px(x - 2, y + 4, WD); c.px(x + 2, y + 4, WD); c.px(x - 2, y + 3, WD); c.px(x + 2, y + 3, WD)
    sheep(10, 14, 1); sheep(22, 20, 0); sheep(20, 11, 1)
    c.rect(4, 21, 8, 5, WM); roof(c, 4, 11, 21, 4, True, False, 1) if False else None
    c.rect(5, 22, 6, 1, WL); c.rect(5, 21, 6, 5, WD) if False else None
    return c
def crystal_mine():
    c = ground(9)
    c.ell(16, 20, 15, 12, lambda x, y, nx, ny: lit(nx, ny, [WD, WM, WM, WL]) if True else None, only={G, GL, GS})
    for y in range(6, 32):
        for x in range(1, 31):
            if c.get(x, y) in (WD, WM, WL) and (x * 7 + y * 5) % 11 == 0: c.px(x, y, WD)
    # the mine mouth: a timbered arch
    c.rect(11, 19, 10, 10, O); c.rect(10, 18, 12, 2, RD); c.rect(10, 18, 2, 11, RD); c.rect(20, 18, 2, 11, RD); c.vline(10, 18, 28, RM); c.hline(10, 21, 18, RM)
    c.px(15, 24, WG); c.px(16, 25, WG)
    c.line(11, 29, 21, 29, WO)
    # crystals around
    for (x, y, h) in ((4, 14, 7), (8, 11, 9), (24, 12, 8), (27, 16, 6), (14, 9, 5)):
        for j in range(h):
            half = max(0, (h - j) // 4 + (1 if j > 1 else 0)) if j else 0
            for k in range(-half, half + 1): c.px(x + k, y + 4 - j + 0, A2 if k < 0 else A1 if k > 0 else WG if j % 3 == 1 else A2)
        c.px(x - 1, y + 2 - h // 2, HI)
    return c
def weaver():
    c = ground(10); shadow(c, 13, 28, 11, 3)
    wall(c, 4, 15, 20, 27, 'plaster'); roof(c, 4, 20, 16, 9, True, True, 2)
    door(c, 9, 19, 5, 9); window(c, 15, 19, 3, 3, True)
    # a loom with bright cloth
    c.rect(23, 14, 2, 15, WO); c.rect(29, 14, 2, 15, WO); c.rect(23, 14, 8, 2, WO)
    for y in range(17, 28):
        for x in range(25, 29): c.px(x, y, A1 if (x + y // 2) % 2 else A2 if y % 4 else HI)
    return c
def cutter():
    c = ground(11); shadow(c, 13, 28, 11, 3)
    wall(c, 4, 15, 20, 27, 'stone'); roof(c, 4, 20, 16, 9, True, True, 2)
    door(c, 9, 19, 5, 9); window(c, 15, 19, 3, 3, False)
    # grindstone
    c.ell(25.5, 24, 4.4, 4.4, lambda x, y, nx, ny: A2 if nx * nx + ny * ny > .5 else WL, only=None)
    c.ell(25.5, 24, 1.3, 1.3, WD); c.rect(22, 28, 8, 2, WO)
    for (x, y) in ((24, 21), (27, 22)): c.px(x, y, HI)
    # a gem on a post
    c.vline(26, 12, 19, WO)
    for j, w in enumerate((0, 1, 2, 2, 1, 0)): 
        for k in range(-w, w + 1): c.px(26 + k, 9 + j, A2 if k <= 0 else A1)
    c.px(25, 10, HI)
    return c
def well():
    c = ground(12); shadow(c, 16, 26, 11, 3)
    c.ell(16, 21, 10, 5.6, lambda x, y, nx, ny: lit(nx, ny, [WD, WM, WL]), only=None)
    c.ell(16, 20, 7.5, 3.6, WIN); c.ell(16, 20.4, 6, 2.6, WG if False else WIN); c.hline(11, 14, 20, WG); c.hline(18, 20, 21, WG)
    for y in range(17, 26):
        for x in range(5, 28):
            if c.get(x, y) == WM and (x + y) % 4 == 0: c.px(x, y, WD)
    c.rect(7, 10, 2, 13, WO); c.rect(23, 10, 2, 13, WO)
    for j in range(5):
        c.hline(5 + j, 26 - j, 10 - j, RM if j % 2 else RD) if False else None
    roof(c, 6, 25, 11, 6, True, True, 2)
    c.vline(16, 12, 17, A1); c.rect(14, 17, 5, 3, A2); c.hline(14, 18, 17, A1)
    return c
def tavern():
    c = ground(13); shadow(c, 16, 28, 13, 3)
    wall(c, 4, 15, 27, 28, 'plaster'); wall(c, 4, 24, 27, 28, 'stone'); roof(c, 4, 27, 16, 10, True, True, 3)
    door(c, 12, 19, 6, 10); c.rect(13, 21, 4, 7, WG)
    window(c, 6, 19, 3, 3, True); window(c, 21, 19, 3, 3, True)
    c.hline(4, 27, 18, WO)
    c.line(26, 12, 26, 17, WO) if False else None
    c.rect(28, 17, 1, 1, WO); c.hline(27, 31, 16, WO); c.rect(28, 17, 3, 4, A2); c.rect(29, 18, 1, 2, A1); c.hline(28, 30, 17, HI)   # a hanging mug sign
    chimney(c, 6, 5, 7)
    c.px(7, 3, A2); c.px(8, 2, A2)
    return c
def site():
    c = ground(14)
    c.ell(16, 19, 13.5, 9, lambda x, y, nx, ny: A1 if (x * 7 + y * 13) % 6 else A2 if (x + y) % 3 else WD, only={G, GL, GS})
    for (x, y) in ((6, 12), (25, 12), (6, 25), (25, 25)):
        c.rect(x, y - 5, 2, 8, WO); c.px(x, y - 5, WL); c.vline(x, y - 4, y + 2, WM)
    c.hline(6, 26, 8, WM); c.hline(6, 26, 9, WD); c.hline(6, 26, 21, WM); c.hline(6, 26, 22, WD)
    c.line(6, 22, 14, 9, WO); c.line(25, 22, 17, 9, WO)
    for (x, y, w) in ((9, 26, 8), (19, 27, 7)):
        c.rect(x, y, w, 2, WL); c.hline(x, x + w - 1, y, HI); c.hline(x, x + w - 1, y + 2, WD)
    for (x, y) in ((11, 15), (17, 17)):
        c.rect(x, y, 5, 3, WL); c.hline(x, x + 4, y, HI); c.vline(x + 4, y, y + 2, WD)
    c.vline(16, 3, 8, WO); c.rect(17, 3, 4, 2, A1)
    return c
def castle(stage):
    c = ground(20 + stage)
    if stage == 0:   # foundation: a stone footing, rubble, a timber crane
        for (x, y) in ((4, 25), (8, 24), (24, 25), (21, 24), (14, 27)): c.rect(x, y, 5, 4, WM); c.hline(x, x + 4, y, WL); c.vline(x + 4, y, y + 3, WD)
        c.rect(3, 15, 26, 11, WD); c.rect(4, 16, 24, 9, WM)
        for y in range(16, 25, 3): c.hline(4, 27, y, WD)
        for k, x in enumerate(range(4, 28, 6)): c.vline(x + (2 if k % 2 else 0), 16, 24, WD)
        c.rect(3, 14, 26, 2, WL); c.hline(3, 28, 14, HI)
        c.rect(14, 4, 2, 11, WO); c.rect(10, 4, 10, 2, WO); c.vline(18, 6, 10, A1); c.rect(17, 10, 3, 2, WL)
        return c
    shadow(c, 16, 30, 14, 2)
    ttop = 14 if stage >= 3 else 8      # where the towers' battlements start
    wall(c, 3, 16, 28, 29, 'stone')
    c.rect(3, 14, 26, 2, WL); [c.rect(x, 12, 3, 3, WL) for x in range(5, 25, 5)]; [c.vline(x + 2, 12, 14, WD) for x in range(5, 25, 5)]
    c.rect(12, 21, 8, 9, O); c.rect(13, 22, 6, 8, WO); c.hline(12, 19, 21, WL); [c.vline(x, 22, 29, WD) for x in (14, 16, 18)]
    for x in (6, 24): c.rect(x, 19, 2, 4, O)
    if stage >= 2:
        for x0 in (0, 24):
            c.rect(x0, ttop, 8, 30 - ttop, WM); c.vline(x0, ttop, 29, WL); c.vline(x0 + 7, ttop, 29, WD)
            for y in range(ttop + 2, 29, 4): c.hline(x0, x0 + 7, y, WD)
            c.rect(x0 + 3, ttop + 6, 2, 4, O); c.px(x0 + 3, ttop + 6, WG)
            c.rect(x0 - 1, ttop - 2, 10, 3, WL); [c.rect(x0 - 1 + k * 4, ttop - 4, 2, 2, WL) for k in range(3)]
    if stage >= 3:
        for x0 in (0, 24):
            for j in range(9):
                half = max(1, 5 - j // 2)
                for k in range(-half, half):
                    c.px(x0 + 4 + k, ttop - 3 - j, RL if k < -half // 2 else RM if k < half // 2 else RD)
            c.px(x0 + 3, ttop - 12, WO); c.vline(x0 + 3, ttop - 13, ttop - 11, WO)
        c.rect(4, 0, 3, 2, RM) if False else None
    if stage >= 4:   # crowned: a gold crown above the gate and a banner on each tower
        c.rect(13, 6, 6, 4, A1); c.px(13, 5, A2); c.px(15, 5, A2); c.px(18, 5, A2); c.hline(13, 18, 9, A2); c.px(16, 7, HI)
        for x0 in (0, 24):
            c.vline(x0 + 4, 0, 3, WO); c.rect(x0 + 5, 0, 3, 2, A1)
    return c
BUILD = {'hut': hut, 'house': house, 'wcut': woodcutter, 'quarry': quarry, 'saw': sawmill, 'mason': mason, 'store': storehouse, 'sheep': sheep_pen,
         'mine': crystal_mine, 'weaver': weaver, 'cutter': cutter, 'well': well, 'tavern': tavern, 'site': site}

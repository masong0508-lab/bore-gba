"""terrain.py - the land of THE LAND BOREFORE TIME as 8 x 8 hardware tiles and 16 x 16 cells (two palette banks: LAND and SEA)."""
import random, math
from pix import Cv, lit, rgb15 as C
# ---- bank 0: LAND (grass, trees, dirt road, flowers) ----
LAND = [C(2,3,8), C(3,9,5), C(5,15,6), C(8,20,7), C(12,24,9), C(17,28,12), C(6,4,3), C(15,9,5),
        C(16,11,6), C(21,16,9), C(26,22,14), C(31,31,29), C(31,27,5), C(30,12,18), C(21,28,6), C(28,6,6)]
LG1, LG2, LG3, LG4, LG5, TR_D, TR_L, DI_D, DI_M, DI_L, WHITE, YEL, PINK, LEAFY, RED = range(1, 16)
# ---- bank 1: SEA (water, shore sand, rock, crystal, planks) ----
SEA = [C(2,3,8), C(3,8,20), C(4,12,26), C(8,18,29), C(17,26,31), C(28,31,31), C(20,16,9), C(27,23,14),
       C(6,6,9), C(11,11,14), C(18,18,21), C(25,25,27), C(3,17,23), C(11,27,31), C(31,31,31), C(19,12,6)]
W1, W2, W3, W4, FOAM, SAND_D, SAND, RK_D, RK_M, RK_L, RK_H, CR_D, CR_L, CR_G, PLANK = range(1, 16)
def rnd(seed): return random.Random(seed)
def grass_tile(i):
    r = rnd(100 + i); c = Cv(8, 8, LG3)
    for y in range(8):
        for x in range(8):
            v = r.random()
            c.p[y][x] = LG2 if v < .13 else LG4 if v > .88 else LG3
    def tuft(x, y):
        c.px(x, y, LG5); c.px(x, y - 1, LG5); c.px(x + 1, y, LG4); c.px(x - 1, y, LG4); c.px(x, y + 1, LG2)
    def flower(x, y, col):
        c.px(x, y, col); c.px(x, y + 1, LG2); c.px(x - 1, y + 1, LG4) if r.random() < .5 else None
    if i == 1: tuft(2, 4); tuft(5, 2)
    if i == 2: flower(3, 3, WHITE)
    if i == 3: flower(5, 4, YEL); tuft(2, 6)
    if i == 4: flower(2, 5, PINK)
    if i == 5: tuft(4, 4)
    if i == 6:
        for y in range(8):
            for x in range(8):
                if r.random() < .22: c.p[y][x] = LG2
    if i == 7:
        for y in range(8):
            for x in range(8):
                if r.random() < .22: c.p[y][x] = LG4
    return c
def meadow_tile(i):
    c = grass_tile(7 if i % 2 else 0); r = rnd(300 + i)
    for k in range(3 + i % 2):
        x, y = r.randrange(1, 7), r.randrange(1, 6); col = (WHITE, YEL, PINK, WHITE)[(i + k) % 4]
        c.px(x, y, col); c.px(x, y + 1, LG2)
        if r.random() < .6: c.px(x + 1, y + 1, LG5)
    return c
def ground_shadow(c, cx, cy, rx, ry):   # a soft shadow on grass: darker greens
    c.ell(cx, cy, rx, ry, lambda x, y, nx, ny: LG1 if nx * nx + ny * ny > .45 else LG1, only={LG2, LG3, LG4, LG5})
def tree_round(seed):
    r = rnd(seed); c = Cv(16, 16, LG3)
    for y in range(16):
        for x in range(16): c.p[y][x] = LG2 if r.random() < .14 else LG4 if r.random() > .9 else LG3
    c.ell(8, 14, 5.2, 1.6, LG1, only={LG2, LG3, LG4, LG5})                  # shadow
    c.rect(7, 9, 2, 5, TR_D); c.px(7, 9, TR_L); c.vline(7, 10, 13, TR_L)      # trunk
    ramp = [LG1, LG2, LG3, LG4, LG5]
    def blob(cx, cy, rx, ry): c.ell(cx, cy, rx, ry, lambda x, y, nx, ny: lit(nx, ny, ramp))
    blob(8, 5.5, 5.4, 4.8); blob(5.2, 7, 3.6, 3.1); blob(10.8, 7.2, 3.6, 3.1)
    for (x, y) in ((6, 3), (9, 4), (5, 6), (10, 6), (7, 7)): c.px(x, y, LG5 if (x + y) % 2 else LEAFY)
    for (x, y) in ((8, 9), (10, 9), (6, 9), (4, 8), (12, 8)): c.px(x, y, LG1)  # under-leaf dark
    return c
def tree_pine(seed):
    r = rnd(seed); c = Cv(16, 16, LG3)
    for y in range(16):
        for x in range(16): c.p[y][x] = LG2 if r.random() < .14 else LG4 if r.random() > .9 else LG3
    c.ell(8, 14, 4.6, 1.5, LG1, only={LG2, LG3, LG4, LG5})
    c.rect(7, 12, 2, 3, TR_D); c.px(7, 12, TR_L)
    for k, (y0, w) in enumerate(((10, 6), (7, 5), (4, 4), (2, 2))):          # stacked boughs, a lit left edge
        for j in range(w):
            y = y0 - 0 + j * 0
        h = 3
        for j in range(h + 1):
            half = (w // 2 + 1) - (h - j) // 2 if False else int(w / 2 * (j + 1) / (h + 1)) + 1
            for x in range(8 - half, 8 + half + 1):
                col = LG1 if x > 8 + half - 2 else LG2 if x > 8 else LG4 if x < 8 - half + 2 else LG3
                c.px(x, y0 - h + j + 1 + 1, col)
    c.px(7, 1, LG5); c.px(8, 2, LG4)
    return c
def tree_twin(seed):
    r = rnd(seed); c = Cv(16, 16, LG3)
    for y in range(16):
        for x in range(16): c.p[y][x] = LG2 if r.random() < .14 else LG4 if r.random() > .9 else LG3
    ramp = [LG1, LG2, LG3, LG4, LG5]
    for cx, cy, rr in ((4.5, 8, 3.3), (11, 6, 3.8)):
        c.ell(cx, 14 if cx < 8 else 13, 3.4, 1.2, LG1, only={LG2, LG3, LG4, LG5})
        c.rect(int(cx), int(cy + rr) - 1, 1, 14 - int(cy + rr) + (0 if cx < 8 else 0), TR_D)
        c.ell(cx, cy, rr, rr * .95, lambda x, y, nx, ny: lit(nx, ny, ramp))
        c.px(int(cx) - 1, int(cy) - 2, LG5)
    return c
def bush_trees(seed):   # a copse: a tall tree behind, a bush in front
    c = tree_round(seed); ramp = [LG1, LG2, LG3, LG4, LG5]
    c.ell(4, 13, 3.2, 2.3, lambda x, y, nx, ny: lit(nx, ny, ramp)); c.px(3, 12, LEAFY); c.px(5, 12, LG5)
    c.ell(12.4, 14, 2.4, 1.7, lambda x, y, nx, ny: lit(nx, ny, ramp))
    return c
def rock_cell(i):
    r = rnd(500 + i); c = Cv(16, 16, RK_D)
    for y in range(16):
        for x in range(16):
            v = r.random(); c.p[y][x] = RK_M if v < .40 else RK_L if v > .965 else RK_D
    # broad strata so the mountain reads as stone, not noise
    for k in range(3):
        y0 = r.randrange(2, 14); x0 = r.randrange(0, 8)
        for x in range(x0, x0 + r.randrange(4, 9)): c.px(x, y0, RK_M if r.random() < .8 else RK_D); c.px(x, y0 + 1, RK_D)
    def crag(cx, cy, rx, ry):
        c.ell(cx, cy, rx + .8, ry + .8, RK_D)
        c.ell(cx, cy, rx, ry, lambda x, y, nx, ny: lit(nx, ny, [RK_D, RK_M, RK_L, RK_L, RK_H]))
        c.px(int(cx - rx * .4), int(cy - ry * .5), RK_H); c.px(int(cx - rx * .2), int(cy - ry * .6), RK_H)
    if i == 0: crag(8, 9, 5.5, 4.2); crag(3.5, 4, 2.4, 1.8); crag(13, 12.5, 2.3, 1.8)
    elif i == 1: crag(5, 10, 4, 3); crag(11.5, 5.5, 4.2, 3.4); crag(12, 12.5, 2.8, 2.1)
    else: crag(8, 6, 5, 4.2); crag(3.8, 12.2, 3, 2.3); crag(12.4, 12, 3.2, 2.4)
    return c
def crystal_cell(i):
    c = rock_cell(2 if i else 1); r = rnd(700 + i)
    spikes = ((5, 12, 3, 8), (9, 13, 4, 11), (12, 11, 3, 7)) if i == 0 else ((4, 12, 3, 6), (8, 12, 5, 10), (12, 12, 3, 8))
    for (x, y, w, h) in spikes:
        for j in range(h):
            half = max(1, (w * (h - j)) // (2 * h) + (1 if j > 1 else 0)) if j else 1
            for k in range(-half, half + 1):
                col = CR_L if k < 0 else CR_D if k > 0 else CR_G if j % 4 == 1 else CR_L
                c.px(x + k, y - j, col)
        c.px(x - 1, y - h + 2, CR_G)
    return c
def deep_tile(i):
    r = rnd(900 + i); c = Cv(8, 8, W1)
    for y in range(8):
        for x in range(8):
            if r.random() < .12: c.p[y][x] = W2
    return c
def water_tile(sx, sy, mask, frame):
    """one quadrant (8 x 8) of a water cell. mask bit0: the land across the quadrant's horizontal edge, bit1: across its vertical edge, bit2: the diagonal cell."""
    Lh, Lv, Ld = mask & 1, (mask >> 1) & 1, (mask >> 2) & 1
    c = Cv(8, 8, W2); ox, oy = sx * 8, sy * 8
    for y in range(8):
        for x in range(8):
            gx, gy = ox + x, oy + y
            dx = (15 - gx) if sx else gx; dy = (15 - gy) if sy else gy   # steps from this quadrant's outer edges
            d = 99.0
            if Lh: d = min(d, dx)
            if Lv: d = min(d, dy)
            if Ld and not Lh and not Lv: d = min(d, math.hypot(dx, dy) - .3)
            if Ld and Lh and Lv: d = min(d, dx, dy)
            # the water: bands moving with the frame
            ph = (gx + gy * 2 + frame * 3) % 12
            col = W2
            if ph in (0, 1): col = W3
            if (gx * 5 + gy * 3 + frame * 2) % 17 == 0: col = W4
            if d < 99:
                if d < 1.0: col = SAND_D if (x + y) % 3 == 0 else SAND
                elif d < 2.0: col = SAND
                elif d < 3.0: col = FOAM if (gx + gy + frame) % 3 else SAND
                elif d < 4.0: col = W4 if (gx + frame) % 2 else W3
            c.p[y][x] = col
    return c
def road_tile(sx, sy, hv, var):
    """a quadrant of a dirt road cell. hv bit0: the road goes on across the horizontal edge, bit1: across the vertical edge. Grass around, a rounded dirt core."""
    c = grass_tile(var * 2 + 1 if var else 0); r = rnd(1100 + sx * 7 + sy * 13 + hv * 31 + var * 101)
    Rh, Rv = hv & 1, (hv >> 1) & 1
    for y in range(8):
        for x in range(8):
            gx, gy = sx * 8 + x, sy * 8 + y
            u = (gx - 8) if sx else (7 - gx); v = (gy - 8) if sy else (7 - gy)   # 0.. outward from the centre
            hw = 5   # half width of the path in px
            inside = (u < hw and v < hw) or (Rh and v < hw) or (Rv and u < hw)
            if u < hw and v < hw and not Rh and not Rv and u >= hw - 1 and v >= hw - 1: inside = False   # a round core
            if (u == hw - 1 and v == hw - 1) and ((Rh and not Rv) or (Rv and not Rh)) and False: inside = False
            if inside:
                edge = (not ((u < hw - 1 and v < hw - 1) or (Rh and v < hw - 1) or (Rv and u < hw - 1)))
                col = DI_D if edge else (DI_L if r.random() < .18 else DI_M)
                if not edge and r.random() < .06: col = DI_D
                c.p[y][x] = col
            else:
                near = (u < hw + 1 and v < hw + 1) or (Rh and v < hw + 1) or (Rv and u < hw + 1)
                if near and c.p[y][x] != LG1 and r.random() < .5: c.p[y][x] = LG2
    return c
def bridge_cell(vertical):
    c = Cv(16, 16, W2)
    for y in range(16):
        for x in range(16):
            ph = (x + y * 2) % 12; c.p[y][x] = W3 if ph in (0, 1) else W2
    if not vertical:
        for y in range(3, 13):
            for x in range(16):
                c.p[y][x] = PLANK if x % 4 else RK_D
            pass
        for x in range(16): c.p[3][x] = TR_L if False else SAND; c.p[12][x] = RK_D
        for x in range(0, 16, 4): c.px(x + 1, 3, SAND); c.px(x + 2, 3, SAND)
        c.hline(0, 15, 13, W1); c.hline(0, 15, 14, W1)
    else:
        for x in range(3, 13):
            for y in range(16): c.p[y][x] = PLANK if y % 4 else RK_D
        for y in range(16): c.p[y][3] = SAND; c.p[y][12] = RK_D
        c.vline(13, 0, 15, W1); c.vline(14, 0, 15, W1)
    return c

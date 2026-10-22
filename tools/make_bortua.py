#!/usr/bin/env python3
# make_bortua.py - BORTUA FIGHTER's art, pre-rendered in 3D (the Donkey Kong Country trick): fighters as shaded models (capsules and ellipsoids,
# ray cast exactly), lit by a key light and a rim light, dithered between three shades a material, outlined, and cut into 64 x 96 sprites;
# and the arena (a ring at sunset over the sea) as a full-colour 240 x 160 picture. Each pose is rendered on a wide canvas and cut to the 64 columns
# it fills (bfaAx says where its root fell), so a lunge, a spin kick and a body lying on the floor all fit one 64 x 128 frame.
#   FIGHTERS  three bodies (STANDARD, HEAVY, SLIM with a ponytail) x three seats (FLAT, ROUND, BIG: the BUTT switch and slider) x every frame
#             of every move (ANIMS below). Colour indices (the game colours each
#             fighter with its own palette): 0 clear, 1 outline, 2-4 skin, 5-7 top, 8-10 bottom (dark, mid, light), 11-12 hair, 13 shoes,
#             14 belt / gloves, 15 the shine.
#   OUTPUT    source/art/bortua_fighters.bin (3 bodies x frames x 4096 bytes: two 64 x 64 sprites, 4bpp tiles in 1D order),
#             source/art/bortua_arena.bin (240 x 160 RGB555), source/art/bortua_fx.bin (the hit spark's 4 frames, two shadows),
#             source/bortuaart.h (frame numbers, each frame's reach and root column, the ring's edges), and to look at:
#             tools/bortua_preview.png (every frame of every body with a ROUND seat; the yellow mark is the root) and tools/bortua_arena_preview.png.
#   LOOKS     [BF_BUTT=ROUND] python3 tools/make_bortua.py sheet BODY ANIM ...  /  python3 tools/make_bortua.py arena  (into $BF_OUT, default /tmp/: bf_sheet.png, bf_arena.png)
# Needs numpy and Pillow.
import numpy as np, os, sys, math, struct
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
CW, CH = 64, 128           # one frame (two 64 x 64 sprites)
RW, RX = 160, 80           # the wide canvas a pose is rendered on, and the root's column in it (each frame is then cut to the 64 columns it fills)
AY = 124                   # the floor's row
SC = 50.0                  # pixels per model unit (a fighter is about 1.8 units tall)
SS = 3                     # samples per pixel along each axis
YAW, PITCH = math.radians(34), math.radians(9)
C = np.array([math.sin(YAW)*math.cos(PITCH), math.sin(PITCH), math.cos(YAW)*math.cos(PITCH)])   # towards the camera
RT = np.array([math.cos(YAW), 0.0, -math.sin(YAW)])                                           # screen right
UP = np.cross(C, RT)                                                                         # screen up
DIR = -C
KEY = np.array([0.42, 0.66, 0.62]); KEY /= np.linalg.norm(KEY)      # from in front of the fighter, above, on the camera's side
RIM = np.array([-0.72, 0.30, -0.62]); RIM /= np.linalg.norm(RIM)    # from behind: a bright edge down the back
HALF = KEY + C; HALF /= np.linalg.norm(HALF)
SKIN, TOP, BOT, HAIR, SHOE, ACC, SEAT, PANTS, TANK, HOSE = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9   # SEAT: the hips and cheeks (bottom colours, or bare / underwear in the MOON);
                                                                                          # PANTS: pulled-down pants; TANK / HOSE: the helium tank (bright) and its hose (dark)

# ---- ray casts (all rays parallel: DIR) ----
def hit_ellipsoid(O, c, r, M=None):
    # M: 3x3 world -> local rotation (rows are the local axes); r: radii
    M = np.eye(3) if M is None else M
    o = ((O - c) @ M.T) / r; d = (M @ DIR) / r
    a = d @ d; b = 2 * (o @ d); cc = (o * o).sum(1) - 1
    h = b * b - 4 * a * cc; t = np.full(len(O), np.inf); m = h >= 0
    t[m] = (-b[m] - np.sqrt(h[m])) / (2 * a)
    tf = np.where(m, t, 0.0)
    p = o + np.outer(tf, d); n = (p / r) @ M; n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
    return t, n, ((O + np.outer(tf, DIR)) - c) @ M.T

def hit_capsule(O, A, B, r):
    ba = B - A; oa = O - A; baba = ba @ ba; bard = ba @ DIR; baoa = oa @ ba; rdoa = oa @ DIR; oaoa = (oa * oa).sum(1)
    a = baba - bard * bard; b = baba * rdoa - baoa * bard; c = baba * oaoa - baoa * baoa - r * r * baba
    h = b * b - a * c; t = np.full(len(O), np.inf); m = h >= 0
    if abs(a) > 1e-9:
        tt = (-b[m] - np.sqrt(h[m])) / a; y = baoa[m] + tt * bard; body = (y > 0) & (y < baba)
        idx = np.where(m)[0]; t[idx[body]] = tt[body]
        rest = idx[~body]; yr = y[~body]
        oc = np.where((yr <= 0)[:, None], oa[rest], (O[rest] - B))
        bb = oc @ DIR; c2 = (oc * oc).sum(1) - r * r; h2 = bb * bb - c2; ok = h2 >= 0
        t[rest[ok]] = -bb[ok] - np.sqrt(h2[ok])
    P = O + np.outer(np.where(np.isfinite(t), t, 0), DIR)
    s = np.clip(((P - A) @ ba) / baba, 0, 1); n = P - (A + np.outer(s, ba)); n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
    return t, n

def roty(deg):
    a = math.radians(deg); c, s = math.cos(a), math.sin(a); return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])
def rotz(deg):
    a = math.radians(deg); c, s = math.cos(a), math.sin(a); return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])
def vdown(deg):   # a limb hanging at deg from straight down, swung forward (+x)
    a = math.radians(deg); return np.array([math.sin(a), -math.cos(a), 0.0])

BODIES = {   # proportions: scale of the whole, of widths, of limb thickness; slim has a ponytail
    'STANDARD': dict(h=1.0, w=1.0, t=1.0, tail=False, hair='short'),
    'HEAVY':    dict(h=1.0, w=1.24, t=1.28, tail=False, hair='crop', belly=1.3),
    'SLIM':     dict(h=0.96, w=0.86, t=0.84, tail=True, hair='long'),
}

BUTTS = ['FLAT', 'ROUND', 'BIG']   # the seat (the game's BUTT switch and sliders): FLAT when it is switched off (or a child), else ROUND or BIG
BUTT = {'FLAT': None, 'ROUND': (0.105, 0.125, 0.115), 'BIG': (0.12, 0.16, 0.135)}   # a cheek: its centre behind the hips, its depth radius, its height radius

def model(P, body, butt='FLAT'):
    """The posed model: a list of (kind, args, material, tag)."""
    b = BODIES[body]; H, W, T = b['h'], b['w'], b['t']
    g = lambda k, d=0.0: P.get(k, d)
    yawM = roty(g('yaw')); fall = g('fall')
    hip = np.array([0.0, (0.92 - g('drop')) * H, 0.0])
    tor = math.radians(g('tor')); up = np.array([math.sin(tor), math.cos(tor), 0.0])
    tw = roty(g('twist'))
    hu = 0.22 * g('puff')   # the HELIUM: the whole upper body rides up on the balloon (the neck stays its own length)
    neck = hip + up * (0.50 + hu) * H
    def side(z): return tw @ np.array([0, 0, z])
    shL = neck - up * 0.06 * H + side(0.215 * W); shR = neck - up * 0.06 * H + side(-0.215 * W)
    items = []
    def cap(a, bb, r, m, tag=''): items.append(('cap', (a, bb, r), m, tag))
    def ell(c, r, m, M=None, tag=''): items.append(('ell', (c, np.asarray(r, float), M), m, tag))
    # torso: chest, belly, pelvis (bottom colour), a belt
    rotT = rotz(-math.degrees(tor)).T @ tw.T
    pfc = 1 + 0.35 * g('puff')
    if not g('gore'):
        ell(hip + up * (0.33 + 0.9 * hu) * H, (0.155 * W * pfc, 0.19 * H * pfc, 0.215 * W * pfc), TOP, rotT, 'chest')
        ell(hip + up * (0.42 + hu) * H, (0.11 * W, 0.07 * H, 0.235 * W * pfc), TOP, rotT, 'shoulders')
    bl = b.get('belly', 1.0)
    pf = g('puff'); gore = g('gore')   # the HELIUM: the belly balloons, every way (up into the chest as much as out in front), the chest with it; GORE: it has burst
    if not gore:
        ell(hip + up * 0.15 * H + rotT.T @ np.array([0.02 * (bl - 1) + 0.10 * pf * W, 0.05 * pf, 0]), (0.13 * W * bl * (1 + 1.1 * pf), 0.13 * H * bl * (1 + 1.35 * pf), 0.17 * W * (1 + 0.8 * pf)), TOP, rotT, 'belly')
    pants = g('pants', 0)   # the MOON: 0 pants on, 1 pulled down off the seat, 2 round the ankles
    if not gore: ell(hip + up * 0.02 * H, (0.135 * W, 0.11 * H, 0.17 * W), SEAT if pants else BOT, rotT, 'pelvis')
    if (BUTT[butt] or pf > 0) and not gore:   # ... and the seat swells with it (a flat one puffs out too while the gas is in)
        bx, rx, ry = BUTT[butt] or BUTT['ROUND']
        gr = 1 + 0.7 * pf; bx = bx + 0.06 * pf
        jg, jb = g('jig'), g('jigb')   # the JIGGLE: jig sways the cheeks one up and one down, jigb bounces both (squashed going down, stretched going up)
        for zs in (1, -1):
            d = jg * zs + jb; sq = 1.0 + 0.13 * d
            ell(hip + rotT.T @ np.array([-bx * W - 0.012 * abs(d), (-0.035 + 0.024 * d) * H, zs * (0.068 + 0.03 * pf) * W]), (rx * W * gr * (2.0 - sq) ** 0.5 * (1 + 0.05 * abs(jb)), ry * H * gr * sq, 0.098 * W * gr), SEAT, rotT, 'seat')
    if not pants and not gore: ell(hip + up * 0.075 * H, (0.138 * W, 0.024 * H, 0.168 * W), ACC, rotT, 'belt')
    # head and neck (the burst sends the head flying)
    head = neck + up * 0.16 * H + np.array([0.025, 0, 0])
    if gore: head = head + np.array([-0.25, 0.3, 0.1]) if gore == 1 else head + np.array([-0.45, -0.05, 0.15]) if gore == 2 else hip + np.array([-0.3, -0.72, 0.12])
    if not gore: cap(neck - up * 0.02, neck + up * 0.07, 0.055 * T, SKIN)
    hrot = rotz(-g('head')).T @ tw.T
    ck = 1 + 0.22 * g('cheek')   # (puffed cheeks)
    ell(head, (0.108 * ck, 0.13, 0.098 * ck), SKIN, hrot, 'face')
    ell(head + np.array([-0.014, 0.022, 0]), (0.116, 0.138, 0.106), HAIR, hrot, 'hair:' + b['hair'])
    if b['tail']: ell(head + np.array([-0.13, -0.05, 0]), (0.062, 0.13, 0.052), HAIR, rotz(28).T, 'tail')
    # arms: 1 the near (lead) arm, 2 the far one; hands come in towards the middle
    for sh, s_, e_, zf in ((shL, g('s1'), g('e1'), 1), (shR, g('s2'), g('e2'), -1)):
        if gore: sh = sh + np.array([0.24 * zf, 0.25, 0.15 * zf]) * (1 if gore == 1 else 1.1) if gore < 3 else hip + np.array([0.18 * zf, -0.86, 0.22 * zf])
        el = sh + vdown(s_) * 0.29 * H; el[2] = sh[2] * 0.92
        wr = el + vdown(s_ + e_) * 0.26 * H; wr[2] = sh[2] * g('handz', 0.55)
        if not gore: ell(sh, (0.075 * T, 0.075 * T, 0.075 * T), TOP, None, 'delt')
        cap(sh, el, 0.066 * T, TOP); cap(el, wr, 0.056 * T, SKIN)
        ell(wr + vdown(s_ + e_) * 0.045, (0.068 * T, 0.066 * T, 0.066 * T), ACC if body != 'SLIM' and not pants else SKIN, None, 'fist')
    if g('tank'):   # the HELIUM tank in the far hand, its hose up to the mouth (tank 1) or round to the back way (tank 2)
        sh = shR; el = sh + vdown(g('s2')) * 0.29 * H; el[2] = sh[2] * 0.92; wr = el + vdown(g('s2') + g('e2')) * 0.26 * H; wr[2] = sh[2] * g('handz', 0.55)
        cap(wr + np.array([0, -0.11, 0.02]), wr + np.array([0, 0.1, 0.02]), 0.062, TANK); cap(wr + np.array([0, 0.1, 0.02]), wr + np.array([0, 0.15, 0.02]), 0.025, HOSE)
        if g('tank') == 1:
            end = head + np.array([0.105, -0.045, 0.0]); mid = (wr + end) / 2 + np.array([0.06, 0.02, 0.05])
            cap(wr + np.array([0, 0.15, 0.02]), mid, 0.018, HOSE); cap(mid, end, 0.018, HOSE)
        else:   # the back way: the hose loops down and round, a little brass nozzle where it goes in (it pops loose on the way out: tank 3)
            bx = BUTT[butt][0] if BUTT[butt] else 0.08
            end = hip + rotT.T @ np.array([-(bx + 0.05) * W, -0.06 * H, 0.0]); loose = g('tank') == 3
            if loose: end = end + np.array([-0.16, -0.02, 0.1])
            mid = (wr + end) / 2 + np.array([-0.1, -0.16, 0.06])
            cap(wr + np.array([0, 0.15, 0.02]), mid, 0.018, HOSE); cap(mid, end, 0.018, HOSE)
            cap(end + (mid - end) * 0.12, end if not loose else end + np.array([0.03, 0, 0]), 0.03, TANK, 'nozzle')
    # legs
    for z, h_, k_ in ((0.1 * W, g('h1'), g('k1')), (-0.1 * W, g('h2'), g('k2'))):
        hj = hip + np.array([0, -0.05, z])
        if gore: hj = hj + np.array([0.22 * np.sign(z), 0.1, 0.1 * np.sign(z)]) if gore < 3 else hip + np.array([0.12 * np.sign(z), -0.52, 0.3 * np.sign(z)])
        kn = hj + vdown(h_) * 0.43 * H; an = kn + vdown(h_ + k_) * 0.42 * H
        if not pants: cap(hj, kn, 0.096 * T, BOT); cap(kn, an, 0.074 * T, BOT)
        elif pants == 1:   # just off the seat: the waistband bunched at the top of the thighs
            cap(hj, kn, 0.096 * T, PANTS); cap(kn, an, 0.074 * T, PANTS); ell(hj + (kn - hj) * 0.22, (0.13 * T, 0.06, 0.13 * T), PANTS, None, 'band')
        else:              # round the ankles: bare legs, the pants in a heap
            cap(hj, kn, 0.092 * T, SKIN); cap(kn, an, 0.07 * T, SKIN); ell(kn + (an - kn) * 0.8, (0.12 * T, 0.09, 0.12 * T), PANTS, None, 'heap')
        fa = math.radians(h_ + k_); fwd = np.array([math.cos(fa), math.sin(fa), 0])
        ell(an + fwd * 0.07 + np.array([0, -0.03, 0]), (0.12, 0.05, 0.062 * T), SHOE, rotz(-math.degrees(fa)).T if abs(h_ + k_) > 1 else None)
    if gore:   # THE BURST: the torso in pieces (blood and meat: TOP and BOT, which the game colours red for these frames), then a puddle with the bits in it
        rg = np.random.default_rng(5); c0 = hip + up * 0.2 * H
        for i in range(26):
            d = rg.normal(size=3); d[1] = abs(d[1]) * 0.6 + d[1] * 0.4; d /= np.linalg.norm(d) + 1e-9; r = rg.uniform(0.028, 0.075)
            if gore == 1: c = c0 + d * rg.uniform(0.12, 0.42)
            elif gore == 2: c = c0 + d * rg.uniform(0.2, 0.5); c[1] = c[1] - 0.25
            else: c = np.array([c0[0] + d[0] * 0.36, -0.02 + r * 0.6, c0[2] + d[2] * 0.4])   # (on the floor)
            ell(c, (r, r * (0.6 if gore == 3 else 1.0), r), BOT if i % 3 else TOP, None, 'chunk')
        if gore == 1:
            for i in range(14):   # spray: thin streaks flying outward
                d = rg.normal(size=3); d /= np.linalg.norm(d) + 1e-9; a = c0 + d * rg.uniform(0.14, 0.28)
                cap(a, a + d * rg.uniform(0.06, 0.16), 0.014, TOP)
        if gore >= 2:   # the puddle, spreading
            ell(np.array([-0.05, -0.012, 0.0]), (0.3 if gore == 2 else 0.44, 0.016, 0.28 if gore == 2 else 0.42), TOP, None, 'puddle')
    # the whole body falls (backwards, about the feet; lifted so it lies on the floor, not in it) and turns (yaw, about the vertical)
    F = rotz(fall); R = yawM @ F; lift = np.array([0.0, 0.14 * math.sin(math.radians(abs(fall))), 0.0])
    out = []
    for kind, args, m, tag in items:
        def X(p): return R @ p + lift
        if kind == 'cap': a, bb, r = args; out.append((kind, (X(a), X(bb), r), m, tag))
        else:
            c, r, M = args; M = np.eye(3) if M is None else M
            out.append((kind, (X(c), r, M @ R.T), m, tag))
    return out

def render(P, body, butt='FLAT'):
    moon = bool(P.get('pants'))
    """The pose ray cast on a wide canvas (RW x CH, the root at RX, AY), down to one colour index a pixel."""
    n = RW * SS; hgt = CH * SS
    xs = (np.arange(n) + 0.5) / SS; ys = (np.arange(hgt) + 0.5) / SS
    gx, gy = np.meshgrid(xs, ys)
    O = (np.outer((gx - RX).ravel() / SC, RT) + np.outer((AY - gy).ravel() / SC, UP)) + C * 6
    N = len(O); tbest = np.full(N, np.inf); nb = np.zeros((N, 3)); mb = np.full(N, -1); dark = np.zeros(N, bool)
    for kind, args, m, tag in model(P, body, butt):
        if kind == 'cap': t, nn = hit_capsule(O, *args); loc = None
        else: t, nn, loc = hit_ellipsoid(O, args[0], args[1], args[2])
        ok = t < tbest
        if tag.startswith('hair:'):   # the hair shell only counts over the top and the back of the head
            sty = tag[5:]; lx, ly = loc[:, 0] / args[1][0], loc[:, 1] / args[1][1]
            if sty == 'crop': keep = (ly > 0.42) | ((lx < -0.45) & (ly > -0.2))
            elif sty == 'long': keep = (ly > 0.3) | ((lx < -0.05) & (ly > -0.75))
            else: keep = (ly > 0.32) | ((lx < -0.2) & (ly > -0.45)) | ((lx < 0.25) & (ly > 0.1))
            ok &= keep
        tbest = np.where(ok, t, tbest); nb[ok] = nn[ok]; mb[ok] = m
        if tag == 'face':
            lx, ly, lz = loc[:, 0] / args[1][0], loc[:, 1] / args[1][1], loc[:, 2] / args[1][2]
            eye = ok & (lx > 0.45) & (ly > 0.0) & (ly < 0.2) & (lz > 0.12) & (lz < 0.55); dark = np.where(ok, eye, dark)
        else: dark = np.where(ok, False, dark)
    hitm = np.isfinite(tbest)
    lam = np.clip(nb @ KEY, 0, 1); rim = np.clip(nb @ RIM, 0, 1) ** 2
    I = 0.16 + 0.84 * lam + 0.6 * rim
    sp = np.clip(nb @ HALF, 0, 1) ** 24; spec = hitm & (sp > 0.5) & ((mb == HAIR) | (mb == SHOE) | (mb == ACC))
    # down to pixels: a pixel is the material most of its samples hit (clear if most hit nothing)
    def blk(v): return v.reshape(CH, SS, RW, SS).transpose(0, 2, 1, 3).reshape(CH, RW, SS * SS)
    mat = blk(mb); Iv = blk(I); dep = blk(np.where(hitm, tbest, 99.0)); spv = blk(spec); dk = blk(dark)
    cnt = np.stack([(mat == k).sum(2) for k in range(-1, 10)], 2)
    solid = cnt[..., 0] * 2 <= SS * SS
    k = np.argmax(cnt[..., 1:], 2); sel = mat == k[..., None]; ns = np.maximum(sel.sum(2), 1)
    bay = np.array([[0.0, 0.5], [0.75, 0.25]]) - 0.375
    yy, xx = np.mgrid[0:CH, 0:RW]
    ii = (Iv * sel).sum(2) / ns + bay[yy & 1, xx & 1] * 0.22
    depth = np.where(solid, np.where(sel, dep, 99.0).min(2), 99.0)
    lv = np.where(ii < 0.52, 0, np.where(ii < 0.86, 1, 2))
    base = np.choose(k, [2, 5, 8, 11, 1 if moon else 13, 14, 8, 13, 15, 1])   # (MOON frames: the pants take 13 / 14, so the shoes go black; the tank is bright, its hose dark)
    out = np.where(np.isin(k, [SKIN, TOP, BOT, SEAT]), base + lv, np.where(k == HAIR, 11 + (lv > 0), np.where(k == PANTS, 13 + (lv > 0), base)))
    out = np.where((spv * sel).sum(2) / ns > 0.34, 15, out)
    out = np.where((dk * sel).sum(2) / ns > 0.4, 1, out)
    out = np.where(solid, out, 0).astype(np.uint8)
    # the outline: round the silhouette, and where one part stands well in front of another
    edge = np.zeros_like(solid)
    for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
        nso = np.zeros_like(solid); nde = np.full(depth.shape, 99.0)
        ys0, ys1 = max(0, -dy), CH - max(0, dy); xs0, xs1 = max(0, -dx), RW - max(0, dx)
        nso[ys0:ys1, xs0:xs1] = solid[ys0 + dy:ys1 + dy, xs0 + dx:xs1 + dx]; nde[ys0:ys1, xs0:xs1] = depth[ys0 + dy:ys1 + dy, xs0 + dx:xs1 + dx]
        edge |= solid & (~nso | (nde < depth - 0.16))
    out[edge] = 1
    return out

def crop(img, nm):
    """The 64-wide window over the wide render that holds the fighter (centred on it), and where the root falls in it."""
    cols = np.where(img.any(0))[0]
    if not len(cols): return img[:, RX - 20:RX + 44], 20
    l, r = cols.min(), cols.max() + 1
    if r - l > CW: sys.stderr.write('\n!! %s is %d px wide (clipped)\n' % (nm, r - l))
    x0 = int(np.clip((l + r) // 2 - CW // 2, 0, RW - CW))
    if img[0].any(): sys.stderr.write('\n!! %s touches the top\n' % nm)
    return img[:, x0:x0 + CW], RX - x0

# ---- the moves: keyframes (degrees; drop in units) ----
IDLE = dict(tor=8, s1=55, e1=95, s2=40, e2=112, h1=24, k1=-34, h2=-22, k2=-16, drop=0.07)
GUARD = dict(IDLE, tor=0, s1=78, e1=118, s2=68, e2=122, handz=0.2)
CROUCH = dict(tor=22, s1=48, e1=100, s2=36, e2=112, h1=72, k1=-128, h2=22, k2=-118, drop=0.32)
def P(base, **kw): d = dict(base); d.update(kw); return d
def walk(i):
    ph = i / 6 * 2 * math.pi
    return P(IDLE, h1=15 + 15 * math.sin(ph), k1=-18 - 16 * max(0, math.cos(ph)), h2=-16 - 15 * math.sin(ph), k2=-10 - 16 * max(0, -math.cos(ph)), drop=0.04 + 0.012 * abs(math.sin(ph)))
BACK = 124   # the turn that puts a fighter's back square to the camera (the MOON)
HYAW = 50    # side on, a little away (the HELIUM: the belly and the seat both in the outline)
MOONP = dict(tor=6, s1=-12, e1=30, s2=-14, e2=28, h1=8, k1=-6, h2=-8, k2=-4, drop=0.03, yaw=BACK, handz=0.9)
HELP = dict(pants=2, yaw=HYAW, tor=2, s1=75, e1=125, s2=40, e2=60, h1=6, k1=-4, h2=-6, k2=-4, drop=0.02, handz=0.0)
HBYAW = 84   # three quarters from behind (the back way: the seat, the hose and the belly all in the outline)
HELB = dict(pants=2, yaw=HBYAW, tor=22, s1=60, e1=100, s2=-55, e2=-40, h1=10, k1=-8, h2=-8, k2=-6, drop=0.04, handz=0.3, head=-18)   # (bent, the far arm back, looking over the shoulder)
HITP = P(IDLE, tor=-22, s1=22, e1=40, s2=-12, e2=55, head=-18, h1=6, k1=-14, h2=-26, k2=-6, drop=0.03)
ANIMS = [
    ('IDLE', [IDLE, P(IDLE, drop=0.055, tor=7, s1=52, s2=37), P(IDLE, drop=0.065, tor=8, s1=53, s2=38), P(IDLE, drop=0.05, tor=7, s1=51, s2=36)]),
    ('WALK', [walk(i) for i in range(6)]),
    ('GUARD', [GUARD]),
    ('CROUCH', [CROUCH]),
    ('CGUARD', [P(CROUCH, s1=78, e1=118, s2=68, e2=122, handz=0.2)]),
    ('PUNCH', [P(IDLE, tor=3, s1=40, e1=112), P(IDLE, tor=12, s1=74, e1=42, handz=0.35), P(IDLE, tor=18, s1=90, e1=0, s2=30, e2=122, h1=26, k1=-12, h2=-28, k2=-4, handz=0.3), P(IDLE, s1=62, e1=78)]),
    ('KICK', [P(IDLE, tor=-4, h1=72, k1=-112), P(IDLE, tor=-10, h1=86, k1=-52, s1=70, e1=110), P(IDLE, tor=-18, h1=96, k1=0, s1=72, e1=115, s2=55, e2=120),
              P(IDLE, tor=-20, h1=100, k1=2, s1=72, e1=115, s2=55, e2=120), P(IDLE, tor=-6, h1=62, k1=-92)]),
    ('LPUNCH', [P(CROUCH, s1=50, e1=84), P(CROUCH, s1=82, e1=8, tor=28, handz=0.3), P(CROUCH, s1=60, e1=70)]),
    ('SWEEP', [P(CROUCH, h1=52, k1=-102), P(CROUCH, h1=78, k1=-36, tor=28), P(CROUCH, h1=88, k1=-2, tor=32, drop=0.36), P(CROUCH, h1=60, k1=-90)]),
    ('PALM', [P(IDLE, tor=0, s1=18, e1=122, drop=0.08), P(IDLE, tor=20, s1=70, e1=30, h1=40, k1=-30, h2=-36, k2=0, drop=0.1),
              P(IDLE, tor=30, s1=88, e1=0, s2=-20, e2=40, h1=40, k1=-38, h2=-40, k2=0, drop=0.12, handz=0.15), P(IDLE, tor=14, s1=60, e1=60, drop=0.08)]),
    ('SPIN', [P(IDLE, yaw=0), P(IDLE, yaw=-70, h1=40, k1=-60), P(IDLE, yaw=-140, h1=70, k1=-110), P(IDLE, yaw=-210, h1=80, k1=-90, tor=-8),
              P(IDLE, yaw=-290, h1=94, k1=0, tor=-16, s1=70, e1=110), P(IDLE, yaw=-350, h1=88, k1=-6, tor=-14, s1=70, e1=110)]),
    ('THROW', [P(IDLE, tor=15, s1=80, e1=20, s2=76, e2=30, handz=0.15), P(IDLE, tor=22, s1=84, e1=40, s2=80, e2=42, drop=0.09, handz=0.1),
               P(IDLE, tor=-14, s1=140, e1=20, s2=135, e2=25, drop=0.02), P(IDLE, tor=-26, s1=160, e1=10, s2=150, e2=12)]),
    ('HIT', [HITP, P(IDLE, tor=26, s1=30, e1=92, s2=20, e2=96, head=12, drop=0.08)]),
    ('KO', [P(HITP, fall=12, yaw=-8), P(HITP, fall=34, yaw=-12), P(HITP, fall=58, yaw=-18, s1=110, e1=20, s2=120, e2=10),
            P(HITP, fall=80, yaw=-22, s1=150, e1=10, s2=160, e2=0, head=10), P(HITP, fall=90, yaw=-20, s1=20, e1=10, s2=-15, e2=10, h1=8, k1=-6, h2=-6, k2=-4, head=10, tor=-4)]),
    ('JUMP', [P(IDLE, h1=72, k1=-122, h2=42, k2=-112, s1=62, e1=92), P(IDLE, h1=32, k1=-52, h2=-10, k2=-42, s1=70, e1=80)]),
    ('JKICK', [P(IDLE, h1=96, k1=0, h2=32, k2=-102, tor=-10, s1=70, e1=110)]),
    ('MOON', [P(IDLE, yaw=62, s1=30, e1=40, s2=20, e2=40), P(MOONP),
              P(MOONP, pants=1, tor=24, s1=0, e1=6, s2=-2, e2=6, drop=0.05),
              P(MOONP, pants=2, tor=64, s1=-10, e1=4, s2=-14, e2=4, drop=0.07, head=-25),
              P(MOONP, pants=2, tor=60, yaw=BACK - 12, s1=-10, e1=4, s2=-14, e2=4, drop=0.06, head=-25, jig=1),    # the wiggle (the cheeks sway)
              P(MOONP, pants=2, tor=60, yaw=BACK + 12, s1=-10, e1=4, s2=-14, e2=4, drop=0.06, head=-25, jig=-1),
              P(MOONP, pants=2, tor=60, yaw=BACK + 12, s1=-70, e1=-10, s2=-14, e2=4, drop=0.06, head=-25, jigb=-1),  # the slap (they squash)
              P(MOONP, pants=2, tor=64, s1=-10, e1=4, s2=-14, e2=4, drop=0.05, head=-25, jigb=1),                   # the bounce after it
              P(MOONP, pants=2, tor=64, s1=-10, e1=4, s2=-14, e2=4, drop=0.07, head=-25, jigb=-0.6),
              P(MOONP, pants=2, tor=42, s1=40, e1=10, s2=30, e2=10, drop=0.04, head=-40, jigb=0.5),                 # the TOOT: frozen, startled
              P(MOONP, pants=2, tor=55, yaw=BACK - 16, s1=30, e1=20, s2=20, e2=20, drop=0.06, head=-30, jig=1.8, jigb=1.2),    # and it all shakes
              P(MOONP, pants=2, tor=55, yaw=BACK + 16, s1=30, e1=20, s2=20, e2=20, drop=0.06, head=-30, jig=-1.8, jigb=-1.2),
              # the HELIUM (12-17): side on, a tank out, the hose to the mouth; the belly balloons; it all comes out shaking; red faced
              P(HELP, tank=1), P(HELP, tank=1, puff=0.4, cheek=0.5), P(HELP, tank=1, puff=0.85, cheek=1, drop=0.0, tor=-6),
              P(HELP, puff=0.6, cheek=0.6, s1=40, e1=30, s2=25, e2=20, yaw=HYAW - 10, jig=1.8, jigb=1.2, tor=-4),
              P(HELP, puff=0.3, cheek=0.3, s1=40, e1=30, s2=25, e2=20, yaw=HYAW + 10, jig=-1.8, jigb=-1.2, tor=0),
              P(HELP, s1=165, e1=70, s2=10, e2=20, head=22, tor=10, drop=0.05),
              # the HELIUM, the back way (18-26): seen from behind, three quarters. Bent over, the tank out behind, a look back over the shoulder;
              # it plugs in (a jolt: up on the toes, cheeks clenched); the belly fills and fills; the hose pops loose; it all comes out, shaking; red faced
              P(HELB, tank=2, twist=-30, head=-12),
              P(HELB, tank=2, twist=-38, head=22, drop=-0.03, jigb=-0.8, tor=14, s1=50, e1=90),
              P(HELB, tank=2, twist=-30, head=4, puff=0.35, cheek=0.25, tor=16),
              P(HELB, tank=2, twist=-22, head=-6, puff=0.7, cheek=0.5, tor=8, drop=0.0, s1=30, e1=40),
              P(HELB, tank=2, twist=-18, head=-14, puff=1.0, cheek=0.75, tor=-6, drop=-0.02, s1=10, e1=20, s2=-70, e2=-30, jigb=0.4),
              P(HELB, tank=3, twist=-30, head=10, puff=0.95, cheek=0.7, tor=4, drop=0.0, s1=20, e1=30, s2=-40, e2=-20, jigb=-0.9),
              P(HELB, twist=-34, head=18, puff=0.55, cheek=0.4, s2=-20, e2=30, yaw=HBYAW - 12, jig=2.2, jigb=1.5, tor=14, drop=0.03),
              P(HELB, twist=-26, head=14, puff=0.2, cheek=0.15, s2=-20, e2=30, yaw=HBYAW + 12, jig=-2.2, jigb=-1.5, tor=18, drop=0.05),
              P(HELP, s1=165, e1=70, s2=-40, e2=-10, head=22, tor=10, drop=0.05, yaw=HBYAW - 20),   # (the far hand over the seat)
              # the BURST (27-31): too much. The mouth way (27, 28) and the back way (29, 30) blow up past full, feet off the floor; then flat on the ring (31)
              P(HELP, tank=1, puff=1.5, cheek=1, drop=-0.04, tor=-8, s1=-50, e1=10, s2=60, e2=50, head=-10, h1=20, k1=-10, h2=-20, k2=-6),
              P(HELP, tank=1, puff=1.9, cheek=1, drop=-0.14, tor=-10, s1=-60, e1=0, s2=70, e2=40, h1=40, k1=-20, h2=-40, k2=-10, head=-14),
              P(HELB, tank=2, twist=-14, head=-16, puff=1.5, cheek=0.9, tor=-6, drop=-0.04, s1=-30, e1=20, s2=-80, e2=-30, jigb=0.6, h1=20, k1=-10, h2=-20, k2=-6),
              P(HELB, tank=2, twist=-10, head=-18, puff=2.0, cheek=1, tor=-10, drop=-0.14, s1=-70, e1=0, s2=-90, e2=-30, h1=40, k1=-20, h2=-40, k2=-10, jigb=0.9),
              # the BURST itself (31-33): the torso blown to bits, everything flying; the bits coming down into a puddle; the puddle with the bits in it
              P(dict(pants=2, gore=1, yaw=HYAW, s1=-120, e1=-20, s2=140, e2=20, h1=50, k1=-130, h2=-50, k2=130, drop=0.0, handz=0.0, head=40)),
              P(dict(pants=2, gore=2, yaw=HYAW, s1=-100, e1=60, s2=110, e2=-60, h1=35, k1=-110, h2=-35, k2=110, drop=0.0, handz=0.0, head=70)),
              P(dict(pants=2, gore=3, yaw=HYAW, s1=-60, e1=100, s2=60, e2=-100, h1=60, k1=-110, h2=-60, k2=110, drop=0.0, handz=0.0, head=90))]),
    ('JIGGLE', [P(IDLE, jigb=1, jig=0.3), P(IDLE, jigb=-1, jig=-0.3, drop=0.075), P(IDLE, jigb=0.5, jig=0.4)]),   # (sometimes, after a landing or a hit)
    ('WIN', [P(IDLE, s1=170, e1=10, s2=30, e2=100, tor=0, drop=0.02), P(IDLE, s1=176, e1=0, s2=28, e2=98, tor=-4, drop=0.0), P(IDLE, s1=166, e1=6, s2=160, e2=8, tor=-6, drop=0.0)]),
]

def pack(img):   # 64 x 128 -> two 64 x 64 sprites, 4bpp tiles, 1D order
    out = bytearray()
    for y0 in (0, 64):
        for ty in range(8):
            for tx in range(8):
                for r in range(8):
                    row = img[y0 + ty * 8 + r, tx * 8:tx * 8 + 8]
                    for q in range(0, 8, 2): out.append(int(row[q]) | (int(row[q + 1]) << 4))
    return bytes(out)

def reach(wide):   # how far in front of the root the fighter reaches (px), at body height (not the feet: a step forward is not a hit)
    cols = np.where((wide[AY - 105:AY - 14] > 0).any(0))[0]; return int(cols.max() - RX) if len(cols) else 0

def demo_pal(skin, top, bot, hair):
    def sh(c, k): return tuple(min(255, int(v * k)) for v in c)
    p = [(0, 0, 0)] * 16; p[1] = (16, 12, 20)
    for i, c in ((2, skin), (5, top), (8, bot)): p[i], p[i + 1], p[i + 2] = sh(c, 0.55), c, sh(c, 1.22)
    p[11], p[12], p[13], p[14], p[15] = sh(hair, 0.6), hair, (36, 30, 34), (220, 40, 40), (255, 250, 230)
    return p

# ---- the arena ----
def arena():
    W, Hh = 240, 160; HZ = 80; F = 276.0; EYE = 1.56; FLOOR = 0.3; X0, X1, Z0, Z1 = -2.05, 2.05, 3.2, 15.0
    y, x = np.mgrid[0:Hh, 0:W].astype(float)
    img = np.zeros((Hh, W, 3))
    rng = np.random.default_rng(7)
    def fbm(u, v, oct=5):
        tot = 0; amp = 1; f = 1
        for o in range(oct):
            ph = rng.random(4) * 10
            tot += amp * (np.sin(u * f * 0.9 + ph[0] + np.sin(v * f * 1.3 + ph[1])) * np.cos(v * f * 1.1 + ph[2] + np.sin(u * f * 0.7 + ph[3])))
            amp *= 0.5; f *= 2.1
        return tot
    # sky: deep blue at the top, orange at the horizon, a low sun, clouds lit from below
    t = np.clip(y / HZ, 0, 1)
    top = np.array([18, 16, 58]); mid = np.array([150, 60, 110]); hor = np.array([255, 150, 70])
    sky = np.where((t < 0.6)[..., None], top + (mid - top) * (t / 0.6)[..., None], mid + (hor - mid) * ((t - 0.6) / 0.4)[..., None])
    SX, SY = 172, 70; dsun = np.hypot(x - SX, (y - SY) * 1.15)
    sky += (np.clip(1 - dsun / 70, 0, 1) ** 2)[..., None] * np.array([120, 70, 20])
    sky = np.where((dsun < 13)[..., None], np.array([255, 236, 170]), sky)
    cl = fbm(x / 26.0, y / 7.0); band = np.clip(1 - np.abs(y - 48) / 26, 0, 1)
    cloud = np.clip(cl * 0.9 + 0.1, 0, 1) * band
    ccol = np.array([90, 40, 90]) + (np.array([255, 140, 120]) - np.array([90, 40, 90])) * np.clip((y - 30) / 40, 0, 1)[..., None]
    sky = sky * (1 - cloud[..., None] * 0.75) + ccol * (cloud[..., None] * 0.75)
    img[:] = sky
    # far islands
    mh = HZ - 6 - (np.sin(x / 17.0) * 4 + np.sin(x / 7.3 + 1) * 2 + np.sin(x / 41.0 + 2) * 5).clip(-3, 10)
    isl = (y >= mh) & (y < HZ); img[isl] = (np.array([70, 30, 70]) * 0.7 + img[isl] * 0.3)
    # the sea
    sea = y >= HZ; d = (y - HZ + 0.5); wz = F * EYE / np.maximum(d, 0.5); wx = (x - 120) * wz / F
    wave = np.sin(wx * 3 + wz * 0.7) * 0.5 + np.sin(wx * 7.1 - wz * 1.9) * 0.3 + np.sin(wz * 4.3) * 0.2
    base = np.array([20, 30, 70]) + (np.array([70, 50, 90]) - np.array([20, 30, 70])) * np.clip(1 - d / 80, 0, 1)[..., None]
    glint = np.clip(1 - np.abs(x - SX) / (8 + d * 0.9), 0, 1) * np.clip(wave + 0.2, 0, 1)
    seac = base + glint[..., None] * np.array([255, 190, 110]) * 0.9 + (wave * 10)[..., None]
    img[sea] = seac[sea]
    # the ring: a stone deck with gold trim, its front wall
    zt = F * (EYE - FLOOR) / np.maximum(y - HZ, 0.01); xt = (x - 120) * zt / F
    deck = (y > HZ) & (xt >= X0) & (xt <= X1) & (zt >= Z0) & (zt <= Z1)
    tile = ((np.floor(xt / 0.62) + np.floor(zt / 0.62)) % 2)
    lite = np.clip(1.15 - np.hypot(xt * 0.3, (zt - 6) * 0.25), 0.55, 1.15)
    dcol = (np.array([150, 150, 170]) * (1 - tile)[..., None] + np.array([105, 108, 135]) * tile[..., None]) * lite[..., None]
    dcol = dcol * (0.85 + 0.15 * np.clip(fbm(xt * 4, zt * 4, 3) * 0.5 + 0.5, 0, 1))[..., None]
    edge = deck & ((np.abs(xt - X0) < 0.06) | (np.abs(xt - X1) < 0.06) | (y < HZ + F * (EYE - FLOOR) / Z1 + 1.0))
    img[deck] = dcol[deck]; img[edge] = np.array([255, 205, 80])
    # the ring's sides (seen at the left and right edges) and its glow on the water
    yw = EYE - (y - HZ) * Z1 / F; xw = (x - 120) * Z1 / F
    for X in (X0, X1):
        zs = F * X / np.where(np.abs(x - 120) < 1e-3, 1e-3, (x - 120)); ys = EYE - (y - HZ) * zs / F
        side = (~deck) & (zs >= Z0) & (zs <= Z1) & (ys >= 0) & (ys <= FLOOR) & (y > HZ) & (np.sign(x - 120) == np.sign(X))
        img[side] = np.array([70, 60, 90]) * (0.7 + 0.3 * (ys[side] / FLOOR))[:, None]
    # lanterns on posts at the ring's back corners, glowing
    for X in (X0, X1):
        px_ = 120 + F * X / Z1; yb = HZ + F * (EYE - FLOOR) / Z1; yt = HZ + F * (EYE - FLOOR - 1.5) / Z1
        post = (np.abs(x - px_) < 1.0) & (y >= yt) & (y <= yb); img[post] = np.array([40, 26, 30])
        dl = np.hypot(x - px_, (y - yt) * 1.2)
        img += (np.clip(1 - dl / 16, 0, 1) ** 2)[..., None] * np.array([255, 170, 60]) * 0.8
        img[dl < 2.2] = np.array([255, 245, 200])
    img = np.clip(img, 0, 255)
    # RGB555 with a little ordered dither
    bay = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0 - 0.5
    dith = img / 8.0 + bay[(y.astype(int) & 3), (x.astype(int) & 3)][..., None] * 0.9
    q = np.clip(np.round(dith), 0, 31).astype(int)
    rgb555 = q[..., 0] | (q[..., 1] << 5) | (q[..., 2] << 10)
    ground = int(HZ + F * (EYE - FLOOR) / 6.0)
    zg = 6.0; ringL = int(120 + F * X0 / zg) + 4; ringR = int(120 + F * X1 / zg) - 4
    return rgb555.astype('<u2'), (q * 8).astype(np.uint8), ground, ringL, ringR

FRB = CW * CH // 2   # bytes a frame

def wtf():
    """THE BURST's send-off: 'WHAT THE FUCK JUST HAPPENED???' in wobbly comic letters, each a 16 x 32 sprite (colours 1 outline, 2 fill, 3 the light
    edge), so the game can jitter them and cycle their colours over everything else. Writes source/art/bortua_wtf.bin and source/bortuawtf.h."""
    from PIL import ImageFont, ImageDraw
    lines = ['WHAT THE FUCK', 'JUST HAPPENED???']
    fp = [f for f in ('/usr/share/fonts/opentype/tlwg/Purisa-Bold.otf', '/usr/share/fonts/opentype/tlwg/Purisa.otf', '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf') if os.path.exists(f)][0]
    font = ImageFont.truetype(fp, 104); rg = np.random.default_rng(9)
    out = bytearray(); xs = []; ln = []; widths = []; preview = []
    for li, line in enumerate(lines):
        x = 0
        for ch in line:
            if ch == ' ': x += 7; continue
            big = Image.new('L', (160, 200), 0); ImageDraw.Draw(big).text((24, 20), ch, font=font, fill=255)
            arr = np.array(big); ph = rg.uniform(0, 6.3)
            for yy in range(arr.shape[0]): arr[yy] = np.roll(arr[yy], int(round(3.5 * math.sin(yy / 22.0 + ph))))   # a wobble down the letter
            big = Image.fromarray(arr).rotate(rg.uniform(-9, 9), resample=Image.BICUBIC)
            bb = big.getbbox(); g = big.crop(bb); gw, gh = g.size; sc = min(24.0 / gh, 11.0 / gw)
            g = g.resize((max(1, int(round(gw * sc))), max(1, int(round(gh * sc)))), Image.LANCZOS); a = np.array(g) > 100
            cell = np.zeros((32, 16), np.uint8); gy = 3 + (24 - a.shape[0]) // 2 + int(rg.integers(-1, 2)); gx = 2 + (11 - a.shape[1]) // 2
            fill = np.zeros((32, 16), bool); fill[gy:gy + a.shape[0], gx:gx + a.shape[1]] = a
            dil = fill.copy()
            for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (-1, -1), (1, -1), (-1, 1)): dil |= np.roll(np.roll(fill, dy, 0), dx, 1)
            lit = fill & ~np.roll(np.roll(fill, 1, 0), 1, 1)   # the light edge: top left
            cell[dil & ~fill] = 1; cell[fill] = 2; cell[lit] = 3
            out += tiles(cell); xs.append(x); ln.append(li); preview.append(cell); x += a.shape[1] + 2
        widths.append(x - 2)
    open(os.path.join(ROOT, 'source', 'art', 'bortua_wtf.bin'), 'wb').write(bytes(out))
    with open(os.path.join(ROOT, 'source', 'bortuawtf.h'), 'w') as f:
        f.write('// bortuawtf.h - GENERATED by tools/make_bortua.py (wtf): the BURST\'s send-off, %d wobbly comic letters as 16 x 32 sprites (4bpp, 1D, 8 tiles each; colours 1 outline, 2 fill, 3 light edge).\n' % len(xs))
        f.write('#define BFA_WTF_N %d\n#define BFA_WTF_W0 %d\n#define BFA_WTF_W1 %d\n' % (len(xs), widths[0], widths[1]))
        f.write('static const u8 bfaWtfX[BFA_WTF_N]={%s};   // each letter\'s column in its line\n' % ','.join(str(v) for v in xs))
        f.write('static const u8 bfaWtfL[BFA_WTF_N]={%s};   // its line\n' % ','.join(str(v) for v in ln))
        f.write('__asm__(".pushsection .rodata.bfaArt,\\"a\\"\\n.balign 4\\n.global bfaWtf\\nbfaWtf:\\n.incbin \\"source/art/bortua_wtf.bin\\"\\n.popsection\\n");\nextern const u32 bfaWtf[];\n')
    sheet = np.zeros((2 * 34, 240, 3), np.uint8) + np.array([40, 30, 60], np.uint8); pal = [(0, 0, 0), (10, 5, 20), (255, 200, 40), (255, 250, 200)]
    for cell, x, l in zip(preview, xs, ln):
        ox = 120 - widths[l] // 2 + x
        for k in (1, 2, 3): sheet[l * 34:l * 34 + 32, ox:ox + 16][cell == k] = pal[k]
    return sheet


def tiles(img):   # any multiple-of-8 picture -> 4bpp tiles, 1D order
    h, w = img.shape; out = bytearray()
    for ty in range(h // 8):
        for tx in range(w // 8):
            for r in range(8):
                row = img[ty * 8 + r, tx * 8:tx * 8 + 8]
                for q in range(0, 8, 2): out.append(int(row[q]) | (int(row[q + 1]) << 4))
    return bytes(out)

def fx():
    """The hit spark (4 frames, 32 x 32, colours 1 dark ... 7 white), the shadow under a fighter (32 x 8) and a small one (16 x 8) for a jump."""
    out = bytearray(); y, x = np.mgrid[0:32, 0:32] + 0.5; dx, dy = x - 16, y - 16; r = np.hypot(dx, dy); a = np.arctan2(dy, dx)
    bay = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0
    for f in range(4):
        R = 5 + f * 3.6; rays = np.abs(np.cos(a * 4 + f * 0.4)) ** 6 + 0.6 * np.abs(np.cos(a * 4 + math.pi / 4 + f * 0.4)) ** 10
        v = np.clip(1.0 - r / (R * (0.45 + 0.75 * rays)), 0, 1)
        if f >= 2: v *= np.clip((r - (f - 1.6) * 4.5) / 3, 0, 1)   # the middle burns out first
        v = v * (1.0 - 0.18 * f)
        q = np.where(v > 0.02, np.clip((v * 7 + bay[y.astype(int) & 3, x.astype(int) & 3] * 0.9).astype(int), 1, 7), 0)
        out += tiles(q.astype(np.uint8))
    for w in (32, 16):
        yy, xx = np.mgrid[0:8, 0:w] + 0.5; e = ((xx - w / 2) / (w / 2)) ** 2 + ((yy - 4) / 3.6) ** 2
        out += tiles(np.where(e < 1, 1, 0).astype(np.uint8))
    rg = np.random.default_rng(11); blobs = [(rg.uniform(-1, 1), rg.uniform(-1, 1), rg.uniform(0.45, 0.8)) for _ in range(7)]
    for f in range(3):   # the TOOT's cloud: three frames, puffing out and thinning (colours 1 dark ... 7 light)
        R = 6 + f * 3.5; v = np.zeros((32, 32))
        for bx_, by_, br in blobs: v = np.maximum(v, np.clip(1 - np.hypot(dx - bx_ * R * 0.8, dy - by_ * R * 0.6) / (br * R), 0, 1))
        v = v ** 0.7 * (1.0 - 0.22 * f)
        q = np.where(v > 0.06 + 0.1 * f, np.clip((v * 6 + 1 + bay[y.astype(int) & 3, x.astype(int) & 3] * 0.9).astype(int), 1, 7), 0)
        out += tiles(q.astype(np.uint8))
    return bytes(out)

def preview(imgs, cols, path, scale=2):
    pal = demo_pal((214, 160, 120), (40, 120, 200), (30, 40, 90), (24, 24, 30))
    rows = (len(imgs) + cols - 1) // cols
    sheet = np.zeros((rows * CH, cols * CW, 3), np.uint8) + np.array([90, 80, 110], np.uint8)
    for j, (img, ax) in enumerate(imgs):
        cell = sheet[(j // cols) * CH:(j // cols + 1) * CH, (j % cols) * CW:(j % cols + 1) * CW]
        for k in range(1, 16): cell[img == k] = pal[k]
        cell[AY + 1:AY + 3, max(0, ax - 1):ax + 2] = (255, 255, 0)   # the root
    Image.fromarray(sheet).resize((cols * CW * scale, rows * CH * scale), Image.NEAREST).save(path)

def job(args):   # one body with one seat: every frame (four run at a time)
    body, butt = args; out = []
    for nm, poses in ANIMS:
        for i, pz in enumerate(poses):
            wide = render(pz, body, butt); img, ax = crop(wide, '%s %s %s %d' % (body, butt, nm, i)); out.append((pack(img), img, ax, reach(wide)))
    sys.stderr.write('%s %s done\n' % (body, butt)); return out

def main():
    from multiprocessing import Pool
    os.makedirs(os.path.join(ROOT, 'source', 'art'), exist_ok=True)
    names = []; starts = {}; n = 0; pants0 = None
    for nm, poses in ANIMS:
        starts[nm] = n
        for pz in poses:
            if pz.get('pants') and pants0 is None: pants0 = n
            n += 1
        names.append((nm, len(poses)))
    bodies = list(BODIES); combos = [(b, t) for b in bodies for t in BUTTS]
    with Pool(4) as pool: res = pool.map(job, combos)
    blob = bytearray(); axs = []; prev = []
    for (body, butt), fr in zip(combos, res):
        for packed, img, ax, r in fr: blob += packed; axs.append(ax)
        if butt == 'ROUND': prev += [(img, ax) for packed, img, ax, r in fr]
    reaches = [r for packed, img, ax, r in res[0]]   # (the standard body, flat: what the hits are measured against)
    open(os.path.join(ROOT, 'source', 'art', 'bortua_fighters.bin'), 'wb').write(blob)
    import subprocess; subprocess.check_call([sys.executable, os.path.join(ROOT, 'tools', 'pack_fighters.py'), '--check'])   # .bin -> .lz (what goes in the ROM)
    open(os.path.join(ROOT, 'source', 'art', 'bortua_fx.bin'), 'wb').write(fx())
    wtf()
    a555, argb, ground, ringL, ringR = arena()
    open(os.path.join(ROOT, 'source', 'art', 'bortua_arena.bin'), 'wb').write(a555.tobytes())
    with open(os.path.join(ROOT, 'source', 'bortuaart.h'), 'w') as f:
        f.write('// bortuaart.h - GENERATED by tools/make_bortua.py: BORTUA FIGHTER\'s pre-rendered frames (source/art/bortua_fighters.bin) and arena (bortua_arena.bin).\n')
        f.write('// A frame: 64 x 128 as two 64 x 64 sprites (4bpp, 1D), BFA_FRB bytes. Every body comes with every seat (BFS_: the game\'s BUTT switch and slider);\n')
        f.write('// frame f of body b with seat s at bfaFighters + ((b*BFA_BUTTS+s)*BFA_NFR+f)*BFA_FRB/4, its root at bfaAx[(b*BFA_BUTTS+s)*BFA_NFR+f].\n')
        f.write('// Colours: 1 outline, 2-4 skin, 5-7 top, 8-10 bottom (the hips and seat), 11-12 hair, 13 shoes, 14 belt / gloves, 15 shine; from BFA_PANTS on\n')
        f.write('// (the MOON with the pants down) 8-10 are the bare seat or the underwear, 13-14 the pulled-down pants, and the shoes are 1.\n')
        f.write('#define BFA_W %d\n#define BFA_H %d\n#define BFA_AY %d\n#define BFA_NFR %d\n#define BFA_BODIES %d\n#define BFA_BUTTS %d\n#define BFA_FRB %d\n#define BFA_PANTS %d\n' % (CW, CH, AY, n, len(bodies), len(BUTTS), FRB, pants0))
        f.write('enum { %s };\nenum { %s };\n' % (', '.join('BFB_' + b for b in bodies), ', '.join('BFS_' + t for t in BUTTS)))
        for nm, k in names: f.write('#define BFA_%s %d\n#define BFA_%s_N %d\n' % (nm, starts[nm], nm, k))
        f.write('static const s8 bfaReach[BFA_NFR]={%s};   // px in front of the root each frame reaches at body height (the standard body)\n' % ','.join(str(max(-128, min(127, r))) for r in reaches))
        f.write('static const u8 bfaAx[BFA_BODIES*BFA_BUTTS*BFA_NFR]={%s};   // the root\'s column in each frame\n' % ','.join(str(a) for a in axs))
        f.write('#define BFA_GROUND %d   // the arena row the fighters stand on\n#define BFA_RINGL %d\n#define BFA_RINGR %d   // the ring\'s edges on that row (a little inside)\n' % (ground, ringL, ringR))
        f.write('__asm__(".pushsection .rodata.bfaArt,\\"a\\"\\n.balign 4\\n.global bfaFighters\\nbfaFighters:\\n.incbin \\"source/art/bortua_fighters.lz\\"\\n.balign 4\\n.global bfaArena\\nbfaArena:\\n.incbin \\"source/art/bortua_arena.bin\\"\\n.balign 4\\n.global bfaFx\\nbfaFx:\\n.incbin \\"source/art/bortua_fx.bin\\"\\n.popsection\\n");\n')
        f.write('extern const u32 bfaFighters[]; extern const u16 bfaArena[]; extern const u32 bfaFx[];   // bfaFx: 4 spark frames (32 x 32, 512 bytes each), a 32 x 8 shadow, a 16 x 8 one, 3 frames of the TOOT cloud (32 x 32)\n')
    preview(prev, n, os.path.join(ROOT, 'tools', 'bortua_preview.png'), 1)
    Image.fromarray(argb).resize((480, 320), Image.NEAREST).save(os.path.join(ROOT, 'tools', 'bortua_arena_preview.png'))
    print('frames', n, 'bytes', len(blob), 'ground', ground, 'ring', ringL, ringR)

if __name__ == '__main__':
    SCR = os.environ.get('BF_OUT', '/tmp/') 
    if len(sys.argv) > 1 and sys.argv[1] == 'sheet':   # make_bortua.py sheet BODY ANIM ...: those animations side by side, for a look
        body = sys.argv[2]; butt = os.environ.get('BF_BUTT', 'FLAT')
        imgs = [crop(render(pz, body, butt), '%s %d' % (nm, i)) for nm in sys.argv[3:] for i, pz in enumerate(dict(ANIMS)[nm])]
        preview(imgs, min(len(imgs), 12), SCR + 'bf_sheet.png', 2)
    elif len(sys.argv) > 1 and sys.argv[1] == 'wtf':   # just the BURST's send-off letters (and a look at them)
        Image.fromarray(wtf()).resize((720, 204), Image.NEAREST).save(SCR + 'bf_wtf.png')
    elif len(sys.argv) > 1 and sys.argv[1] == 'arena':
        Image.fromarray(arena()[1]).resize((480, 320), Image.NEAREST).save(SCR + 'bf_arena.png')
    else: main()

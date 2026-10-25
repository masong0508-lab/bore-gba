"""spr.py - sprites: the four giants (32 x 64 canvases, feet on row 60), the Nubbins (8 x 16), the carried goods, the cursor and the smoke.
Giant palette roles: 1 outline 2 skin 3 skin shade 4 hair 5 hair light 6 shirt 7 shirt shade 8 shirt light 9 trousers 10 trousers shade 11 boots 12 hat/accessory 13 accessory light 14 belt / detail 15 white"""
import math
from pix import Cv, lit, rgb15 as C
def gp(skin, hair, shirt, pants, hat, boots=C(9,5,3), belt=C(14,9,4)):
    def sh(c, k): r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31; return C(int(r * k), int(g * k), int(b * k))
    def li(c, k): r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31; return C(r + int((31 - r) * k), g + int((31 - g) * k), b + int((31 - b) * k))
    return [0, C(2,3,8), skin, sh(skin, .72), hair, li(hair, .35), shirt, sh(shirt, .68), li(shirt, .35), pants, sh(pants, .68), boots, hat, li(hat, .45), belt, C(31,31,31)]
GIANTS = [
 dict(name='A', w=14, hair='crop', beard=0, hat=0, tool=0, pal=gp(C(27,20,15), C(9,6,3), C(6,20,8), C(17,13,8), C(6,20,8))),                       # the all-rounder: brown hair, green tunic
 dict(name='B', w=11, hair='spiky', beard=0, hat=0, tool=1, pal=gp(C(29,22,16), C(28,24,6), C(28,14,3), C(7,11,22), C(27,5,5), belt=C(27,5,5))),   # the runner: lean, a red headband, orange vest
 dict(name='C', w=18, hair='bald', beard=1, hat=0, tool=0, pal=gp(C(26,18,13), C(20,20,21), C(17,8,24), C(15,10,6), C(17,8,24), belt=C(26,22,6))),   # the lifter: huge, bald, a grey beard, purple vest
 dict(name='D', w=14, hair='crop', beard=2, hat=1, tool=2, pal=gp(C(27,20,15), C(10,6,3), C(28,26,4), C(5,10,24), C(30,24,3), belt=C(10,6,3))),     # the builder: a yellow hard hat, a moustache, blue overalls
]
def giant(g, view, frame):
    c = Cv(32, 64, 0); w = g['w']; hw = w // 2
    bob = (0, -1, 0)[frame] if frame < 3 else 0
    step = (0, 1, -1)[frame]                       # which leg is forward: 0 neither
    cx = 16
    def leg(x0, y0, h, forward, side=False):
        yy = y0 - (2 if forward else 0)
        c.rect(x0, yy, 5, h - 3 + (2 if forward else 0), 9); c.vline(x0 + 4, yy, yy + h - 4, 10); c.rect(x0 - (1 if side else 0), yy + h - 3, 6 + (1 if side else 0), 3, 11); c.hline(x0, x0 + 4, yy + h - 3, 11)
    torso_top = 34 + bob; torso_bot = 48 + bob
    if view in ('d', 'u'):
        leg(cx - 6, 47, 14, step == 1); leg(cx + 1, 47, 14, step == -1)
        # torso: rounded, lit from the left
        for y in range(torso_top, torso_bot):
            t = (y - torso_top) / (torso_bot - torso_top); ww = hw + (1 if t < .15 else 0)
            for x in range(cx - ww, cx + ww):
                col = 8 if x < cx - ww + 2 else 7 if x >= cx + ww - 3 else 6
                c.px(x, y, col)
        c.hline(cx - hw, cx + hw - 1, torso_bot - 3, 14); c.rect(cx - 1, torso_bot - 3, 3, 2, 13)    # belt and buckle
        if g['tool'] == 2:   # overalls: bib and straps
            c.rect(cx - 4, torso_top + 1, 8, 7, 9); c.vline(cx - 3, torso_top, torso_top - 1, 9); c.vline(cx + 2, torso_top, torso_top - 1, 9)
            c.px(cx - 4, torso_top + 7, 13); c.px(cx + 3, torso_top + 7, 13); c.hline(cx - 4, cx + 3, torso_top + 1, 10)
        if g['tool'] == 1: c.line(cx - hw, torso_top + 1, cx + hw - 1, torso_bot - 5, 14)   # a bandolier
        # arms
        aw = 4 + (1 if w > 16 else 0)
        for side, sgn in ((0, -1), (1, 1)):
            swing = (-3 if (step == 1) == (side == 0) else 3) if step else 0
            ax = cx - hw - aw + (0 if side == 0 else 2 * hw + aw)
            ay = torso_top + 1 + (swing if step else 0) * 0
            ah = 13 + (0 if not step else (-1 if swing < 0 else 1))
            c.rect(ax, ay, aw, ah - 3, 6); c.vline(ax if side == 0 else ax + aw - 1, ay, ay + ah - 4, 8 if side == 0 else 7)
            c.ell(ax + aw / 2, ay + ah - 2, aw / 2 + .6, 2.4, lambda x, y, nx, ny: 2 if nx < .3 else 3)
        # head
        hy = 27 + bob
        c.ell(cx, hy, 7, 6.6, lambda x, y, nx, ny: lit(nx, ny, [3, 2, 2, 2]) if view == 'd' else 4)
        c.rect(cx - 2, torso_top - 2, 4, 3, 3)           # neck
        if view == 'd':
            for ex in (cx - 3, cx + 2): c.px(ex, hy - 1, 15); c.px(ex, hy, 1) if False else c.px(ex + (0), hy - 1, 15); c.px(ex, hy, 1)
            c.px(cx - 3, hy - 1, 15); c.px(cx + 2, hy - 1, 15); c.px(cx - 3, hy, 1); c.px(cx + 2, hy, 1)
            c.hline(cx - 1, cx, hy + 3, 3) if True else None
            c.px(cx - 4, hy + 1, 3); c.px(cx + 3, hy + 1, 3)
            if g['beard'] == 1:
                for y in range(hy + 2, hy + 9):
                    for x in range(cx - 5 + (y - hy - 2) // 3, cx + 5 - (y - hy - 2) // 3): c.px(x, y, 4 if x > cx - 3 else 5)
                c.hline(cx - 1, cx, hy + 3, 3)
            if g['beard'] == 2: c.hline(cx - 3, cx + 2, hy + 2, 4); c.px(cx - 4, hy + 3, 4); c.px(cx + 3, hy + 3, 4)
        # hair / hat
        if view == 'u': c.ell(cx, hy - .4, 7.2, 6.8, lambda x, y, nx, ny: 5 if nx < -.3 and ny < 0 else 4)
        else:
            if g['hair'] == 'crop': c.ell(cx, hy - 3.2, 7.2, 4.2, lambda x, y, nx, ny: 5 if nx < -.3 and ny < 0 else 4, only={2, 3, 0})
            if g['hair'] == 'spiky':
                c.ell(cx, hy - 3.4, 7, 4, 4, only={2, 3, 0})
                for k, x in enumerate(range(cx - 6, cx + 6, 3)): c.vline(x + 1, hy - 11 + (k % 2) * 2, hy - 5, 4); c.px(x + 1, hy - 11 + (k % 2) * 2, 5)
                c.hline(cx - 7, cx + 6, hy - 3, 12); c.hline(cx - 7, cx + 6, hy - 4, 12); c.vline(cx + 7, hy - 4, hy - 1, 12); c.vline(cx + 8, hy - 3, hy, 12)   # the headband and its tail
            if g['hair'] == 'bald': c.ell(cx - 2, hy - 5, 2.2, 1.2, 15, only={2, 3})
        if g['hat']:
            c.ell(cx, hy - 4.5, 8, 4.4, lambda x, y, nx, ny: 13 if nx < -.35 and ny < .1 else 12, only={2, 3, 0, 4, 5}); c.hline(cx - 8, cx + 7, hy - 2, 12); c.hline(cx - 8, cx + 7, hy - 1, 14)
    else:   # side view, facing right
        back = 1 if step == 1 else 0
        c.rect(cx - 6 - (2 if step == -1 else 0) + 2, 47 - (2 if step == 1 else 0), 5, 11 + (2 if step == 1 else 0), 10); c.rect(cx - 6 - (2 if step == -1 else 0) + 2, 57, 6, 3, 11)   # the far leg
        c.rect(cx - 1 + (2 if step == 1 else 0) - (0 if step else 0), 47 - (2 if step == -1 else 0), 5, 11 + (2 if step == -1 else 0), 9); c.vline(cx + 3 + (2 if step == 1 else 0), 47, 56, 10); c.rect(cx - 1 + (2 if step == 1 else 0), 57, 7, 3, 11)
        tw = max(6, hw - 1)
        for y in range(torso_top, torso_bot):
            for x in range(cx - tw, cx + tw): c.px(x, y, 8 if x < cx - tw + 2 else 7 if x >= cx + tw - 2 else 6)
        c.hline(cx - tw, cx + tw - 1, torso_bot - 3, 14)
        if g['tool'] == 2: c.rect(cx - 2, torso_top + 1, 5, 7, 9)
        sw = (3 if step == 1 else -3 if step == -1 else 0)
        ax = cx - 2 + sw; c.rect(ax, torso_top + 1, 4, 11, 6); c.vline(ax + 3, torso_top + 1, torso_top + 10, 7); c.ell(ax + 2, torso_top + 12, 2.6, 2.4, 2)
        hy = 27 + bob
        c.ell(cx + 1, hy, 6.6, 6.4, lambda x, y, nx, ny: lit(nx, ny, [3, 2, 2, 2])); c.rect(cx - 1, torso_top - 2, 4, 3, 3)
        c.px(cx + 8, hy + 1, 2); c.px(cx + 8, hy + 2, 3); c.px(cx + 5, hy - 1, 15); c.px(cx + 6, hy - 1, 1); c.px(cx + 4, hy + 3, 3)
        if g['beard'] == 1:
            for y in range(hy + 2, hy + 9):
                for x in range(cx + 1 + (y - hy - 2) // 3, cx + 8 - (y - hy - 2) // 3): c.px(x, y, 4 if x > cx + 3 else 5)
        if g['beard'] == 2: c.hline(cx + 4, cx + 8, hy + 2, 4)
        if g['hair'] == 'crop': c.ell(cx, hy - 2.8, 7, 4.4, lambda x, y, nx, ny: 5 if nx < -.3 and ny < 0 else 4, only={2, 3, 0}); c.rect(cx - 6, hy - 3, 4, 7, 4)
        if g['hair'] == 'spiky':
            c.ell(cx, hy - 3, 6.8, 4.2, 4, only={2, 3, 0}); c.rect(cx - 6, hy - 3, 4, 6, 4)
            for k, x in enumerate(range(cx - 4, cx + 5, 3)): c.vline(x, hy - 10 + (k % 2) * 2, hy - 4, 4); c.px(x, hy - 10 + (k % 2) * 2, 5)
            c.hline(cx - 6, cx + 7, hy - 3, 12); c.hline(cx - 6, cx + 7, hy - 4, 12); c.vline(cx - 7, hy - 3, hy + 1, 12); c.vline(cx - 8, hy - 2, hy + 2, 12)
        if g['hat']: c.ell(cx, hy - 4.5, 7.8, 4.4, lambda x, y, nx, ny: 13 if nx < -.35 and ny < .1 else 12, only={2, 3, 0, 4, 5}); c.hline(cx - 7, cx + 10, hy - 2, 12); c.hline(cx - 7, cx + 10, hy - 1, 14)
    for y in range(57, 64):   # a soft shadow under the feet: a dithered dark ellipse
        for x in range(4, 28):
            nx, ny = (x + .5 - 16) / 11.5, (y + .5 - 60.5) / 3.2
            if nx * nx + ny * ny <= 1 and c.p[y][x] == 0 and (x + y) % 2 == 0: c.p[y][x] = 1
    c.outline(1, 0)
    return c
GV = ('d', 'd', 'd', 's', 's', 's', 'u', 'u', 'u')   # frames 0-2 down (stand, step, step), 3-5 side, 6-8 up
def giant_frames(g): return [giant(g, v, f) for v, f in zip(GV, (0, 1, 2, 0, 1, 2, 0, 1, 2))]
# ---- the Nubbins: 8 x 16, feet on row 15 (an 8 x 16 OBJ).  roles: 1 outline 2 skin 3 skin shade 4 hat 5 hat light 6 tunic 7 tunic shade 8 boots 9 white ----
NUBPAL = [0, C(2,3,8), C(29,22,16), C(23,16,11), 0, 0, 0, 0, C(9,5,3), C(31,31,31), 0, 0, 0, 0, 0, 0]
def nub_pal(hat, tunic):
    def sh(c, k): r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31; return C(int(r * k), int(g * k), int(b * k))
    def li(c, k): r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31; return C(r + int((31 - r) * k), g + int((31 - g) * k), b + int((31 - b) * k))
    p = NUBPAL[:]; p[4] = hat; p[5] = li(hat, .4); p[6] = tunic; p[7] = sh(tunic, .68); return p
NUB_LOOKS = [nub_pal(C(26,6,6), C(8,16,26)), nub_pal(C(6,18,8), C(26,22,8)), nub_pal(C(26,20,4), C(14,9,22)), nub_pal(C(8,12,26), C(22,8,8))]
def nubbin(view, frame):
    c = Cv(8, 16, 0); b = 1 if frame == 1 else 0
    if view == 's':
        c.rect(2, 12, 2, 3, 6) ; c.rect(2 + (1 if frame == 1 else 0), 14 - b, 2, 2, 8) ; c.px(3 + (0 if frame else 1), 15, 8)
        c.rect(2, 9, 4, 4, 6); c.vline(2, 9, 12, 7)
        c.ell(4, 6, 3, 2.9, lambda x, y, nx, ny: 3 if nx > .5 else 2); c.hline(2, 6, 3, 4); c.hline(3, 6, 2, 4); c.px(2, 3, 5); c.px(5, 6, 1); c.px(6, 7, 3)
    else:
        c.rect(2, 12, 4, 2, 6); c.rect(2, 14 - b, 2, 2, 8); c.rect(4, 14 - (1 - b) * 0 - (1 if frame == 2 else 0), 2, 2, 8)
        c.rect(1, 9, 6, 4, 6); c.vline(6, 9, 12, 7); c.px(1, 12, 2); c.px(6, 12, 2)
        c.ell(4, 6, 3.2, 3, lambda x, y, nx, ny: 3 if nx > .55 else 2); c.hline(1, 6, 3, 4); c.hline(2, 5, 2, 4); c.px(2, 3, 5)
        if view == 'd': c.px(3, 6, 1); c.px(5, 6, 1)
        else: c.hline(1, 6, 5, 4); c.hline(2, 5, 6, 4)
    c.outline(1, 0)
    return c
NUBF = [nubbin(v, f) for v in ('s', 'd', 'u') for f in (0, 1)]    # 6 frames: side stand / step, down stand / step, up stand / step
# ---- little icons for what a carrier holds (8 x 8, own palette) and what the giant holds ----
GOODPAL = [0, C(2,3,8), C(17,11,5), C(24,17,9), C(16,16,19), C(23,23,26), C(29,29,30), C(5,18,24), C(13,27,31), C(30,30,28), C(28,24,14), C(21,9,26), C(28,14,20), C(10,10,12), C(0,0,0), C(31,31,31)]
def goodicon(g):
    c = Cv(8, 8, 0)
    if g == 0:   # wood: a log
        c.ell(4, 4, 3.6, 2.8, lambda x, y, nx, ny: 2 if nx * nx + ny * ny > .5 else 3); c.px(4, 4, 2); c.px(3, 4, 3)
    elif g == 1:  # stone
        c.ell(4, 4, 3.4, 2.9, lambda x, y, nx, ny: lit(nx, ny, [4, 4, 5, 6]))
    elif g == 2:  # wool
        c.ell(4, 4, 3.4, 3, lambda x, y, nx, ny: lit(nx, ny, [6, 9, 9, 15])); c.px(3, 3, 15)
    elif g == 3:  # crystal
        for j, w in enumerate((0, 1, 1, 2, 2, 1)):
            for k in range(-w, w + 1): c.px(4 + k, 1 + j, 8 if k <= 0 else 7)
        c.px(3, 3, 15)
    elif g == 4:  # planks
        for y in (1, 3, 5): c.rect(1, y, 6, 2, 3); c.hline(1, 6, y, 10)
    elif g == 5:  # cut stone block
        c.rect(1, 2, 6, 5, 6); c.hline(1, 6, 2, 15); c.vline(6, 2, 6, 4); c.hline(1, 6, 6, 4)
    elif g == 6:  # cloth
        c.rect(1, 1, 6, 6, 12); c.hline(1, 6, 1, 15); c.hline(1, 6, 3, 11); c.hline(1, 6, 5, 11); c.vline(6, 1, 6, 11)
    elif g == 7:  # polished crystal
        for j, w in enumerate((0, 1, 2, 2, 1, 0)):
            for k in range(-w, w + 1): c.px(4 + k, 1 + j, 15 if k < 0 else 8 if k == 0 else 7)
        c.px(3, 2, 15)
    c.outline(1, 0)
    return c
GOODS = [goodicon(g) for g in range(8)]
def smoke(f):   # a puff, 8 x 8
    c = Cv(8, 8, 0); r = (1.6, 2.4, 3.1, 3.4)[f]; c.ell(4, 4, r, r * .8, lambda x, y, nx, ny: 6 if nx < .2 else 5); return c
SMOKEPAL = [0, 0, 0, 0, 0, C(25,25,27), C(29,29,30)] + [0] * 9
def cursor():   # a 16 x 16 square of four corner brackets
    c = Cv(16, 16, 0)
    for (x0, y0, dx, dy) in ((0, 0, 1, 1), (15, 0, -1, 1), (0, 15, 1, -1), (15, 15, -1, -1)):
        for k in range(4): c.px(x0 + dx * k, y0, 15); c.px(x0, y0 + dy * k, 15); c.px(x0 + dx * k, y0 + dy, 14); c.px(x0 + dx, y0 + dy * k, 14)
    return c
CURPAL = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(4,10,3), C(31,31,12)]
# ---- the CROWDS: small figures (5 x 8) packed into one sprite, so a throng of dozens costs one OAM entry.  Own palette (OBJ bank 8): 1 outline 2 skin 3 skin shade 4..7 the four hats 8..11 the four tunics 12 boots 13 white ----
CROWDPAL = [0, C(2,3,8), C(29,22,16), C(23,16,11), C(26,6,6), C(6,18,8), C(26,20,4), C(8,12,26), C(8,16,26), C(26,22,8), C(14,9,22), C(22,8,8), C(9,5,3), C(31,31,31), 0, 0]
def crowd_fig(c, x, yf, look, up):   # one little figure, left column x, feet on row yf (up: a hop of one row)
    yf -= up; hat, tun = 4 + look % 4, 8 + look % 4
    c.rect(x + 1, yf - 7, 3, 1, hat); c.rect(x + 1, yf - 6, 3, 2, 2); c.px(x + 3, yf - 5, 3); c.px(x + 3, yf - 6, 3)
    c.rect(x, yf - 4, 5, 2, tun); c.rect(x + 1, yf - 2, 3, 1, tun); c.px(x, yf - 4, 2); c.px(x + 4, yf - 4, 2)
    c.px(x + 1, yf - 1, 12); c.px(x + 3, yf - 1, 12); c.px(x + 1, yf, 12); c.px(x + 3, yf, 12)
def crowd_sheet(w, h, rows, frame):   # rows: [(feet y, first x, step, count)]; the figures go back to front
    c = Cv(w, h, 0); k = 0
    for ri, (yf, x0, st, n) in enumerate(rows):
        for i in range(n):
            k += 7; look = (i * 3 + ri * 5 + k) % 4; hop = 1 if (i + ri + frame) % 3 == 0 and frame else 0
            x = x0 + i * st + ((k * 5 + i) % 3 - 1); x = max(1, min(w - 6, x))   # a shuffle of a pixel, so the rows do not look drawn with a ruler
            crowd_fig(c, x, yf + ((k + ri) % 4 == 0), look, hop)
    c.outline(1, 0); return c
def crowd_frames(f):   # a group of 5 (16 x 16), a crowd of 13 (32 x 16), a throng of 26 (32 x 32)
    return [crowd_sheet(16, 16, [(10, 1, 4, 3), (14, 3, 5, 2)], f),
            crowd_sheet(32, 16, [(10, 1, 4, 7), (14, 3, 4, 6)], f),
            crowd_sheet(32, 32, [(10, 1, 4, 7), (16, 3, 4, 6), (22, 1, 4, 7), (28, 3, 4, 6)], f)]
CROWD = [crowd_frames(0), crowd_frames(1)]      # [frame][size]

#!/usr/bin/env python3
"""THE CATALOG: 200 more things to buy for your lot, in the same textured-box model as the other item art (see make_skate_items.py).
Run:  python3 tools/make_catalog.py   then   sh tools/bake_items.sh   (bakes every sprite into source/itemrom.h, in ROM)
Writes  source/catalogart.h   the boxes and textures (host-only: itembake.h includes it for the baker)
        source/catalogbake.h  the bakeOne(...) lines (itembake.h includes it inside bakeAll)
        source/catalogids.h   CAT_N (items) and CAT_SPR_N (sprites), for itemids.h
        source/catalog.h      the table the game reads: name, the map character it behaves as, price, sprite, turns or not, category, swatch colour
A catalog item is a LOOK: the tile keeps an ordinary map character (a chair is 'h', a sofa 'C', a fridge 'F' ...) so everything that already
works on that character (sitting, sleeping, eating, the room score, walking round it, saving) keeps working; only the picture is new (itemMap).
Every item here is 1 tile. 'rot' items turn to face away from the nearest wall like the other furniture (4 sprites); the rest have one sprite.
"""
import os, re
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PALS = []; MATS = []; OBJS = []; ITEMS = []
def pal(name, cols): PALS.append((name, cols)); return cols
class Mat:
    def __init__(s, name, pal, rows): s.name, s.pal, s.rows = name, pal, rows; s.h = len(rows); s.w = len(rows[0])
def mat(name, p, rows):
    for o in MATS:
        if o.name == name: assert o.rows == rows and o.pal is p, name; return o
    m = Mat(name, p, rows); MATS.append(m); return m
class Box:
    def __init__(s, a0, b0, a1, b1, z0, z1, m): s.a0, s.b0, s.a1, s.b1, s.z0, s.z1, s.m = a0, b0, a1, b1, z0, z1, m

def clamp(c): return tuple(max(0, min(31, int(round(v)))) for v in c)
def sh(c, k): return clamp((c[0] * k, c[1] * k, c[2] * k))
def lit(c, k=1.25): return clamp((c[0] * k + 2, c[1] * k + 2, c[2] * k + 2))
def ramp(c): return [sh(c, 0.45), sh(c, 0.72), c, lit(c), lit(c, 1.5)]   # a..e: shadow, dark, mid, light, shine

_mn = [0]
def M(p, rows):   # a texture with an automatic name
    _mn[0] += 1; return mat('mC%d' % _mn[0], p, rows)
def flat(p, ch): return M(p, [ch])
def faces(front, side, top): return [front, side, front, side, top]   # (front and back the same, both sides the same)
def solid(p, ch): m = flat(p, ch); return [m] * 5

# ---- texture helpers (letters a..e are the ramp of the item's main colour; later letters are extras) ----
def grain(p, w=8, h=8, base='c', streak='b', every=3): return M(p, [''.join(streak if (x + y) % every == 0 else base for x in range(w)) for y in range(h)])
def dots(p, w=8, h=8, base='c', dot='d', step=3): return M(p, [''.join(dot if (x % step == 1 and y % step == 1) else base for x in range(w)) for y in range(h)])
def stripes(p, w=8, h=8, a='c', b='d', horiz=True, every=2): return M(p, [''.join((b if ((y if horiz else x) // every) % 2 else a) for x in range(w)) for y in range(h)])
def panel(p, w=8, h=8, edge='b', fill='c'): return M(p, [edge * w] + [edge + fill * (w - 2) + edge for _ in range(h - 2)] + [edge * w])
def gradient(p, h=8, letters='abcde'): return M(p, [letters[min(len(letters) - 1, i * len(letters) // h)] for i in range(h)])

# ---- box helpers ----
def legs(a0, b0, a1, b1, h, m, t=1, z0=0): return [Box(a0, b0, a0 + t, b0 + t, z0, h, [m] * 5), Box(a1 - t, b0, a1, b0 + t, z0, h, [m] * 5),
                                              Box(a0, b1 - t, a0 + t, b1, z0, h, [m] * 5), Box(a1 - t, b1 - t, a1, b1, z0, h, [m] * 5)]
def block(a0, b0, a1, b1, z0, z1, fm): return [Box(a0, b0, a1, b1, z0, z1, fm)]

MAXB = 40   # boxes an item may have (drawObj's painter table in itembake.h)
def item(name, ch, price, cat, boxes, rot=0, nsh=11, col=None, fn=0):   # fn: what R does at it (counters.h): bit 0 a sink, bit 1 a microwave
    assert len(name) <= 18, name
    assert 1 <= len(boxes) <= MAXB, (name, len(boxes))
    for x in boxes:   # inside the tile, no taller than the canvas (z1 <= 28 keeps the back corner on the sprite), 5 real faces
        assert 0 <= x.a0 < x.a1 <= 8 and 0 <= x.b0 < x.b1 <= 8 and 0 <= x.z0 < x.z1 <= 28, (name, vars(x))
        assert len(x.m) == 5 and all(isinstance(m, Mat) for m in x.m), name
    ITEMS.append(dict(name=name, ch=ch, price=price, cat=cat, boxes=boxes, rot=rot, nsh=nsh, col=col or (20, 20, 20), fn=fn))

# ====================================================================== builders
def chair(p, wood=None, cushion=True):
    wood = wood or p; leg = flat(wood, 'b'); seatT = dots(p, 6, 6) if cushion else grain(p, 6, 6)
    seat = faces(flat(p, 'b'), flat(p, 'b'), seatT)
    back = faces(stripes(p, 6, 8, 'c', 'b', False, 2), flat(p, 'b'), flat(p, 'd'))
    return legs(1, 1, 7, 7, 5, leg) + block(1, 1, 7, 7, 5, 7, seat) + block(1, 1, 7, 2, 7, 15, back)
def armchair(p, wood):
    leg = flat(wood, 'b'); cush = faces(flat(p, 'c'), flat(p, 'c'), dots(p, 6, 6, 'c', 'd'))
    arm = solid(p, 'b'); back = faces(stripes(p, 8, 8, 'c', 'b', True, 3), flat(p, 'b'), flat(p, 'd'))
    return legs(0, 0, 8, 8, 2, leg) + block(1, 1, 7, 8, 2, 6, cush) + block(0, 1, 1, 8, 2, 9, arm) + block(7, 1, 8, 8, 2, 9, arm) + block(0, 0, 8, 2, 2, 13, back)
def stool(p, metal):
    leg = flat(metal, 'c'); top = faces(flat(p, 'b'), flat(p, 'b'), dots(p, 4, 4, 'c', 'd', 2))
    return [Box(3, 3, 5, 5, 0, 1, [leg] * 5), Box(3, 3, 5, 5, 1, 11, [leg] * 5)] + block(2, 2, 6, 6, 11, 13, top)
def office(p, metal):
    base = flat(metal, 'c'); cush = faces(flat(p, 'c'), flat(p, 'c'), dots(p, 6, 6)); back = faces(panel(p, 6, 8, 'b', 'c'), flat(p, 'b'), flat(p, 'd'))
    return [Box(1, 1, 7, 7, 0, 1, [base] * 5), Box(3, 3, 5, 5, 1, 6, [base] * 5)] + block(1, 1, 7, 7, 6, 8, cush) + block(1, 1, 7, 2, 8, 17, back)
def rocker(p):
    leg = flat(p, 'b'); seat = faces(flat(p, 'b'), flat(p, 'b'), grain(p, 6, 6)); back = faces(stripes(p, 6, 10, 'c', 'a', False, 2), flat(p, 'b'), flat(p, 'd'))
    return [Box(0, 1, 1, 8, 0, 1, [leg] * 5), Box(7, 1, 8, 8, 0, 1, [leg] * 5)] + legs(1, 1, 7, 7, 5, leg, 1, 1) + block(1, 1, 7, 7, 5, 7, seat) + block(1, 1, 7, 2, 7, 17, back)
def throne(p, gold):
    g = flat(gold, 'c'); cush = faces(flat(p, 'c'), flat(p, 'c'), dots(p, 6, 6, 'c', 'd')); back = faces(panel(gold, 8, 14, 'c', 'b'), flat(gold, 'b'), flat(gold, 'd'))
    return block(0, 0, 8, 8, 0, 4, solid(gold, 'b')) + block(1, 1, 7, 8, 4, 7, cush) + block(0, 1, 1, 8, 4, 11, [g] * 5) + block(7, 1, 8, 8, 4, 11, [g] * 5) + block(0, 0, 8, 2, 4, 20, back) + block(3, 0, 5, 1, 20, 23, [g] * 5)
def couch(p, wood, wide=True, pattern=None):
    leg = flat(wood, 'b'); top = pattern or dots(p, 8, 6, 'c', 'd')
    cush = faces(stripes(p, 8, 4, 'c', 'b', False, 4), flat(p, 'b'), top); arm = solid(p, 'b'); back = faces(stripes(p, 8, 7, 'c', 'b', False, 4), flat(p, 'b'), flat(p, 'd'))
    bx = legs(0, 0, 8, 8, 2, leg) + block(0, 2, 8, 8, 2, 6, cush) + block(0, 0, 8, 2, 2, 11, back)
    if wide: bx += block(0, 2, 1, 8, 6, 8, arm) + block(7, 2, 8, 8, 6, 8, arm)
    return bx
def futon(p, wood):
    frame = flat(wood, 'b'); top = stripes(p, 8, 6, 'c', 'd', True, 2)
    return legs(0, 0, 8, 8, 2, frame) + block(0, 0, 8, 8, 2, 3, solid(wood, 'c')) + block(0, 2, 8, 8, 3, 5, faces(flat(p, 'b'), flat(p, 'b'), top)) + block(0, 0, 8, 2, 3, 10, faces(flat(p, 'c'), flat(p, 'b'), flat(p, 'd')))
def chaise(p, wood):
    leg = flat(wood, 'b'); cush = faces(flat(p, 'b'), flat(p, 'b'), dots(p, 8, 8)); roll = solid(p, 'd')
    return legs(0, 0, 8, 8, 2, leg) + block(0, 0, 8, 8, 2, 5, cush) + block(0, 0, 8, 2, 5, 9, roll)
def bed(p, wood, big=False, headboard=True, pillows=2):
    frame = flat(wood, 'b'); quilt = faces(flat(p, 'b'), flat(p, 'b'), dots(p, 8, 8, 'c', 'd', 2)); pil = solid(pal('pWhite', [(28, 28, 30), (31, 31, 31), (24, 24, 26), (31, 31, 31), (31, 31, 31)]) if False else P_WHITE, 'b')
    bx = block(0, 0, 8, 8, 0, 2, solid(wood, 'c')) + block(0, 0, 8, 8, 2, 5, quilt)
    if headboard: bx += block(0, 0, 8, 1, 2, 9, faces(panel(wood, 8, 7, 'b', 'c'), flat(wood, 'b'), flat(wood, 'd')))
    if pillows == 1: bx += block(2, 1, 6, 3, 5, 6, pil)
    else: bx += block(1, 1, 4, 3, 5, 6, pil) + block(4, 1, 7, 3, 5, 6, pil)
    return bx
def bunk(p, wood):
    post = flat(wood, 'b'); quilt = faces(flat(p, 'b'), flat(p, 'b'), dots(p, 8, 8, 'c', 'd', 2)); q2 = faces(flat(p, 'd'), flat(p, 'c'), stripes(p, 8, 8, 'c', 'e', True, 2))
    return legs(0, 0, 8, 8, 20, post) + block(0, 0, 8, 8, 1, 4, quilt) + block(0, 0, 8, 8, 11, 14, q2) + block(0, 0, 1, 8, 14, 16, solid(wood, 'c'))
def canopy(p, wood):
    post = flat(wood, 'b')
    return bed(p, wood, True) + legs(0, 0, 8, 8, 22, post) + block(0, 0, 8, 8, 22, 23, faces(flat(p, 'c'), flat(p, 'c'), stripes(p, 8, 8, 'c', 'd', True, 2)))
def racecar(p):
    body = faces(stripes(p, 8, 4, 'c', 'e', True, 2), flat(p, 'c'), flat(p, 'c')); wheel = flat(P_BLACK, 'b'); quilt = faces(flat(P_WHITE, 'b'), flat(P_WHITE, 'b'), dots(P_WHITE, 6, 6, 'c', 'd', 2))
    return [Box(0, 1, 2, 3, 0, 3, [wheel] * 5), Box(6, 1, 8, 3, 0, 3, [wheel] * 5), Box(0, 5, 2, 7, 0, 3, [wheel] * 5), Box(6, 5, 8, 7, 0, 3, [wheel] * 5)] + block(0, 0, 8, 8, 1, 4, body) + block(1, 1, 7, 7, 4, 6, quilt) + block(0, 0, 8, 1, 4, 8, faces(flat(p, 'd'), flat(p, 'c'), flat(p, 'e')))
def table(p, legm=None, glass=False, round_=False, tall=False, cloth=None):
    legm = legm or flat(p, 'b'); h = 12 if tall else 7
    top = cloth or (M(p, ['ddcddcdd', 'dccccccd', 'ccccbccc', 'dccccccd', 'ccbccccc', 'dccccccd', 'cccccbcc', 'ddcddcdd']) if glass else grain(p, 8, 8))
    if round_: return [Box(3, 3, 5, 5, 0, h - 1, [legm] * 5), Box(1, 1, 7, 7, h - 1, h, faces(flat(p, 'b'), flat(p, 'b'), top)), Box(2, 2, 6, 6, h, h + 1, faces(flat(p, 'c'), flat(p, 'c'), top))]
    return legs(0, 0, 8, 8, h - 1, legm) + block(0, 0, 8, 8, h - 1, h + 1, faces(flat(p, 'b'), flat(p, 'c'), top))
def desk(p, drawers=True, screen=None):
    leg = flat(p, 'b'); bx = legs(0, 0, 8, 8, 7, leg) + block(0, 0, 8, 8, 7, 9, faces(flat(p, 'b'), flat(p, 'c'), grain(p, 8, 8)))
    if drawers: bx += block(5, 1, 8, 8, 0, 7, faces(M(p, ['bbb', 'bcb', 'bdb', 'bcb', 'bbb', 'bcb', 'bdb']), flat(p, 'b'), flat(p, 'c')))
    if screen is not None: bx += screen
    return bx
def monitor(z, a0=2, a1=6):
    scr = faces(M(P_SCREEN, ['aaaa', 'abba', 'acca', 'aaaa']), flat(P_BLACK, 'b'), flat(P_BLACK, 'b'))
    return [Box(3, 3, 5, 5, z, z + 1, solid(P_BLACK, 'c')), Box(a0, 2, a1, 3, z + 1, z + 6, scr)]
def shelf(p, rows=4, h=20, books=True):
    side = flat(p, 'b'); bx = block(0, 0, 1, 8, 0, h, [side] * 5) + block(7, 0, 8, 8, 0, h, [side] * 5) + block(0, 7, 8, 8, 0, h, [side] * 5)
    for r in range(rows):
        z = 1 + r * (h - 2) // rows; bx += block(0, 0, 8, 8, z, z + 1, solid(p, 'c'))
        if books: bx += block(1, 2, 7, 7, z + 1, z + (h - 2) // rows - 1, faces(M(P_BOOKS, ['abcdef', 'abcdef', 'abcdef']), flat(P_BOOKS, 'a'), flat(P_BOOKS, 'g')))
    return bx
def wardrobe(p, h=22):
    door = faces(M(p, ['bbbbbbbb', 'bcccbccc' + '', 'bcccbccc', 'bccdbdcc', 'bcccbccc', 'bcccbccc', 'bcccbccc', 'bbbbbbbb']), flat(p, 'b'), flat(p, 'c'))
    return block(0, 1, 8, 8, 0, h, door)
def dresser(p, h=11):
    front = faces(M(p, ['bbbbbbbb', 'bccdcccb', 'bbbbbbbb', 'bccdcccb', 'bbbbbbbb', 'bccdcccb', 'bbbbbbbb']), flat(p, 'b'), grain(p))
    return block(0, 1, 8, 8, 0, h, front) + legs(0, 1, 8, 8, 1, flat(p, 'a'))
def lamp(p, shade, h=22, shade_h=6, wide=3):
    pole = flat(P_METAL, 'c'); sm = faces(flat(shade, 'c'), flat(shade, 'c'), flat(shade, 'd'))
    return [Box(2, 2, 6, 6, 0, 1, [pole] * 5), Box(3, 3, 5, 5, 1, h - shade_h, [pole] * 5), Box(4 - wide, 4 - wide, 4 + wide, 4 + wide, h - shade_h, h, sm)]
def tablelamp(p, shade): return lamp(p, shade, 12, 5, 3)
def lava(p, glow):
    base = flat(P_METAL, 'c'); liq = faces(M(glow, ['acca', 'adda', 'caac', 'cddc', 'acca', 'dccd', 'adda', 'cccc']), flat(glow, 'b'), flat(glow, 'c'))
    return [Box(2, 2, 6, 6, 0, 3, [base] * 5), Box(2, 2, 6, 6, 3, 13, liq), Box(3, 3, 5, 5, 13, 15, [base] * 5)]
def plant(p, pot, leaves, h=14, pot_h=4, spread=1):
    potm = faces(stripes(pot, 6, 4, 'c', 'b', True, 3), flat(pot, 'c'), flat(P_SOIL, 'a')); lm = faces(dots(leaves, 8, 8, 'c', 'd', 2), dots(leaves, 8, 8, 'c', 'b', 2), dots(leaves, 8, 8, 'c', 'e', 2))
    return block(2, 2, 6, 6, 0, pot_h, potm) + [Box(3, 3, 5, 5, pot_h, pot_h + 3, solid(P_SOIL, 'b'))] + block(1 - spread + 1, 1 - spread + 1, 7 + spread - 1, 7 + spread - 1, pot_h + 2, h, lm)
def cactus(p, pot, tall=True):
    potm = faces(flat(pot, 'c'), flat(pot, 'c'), flat(P_SOIL, 'a')); cm = faces(stripes(p, 2, 8, 'c', 'b', False, 1), stripes(p, 2, 8, 'c', 'b', False, 1), flat(p, 'd'))
    bx = block(2, 2, 6, 6, 0, 4, potm)
    if tall: bx += block(3, 3, 5, 5, 4, 18, cm) + block(1, 3, 3, 5, 9, 14, cm) + block(5, 3, 7, 5, 7, 12, cm)
    else: bx += block(2, 2, 6, 6, 4, 9, cm) + block(3, 3, 5, 5, 9, 10, solid(P_PINK, 'c'))
    return bx
def appliance(p, h=16, front=None):
    f = front or panel(p, 8, h, 'b', 'c'); return block(1, 1, 7, 7, 0, h, faces(f, flat(p, 'c'), flat(p, 'd')))
def stove(p):
    f = M(p, ['bbbbbbbb', 'bccccccb', 'bcbbbbcb', 'bccccccb', 'bddccddb', 'bccccccb', 'bbbbbbbb'])
    top = M(P_BLACK, ['cccccccc', 'cbbccbbc', 'cbbccbbc', 'cccccccc', 'cbbccbbc', 'cbbccbbc', 'cccccccc', 'cccccccc'])
    return block(0, 1, 8, 8, 0, 12, [f, flat(p, 'c'), f, flat(p, 'c'), top])
def counter(p, top):
    f = M(p, ['bbbbbbbb', 'bcccbcccb'[:8], 'bcbcbcbcb'[:8], 'bcccbcccb'[:8], 'bbbbbbbb', 'bcccbcccb'[:8], 'bbbbbbbb']); return block(0, 1, 8, 8, 0, 11, [f, flat(p, 'c'), f, flat(p, 'c'), top]) + block(0, 0, 8, 8, 11, 12, faces(flat(p, 'd'), flat(p, 'd'), top))
def toilet(p):
    bowl = faces(flat(p, 'c'), flat(p, 'c'), M(p, ['cccc', 'cbbc', 'cbbc', 'cccc'])); tank = faces(panel(p, 4, 6, 'c', 'd'), flat(p, 'c'), flat(p, 'd'))
    return [Box(3, 4, 5, 6, 0, 2, solid(p, 'b')), Box(2, 3, 6, 7, 2, 5, bowl), Box(2, 1, 6, 3, 3, 9, tank)]
def shower(p, glass):
    tray = solid(p, 'c'); g = faces(M(glass, ['abbbbbba', 'abbbbbba', 'abcbbbba', 'abbbbbba']), flat(glass, 'b'), flat(glass, 'b')); head = flat(P_METAL, 'c')
    return block(0, 0, 8, 8, 0, 1, tray) + block(0, 0, 8, 1, 1, 20, faces(panel(p, 8, 19, 'b', 'c'), flat(p, 'b'), flat(p, 'c'))) + block(0, 0, 1, 8, 1, 20, g) + [Box(3, 1, 5, 2, 17, 18, [head] * 5)]
def tub(p, claw=False):
    tm = faces(flat(p, 'c'), flat(p, 'c'), M(P_WATER, ['abbbbbba', 'bccccccb', 'bcccdccb', 'bccccccb', 'bcdcccc b'.replace(' ', ''), 'bccccccb', 'bccccccb', 'abbbbbba']))
    bx = block(0, 1, 8, 8, 2 if claw else 0, 7, tm)
    if claw: bx += legs(0, 1, 8, 8, 2, flat(P_GOLD, 'c'))
    return bx
def tv(p, w=6, h=5, z=0, stand=True, screen=None):
    scr = faces(screen or M(P_SCREEN, ['aaaaaa', 'abbbca', 'abbbba', 'acbbba', 'aaaaaa']), flat(p, 'b'), flat(p, 'b')); a0 = 4 - w // 2
    bx = []
    if stand: bx += block(1, 1, 7, 7, 0, 5, faces(panel(p, 6, 5, 'b', 'c'), flat(p, 'b'), flat(p, 'c'))); z = 5
    return bx + block(a0, 3, a0 + w, 5, z, z + h, scr)
def arcade(p):
    body = faces(M(p, ['bbbbbbbb', 'bccccccb', 'bccddccb', 'bccccccb', 'bbbbbbbb', 'bcbccbcb', 'bbbbbbbb', 'bccccccb']), flat(p, 'b'), flat(p, 'c'))
    scr = faces(M(P_SCREEN, ['aaaaaa', 'abcbca', 'acbcba', 'abcbca', 'aaaaaa']), flat(p, 'b'), flat(p, 'c'))
    return block(1, 2, 7, 8, 0, 12, body) + block(1, 2, 7, 8, 12, 18, scr) + block(1, 1, 7, 2, 12, 20, faces(flat(p, 'd'), flat(p, 'b'), flat(p, 'e')))
def jukebox(p):
    body = faces(M(p, ['bbbbbbbb', 'bcdcdcdb', 'bcccccbb', 'bddddddb', 'bcccccbb', 'bcdcdcdb', 'bbbbbbbb']), flat(p, 'b'), flat(p, 'c'))
    dome = faces(M(P_GLASS, ['abbbbbba', 'bccccccb', 'bcddddcb']), flat(P_GLASS, 'b'), flat(P_GLASS, 'c'))
    return block(1, 1, 7, 7, 0, 12, body) + block(2, 1, 6, 6, 12, 16, dome)
def speakers(p):
    cone = M(p, ['bbbbbb', 'bcccc b'.replace(' ', ''), 'bcaacb', 'bcaacb', 'bccccb', 'bbbbbb']); f = faces(M(p, ['bbbbbb'] + cone.rows[1:5] + ['bbbbbb'] + cone.rows[1:5] + ['bbbbbb']), flat(p, 'b'), flat(p, 'c'))
    return block(1, 1, 7, 7, 0, 16, f)
def statue(p, pedestal, h=16):
    ped = faces(panel(pedestal, 6, 6, 'b', 'c'), flat(pedestal, 'c'), flat(pedestal, 'd'))
    return block(1, 1, 7, 7, 0, 6, ped) + [Box(3, 3, 5, 5, 6, h - 3, solid(p, 'c')), Box(2, 3, 6, 5, h - 7, h - 4, solid(p, 'd')), Box(3, 3, 5, 5, h - 3, h, solid(p, 'd'))]
def easel(wood, art):
    leg = flat(wood, 'b'); canvas = faces(art, flat(P_WHITE, 'c'), flat(P_WHITE, 'c'))
    return [Box(1, 6, 2, 7, 0, 14, [leg] * 5), Box(6, 6, 7, 7, 0, 14, [leg] * 5), Box(3, 2, 4, 3, 0, 10, [leg] * 5)] + block(1, 5, 7, 6, 5, 13, canvas)
def vase(p, h=12):
    return [Box(3, 3, 5, 5, 0, 2, solid(p, 'b')), Box(2, 2, 6, 6, 2, h - 3, faces(stripes(p, 4, 8, 'c', 'd', True, 2), flat(p, 'c'), flat(p, 'c'))), Box(3, 3, 5, 5, h - 3, h, solid(p, 'c'))]
def cone(p):
    return [Box(1, 1, 7, 7, 0, 1, solid(p, 'b')), Box(2, 2, 6, 6, 1, 5, faces(stripes(p, 4, 4, 'c', 'e', True, 2), flat(p, 'c'), flat(p, 'c'))), Box(3, 3, 5, 5, 5, 10, faces(stripes(p, 2, 5, 'c', 'e', True, 2), flat(p, 'c'), flat(p, 'd')))]
def tree(p, pot):
    trunk = solid(P_WOOD, 'b'); tiers = [(1, 7, 6, 11), (2, 6, 11, 15), (3, 5, 15, 19)]
    bx = block(2, 2, 6, 6, 0, 2, solid(pot, 'c')) + [Box(3, 3, 5, 5, 2, 7, trunk)]
    for a0, a1, z0, z1 in tiers: bx += block(a0, a0, a1, a1, z0, z1, faces(dots(p, 8, 4, 'c', 'f', 2), dots(p, 8, 4, 'c', 'f', 2), dots(p, 8, 8, 'c', 'g', 2)))
    return bx + [Box(3, 3, 5, 5, 19, 21, solid(P_GOLD, 'd'))]
def box_item(p, h=6, tape=True):
    f = M(p, ['bbbbbbbb', 'bccccccb', 'bcccdccb', 'bccccccb', 'bbbbbbbb']) if tape else panel(p)
    return block(0, 0, 8, 8, 0, h, [f, flat(p, 'c'), f, flat(p, 'c'), M(p, ['cccddccc'] * 8) if tape else flat(p, 'd')])

# ====================================================================== palettes
P_WHITE = pal('pCWhite', ramp((29, 29, 30))); P_BLACK = pal('pCBlack', ramp((6, 6, 8))); P_METAL = pal('pCMetal', ramp((18, 19, 23)))
P_GOLD = pal('pCGold', ramp((28, 22, 6))); P_SOIL = pal('pCSoil', ramp((10, 6, 3))); P_WOOD = pal('pCWood', ramp((18, 11, 5)))
P_SCREEN = pal('pCScreen', [(2, 3, 6), (8, 20, 30), (16, 28, 31), (30, 30, 31), (24, 12, 30)]); P_GLASS = pal('pCGlass', ramp((20, 26, 30)))
P_WATER = pal('pCWater', ramp((12, 20, 30))); P_PINK = pal('pCPink', ramp((30, 14, 22)))
P_BOOKS = pal('pCBooks', [(24, 6, 6), (6, 12, 26), (26, 22, 6), (8, 22, 10), (26, 12, 28), (28, 28, 28), (6, 5, 5)])
OAK = pal('pCOak', ramp((22, 14, 6))); PINE = pal('pCPine', ramp((26, 20, 11))); DARK = pal('pCDark', ramp((9, 6, 4))); WHITEW = pal('pCWhiteW', ramp((28, 28, 29)))
RED = pal('pCRed', ramp((26, 5, 5))); BLUE = pal('pCBlue', ramp((6, 10, 26))); GREEN = pal('pCGreen', ramp((6, 20, 8))); GREY = pal('pCGrey', ramp((15, 15, 17)))
MUST = pal('pCMust', ramp((26, 20, 4))); TEAL = pal('pCTeal', ramp((5, 20, 20))); PURP = pal('pCPurp', ramp((16, 6, 24))); BROWN = pal('pCBrown', ramp((16, 9, 4)))
MINT = pal('pCMint', ramp((14, 28, 20))); ORANGE = pal('pCOrange', ramp((28, 14, 3))); LEAF = pal('pCLeaf', ramp((8, 22, 7)) + [(6, 16, 5), (12, 28, 10)]); LEAF2 = pal('pCLeaf2', ramp((14, 24, 8)) + [(10, 18, 6), (20, 30, 12)])
TERRA = pal('pCTerra', ramp((22, 11, 5))); STONE = pal('pCStone', ramp((17, 17, 16))); MARBLE = pal('pCMarble', ramp((27, 27, 28))); SKY = pal('pCSky', ramp((12, 22, 31)))
NEON = pal('pCNeon', [(4, 2, 8), (31, 8, 24), (31, 20, 30), (31, 31, 31), (20, 4, 16)]); GLOW = pal('pCGlow', ramp((24, 28, 8)))
YELLOW = pal('pCYellow', ramp((30, 26, 4))); CYAN = pal('pCCyan', ramp((8, 26, 30))); LILAC = pal('pCLilac', ramp((22, 16, 30))); STEEL = pal('pCSteel', ramp((20, 22, 26)))

# ====================================================================== the catalogue (18 categories, 200 items)
CATS = []
def cat(name, adult=0): CATS.append((name, adult)); return len(CATS) - 1
C = cat('CHAIRS')
item('OAK CHAIR', 'h', 60, C, chair(OAK), 1, col=(22, 14, 6)); item('WHITE CHAIR', 'h', 70, C, chair(WHITEW, cushion=False), 1, col=(28, 28, 29))
item('BLACK CHAIR', 'h', 80, C, chair(DARK), 1, col=(9, 6, 4)); item('RED ARMCHAIR', 'h', 260, C, armchair(RED, DARK), 1, col=(26, 5, 5))
item('BLUE ARMCHAIR', 'h', 260, C, armchair(BLUE, DARK), 1, col=(6, 10, 26)); item('GREEN ARMCHAIR', 'h', 260, C, armchair(GREEN, OAK), 1, col=(6, 20, 8))
item('BAR STOOL', 'h', 90, C, stool(RED, STEEL), 0, col=(20, 22, 26)); item('OFFICE CHAIR', 'h', 180, C, office(DARK, STEEL), 1, col=(9, 6, 4))
item('ROCKING CHAIR', 'h', 150, C, rocker(PINE), 1, col=(26, 20, 11)); item('FOLDING CHAIR', 'h', 25, C, chair(GREY, STEEL, cushion=False), 1, col=(15, 15, 17))
item('THRONE', 'h', 2500, C, throne(PURP, P_GOLD), 1, col=(28, 22, 6)); item('WICKER CHAIR', 'h', 110, C, chair(MUST, PINE), 1, col=(26, 20, 4))
C = cat('SOFAS')
item('RED COUCH', 'C', 450, C, couch(RED, DARK), 1, col=(26, 5, 5)); item('BLUE COUCH', 'C', 450, C, couch(BLUE, DARK), 1, col=(6, 10, 26))
item('GREY COUCH', 'C', 420, C, couch(GREY, DARK), 1, col=(15, 15, 17)); item('MUSTARD COUCH', 'C', 480, C, couch(MUST, OAK), 1, col=(26, 20, 4))
item('PINK LOVESEAT', 'C', 380, C, couch(P_PINK, WHITEW, False), 1, col=(30, 14, 22)); item('TEAL LOVESEAT', 'C', 380, C, couch(TEAL, DARK, False), 1, col=(5, 20, 20))
item('LEATHER SOFA', 'C', 900, C, couch(BROWN, DARK, True, grain(BROWN, 8, 6)), 1, col=(16, 9, 4)); item('WHITE LEATHER', 'C', 950, C, couch(WHITEW, STEEL, True, grain(WHITEW, 8, 6)), 1, col=(28, 28, 29))
item('FUTON', 'C', 220, C, futon(GREEN, PINE), 1, col=(6, 20, 8)); item('CHAISE LONGUE', 'C', 700, C, chaise(PURP, P_GOLD), 1, col=(16, 6, 24))
item('FLORAL COUCH', 'C', 500, C, couch(MINT, OAK, True, dots(MINT, 8, 6, 'c', 'e', 2)), 1, col=(14, 28, 20)); item('VELVET SOFA', 'C', 1200, C, couch(PURP, P_GOLD, True, stripes(PURP, 8, 6, 'c', 'd', True, 2)), 1, col=(16, 6, 24))
C = cat('BEDS')
item('SINGLE BED BLUE', 'S', 300, C, bed(BLUE, OAK, pillows=1), 1, col=(6, 10, 26)); item('SINGLE BED RED', 'S', 300, C, bed(RED, OAK, pillows=1), 1, col=(26, 5, 5))
item('SINGLE BED GREEN', 'S', 300, C, bed(GREEN, PINE, pillows=1), 1, col=(6, 20, 8)); item('DOUBLE BED', 'S', 600, C, bed(PURP, DARK), 1, col=(16, 6, 24))
item('KING BED', 'S', 1100, C, bed(WHITEW, DARK), 1, col=(28, 28, 29)); item('BUNK BED', 'S', 500, C, bunk(RED, PINE), 1, col=(26, 20, 11))
item('CANOPY BED', 'S', 1800, C, canopy(P_PINK, WHITEW), 1, col=(30, 14, 22)); item('RACECAR BED', 'S', 700, C, racecar(RED), 1, col=(26, 5, 5))
item('CAMP COT', 'S', 80, C, bed(GREEN, STEEL, headboard=False, pillows=1), 1, col=(6, 20, 8)); item('WATER BED', 'S', 900, C, bed(P_WATER, DARK), 1, col=(12, 20, 30))
C = cat('TABLES')
item('COFFEE TABLE', 't', 120, C, table(OAK), col=(22, 14, 6)); item('GLASS TABLE', 't', 260, C, table(P_GLASS, flat(STEEL, 'c'), glass=True), col=(20, 26, 30))
item('OAK DINING TABLE', 't', 300, C, table(OAK, tall=True), col=(22, 14, 6)); item('MARBLE TABLE', 't', 900, C, table(MARBLE, flat(P_GOLD, 'c'), tall=True), col=(27, 27, 28))
item('ROUND TABLE', 't', 200, C, table(PINE, round_=True, tall=True), col=(26, 20, 11)); item('SIDE TABLE', 't', 60, C, table(DARK), col=(9, 6, 4))
item('BAR TABLE', 't', 180, C, table(DARK, flat(STEEL, 'c'), round_=True, tall=True), col=(20, 22, 26)); item('CARD TABLE', 't', 150, C, table(GREEN, flat(DARK, 'b'), tall=True, cloth=dots(GREEN, 8, 8, 'c', 'b', 3)), col=(6, 20, 8))
item('WORKBENCH', 't', 220, C, table(PINE, flat(STEEL, 'c'), tall=True) + block(0, 0, 8, 1, 13, 17, faces(M(STEEL, ['cccccccc', 'cbcbcbcb', 'cccccccc']), flat(STEEL, 'c'), flat(STEEL, 'd'))), col=(26, 20, 11))
item('PING PONG TABLE', 't', 400, C, table(GREEN, flat(STEEL, 'c'), tall=True, cloth=M(GREEN, ['ecccccce', 'eccccccce'[:8], 'eeeeeeee', 'eccccccce'[:8], 'ecccccce', 'ecccccce', 'eeeeeeee', 'ecccccce'])) + block(0, 3, 8, 4, 13, 15, solid(WHITEW, 'c')), col=(6, 20, 8))
item('POOL TABLE', 't', 1500, C, table(GREEN, flat(DARK, 'b'), tall=True, cloth=M(GREEN, ['bbbbbbbb', 'bccccccb', 'bcdcccbb', 'bccccccb', 'bcccdccb', 'bccbcccb', 'bccccccb', 'bbbbbbbb'])), col=(6, 20, 8))
item('KIDS TABLE', 't', 50, C, table(YELLOW, flat(RED, 'c')), col=(30, 26, 4))
C = cat('DESKS')
item('OAK DESK', 'd', 350, C, desk(OAK), 1, col=(22, 14, 6)); item('WHITE DESK', 'd', 380, C, desk(WHITEW), 1, col=(28, 28, 29))
item('COMPUTER DESK', 'd', 700, C, desk(DARK, True, monitor(9)), 1, col=(9, 6, 4)); item('DRAFTING TABLE', 'd', 420, C, desk(PINE, False) + block(1, 1, 7, 2, 9, 15, faces(panel(WHITEW, 6, 6, 'b', 'c'), flat(WHITEW, 'b'), flat(WHITEW, 'c'))), 1, col=(26, 20, 11))
item('VANITY', 'd', 500, C, desk(WHITEW, True) + block(1, 1, 7, 2, 9, 17, faces(panel(P_GLASS, 6, 8, 'a', 'd'), flat(WHITEW, 'b'), flat(WHITEW, 'c'))), 1, col=(28, 28, 29))
item('LECTERN', 'd', 200, C, [Box(2, 2, 6, 6, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 11, solid(DARK, 'c')), Box(1, 1, 7, 7, 11, 13, faces(flat(DARK, 'b'), flat(DARK, 'c'), grain(DARK, 8, 8)))], 1, col=(9, 6, 4))
item('RECEPTION DESK', 'd', 800, C, block(0, 1, 8, 8, 0, 10, faces(panel(TEAL, 8, 10, 'b', 'c'), flat(TEAL, 'c'), grain(OAK, 8, 8))) + block(0, 0, 8, 1, 0, 13, faces(flat(TEAL, 'd'), flat(TEAL, 'c'), flat(OAK, 'd'))), 1, col=(5, 20, 20))
item('LAB BENCH', 'd', 650, C, desk(WHITEW, True, [Box(2, 3, 4, 5, 9, 13, faces(M(P_GLASS, ['bccb', 'bddb', 'bccb', 'bbbb']), flat(P_GLASS, 'c'), flat(P_GLASS, 'd'))), Box(5, 3, 6, 5, 9, 11, solid(GREEN, 'd'))]), 1, col=(28, 28, 29))
item('DJ BOOTH', 'd', 1200, C, desk(DARK, False) + block(1, 2, 3, 6, 9, 10, faces(flat(DARK, 'c'), flat(DARK, 'c'), M(STEEL, ['cccc', 'cbbc', 'cbbc', 'cccc']))) + block(5, 2, 7, 6, 9, 10, faces(flat(DARK, 'c'), flat(DARK, 'c'), M(STEEL, ['cccc', 'cbbc', 'cbbc', 'cccc']))) + block(3, 3, 5, 5, 9, 11, faces(flat(NEON, 'b'), flat(NEON, 'b'), dots(NEON, 2, 2, 'b', 'd', 2))), 1, col=(31, 8, 24))
item('TEACHER DESK', 'd', 300, C, desk(PINE, True, [Box(2, 3, 4, 4, 9, 10, solid(RED, 'c'))]), 1, col=(26, 20, 11))
C = cat('STORAGE')
item('TALL BOOKSHELF', 'b', 400, C, shelf(OAK, 5, 22), 0, col=(22, 14, 6)); item('WHITE BOOKSHELF', 'b', 380, C, shelf(WHITEW, 4, 18), 0, col=(28, 28, 29))
item('COMIC RACK', 'b', 150, C, shelf(RED, 3, 12), 0, col=(26, 5, 5)); item('OAK WARDROBE', 'i', 600, C, wardrobe(OAK), 1, col=(22, 14, 6))
item('WHITE WARDROBE', 'i', 650, C, wardrobe(WHITEW), 1, col=(28, 28, 29)); item('PINE DRESSER', 'i', 300, C, dresser(PINE), 1, col=(26, 20, 11))
item('BLACK DRESSER', 'i', 340, C, dresser(DARK), 1, col=(9, 6, 4)); item('CABINET', 'i', 260, C, wardrobe(GREY, 14), 1, col=(15, 15, 17))
item('LOCKER', 'i', 180, C, block(1, 1, 7, 7, 0, 22, faces(M(STEEL, ['bbbbbb', 'bccccb', 'bcbbcb', 'bccccb', 'bccdcb', 'bccccb', 'bbbbbb']), flat(STEEL, 'c'), flat(STEEL, 'd'))), 1, col=(20, 22, 26))
item('SAFE', 'i', 1000, C, block(1, 1, 7, 7, 0, 10, faces(M(DARK, ['bbbbbb', 'bccccb', 'bcdccb', 'bcccdb', 'bccccb', 'bbbbbb']), flat(DARK, 'c'), flat(DARK, 'd'))) + [Box(3, 0, 5, 1, 4, 6, solid(STEEL, 'd'))], 1, col=(9, 6, 4))
item('TRUNK', 'i', 140, C, block(0, 1, 8, 7, 0, 7, faces(stripes(BROWN, 8, 7, 'c', 'b', False, 4), flat(BROWN, 'c'), M(BROWN, ['bcccbccc' + '', 'bcccbccc', 'bdddbddd', 'bcccbccc', 'bcccbccc', 'bcccbccc']))), 0, col=(16, 9, 4))
item('WINE RACK', 'b', 320, C, shelf(DARK, 4, 16, False) + [Box(2, 2, 6, 7, 2 + r * 3, 4 + r * 3, faces(M(GREEN, ['abba', 'abba']), flat(GREEN, 'b'), flat(GREEN, 'b'))) for r in range(4)], 0, col=(9, 6, 4))
C = cat('LIGHTS')
item('BRASS LAMP', 'l', 150, C, lamp(P_GOLD, MUST), col=(28, 22, 6)); item('BLACK LAMP', 'l', 140, C, lamp(DARK, WHITEW), col=(9, 6, 4))
item('ARC LAMP', 'l', 400, C, [Box(2, 2, 6, 6, 0, 1, solid(STEEL, 'c')), Box(3, 6, 5, 7, 1, 20, solid(STEEL, 'c')), Box(3, 2, 5, 7, 19, 20, solid(STEEL, 'c')), Box(2, 1, 6, 4, 15, 20, faces(flat(STEEL, 'c'), flat(STEEL, 'c'), flat(GLOW, 'd')))], col=(20, 22, 26))
item('TABLE LAMP', 'l', 90, C, tablelamp(DARK, RED), col=(26, 5, 5)); item('PINK LAVA LAMP', 'l', 120, C, lava(DARK, P_PINK), col=(30, 14, 22))
item('GREEN LAVA LAMP', 'l', 120, C, lava(DARK, GREEN), col=(6, 20, 8)); item('NEON BORE SIGN', 'l', 500, C, [Box(2, 3, 6, 5, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 8, solid(DARK, 'c')), Box(0, 3, 8, 4, 8, 16, faces(M(NEON, ['aaaaaaaa', 'abbabbaa', 'abababaa', 'abbabbaa', 'abababaa', 'abbabbaa', 'aaaaaaaa', 'acccccca']), flat(NEON, 'a'), flat(NEON, 'a')))], col=(31, 8, 24))
item('NEON ARROW', 'l', 420, C, [Box(2, 3, 6, 5, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 8, solid(DARK, 'c')), Box(0, 3, 8, 4, 8, 16, faces(M(NEON, ['aaadaaaa', 'aaddaaaa', 'addddddd', 'dddddddd', 'addddddd', 'aaddaaaa', 'aaadaaaa', 'aaaaaaaa']), flat(NEON, 'a'), flat(NEON, 'a')))], col=(31, 20, 30))
item('CHANDELIER', 'l', 1600, C, [Box(2, 2, 6, 6, 0, 1, solid(P_GOLD, 'b')), Box(3, 3, 5, 5, 1, 16, solid(P_GOLD, 'c')), Box(0, 0, 8, 8, 16, 19, faces(M(P_GOLD, ['cdcdcdcd', 'dededede', 'cdcdcdcd']), flat(P_GOLD, 'd'), flat(P_GOLD, 'e'))), Box(2, 2, 6, 6, 19, 22, faces(flat(P_GLASS, 'd'), flat(P_GLASS, 'd'), flat(P_GLASS, 'e')))], col=(28, 22, 6))
item('CANDELABRA', 'l', 260, C, [Box(3, 3, 5, 5, 0, 1, solid(P_GOLD, 'b')), Box(3, 3, 5, 5, 1, 10, solid(P_GOLD, 'c')), Box(1, 3, 7, 5, 10, 11, solid(P_GOLD, 'c')), Box(1, 3, 2, 5, 11, 15, solid(WHITEW, 'c')), Box(4, 3, 5, 5, 11, 16, solid(WHITEW, 'c')), Box(6, 3, 7, 5, 11, 15, solid(WHITEW, 'c')), Box(1, 3, 2, 5, 15, 16, solid(GLOW, 'd')), Box(4, 3, 5, 5, 16, 17, solid(GLOW, 'd')), Box(6, 3, 7, 5, 15, 16, solid(GLOW, 'd'))], col=(28, 22, 6))
item('TIKI TORCH', 'l', 60, C, [Box(3, 3, 5, 5, 0, 14, faces(stripes(PINE, 2, 8, 'c', 'b', True, 2), stripes(PINE, 2, 8, 'c', 'b', True, 2), flat(PINE, 'c'))), Box(2, 2, 6, 6, 14, 18, faces(stripes(BROWN, 4, 4, 'c', 'b', False, 1), flat(BROWN, 'c'), flat(BROWN, 'c'))), Box(3, 3, 5, 5, 18, 22, faces(M(ORANGE, ['cd', 'dc', 'ce', 'ec']), flat(ORANGE, 'd'), flat(YELLOW, 'd')))], col=(28, 14, 3))
item('STREET LAMP', 'l', 350, C, [Box(2, 2, 6, 6, 0, 2, solid(DARK, 'b')), Box(3, 3, 5, 5, 2, 24, solid(DARK, 'c')), Box(2, 2, 6, 6, 24, 28, faces(M(GLOW, ['bddb', 'dccd', 'dccd', 'bddb']), flat(GLOW, 'c'), flat(DARK, 'c')))], col=(9, 6, 4))
C = cat('PLANTS')
item('POTTED PALM', 'p', 180, C, plant(LEAF, TERRA, LEAF, 20, 4, 2), col=(8, 22, 7)); item('FERN', 'p', 60, C, plant(LEAF2, GREY, LEAF2, 11, 3, 2), col=(14, 24, 8))
item('TALL CACTUS', 'p', 90, C, cactus(LEAF, TERRA, True), col=(8, 22, 7)); item('ROUND CACTUS', 'p', 70, C, cactus(LEAF, TERRA, False), col=(8, 22, 7))
item('BONSAI', 'p', 400, C, plant(LEAF, DARK, LEAF, 10, 2, 1), col=(8, 22, 7)); item('SUNFLOWER', 'p', 50, C, [Box(2, 2, 6, 6, 0, 3, solid(TERRA, 'c')), Box(3, 3, 5, 5, 3, 16, solid(LEAF, 'c')), Box(2, 3, 6, 4, 16, 20, faces(M(YELLOW, ['bddb', 'dccd', 'dccd', 'bddb']), flat(YELLOW, 'd'), flat(YELLOW, 'd')))], col=(30, 26, 4))
item('ROSE BUSH', 'p', 120, C, plant(LEAF, TERRA, LEAF, 12, 3, 2) + [Box(2, 2, 3, 3, 10, 11, solid(RED, 'd')), Box(5, 3, 6, 4, 11, 12, solid(RED, 'd')), Box(3, 5, 4, 6, 9, 10, solid(RED, 'd'))], col=(26, 5, 5))
item('HEDGE', 'p', 80, C, block(0, 0, 8, 8, 0, 10, faces(dots(LEAF, 8, 8, 'c', 'b', 2), dots(LEAF, 8, 8, 'c', 'b', 2), dots(LEAF, 8, 8, 'c', 'e', 2))), col=(8, 22, 7))
item('TOPIARY BALL', 'p', 200, C, [Box(2, 2, 6, 6, 0, 3, solid(STONE, 'c')), Box(3, 3, 5, 5, 3, 8, solid(P_WOOD, 'b')), Box(1, 1, 7, 7, 8, 14, faces(dots(LEAF, 6, 6, 'c', 'b', 2), dots(LEAF, 6, 6, 'c', 'b', 2), dots(LEAF, 6, 6, 'c', 'e', 2))), Box(2, 2, 6, 6, 14, 16, solid(LEAF, 'c'))], col=(8, 22, 7))
item('FICUS', 'p', 150, C, plant(LEAF2, WHITEW, LEAF2, 18, 4, 1), col=(14, 24, 8)); item('MONSTERA', 'p', 220, C, plant(LEAF, DARK, LEAF, 15, 4, 2), col=(8, 22, 7))
item('VENUS FLYTRAP', 'p', 140, C, plant(LEAF2, RED, LEAF2, 9, 3, 1) + [Box(2, 2, 4, 4, 9, 11, solid(P_PINK, 'c')), Box(4, 4, 6, 6, 9, 11, solid(P_PINK, 'c'))], col=(30, 14, 22))
C = cat('KITCHEN')
item('RED FRIDGE', 'F', 900, C, appliance(RED, 16, M(RED, ['bbbbbbbb', 'bccccccb', 'bccccdcb', 'bbbbbbbb', 'bccccccb', 'bccccdcb', 'bccccccb', 'bbbbbbbb'])), col=(26, 5, 5))
item('MINT FRIDGE', 'F', 950, C, appliance(MINT, 16, M(MINT, ['bbbbbbbb', 'bccccccb', 'bccccdcb', 'bbbbbbbb', 'bccccccb', 'bccccdcb', 'bccccccb', 'bbbbbbbb'])), col=(14, 28, 20))
item('STEEL FRIDGE', 'F', 1400, C, appliance(STEEL, 18, M(STEEL, ['bbbbbbbb', 'bcccbcccb'[:8], 'bcdcbcdcb'[:8], 'bcccbcccb'[:8], 'bbbbbbbb', 'bccccccb', 'bccccdcb', 'bbbbbbbb'])), col=(20, 22, 26))
item('VENDING MACHINE', 'F', 1200, C, appliance(RED, 18, M(RED, ['bbbbbbbb', 'baaaaacb', 'badedacb', 'baeeeacb', 'badedacb', 'baaaaacb', 'bccccccb', 'bbbbbbbb'])), col=(26, 5, 5))
item('STOVE', 'y', 700, C, stove(WHITEW), col=(28, 28, 29)); item('OVEN', 'y', 650, C, appliance(DARK, 10, M(DARK, ['bbbbbbbb', 'bcdcdccb', 'bbbbbbbb', 'baaaaaab', 'baaaaaab', 'bbbbbbbb'])), col=(9, 6, 4))
item('DISHWASHER', 'y', 600, C, appliance(STEEL, 10, panel(STEEL, 8, 10, 'b', 'c')), col=(20, 22, 26)); item('KITCHEN SINK', 'y', 350, C, counter(WHITEW, M(STEEL, ['cccccccc', 'cbbbbbbc', 'cbaaaabc', 'cbaaaabc', 'cbbbbbbc', 'cccccccc', 'cccddccc', 'cccccccc'])) + [Box(3, 1, 5, 2, 12, 16, solid(STEEL, 'd'))], col=(20, 22, 26))
item('OAK COUNTER', 'y', 280, C, counter(OAK, grain(OAK, 8, 8)), col=(22, 14, 6)); item('MARBLE COUNTER', 'y', 600, C, counter(WHITEW, M(MARBLE, ['cccccccc', 'ccdccccc', 'cccccdcc', 'cdcccccc', 'cccccccc', 'cccdcccc', 'cccccccd', 'cccccccc'])), col=(27, 27, 28))
item('MICROWAVE STAND', 'y', 200, C, table(GREY, flat(STEEL, 'c'), tall=True) + block(1, 2, 7, 6, 13, 17, faces(M(DARK, ['bbbbbb', 'baaacb', 'baaadb', 'bbbbbb']), flat(DARK, 'c'), flat(DARK, 'c'))), col=(15, 15, 17))
item('KETTLE STAND', 'c', 90, C, table(PINE) + [Box(3, 3, 5, 5, 8, 12, faces(flat(STEEL, 'c'), flat(STEEL, 'c'), flat(STEEL, 'd'))), Box(5, 3, 6, 5, 9, 11, solid(DARK, 'c'))], col=(20, 22, 26))
C = cat('BATH')
item('PINK TOILET', 'T', 300, C, toilet(P_PINK), 1, col=(30, 14, 22)); item('BLACK TOILET', 'T', 350, C, toilet(DARK), 1, col=(9, 6, 4))
item('GOLD TOILET', 'T', 5000, C, toilet(P_GOLD), 1, col=(28, 22, 6)); item('GLASS SHOWER', 'H', 800, C, shower(WHITEW, P_GLASS), 1, col=(20, 26, 30))
item('BLUE SHOWER', 'H', 650, C, shower(SKY, P_GLASS), 1, col=(12, 22, 31)); item('BATHTUB', 'H', 700, C, tub(WHITEW), 0, col=(28, 28, 29))
item('CLAWFOOT TUB', 'H', 1500, C, tub(WHITEW, True), 0, col=(28, 22, 6)); item('BATHROOM SINK', 'H', 250, C, [Box(3, 3, 5, 5, 0, 8, solid(WHITEW, 'c')), Box(1, 2, 7, 7, 8, 11, faces(flat(WHITEW, 'c'), flat(WHITEW, 'c'), M(P_WATER, ['abbbbbba', 'bccccccb', 'bccccccb', 'bccccccb', 'abbbbbba']))), Box(3, 1, 5, 2, 11, 15, solid(STEEL, 'd'))], 1, col=(28, 28, 29))
item('HOT TUB', 'H', 3000, C, block(0, 0, 8, 8, 0, 7, faces(stripes(PINE, 8, 7, 'c', 'b', False, 2), stripes(PINE, 8, 7, 'c', 'b', False, 2), M(P_WATER, ['bbbbbbbb', 'bcdccdcb', 'bccdcccb', 'bdccccdb', 'bccdcdcb', 'bcccdccb', 'bdcccccb', 'bbbbbbbb']))), 0, col=(12, 20, 30))
item('URINAL', 'T', 200, C, [Box(3, 1, 5, 2, 0, 12, solid(WHITEW, 'b')), Box(2, 1, 6, 4, 6, 12, faces(flat(WHITEW, 'c'), flat(WHITEW, 'c'), M(WHITEW, ['cccc', 'cbbc', 'cbbc', 'cccc']))), Box(3, 1, 5, 2, 12, 15, solid(STEEL, 'c'))], 1, col=(28, 28, 29))
C = cat('TECH')
item('SMALL TV', 'v', 300, C, tv(DARK, 4, 4), 1, col=(9, 6, 4)); item('BIG TV', 'v', 800, C, tv(GREY, 6, 6), 1, col=(15, 15, 17))
item('FLAT TV', 'v', 1500, C, [Box(2, 3, 6, 4, 0, 1, solid(STEEL, 'c')), Box(3, 3, 5, 4, 1, 3, solid(STEEL, 'c'))] + tv(DARK, 8, 8, 3, False, M(P_SCREEN, ['aaaaaaaa', 'abbbbcca', 'abbbccca', 'adbbccca', 'abbbbbba', 'abcbbbba', 'abbbbbba', 'aaaaaaaa'])), 1, col=(20, 22, 26))
item('COMPUTER', 'd', 900, C, desk(GREY, False, monitor(9) + [Box(5, 4, 7, 6, 9, 10, faces(flat(GREY, 'b'), flat(GREY, 'b'), dots(GREY, 2, 2, 'c', 'a', 2)))]), 1, col=(15, 15, 17))
item('CONSOLE AND TV', 'v', 1100, C, tv(DARK, 6, 5) + [Box(2, 5, 6, 7, 5, 6, faces(flat(DARK, 'b'), flat(DARK, 'b'), M(DARK, ['ccbc', 'cccc'])))], 1, col=(9, 6, 4))
item('JUKEBOX', 'R', 1800, C, jukebox(RED), 0, col=(26, 5, 5)); item('BIG SPEAKERS', 'A', 1300, C, speakers(DARK), 0, col=(9, 6, 4))
item('TURNTABLE', 'R', 500, C, table(DARK) + [Box(1, 1, 7, 7, 8, 10, faces(flat(STEEL, 'c'), flat(STEEL, 'c'), M(DARK, ['cccccccc', 'cbbbbbbc', 'cbcccccc'[:8], 'cbcdcccc', 'cbcccccc'[:8], 'cbbbbbbc', 'cccccccc', 'cccccccc'])))], 0, col=(20, 22, 26))
item('SYNTHESIZER', 'R', 700, C, [Box(1, 1, 7, 7, 0, 9, solid(STEEL, 'b')), Box(0, 1, 8, 7, 9, 11, faces(flat(DARK, 'c'), flat(DARK, 'c'), M(WHITEW, ['acacacac', 'acacacac', 'cccccccc', 'cccccccc', 'bbbbbbbb', 'bdbdbdbd'])))], 0, col=(9, 6, 4))
item('ARCADE CABINET', 'v', 2000, C, arcade(BLUE), 1, col=(6, 10, 26)); item('PINBALL', 'v', 1800, C, [Box(1, 1, 7, 7, 0, 8, solid(DARK, 'b')), Box(0, 0, 8, 8, 8, 12, faces(flat(RED, 'c'), flat(RED, 'c'), M(YELLOW, ['cbcbcbcb', 'bcdcdcbc', 'cbcbcbcb', 'bcdcdcbc', 'cbcbcbcb', 'bdcbcdbc', 'cbcbcbcb', 'bbbbbbbb']))), Box(0, 0, 8, 1, 12, 22, faces(M(NEON, ['aaaaaaaa', 'abbbbbba', 'abcbcbba', 'abbbbbba', 'aaaaaaaa']), flat(DARK, 'c'), flat(DARK, 'c')))], 1, col=(26, 5, 5))
item('ROBOT BUDDY', 'v', 2500, C, [Box(2, 2, 6, 6, 0, 2, solid(STEEL, 'b')), Box(2, 2, 6, 6, 2, 10, faces(M(STEEL, ['cccc', 'cdbc', 'cccc', 'cbbc', 'cccc']), flat(STEEL, 'c'), flat(STEEL, 'c'))), Box(2, 2, 6, 6, 10, 14, faces(M(STEEL, ['cccc', 'cfcf'.replace('f', 'e'), 'cccc', 'cbbc']), flat(STEEL, 'c'), flat(STEEL, 'd'))), Box(3, 3, 5, 5, 14, 16, solid(RED, 'd'))], 0, col=(20, 22, 26))
C = cat('ART')
item('SUNSET PAINTING', 'l', 300, C, easel(PINE, M(SKY, ['aabbbbaa', 'bbccccbb', 'ccddddcc', 'ddeeeedd', 'eeeeeeee', 'ccccddcc', 'bbbbbbbb', 'aaaaaaaa'])), col=(12, 22, 31))
item('PORTRAIT', 'l', 400, C, easel(DARK, M(TERRA, ['bbbbbbbb', 'bbbddbbb', 'bbdccdbb', 'bbdccdbb', 'bbbddbbb', 'bbddddbb', 'bddddddb', 'bbbbbbbb'])), col=(22, 11, 5))
item('ABSTRACT ART', 'l', 900, C, easel(DARK, M(P_BOOKS, ['aabbccdd', 'aabbccdd', 'eeffgg' + 'aa', 'eeffggaa', 'ddccbbaa', 'ddccbbaa', 'ggffeedd', 'ggffeedd'])), col=(26, 12, 28))
item('BORE LOGO ART', 'l', 1500, C, easel(P_GOLD, M(P_GOLD, ['aaaaaaaa', 'addadada', 'adadaddd', 'addadadd', 'adadaddd', 'addadada', 'aaaaaaaa', 'aeeeeeea'])), col=(28, 22, 6))
item('MARBLE BUST', 'l', 700, C, statue(MARBLE, DARK, 15), col=(27, 27, 28)); item('VENUS STATUE', 'l', 1800, C, statue(MARBLE, MARBLE, 20), col=(27, 27, 28))
item('ABSTRACT STATUE', 'l', 1200, C, [Box(1, 1, 7, 7, 0, 3, solid(DARK, 'b')), Box(2, 3, 4, 5, 3, 14, solid(STEEL, 'c')), Box(4, 3, 6, 5, 7, 18, solid(STEEL, 'd')), Box(2, 3, 6, 5, 14, 16, solid(RED, 'c'))], col=(20, 22, 26))
item('TROPHY CASE', 'l', 800, C, shelf(DARK, 3, 16, False) + [Box(2 + i * 2, 3, 3 + i * 2, 5, 3 + r * 5, 6 + r * 5, solid(P_GOLD, 'd')) for r in range(2) for i in range(2)], col=(28, 22, 6))
item('GLOBE', 'l', 350, C, [Box(2, 2, 6, 6, 0, 1, solid(OAK, 'b')), Box(3, 3, 5, 5, 1, 7, solid(OAK, 'c')), Box(1, 1, 7, 7, 7, 13, faces(M(BLUE, ['cccddccc', 'cdddccdc', 'ccdddccc', 'cccccdcc', 'cdcccddc', 'cccccccc']), flat(BLUE, 'c'), flat(BLUE, 'd')))], col=(6, 10, 26))
item('TALL VASE', 'l', 250, C, vase(TERRA, 16), col=(22, 11, 5)); item('BLUE VASE', 'l', 300, C, vase(BLUE, 12), col=(6, 10, 26))
item('TOTEM POLE', 'l', 1000, C, [Box(2, 2, 6, 6, 0, 7, faces(M(RED, ['cccc', 'cbbc', 'cddc', 'cccc', 'cbbc', 'cccc']), flat(RED, 'c'), flat(RED, 'c'))), Box(2, 2, 6, 6, 7, 14, faces(M(YELLOW, ['cccc', 'cbbc', 'cddc', 'cccc', 'cbbc', 'cccc']), flat(YELLOW, 'c'), flat(YELLOW, 'c'))), Box(2, 2, 6, 6, 14, 21, faces(M(TEAL, ['cccc', 'cbbc', 'cddc', 'cccc', 'cbbc', 'cccc']), flat(TEAL, 'c'), flat(TEAL, 'c'))), Box(0, 3, 8, 5, 19, 21, solid(TEAL, 'd'))], col=(26, 5, 5))
C = cat('OUTDOOR')
item('BBQ GRILL', 'y', 400, C, legs(1, 1, 7, 7, 8, flat(DARK, 'b')) + block(1, 1, 7, 7, 8, 12, faces(flat(DARK, 'c'), flat(DARK, 'c'), M(STEEL, ['bcbcbcbc'] * 8))) + [Box(1, 1, 7, 4, 12, 16, faces(flat(DARK, 'c'), flat(DARK, 'c'), flat(DARK, 'd')))], col=(9, 6, 4))
item('MAILBOX', 'Y', 60, C, [Box(3, 3, 5, 5, 0, 10, solid(P_WOOD, 'b')), Box(2, 2, 6, 7, 10, 14, faces(M(BLUE, ['cccc', 'cccc', 'cbbc', 'cccc']), flat(BLUE, 'c'), flat(BLUE, 'd'))), Box(6, 2, 7, 3, 12, 16, solid(RED, 'd'))], col=(6, 10, 26))
item('FOUNTAIN', 'q', 1600, C, block(0, 0, 8, 8, 0, 4, faces(stripes(STONE, 8, 4, 'c', 'b', True, 2), stripes(STONE, 8, 4, 'c', 'b', True, 2), M(P_WATER, ['bbbbbbbb', 'bcccccc b'.replace(' ', ''), 'bcdcccdb'[:8], 'bccccccb', 'bccccccb', 'bcdcccdb'[:8], 'bccccccb', 'bbbbbbbb']))) + [Box(3, 3, 5, 5, 4, 9, solid(STONE, 'c')), Box(2, 2, 6, 6, 9, 11, faces(flat(STONE, 'c'), flat(STONE, 'c'), flat(P_WATER, 'd'))), Box(3, 3, 5, 5, 11, 14, solid(P_WATER, 'e'))], col=(12, 20, 30))
item('FIRE PIT', 'o', 300, C, block(1, 1, 7, 7, 0, 3, faces(stripes(STONE, 6, 3, 'c', 'b', True, 1), stripes(STONE, 6, 3, 'c', 'b', True, 1), M(STONE, ['cccccc', 'cbbbbc', 'cbaabc', 'cbaabc', 'cbbbbc', 'cccccc']))) + [Box(3, 3, 5, 5, 3, 7, faces(M(ORANGE, ['cd', 'ec']), flat(ORANGE, 'd'), flat(YELLOW, 'd')))], col=(28, 14, 3))
item('LAMP POST', 'l', 300, C, [Box(2, 2, 6, 6, 0, 2, solid(GREEN, 'a')), Box(3, 3, 5, 5, 2, 23, solid(GREEN, 'b')), Box(2, 2, 6, 6, 23, 26, faces(M(GLOW, ['bddb', 'dccd', 'dccd', 'bddb']), flat(GLOW, 'c'), flat(GREEN, 'b'))), Box(3, 3, 5, 5, 26, 28, solid(GREEN, 'b'))], col=(6, 20, 8))
item('FENCE POST', '#', 20, C, [Box(3, 3, 5, 5, 0, 10, solid(WHITEW, 'c')), Box(0, 3, 8, 4, 4, 5, solid(WHITEW, 'c')), Box(0, 3, 8, 4, 8, 9, solid(WHITEW, 'c')), Box(3, 3, 5, 5, 10, 11, solid(WHITEW, 'd'))], col=(28, 28, 29))
item('HYDRANT', 'Y', 120, C, [Box(2, 2, 6, 6, 0, 1, solid(RED, 'b')), Box(3, 3, 5, 5, 1, 9, solid(RED, 'c')), Box(1, 3, 7, 5, 5, 7, solid(RED, 'd')), Box(3, 3, 5, 5, 9, 11, solid(RED, 'd'))], col=(26, 5, 5))
item('BIRD BATH', 'p', 200, C, [Box(3, 3, 5, 5, 0, 8, solid(STONE, 'c')), Box(1, 1, 7, 7, 8, 10, faces(flat(STONE, 'c'), flat(STONE, 'c'), M(P_WATER, ['abbbbbba', 'bccccccb', 'bccdcccb', 'bccccccb', 'bccccccb', 'bcdccccb', 'bccccccb', 'abbbbbba']))), Box(5, 5, 6, 6, 10, 12, solid(BLUE, 'd'))], col=(17, 17, 16))
item('GARDEN GNOME', 'p', 90, C, [Box(3, 3, 5, 5, 0, 4, solid(BLUE, 'c')), Box(3, 3, 5, 5, 4, 7, solid(TERRA, 'd')), Box(3, 3, 5, 5, 5, 6, solid(WHITEW, 'c')), Box(3, 3, 5, 5, 7, 12, faces(M(RED, ['cc', 'cd', 'dd']), flat(RED, 'c'), flat(RED, 'd')))], col=(26, 5, 5))
item('PINK FLAMINGO', 'p', 40, C, [Box(3, 3, 4, 4, 0, 8, solid(STEEL, 'c')), Box(4, 4, 5, 5, 0, 8, solid(STEEL, 'c')), Box(2, 2, 6, 6, 8, 12, faces(flat(P_PINK, 'c'), flat(P_PINK, 'c'), flat(P_PINK, 'd'))), Box(5, 3, 6, 4, 12, 17, solid(P_PINK, 'c')), Box(5, 2, 7, 4, 17, 19, solid(P_PINK, 'd')), Box(7, 3, 8, 4, 17, 18, solid(DARK, 'c'))], col=(30, 14, 22))
item('PARK BENCH', 'N', 150, C, [Box(0, 2, 1, 6, 0, 7, solid(DARK, 'b')), Box(7, 2, 8, 6, 0, 7, solid(DARK, 'b')), Box(0, 2, 8, 6, 7, 9, faces(flat(PINE, 'b'), flat(PINE, 'b'), stripes(PINE, 8, 4, 'c', 'b', True, 1))), Box(0, 1, 8, 2, 7, 15, faces(stripes(PINE, 8, 8, 'c', 'b', True, 2), flat(PINE, 'b'), flat(PINE, 'd')))], 0, col=(26, 20, 11))
item('TENT', 'S', 200, C, [Box(0, 0, 8, 8, 0, 1, solid(GREEN, 'a')), Box(1, 0, 7, 8, 1, 5, faces(M(GREEN, ['cccccc', 'cbaabc', 'cbaabc', 'cccccc']), flat(GREEN, 'c'), flat(GREEN, 'c'))), Box(2, 0, 6, 8, 5, 9, faces(flat(GREEN, 'c'), flat(GREEN, 'c'), flat(GREEN, 'd'))), Box(3, 0, 5, 8, 9, 12, faces(flat(GREEN, 'd'), flat(GREEN, 'd'), flat(GREEN, 'e')))], 0, col=(6, 20, 8))
C = cat('KIDS')
item('TOY BOX', 'b', 120, C, block(0, 1, 8, 7, 0, 6, faces(M(RED, ['bbbbbbbb', 'bcdcdcdb', 'bdcdcdcb', 'bcdcdcdb', 'bbbbbbbb']), flat(YELLOW, 'c'), M(BLUE, ['cccccccc', 'cbbccbbc', 'cccccccc', 'cccccccc', 'cbbccbbc', 'cccccccc']))), 0, col=(26, 5, 5))
item('TEDDY BEAR', 'p', 60, C, [Box(2, 2, 6, 6, 0, 5, solid(BROWN, 'c')), Box(1, 2, 2, 4, 0, 3, solid(BROWN, 'c')), Box(6, 2, 7, 4, 0, 3, solid(BROWN, 'c')), Box(2, 2, 6, 6, 5, 9, faces(M(BROWN, ['cccc', 'cbbc', 'cccc', 'cdcc']), flat(BROWN, 'c'), flat(BROWN, 'd'))), Box(2, 2, 3, 3, 9, 10, solid(BROWN, 'c')), Box(5, 2, 6, 3, 9, 10, solid(BROWN, 'c'))], col=(16, 9, 4))
item('ROCKING HORSE', 'h', 150, C, [Box(0, 2, 8, 3, 0, 1, solid(PINE, 'b')), Box(0, 5, 8, 6, 0, 1, solid(PINE, 'b')), Box(2, 2, 3, 6, 1, 6, solid(PINE, 'c')), Box(5, 2, 6, 6, 1, 6, solid(PINE, 'c')), Box(1, 3, 7, 5, 6, 9, faces(flat(WHITEW, 'c'), flat(WHITEW, 'c'), dots(WHITEW, 6, 2, 'c', 'b', 2))), Box(6, 3, 7, 5, 9, 13, faces(M(WHITEW, ['cc', 'cb', 'cc']), flat(WHITEW, 'c'), flat(WHITEW, 'd')))], 1, col=(26, 20, 11))
item('DOLLHOUSE', 'b', 200, C, block(1, 1, 7, 7, 0, 8, faces(M(P_PINK, ['cccccc', 'cbbcbb' + '', 'cbbcbb', 'cccccc', 'cbbcdc', 'cbbcdc']), flat(P_PINK, 'c'), flat(P_PINK, 'c'))) + [Box(1, 1, 7, 7, 8, 12, faces(M(RED, ['bccccb', 'cccccc']), flat(RED, 'c'), flat(RED, 'd')))], col=(30, 14, 22))
item('BOUNCY CASTLE', 'U', 900, C, block(0, 0, 8, 8, 0, 6, faces(stripes(RED, 8, 6, 'c', 'e', False, 2), stripes(RED, 8, 6, 'c', 'e', False, 2), dots(YELLOW, 8, 8, 'c', 'd', 2))) + legs(0, 0, 8, 8, 12, flat(BLUE, 'c'), 2, 6) + [Box(1, 1, 3, 3, 12, 14, solid(RED, 'd')), Box(5, 5, 7, 7, 12, 14, solid(RED, 'd'))], col=(26, 5, 5))
item('CRIB', 'S', 250, C, legs(0, 0, 8, 8, 10, flat(WHITEW, 'c')) + block(0, 0, 8, 8, 2, 4, faces(flat(WHITEW, 'c'), flat(WHITEW, 'c'), dots(SKY, 8, 8, 'c', 'd', 2))) + block(0, 0, 8, 1, 4, 10, faces(stripes(WHITEW, 8, 6, 'c', 'a', False, 2), flat(WHITEW, 'c'), flat(WHITEW, 'c'))) + block(0, 7, 8, 8, 4, 10, faces(stripes(WHITEW, 8, 6, 'c', 'a', False, 2), flat(WHITEW, 'c'), flat(WHITEW, 'c'))), 1, col=(28, 28, 29))
item('TOY ROBOT', 'v', 300, C, [Box(2, 2, 6, 6, 0, 1, solid(STEEL, 'b')), Box(2, 2, 6, 6, 1, 6, faces(M(YELLOW, ['cccc', 'cbdc', 'cccc', 'cddc']), flat(YELLOW, 'c'), flat(YELLOW, 'c'))), Box(2, 2, 6, 6, 6, 9, faces(M(RED, ['cccc', 'cbbc', 'cccc']), flat(RED, 'c'), flat(RED, 'd'))), Box(3, 3, 5, 5, 9, 11, solid(STEEL, 'c'))], col=(30, 26, 4))
item('PINATA', 'p', 80, C, [Box(3, 3, 5, 5, 0, 12, solid(STEEL, 'b')), Box(1, 1, 7, 7, 12, 17, faces(stripes(P_BOOKS, 6, 5, 'a', 'c', False, 1), stripes(P_BOOKS, 6, 5, 'b', 'd', False, 1), stripes(P_BOOKS, 6, 6, 'e', 'c', True, 1))), Box(6, 2, 8, 4, 15, 19, solid(YELLOW, 'd'))], col=(26, 22, 6))
item('BALL PIT', 'U', 500, C, block(0, 0, 8, 8, 0, 6, faces(stripes(BLUE, 8, 6, 'c', 'd', True, 2), stripes(BLUE, 8, 6, 'c', 'd', True, 2), M(P_BOOKS, ['abcdabcd', 'cdabcdab', 'bcdabcda', 'dabcdabc', 'abcdabcd', 'cdabcdab', 'bcdabcda', 'dabcdabc']))), col=(6, 10, 26))
item('BLOCKS TOWER', 'p', 40, C, [Box(2, 2, 6, 6, 0, 3, solid(RED, 'c')), Box(2, 2, 6, 6, 3, 6, solid(BLUE, 'c')), Box(3, 3, 5, 5, 6, 9, solid(YELLOW, 'c')), Box(3, 3, 5, 5, 9, 11, solid(GREEN, 'c'))], col=(30, 26, 4))
C = cat('SKATE 2')
item('TRAFFIC CONE', 'Y', 15, C, cone(ORANGE), col=(28, 14, 3)); item('GRIND BOX', 'L', 250, C, block(0, 0, 8, 8, 0, 9, faces(stripes(PINE, 8, 9, 'c', 'b', True, 3), stripes(PINE, 8, 9, 'c', 'b', True, 3), M(STEEL, ['cccccccc'] + ['cbbbbbbc'] * 6 + ['cccccccc']))), col=(26, 20, 11))
item('SHORT RAIL', '=', 120, C, [Box(2, 3, 6, 5, 0, 1, solid(STEEL, 'b')), Box(3, 3, 5, 5, 1, 7, solid(STEEL, 'c')), Box(1, 3, 7, 5, 7, 9, faces(flat(YELLOW, 'c'), flat(YELLOW, 'c'), flat(YELLOW, 'd')))], col=(30, 26, 4))
item('A FRAME', 'X', 600, C, [Box(0, 0, 8, 8, 0, 4, solid(STONE, 'c')), Box(1, 1, 7, 7, 4, 8, solid(STONE, 'b')), Box(2, 2, 6, 6, 8, 12, faces(flat(STONE, 'c'), flat(STONE, 'c'), flat(STEEL, 'd')))], col=(17, 17, 16))
item('BLUE OIL DRUM', 'O', 40, C, block(1, 1, 7, 7, 0, 13, faces(stripes(BLUE, 6, 13, 'c', 'b', True, 4), stripes(BLUE, 6, 13, 'c', 'b', True, 4), M(BLUE, ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb']))), col=(6, 10, 26))
item('RED BARRIER', 'J', 90, C, [Box(0, 2, 8, 6, 0, 4, solid(RED, 'c')), Box(1, 3, 7, 5, 4, 10, faces(stripes(RED, 6, 6, 'c', 'e', True, 2), flat(RED, 'c'), flat(RED, 'd')))], col=(26, 5, 5))
item('BOLLARD', 'Y', 50, C, [Box(2, 2, 6, 6, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 10, faces(stripes(DARK, 2, 9, 'c', 'e', True, 3), stripes(DARK, 2, 9, 'c', 'e', True, 3), flat(YELLOW, 'd')))], col=(9, 6, 4))
item('MANUAL PAD 2', 'M', 150, C, block(0, 0, 8, 8, 0, 3, faces(flat(STONE, 'b'), flat(STONE, 'b'), M(STONE, ['dddddddd', 'dccccccd', 'dcbcccbd', 'dccccccd', 'dccbcccd', 'dccccccd', 'dcbccbcd', 'dddddddd']))), col=(17, 17, 16))
item('HUBBA LEDGE', 'L', 300, C, block(0, 0, 8, 8, 0, 9, faces(stripes(STONE, 8, 9, 'c', 'b', True, 4), stripes(STONE, 8, 9, 'c', 'b', True, 4), M(STONE, ['dddddddd'] + ['dccccccd'] * 6 + ['dddddddd']))), col=(17, 17, 16))
item('BOARD RACK', 'b', 200, C, shelf(DARK, 2, 14, False) + [Box(2 + i * 2, 3, 3 + i * 2, 6, 2, 12, faces(M([RED, BLUE, GREEN][i], ['c', 'd', 'c', 'b']), flat([RED, BLUE, GREEN][i], 'c'), flat([RED, BLUE, GREEN][i], 'c'))) for i in range(3)], col=(9, 6, 4))
C = cat('GROWN-UPS', 1)
item('BONG', 'G', 150, C, [Box(2, 2, 6, 6, 0, 2, solid(P_GLASS, 'b')), Box(2, 2, 6, 6, 2, 6, faces(M(P_WATER, ['bccb', 'bddb', 'bccb', 'bbbb']), flat(P_WATER, 'c'), flat(P_GLASS, 'c'))), Box(3, 3, 5, 5, 6, 16, faces(flat(P_GLASS, 'c'), flat(P_GLASS, 'c'), flat(P_GLASS, 'd'))), Box(5, 3, 7, 4, 7, 9, solid(STEEL, 'c'))], col=(20, 26, 30))
item('HOOKAH', 'G', 300, C, [Box(2, 2, 6, 6, 0, 5, faces(M(PURP, ['cdcd', 'dcdc', 'cdcd', 'bbbb']), flat(PURP, 'c'), flat(PURP, 'd'))), Box(3, 3, 5, 5, 5, 14, solid(P_GOLD, 'c')), Box(2, 2, 6, 6, 14, 16, solid(P_GOLD, 'd')), Box(0, 3, 2, 4, 3, 4, solid(RED, 'c'))], col=(16, 6, 24))
item('BEER KEG', 'F', 200, C, block(1, 1, 7, 7, 0, 11, faces(stripes(STEEL, 6, 11, 'c', 'b', True, 5), stripes(STEEL, 6, 11, 'c', 'b', True, 5), M(STEEL, ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb']))) + [Box(3, 3, 5, 5, 11, 15, solid(STEEL, 'c'))], col=(20, 22, 26))
item('SLOT MACHINE', 'v', 1500, C, [Box(1, 1, 7, 7, 0, 10, faces(panel(RED, 6, 10, 'b', 'c'), flat(RED, 'c'), flat(RED, 'c'))), Box(1, 1, 7, 7, 10, 16, faces(M(WHITEW, ['bbbbbb', 'bcbcbc'.replace('c', 'd'), 'bdbdbd', 'bbbbbb', 'bcccc b'.replace(' ', '')]), flat(RED, 'c'), flat(RED, 'd'))), Box(7, 3, 8, 4, 11, 16, solid(STEEL, 'c')), Box(7, 3, 8, 4, 16, 17, solid(RED, 'd'))], 1, col=(26, 5, 5))
item('POKER TABLE', 't', 800, C, table(GREEN, flat(DARK, 'b'), round_=True, tall=True) + [Box(1, 1, 7, 7, 13, 14, faces(flat(BROWN, 'c'), flat(BROWN, 'c'), M(GREEN, ['abbbbbba', 'bccccccb', 'bcdcccdb'[:8], 'bccccccb', 'bccccccb', 'bcdcccdb'[:8], 'bccccccb', 'abbbbbba'])))], col=(6, 20, 8))
item('BAR COUNTER', 'y', 700, C, counter(DARK, grain(OAK, 8, 8)) + [Box(2, 1, 3, 2, 12, 16, solid(P_GLASS, 'd')), Box(5, 1, 6, 2, 12, 15, solid(GREEN, 'c'))], col=(9, 6, 4))
item('WINE FRIDGE', 'F', 900, C, appliance(DARK, 14, M(DARK, ['bbbbbbbb', 'baaaaaab', 'bacacaab', 'baaaaaab', 'bacacaab', 'baaaaaab', 'bbbbbbbb'])), col=(9, 6, 4))
item('NEON BEER SIGN', 'l', 400, C, [Box(2, 3, 6, 5, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 8, solid(DARK, 'c')), Box(0, 3, 8, 4, 8, 16, faces(M(GLOW, ['aaaaaaaa', 'addddaaa', 'adaadaaa', 'addddaaa', 'adaaaaaa', 'addddaaa', 'aaaaaaaa', 'aeeeeeea']), flat(GLOW, 'a'), flat(GLOW, 'a')))], col=(24, 28, 8))
item('DARTBOARD', 'v', 150, C, [Box(3, 3, 5, 5, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 12, solid(DARK, 'c')), Box(1, 3, 7, 4, 10, 16, faces(M(RED, ['abbbbbba', 'bcdccdcb', 'bdcbbcdb', 'bccbbccb', 'bdcbbcdb', 'bcdccdcb', 'abbbbbba'][:6]), flat(DARK, 'c'), flat(DARK, 'c')))], 1, col=(26, 5, 5))
item('CIGAR HUMIDOR', 'b', 350, C, table(BROWN) + [Box(1, 1, 7, 7, 8, 11, faces(panel(BROWN, 6, 3, 'b', 'c'), flat(BROWN, 'c'), M(BROWN, ['cccccc', 'cddddc', 'cccccc'])))], col=(16, 9, 4))
C = cat('SEASONAL')
item('CHRISTMAS TREE', 'p', 300, C, tree(LEAF, RED), col=(8, 22, 7)); item('PUMPKIN', 'p', 30, C, [Box(1, 1, 7, 7, 0, 6, faces(M(ORANGE, ['cbcbcbcb'] * 2 + ['cbcbcbcb'] * 4), flat(ORANGE, 'c'), M(ORANGE, ['bcbcbcbc'] * 8))), Box(3, 3, 5, 5, 6, 8, solid(LEAF, 'b')), Box(2, 0, 3, 1, 2, 4, solid(GLOW, 'd')), Box(5, 0, 6, 1, 2, 4, solid(GLOW, 'd'))], col=(28, 14, 3))
item('SNOWMAN', 'p', 50, C, [Box(1, 1, 7, 7, 0, 6, solid(WHITEW, 'c')), Box(2, 2, 6, 6, 6, 11, solid(WHITEW, 'd')), Box(2, 2, 6, 6, 11, 15, faces(M(WHITEW, ['dddd', 'dbbd', 'dddd', 'dbbd']), flat(WHITEW, 'd'), flat(WHITEW, 'e'))), Box(3, 1, 5, 2, 12, 13, solid(ORANGE, 'd')), Box(2, 2, 6, 6, 15, 16, solid(DARK, 'c')), Box(3, 3, 5, 5, 16, 19, solid(DARK, 'c'))], col=(28, 28, 29))
item('MENORAH', 'l', 200, C, [Box(3, 3, 5, 5, 0, 1, solid(P_GOLD, 'b')), Box(3, 3, 5, 5, 1, 8, solid(P_GOLD, 'c')), Box(0, 3, 8, 5, 8, 9, solid(P_GOLD, 'c'))] + [Box(i, 3, i + 1, 5, 9, 12 if i != 4 else 14, solid(P_GOLD, 'd')) for i in range(0, 8)] + [Box(i, 3, i + 1, 5, 12 if i != 4 else 14, 13 if i != 4 else 15, solid(GLOW, 'e')) for i in range(0, 8)], col=(28, 22, 6))
item('CAKE TABLE', 't', 120, C, table(WHITEW, tall=True) + [Box(2, 2, 6, 6, 13, 16, faces(stripes(P_PINK, 4, 3, 'c', 'e', True, 1), flat(P_PINK, 'c'), flat(WHITEW, 'd'))), Box(3, 3, 5, 5, 16, 18, solid(P_PINK, 'd')), Box(4, 4, 5, 5, 18, 20, solid(GLOW, 'e'))], col=(30, 14, 22))
item('GIFT PILE', 'b', 100, C, [Box(0, 0, 5, 5, 0, 4, faces(M(RED, ['cccdcc'[:5], 'cccdcc'[:5], 'ddddd', 'cccdcc'[:5]]), flat(RED, 'c'), M(RED, ['ccdcc', 'ccdcc', 'ddddd', 'ccdcc', 'ccdcc']))), Box(4, 3, 8, 8, 0, 3, faces(M(BLUE, ['cdcc', 'cdcc', 'dddd']), flat(BLUE, 'c'), M(BLUE, ['cdcc', 'cdcc', 'dddd', 'cdcc', 'cdcc']))), Box(1, 1, 4, 4, 4, 7, faces(M(YELLOW, ['cdc', 'ddd', 'cdc']), flat(YELLOW, 'c'), M(YELLOW, ['cdc', 'ddd', 'cdc'])))], col=(26, 5, 5))
item('DISCO BALL', 'l', 600, C, [Box(2, 2, 6, 6, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 20, solid(STEEL, 'c')), Box(1, 1, 7, 7, 13, 19, faces(M(STEEL, ['cdcdcd', 'dedede', 'cdcdcd', 'dedede', 'cdcdcd', 'dedede']), M(STEEL, ['dcdcdc', 'ededed', 'dcdcdc', 'ededed', 'dcdcdc', 'ededed']), M(STEEL, ['cdcdcd', 'dedede', 'cdcdcd', 'dedede', 'cdcdcd', 'dedede'])))], col=(20, 22, 26))
item('FIREWORKS BOX', '#', 80, C, box_item(RED, 5) + [Box(2, 2, 3, 3, 5, 12, solid(BLUE, 'c')), Box(4, 3, 5, 4, 5, 14, solid(YELLOW, 'c')), Box(3, 5, 4, 6, 5, 10, solid(GREEN, 'c')), Box(4, 3, 5, 4, 14, 15, solid(RED, 'd'))], col=(26, 5, 5))
item('HEART BALLOON', 'p', 25, C, [Box(3, 3, 5, 5, 0, 1, solid(STEEL, 'b')), Box(4, 4, 5, 5, 1, 14, solid(WHITEW, 'c')), Box(2, 2, 7, 7, 14, 19, faces(M(RED, ['cdccc', 'ddddc', 'ccccc', 'ddddd', 'cdddc']), flat(RED, 'c'), M(RED, ['dccdc', 'ddddd', 'ddddd', 'cdddc', 'ccdcc'])))], col=(26, 5, 5))
item('EASTER BASKET', 'p', 45, C, [Box(1, 1, 7, 7, 0, 5, faces(dots(PINE, 6, 5, 'c', 'b', 2), dots(PINE, 6, 5, 'c', 'b', 2), M(P_BOOKS, ['ccddee', 'dceeaa', 'eecdcc', 'adcdee', 'ccaadd', 'ddeecc']))), Box(3, 3, 5, 5, 5, 11, solid(PINE, 'c'))], col=(26, 20, 11))
C = cat('MISC')
item('PIGGY BANK', 'b', 70, C, table(WHITEW) + [Box(2, 2, 6, 6, 8, 12, faces(M(P_PINK, ['cccc', 'cbcc', 'cccc', 'cdcc']), flat(P_PINK, 'c'), M(P_PINK, ['cccc', 'cbbc', 'cccc', 'cccc']))), Box(6, 3, 7, 5, 9, 11, solid(P_PINK, 'd'))], col=(30, 14, 22))
item('CARDBOARD BOX', '#', 5, C, box_item(pal('pCCard', ramp((22, 16, 8))), 7), col=(22, 16, 8)); item('SUITCASE', '#', 60, C, block(1, 0, 7, 8, 0, 4, faces(stripes(BROWN, 6, 4, 'c', 'b', False, 3), stripes(BROWN, 8, 4, 'c', 'b', False, 3), M(BROWN, ['cccccccc', 'cbbbbbbc', 'cbccccbc', 'cbccddbc', 'cbccccbc', 'cbbbbbbc']))) + [Box(3, 3, 5, 5, 4, 6, solid(DARK, 'c'))], col=(16, 9, 4))
item('DRUM KIT', 'R', 900, C, [Box(2, 2, 7, 7, 0, 5, faces(flat(RED, 'c'), flat(RED, 'c'), M(WHITEW, ['bbbbb', 'bcccb', 'bcccb', 'bcccb', 'bbbbb']))), Box(0, 4, 2, 6, 3, 8, faces(flat(RED, 'c'), flat(RED, 'c'), flat(WHITEW, 'c'))), Box(5, 0, 8, 3, 6, 9, faces(flat(RED, 'c'), flat(RED, 'c'), flat(WHITEW, 'c'))), Box(1, 1, 2, 2, 5, 12, solid(STEEL, 'c')), Box(0, 0, 3, 3, 12, 13, solid(P_GOLD, 'd'))], col=(26, 5, 5))
item('GUITAR STAND', 'R', 350, C, [Box(2, 2, 6, 6, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 4, solid(DARK, 'c')), Box(2, 3, 6, 4, 2, 9, faces(M(RED, ['cccc', 'cdcc', 'cccc', 'bbbb', 'cccc', 'cccc', 'cdcc']), flat(RED, 'c'), flat(RED, 'c'))), Box(3, 3, 4, 4, 9, 19, solid(DARK, 'c')), Box(3, 3, 5, 4, 19, 21, solid(DARK, 'd'))], col=(26, 5, 5))
item('MANNEQUIN', 'p', 300, C, [Box(2, 2, 6, 6, 0, 1, solid(DARK, 'b')), Box(3, 3, 5, 5, 1, 8, solid(STEEL, 'c')), Box(2, 2, 6, 6, 8, 16, faces(panel(TEAL, 4, 8, 'b', 'c'), flat(TEAL, 'c'), flat(WHITEW, 'c'))), Box(3, 3, 5, 5, 16, 19, solid(WHITEW, 'c'))], col=(5, 20, 20))
item('CRYSTAL BALL', 'l', 450, C, table(PURP, cloth=dots(PURP, 8, 8, 'c', 'e', 2)) + [Box(3, 3, 5, 5, 8, 9, solid(P_GOLD, 'c')), Box(2, 2, 6, 6, 9, 13, faces(M(P_GLASS, ['cddc', 'deed', 'deed', 'cddc']), flat(P_GLASS, 'd'), flat(P_GLASS, 'e')))], col=(16, 6, 24))
item('GRANDFATHER CLOCK', 'i', 1200, C, block(1, 2, 7, 7, 0, 16, faces(M(OAK, ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb']), flat(OAK, 'c'), flat(OAK, 'c'))) + block(0, 1, 8, 8, 16, 24, faces(M(OAK, ['bbbbbbbb', 'bccccccb', 'bcddddcb', 'bcdedecb', 'bcdddecb', 'bcddddcb', 'bccccccb', 'bbbbbbbb']), flat(OAK, 'c'), flat(OAK, 'd'))), 1, col=(22, 14, 6))
item('PUNCHING BAG', 'm', 400, C, [Box(2, 2, 6, 6, 0, 2, solid(DARK, 'b')), Box(3, 3, 5, 5, 2, 22, solid(STEEL, 'c')), Box(2, 2, 6, 6, 8, 20, faces(stripes(RED, 4, 12, 'c', 'b', True, 4), stripes(RED, 4, 12, 'c', 'b', True, 4), flat(RED, 'd')))], col=(26, 5, 5))
item('WEIGHT BENCH', 'm', 500, C, legs(1, 1, 7, 7, 6, flat(STEEL, 'c')) + block(2, 0, 6, 8, 6, 8, faces(flat(DARK, 'c'), flat(DARK, 'c'), flat(DARK, 'd'))) + [Box(0, 1, 1, 2, 0, 14, solid(STEEL, 'c')), Box(7, 1, 8, 2, 0, 14, solid(STEEL, 'c')), Box(0, 1, 8, 2, 13, 14, solid(STEEL, 'd')), Box(0, 0, 1, 3, 11, 16, solid(DARK, 'c')), Box(7, 0, 8, 3, 11, 16, solid(DARK, 'c'))], col=(20, 22, 26))

# ====================================================================== BUILD batch: doors, windows, the pool, the garden (the room builder's build-mode items)
def door(panel_m, frame, h=20, half=False, open_=False):   # a door standing in the wall line: two posts, a lintel and the leaf (r=0 lies along a, its face to +b)
    post = solid(frame, 'b'); bx = [Box(0, 3, 1, 5, 0, h, post), Box(7, 3, 8, 5, 0, h, post), Box(0, 3, 8, 5, h, h + 2, solid(frame, 'c'))]
    if open_: return bx
    z0 = 7 if half else 0; z1 = 15 if half else h
    return bx + [Box(1, 3, 7, 5, z0, z1, faces(panel_m, flat(frame, 'b'), flat(frame, 'c')))]
def leaf(p, rows): return M(p, rows)
PANEL6 = ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb', 'bbbbbb', 'bcccdb', 'bccccb', 'bbbbbb']
X_BRACE = ['bccccb', 'cbccbc', 'ccbbcc', 'ccbbcc', 'cbccbc', 'bccccb', 'bccccb', 'cbccbc', 'ccbbcc', 'ccbbcc', 'cbccbc', 'bccccb']
BARS6 = ['cbcbcb', 'cbcbcb'] * 6
C = cat('DOORS')
item('OAK DOOR', 'D', 120, C, door(leaf(OAK, PANEL6), DARK), 1, col=(22, 14, 6)); item('WHITE DOOR', 'D', 120, C, door(leaf(WHITEW, PANEL6), WHITEW), 1, col=(28, 28, 29))
item('RED DOOR', 'D', 150, C, door(leaf(RED, PANEL6), WHITEW), 1, col=(26, 5, 5)); item('BLACK DOOR', 'D', 150, C, door(leaf(DARK, PANEL6), DARK), 1, col=(9, 6, 4))
item('BARN DOOR', 'D', 200, C, door(leaf(TERRA, X_BRACE), BROWN), 1, col=(22, 11, 5)); item('GLASS DOOR', 'D', 350, C, door(M(P_GLASS, ['bbbbbb', 'bccdcb', 'bccdcb', 'bcccdb', 'bccccb', 'bbbbbb']), STEEL), 1, col=(20, 26, 30))
item('SALOON DOORS', 'D', 180, C, door(M(OAK, ['cbcbcb', 'cbcbcb', 'bbbbbb', 'cbcbcb']), OAK, half=True), 1, col=(22, 14, 6)); item('IRON GATE', 'D', 300, C, door(leaf(P_BLACK, BARS6), P_BLACK), 1, col=(6, 6, 8))
item('STEEL DOOR', 'D', 400, C, door(M(STEEL, ['bbbbbb', 'bccccb', 'bcdccb', 'bccccb', 'bccccb', 'bcbbcb', 'bccccb', 'bbbbbb']), P_METAL), 1, col=(20, 22, 26))
item('VAULT DOOR', 'D', 2500, C, door(M(P_METAL, ['bbbbbb', 'bcccdb', 'bcbbcb', 'bcbecb', 'bcbbcb', 'bccccb', 'bbbbbb']), P_METAL), 1, col=(18, 19, 23))
item('ARCHWAY', 'D', 90, C, door(None, STONE, open_=True), 1, col=(17, 17, 16)); item('GARDEN GATE', 'D', 80, C, door(M(WHITEW, ['cbcbcb', 'cbcbcb', 'cccccc', 'cbcbcb', 'cbcbcb', 'cccccc']), WHITEW, half=True), 1, col=(28, 28, 29))
def window(frame, glass_m, w=6, z0=6, z1=18):   # the BUY thumbnail of a window: a pane in a frame (the wall drawer paints the real thing: winSt in main.c)
    a0 = (8 - w) // 2; return [Box(a0, 3, a0 + w, 5, z0, z1, faces(glass_m, flat(frame, 'c'), flat(frame, 'c'))), Box(a0, 3, a0 + w, 5, z0 - 1, z0, solid(frame, 'c')), Box(a0, 3, a0 + w, 5, z1, z1 + 1, solid(frame, 'c')), Box(a0 - 1 if a0 else 0, 3, a0 if a0 else 1, 5, z0 - 1, z1 + 1, solid(frame, 'c')), Box(a0 + w if a0 + w < 8 else 7, 3, min(8, a0 + w + 1), 5, z0 - 1, z1 + 1, solid(frame, 'c'))]
SKYG = M(SKY, ['ccccdccc', 'cccdcccc', 'ccdccccc', 'cccccccc'])
C = cat('WINDOWS')   # (same order as winSt's new kinds in main.c: kind = 3 + the item's place here)
item('PORTHOLE', 'E', 300, C, window(STEEL, SKYG, 4, 8, 14), col=(20, 22, 26)); item('ARCH WINDOW', 'E', 350, C, window(WHITEW, SKYG, 6, 4, 18), col=(28, 28, 29))
item('TALL WINDOW', 'E', 320, C, window(WHITEW, SKYG, 4, 3, 21), col=(28, 28, 29)); item('SMALL WINDOW', 'E', 120, C, window(WHITEW, SKYG, 4, 8, 13), col=(28, 28, 29))
item('DOUBLE WINDOW', 'E', 420, C, window(WHITEW, M(SKY, ['cccbcccc', 'cccbcccc', 'bbbbbbbb', 'cccbcccc']), 8, 6, 18), col=(28, 28, 29))
item('STAINED GLASS', 'E', 900, C, window(DARK, M(P_BOOKS, ['abcdabcd', 'bcdabcda', 'cdabcdab', 'dabcdabc']), 6, 4, 18), col=(26, 12, 28))
item('SHUTTERED', 'E', 260, C, window(GREEN, SKYG, 4, 6, 18), col=(6, 20, 8)); item('BARRED WINDOW', 'E', 200, C, window(P_BLACK, M(SKY, ['cbcbcbcc'] * 4), 6, 6, 18), col=(6, 6, 8))
item('BLACK FRAME', 'E', 280, C, window(P_BLACK, SKYG, 6, 6, 18), col=(6, 6, 8)); item('FRENCH WINDOW', 'E', 600, C, window(WHITEW, M(SKY, ['cccbcccc'] * 4), 6, 2, 21), col=(28, 28, 29))
item('NEON WINDOW', 'E', 500, C, window(NEON, M(P_BLACK, ['bbccbbcc', 'ccbbccbb'] * 2), 6, 6, 18), col=(31, 8, 24)); item('BAY WINDOW', 'E', 700, C, window(WHITEW, M(SKY, ['ccbccbcc'] * 4), 8, 5, 19), col=(28, 28, 29))
def lounger(p, wood):
    frame = solid(wood, 'b'); pad = faces(stripes(p, 8, 2, 'c', 'd', False, 2), flat(p, 'c'), stripes(p, 8, 4, 'c', 'd', False, 2))
    return legs(0, 2, 8, 6, 3, flat(wood, 'b')) + block(0, 2, 8, 6, 3, 5, pad) + [Box(0, 2, 2, 6, 5, 9, faces(flat(p, 'c'), flat(p, 'c'), stripes(p, 2, 4, 'c', 'd', True, 1)))]
C = cat('POOL')
item('POOL LADDER', 'p', 150, C, [Box(1, 3, 2, 4, 0, 14, solid(STEEL, 'c')), Box(6, 3, 7, 4, 0, 14, solid(STEEL, 'c')), Box(1, 3, 7, 4, 13, 14, solid(STEEL, 'd'))] + [Box(2, 3, 6, 4, z, z + 1, solid(STEEL, 'c')) for z in (3, 6, 9)], col=(20, 22, 26))
item('DIVING BOARD', 'p', 450, C, [Box(2, 1, 6, 4, 0, 6, faces(panel(STEEL, 4, 6, 'b', 'c'), flat(STEEL, 'c'), flat(STEEL, 'c'))), Box(2, 0, 6, 8, 6, 7, faces(flat(SKY, 'b'), flat(SKY, 'c'), M(SKY, ['cccc', 'cdcc', 'cccc', 'ccdc'] * 2)))], 1, col=(12, 22, 31))
item('LIFEGUARD CHAIR', 'h', 600, C, legs(1, 1, 7, 7, 14, flat(WHITEW, 'c')) + block(1, 1, 7, 7, 14, 16, faces(flat(WHITEW, 'c'), flat(WHITEW, 'c'), flat(WHITEW, 'd'))) + block(1, 1, 7, 3, 16, 24, faces(flat(RED, 'c'), flat(RED, 'c'), flat(RED, 'd'))) + [Box(2, 1, 6, 3, 19, 21, solid(WHITEW, 'c'))], 1, col=(28, 28, 29))
item('SUN LOUNGER', 'N', 220, C, lounger(WHITEW, STEEL), 1, col=(28, 28, 29)); item('STRIPED LOUNGER', 'N', 240, C, lounger(SKY, PINE), 1, col=(12, 22, 31)); item('PINK LOUNGER', 'N', 240, C, lounger(P_PINK, WHITEW), 1, col=(30, 14, 22))
item('BEACH UMBRELLA', 'p', 180, C, [Box(3, 3, 5, 5, 0, 2, solid(STEEL, 'b')), Box(4, 4, 5, 5, 2, 20, solid(STEEL, 'c'))] + block(0, 0, 8, 8, 20, 23, faces(stripes(RED, 8, 3, 'c', 'e', False, 2), stripes(RED, 8, 3, 'c', 'e', False, 2), stripes(RED, 8, 8, 'c', 'e', False, 2))) + [Box(2, 2, 6, 6, 23, 25, faces(stripes(RED, 4, 2, 'c', 'e', False, 2), flat(RED, 'c'), flat(RED, 'e')))], col=(26, 5, 5))
item('POOL FLOAT', 'p', 40, C, block(0, 0, 8, 8, 0, 3, faces(flat(YELLOW, 'c'), flat(YELLOW, 'c'), M(YELLOW, ['bccccccb', 'cddddddc', 'cdbbbbdc', 'cdbaabdc', 'cdbaabdc', 'cdbbbbdc', 'cddddddc', 'bccccccb']))), col=(30, 26, 4))
item('BEACH BALL', 'p', 25, C, [Box(2, 2, 6, 6, 0, 1, solid(GREY, 'a')), Box(2, 2, 6, 6, 1, 5, faces(M(P_BOOKS, ['abcd', 'abcd', 'abcd', 'abcd']), M(P_BOOKS, ['dabc'] * 4), M(P_BOOKS, ['abcd', 'bcda', 'cdab', 'dabc'])))], col=(24, 6, 6))
item('TOWEL RACK', 'p', 90, C, [Box(1, 3, 2, 5, 0, 12, solid(STEEL, 'c')), Box(6, 3, 7, 5, 0, 12, solid(STEEL, 'c')), Box(1, 3, 7, 5, 11, 12, solid(STEEL, 'd')), Box(2, 3, 4, 5, 4, 11, faces(stripes(SKY, 2, 7, 'c', 'e', True, 2), flat(SKY, 'c'), flat(SKY, 'c'))), Box(4, 3, 6, 5, 5, 11, faces(stripes(P_PINK, 2, 6, 'c', 'e', True, 2), flat(P_PINK, 'c'), flat(P_PINK, 'c')))], col=(20, 22, 26))
item('KIDDIE POOL', 'p', 60, C, block(0, 0, 8, 8, 0, 3, faces(flat(SKY, 'c'), flat(SKY, 'c'), M(P_WATER, ['bbbbbbbb', 'bccccccb', 'bcdccdcb', 'bccccccb', 'bccdcccb', 'bcccccdb', 'bccccccb', 'bbbbbbbb']))), col=(12, 20, 30))
item('OUTDOOR SHOWER', 'H', 350, C, [Box(3, 1, 5, 2, 0, 24, solid(STEEL, 'c')), Box(1, 1, 7, 7, 0, 1, faces(flat(PINE, 'b'), flat(PINE, 'b'), stripes(PINE, 6, 6, 'c', 'b', True, 1))), Box(2, 1, 6, 3, 22, 24, solid(STEEL, 'd'))], 1, col=(20, 22, 26))
C = cat('BEACH')
item('SNACK CART', 'y', 500, C, legs(1, 1, 7, 7, 4, flat(STEEL, 'b')) + block(0, 1, 8, 7, 4, 12, faces(panel(WHITEW, 8, 8, 'b', 'c'), flat(WHITEW, 'c'), flat(RED, 'c'))) + [Box(0, 1, 8, 7, 18, 20, faces(stripes(RED, 8, 2, 'c', 'e', False, 2), stripes(RED, 6, 2, 'c', 'e', False, 2), stripes(RED, 8, 6, 'c', 'e', False, 2))), Box(0, 1, 1, 2, 12, 18, solid(STEEL, 'c')), Box(7, 6, 8, 7, 12, 18, solid(STEEL, 'c'))], 1, col=(26, 5, 5))
item('COOLER', '#', 45, C, block(1, 2, 7, 6, 0, 5, faces(panel(BLUE, 6, 5, 'b', 'c'), flat(BLUE, 'c'), flat(WHITEW, 'c'))) + [Box(1, 2, 7, 6, 5, 6, solid(WHITEW, 'd'))], col=(6, 10, 26))
item('POOL NOODLES', 'p', 15, C, [Box(1, 3, 2, 4, 0, 16, solid(P_PINK, 'c')), Box(3, 2, 4, 3, 0, 14, solid(GREEN, 'c')), Box(5, 4, 6, 5, 0, 17, solid(YELLOW, 'c')), Box(0, 2, 7, 5, 0, 1, solid(GREY, 'b'))], col=(30, 14, 22))
item('FLAMINGO FLOAT', 'p', 70, C, block(0, 0, 8, 8, 0, 3, faces(flat(P_PINK, 'c'), flat(P_PINK, 'c'), M(P_PINK, ['bccccccb', 'cddddddc', 'cdbbbbdc', 'cdbccbdc', 'cdbccbdc', 'cdbbbbdc', 'cddddddc', 'bccccccb']))) + [Box(6, 3, 7, 5, 3, 9, solid(P_PINK, 'c')), Box(5, 3, 8, 5, 9, 11, faces(M(P_PINK, ['ccb', 'ccb']), flat(P_PINK, 'c'), flat(P_PINK, 'd')))], col=(30, 14, 22))
item('TIKI BAR', 'y', 900, C, counter(PINE, grain(PINE, 8, 8)) + [Box(0, 0, 1, 1, 12, 22, solid(P_WOOD, 'c')), Box(7, 0, 8, 1, 12, 22, solid(P_WOOD, 'c')), Box(0, 0, 8, 8, 22, 25, faces(stripes(MUST, 8, 3, 'c', 'b', True, 1), stripes(MUST, 8, 3, 'c', 'b', True, 1), stripes(MUST, 8, 8, 'c', 'b', True, 1)))], col=(26, 20, 11))
item('TIKI TORCH', 'l', 80, C, [Box(3, 3, 5, 5, 0, 1, solid(P_WOOD, 'b')), Box(4, 4, 5, 5, 1, 18, solid(P_WOOD, 'c')), Box(3, 3, 6, 6, 18, 22, faces(stripes(P_WOOD, 3, 4, 'c', 'b', True, 1), flat(P_WOOD, 'c'), flat(ORANGE, 'c'))), Box(3, 3, 6, 6, 22, 25, faces(M(ORANGE, ['cdc', 'dec', 'cdd']), flat(ORANGE, 'd'), flat(YELLOW, 'd')))], col=(28, 14, 3))
item('SURFBOARD', 'p', 300, C, [Box(3, 1, 5, 7, 0, 1, solid(GREY, 'b')), Box(3, 1, 5, 7, 1, 2, faces(flat(CYAN, 'c'), flat(CYAN, 'c'), M(CYAN, ['bcbb', 'bccb', 'cddc', 'cddc', 'cddc', 'bccb']))), Box(3, 2, 5, 6, 2, 3, faces(flat(CYAN, 'c'), flat(CYAN, 'c'), M(CYAN, ['cdc', 'ddd', 'cdc'])))] + [Box(3, 1, 4, 2, 1, 16, solid(CYAN, 'c')), Box(3, 1, 4, 2, 16, 17, solid(CYAN, 'd'))], col=(8, 26, 30))
item('LIFE RING', 'p', 50, C, [Box(3, 3, 5, 5, 0, 1, solid(STEEL, 'b')), Box(4, 4, 5, 5, 1, 10, solid(STEEL, 'c')), Box(1, 3, 7, 5, 8, 14, faces(M(RED, ['ccbbcc', 'cbeebc', 'beeeeb', 'beeeeb', 'cbeebc', 'ccbbcc']), flat(RED, 'c'), flat(RED, 'c')))], col=(26, 5, 5))
item('CABANA', 'U', 650, C, [Box(0, 0, 1, 1, 0, 20, solid(P_WOOD, 'c')), Box(7, 0, 8, 1, 0, 20, solid(P_WOOD, 'c')), Box(0, 7, 1, 8, 0, 20, solid(P_WOOD, 'c')), Box(7, 7, 8, 8, 0, 20, solid(P_WOOD, 'c')), Box(0, 0, 8, 8, 20, 22, faces(stripes(WHITEW, 8, 2, 'c', 'b', False, 2), stripes(WHITEW, 8, 2, 'c', 'b', False, 2), stripes(WHITEW, 8, 8, 'c', 'b', False, 2))), Box(1, 1, 7, 7, 0, 4, faces(flat(SKY, 'c'), flat(SKY, 'c'), flat(SKY, 'd')))], col=(28, 28, 29))
item('SAND CASTLE', 'p', 10, C, [Box(1, 1, 7, 7, 0, 3, solid(MUST, 'c')), Box(2, 2, 6, 6, 3, 7, faces(M(MUST, ['cbcc', 'cccc', 'ccbc', 'cccc']), flat(MUST, 'c'), flat(MUST, 'd'))), Box(1, 1, 3, 3, 3, 10, solid(MUST, 'd')), Box(5, 5, 7, 7, 3, 10, solid(MUST, 'd')), Box(3, 3, 5, 5, 7, 12, solid(MUST, 'd')), Box(3, 3, 4, 4, 12, 15, solid(RED, 'c'))], col=(26, 20, 4))
def shrub(p, w=3, h=8, z0=0, trunk=0):
    lm = faces(dots(p, 8, 8, 'c', 'f', 2), dots(p, 8, 8, 'c', 'b', 2), dots(p, 8, 8, 'c', 'g', 2))
    bx = [Box(3, 3, 5, 5, 0, z0, solid(P_WOOD, 'b'))] if trunk else []
    return bx + block(4 - w, 4 - w, 4 + w, 4 + w, z0, z0 + h, lm)
def bigtree(p, trunk_h=10, crown=4, ch=12):
    return [Box(3, 3, 5, 5, 0, trunk_h, faces(grain(P_WOOD, 2, 8, 'c', 'b', 2), grain(P_WOOD, 2, 8, 'c', 'b', 2), flat(P_WOOD, 'b')))] + block(4 - crown, 4 - crown, 4 + crown, 4 + crown, trunk_h, trunk_h + ch, faces(dots(p, 8, 8, 'c', 'f', 2), dots(p, 8, 8, 'c', 'b', 2), dots(p, 8, 8, 'c', 'g', 2))) + block(2, 2, 6, 6, trunk_h + ch, trunk_h + ch + 3, faces(dots(p, 4, 3, 'c', 'f', 2), dots(p, 4, 3, 'c', 'f', 2), dots(p, 4, 4, 'c', 'g', 2)))
BLOSSOM = pal('pCBlossom', ramp((30, 18, 24)) + [(28, 12, 20), (31, 26, 29)]); AUTUMN = pal('pCAutumn', ramp((26, 12, 4)) + [(22, 8, 3), (30, 20, 6)])
C = cat('GARDEN')
item('HEDGE', '#', 60, C, block(0, 0, 8, 8, 0, 14, faces(dots(LEAF, 8, 8, 'c', 'f', 2), dots(LEAF, 8, 8, 'c', 'b', 2), dots(LEAF, 8, 8, 'c', 'g', 2))), col=(8, 22, 7))
item('TOPIARY BALL', 'p', 140, C, [Box(2, 2, 6, 6, 0, 4, faces(panel(TERRA, 4, 4, 'b', 'c'), flat(TERRA, 'c'), flat(P_SOIL, 'a')))] + shrub(LEAF, 2, 6, 7, 1), col=(8, 22, 7))
item('TOPIARY CONE', 'p', 160, C, [Box(2, 2, 6, 6, 0, 4, faces(panel(TERRA, 4, 4, 'b', 'c'), flat(TERRA, 'c'), flat(P_SOIL, 'a')))] + shrub(LEAF, 3, 4, 4) + shrub(LEAF, 2, 4, 8) + shrub(LEAF, 1, 4, 12), col=(8, 22, 7))
item('RED FLOWER BED', 'Z', 70, C, block(0, 0, 8, 8, 0, 2, faces(flat(P_WOOD, 'b'), flat(P_WOOD, 'b'), flat(P_SOIL, 'b'))) + block(1, 1, 7, 7, 2, 5, faces(dots(LEAF, 6, 3, 'c', 'b', 2), dots(LEAF, 6, 3, 'c', 'b', 2), dots(RED, 6, 6, 'a', 'd', 2))), col=(26, 5, 5))
item('YELLOW FLOWER BED', 'Z', 70, C, block(0, 0, 8, 8, 0, 2, faces(flat(P_WOOD, 'b'), flat(P_WOOD, 'b'), flat(P_SOIL, 'b'))) + block(1, 1, 7, 7, 2, 5, faces(dots(LEAF, 6, 3, 'c', 'b', 2), dots(LEAF, 6, 3, 'c', 'b', 2), dots(YELLOW, 6, 6, 'a', 'd', 2))), col=(30, 26, 4))
item('TULIP BED', 'Z', 90, C, block(0, 0, 8, 8, 0, 2, faces(flat(STONE, 'b'), flat(STONE, 'b'), flat(P_SOIL, 'b'))) + [Box(1 + i * 2, 1 + j * 2, 2 + i * 2, 2 + j * 2, 2, 5 + ((i + j) & 1), solid(LEAF, 'c')) for i in range(3) for j in range(3)] + [Box(1 + i * 2, 1 + j * 2, 2 + i * 2, 2 + j * 2, 5 + ((i + j) & 1), 7 + ((i + j) & 1), solid([RED, P_PINK, YELLOW][(i + j) % 3], 'd')) for i in range(3) for j in range(3)], col=(30, 14, 22))
item('ROSE BUSH', 'p', 110, C, shrub(LEAF, 3, 8) + [Box(1 + i * 2, 1 + ((i * 3) % 5), 2 + i * 2, 2 + ((i * 3) % 5), 8, 9, solid(RED, 'd')) for i in range(3)], col=(26, 5, 5))
item('SHRUB', 'p', 50, C, shrub(LEAF2, 3, 7), col=(14, 24, 8))
item('SUNDIAL', 'p', 220, C, [Box(2, 2, 6, 6, 0, 1, solid(STONE, 'b')), Box(3, 3, 5, 5, 1, 7, solid(STONE, 'c')), Box(1, 1, 7, 7, 7, 9, faces(flat(STONE, 'c'), flat(STONE, 'c'), M(P_GOLD, ['bccccccb', 'cbccccbc', 'ccbccbcc', 'cccbbccc', 'cccbbccc', 'ccbccbcc', 'cbccccbc', 'bccccccb']))), Box(3, 4, 5, 5, 9, 12, solid(P_GOLD, 'c'))], col=(28, 22, 6))
item('WISHING WELL', 'p', 400, C, block(1, 1, 7, 7, 0, 6, faces(M(STONE, ['bcbcbc', 'cbcbcb', 'bcbcbc', 'cbcbcb', 'bcbcbc', 'cbcbcb']), M(STONE, ['cbcbcb', 'bcbcbc'] * 3), M(P_WATER, ['bbbbbb', 'bccccb', 'bcdccb', 'bccccb', 'bccdcb', 'bbbbbb']))) + [Box(1, 3, 2, 5, 6, 14, solid(P_WOOD, 'c')), Box(6, 3, 7, 5, 6, 14, solid(P_WOOD, 'c')), Box(0, 2, 8, 6, 14, 17, faces(flat(TERRA, 'c'), flat(TERRA, 'b'), stripes(TERRA, 8, 4, 'c', 'b', False, 1)))], col=(17, 17, 16))
item('STONE LANTERN', 'l', 260, C, [Box(2, 2, 6, 6, 0, 2, solid(STONE, 'b')), Box(3, 3, 5, 5, 2, 10, solid(STONE, 'c')), Box(1, 1, 7, 7, 10, 11, solid(STONE, 'c')), Box(2, 2, 6, 6, 11, 15, faces(M(GLOW, ['bddb', 'dccd', 'dccd', 'bddb']), flat(GLOW, 'c'), flat(STONE, 'c'))), Box(1, 1, 7, 7, 15, 17, solid(STONE, 'd')), Box(3, 3, 5, 5, 17, 19, solid(STONE, 'd'))], col=(17, 17, 16))
item('GARDEN STATUE', 'l', 650, C, statue(STONE, STONE, 17), col=(17, 17, 16))
C = cat('TREES')
item('BOXWOOD', 'p', 80, C, block(1, 1, 7, 7, 0, 9, faces(dots(LEAF, 6, 9, 'c', 'b', 2), dots(LEAF, 6, 9, 'c', 'b', 2), dots(LEAF, 6, 6, 'c', 'g', 2))), col=(8, 22, 7))
item('OAK TREE', 'p', 350, C, bigtree(LEAF, 10, 4, 12), col=(8, 22, 7)); item('PALM TREE', 'p', 400, C, [Box(3, 3, 5, 5, 0, 20, faces(stripes(P_WOOD, 2, 8, 'c', 'b', True, 2), stripes(P_WOOD, 2, 8, 'c', 'b', True, 2), flat(P_WOOD, 'b')))] + [Box(0, 3, 8, 5, 20, 22, faces(dots(LEAF2, 8, 2, 'c', 'b', 2), flat(LEAF2, 'c'), M(LEAF2, ['bccccccb', 'cccccccc']))), Box(3, 0, 5, 8, 20, 22, faces(flat(LEAF2, 'c'), dots(LEAF2, 8, 2, 'c', 'b', 2), M(LEAF2, ['bc', 'cc', 'cc', 'cc', 'cc', 'cc', 'cc', 'cb']))), Box(2, 2, 6, 6, 22, 25, faces(dots(LEAF2, 4, 3, 'c', 'b', 2), dots(LEAF2, 4, 3, 'c', 'b', 2), dots(LEAF2, 4, 4, 'c', 'g', 2))), Box(3, 3, 5, 5, 19, 20, solid(BROWN, 'c'))], col=(14, 24, 8))
item('PINE TREE', 'p', 300, C, [Box(3, 3, 5, 5, 0, 6, solid(P_WOOD, 'b'))] + shrub(LEAF, 4, 5, 6) + shrub(LEAF, 3, 5, 11) + shrub(LEAF, 2, 5, 16) + shrub(LEAF, 1, 5, 21), col=(8, 22, 7))
item('BIRCH TREE', 'p', 320, C, [Box(3, 3, 5, 5, 0, 12, faces(M(P_WHITE, ['cc', 'ca', 'cc', 'ac', 'cc', 'cc', 'ca', 'cc']), M(P_WHITE, ['cc', 'cc', 'ac', 'cc', 'ca', 'cc', 'cc', 'ac']), flat(P_WHITE, 'b')))] + block(1, 1, 7, 7, 12, 22, faces(dots(LEAF2, 6, 8, 'c', 'f', 2), dots(LEAF2, 6, 8, 'c', 'b', 2), dots(LEAF2, 6, 6, 'c', 'g', 2))) + block(2, 2, 6, 6, 22, 25, faces(dots(LEAF2, 4, 3, 'c', 'f', 2), dots(LEAF2, 4, 3, 'c', 'f', 2), dots(LEAF2, 4, 4, 'c', 'g', 2))), col=(14, 24, 8))
item('CHERRY BLOSSOM', 'p', 500, C, bigtree(BLOSSOM, 9, 4, 11), col=(30, 18, 24)); item('AUTUMN MAPLE', 'p', 380, C, bigtree(AUTUMN, 10, 4, 11), col=(26, 12, 4))
item('WILLOW', 'p', 450, C, [Box(3, 3, 5, 5, 0, 14, solid(P_WOOD, 'b'))] + block(0, 0, 8, 8, 8, 20, faces(stripes(LEAF2, 8, 12, 'c', 'b', False, 1), stripes(LEAF2, 8, 12, 'c', 'b', False, 1), dots(LEAF2, 8, 8, 'c', 'g', 2))) + block(2, 2, 6, 6, 20, 23, faces(dots(LEAF2, 4, 3, 'c', 'b', 2), dots(LEAF2, 4, 3, 'c', 'b', 2), flat(LEAF2, 'g'))), col=(14, 24, 8))
item('BONSAI', 'p', 280, C, [Box(1, 2, 7, 6, 0, 2, faces(panel(TERRA, 6, 2, 'b', 'c'), flat(TERRA, 'c'), flat(P_SOIL, 'a'))), Box(3, 3, 5, 5, 2, 6, solid(P_WOOD, 'b')), Box(1, 2, 5, 5, 6, 8, faces(dots(LEAF, 4, 2, 'c', 'b', 2), flat(LEAF, 'c'), dots(LEAF, 4, 3, 'c', 'g', 2))), Box(4, 3, 7, 6, 8, 10, faces(dots(LEAF, 3, 2, 'c', 'b', 2), flat(LEAF, 'c'), dots(LEAF, 3, 3, 'c', 'g', 2)))], col=(8, 22, 7))
item('DEAD TREE', 'p', 90, C, [Box(3, 3, 5, 5, 0, 16, faces(grain(DARK, 2, 8, 'c', 'b', 2), grain(DARK, 2, 8, 'c', 'b', 2), flat(DARK, 'b'))), Box(1, 3, 3, 4, 10, 11, solid(DARK, 'c')), Box(1, 3, 2, 4, 11, 15, solid(DARK, 'c')), Box(5, 4, 7, 5, 12, 13, solid(DARK, 'c')), Box(6, 4, 7, 5, 13, 18, solid(DARK, 'c'))], col=(9, 6, 4))
item('TREE STUMP', 'N', 30, C, block(1, 1, 7, 7, 0, 6, faces(grain(P_WOOD, 6, 6, 'c', 'b', 2), grain(P_WOOD, 6, 6, 'c', 'b', 2), M(PINE, ['bccccb', 'cbbbbc', 'cbccbc', 'cbccbc', 'cbbbbc', 'bccccb']))), col=(18, 11, 5))
item('APPLE TREE', 'p', 420, C, bigtree(LEAF, 9, 4, 11) + [Box(1 + i * 2, 1 + ((i * 2) % 5), 2 + i * 2, 2 + ((i * 2) % 5), 12 + (i % 3) * 3, 13 + (i % 3) * 3, solid(RED, 'd')) for i in range(3)], col=(26, 5, 5))
C = cat('YARD')
item('SWING SET', 'U', 350, C, [Box(0, 1, 1, 2, 0, 18, solid(STEEL, 'c')), Box(0, 6, 1, 7, 0, 18, solid(STEEL, 'c')), Box(7, 1, 8, 2, 0, 18, solid(STEEL, 'c')), Box(7, 6, 8, 7, 0, 18, solid(STEEL, 'c')), Box(0, 3, 8, 5, 18, 19, solid(STEEL, 'd')), Box(2, 3, 3, 5, 6, 18, solid(GREY, 'b')), Box(5, 3, 6, 5, 6, 18, solid(GREY, 'b')), Box(2, 3, 6, 5, 5, 6, solid(RED, 'c'))], col=(20, 22, 26))
item('SANDBOX', 'U', 120, C, block(0, 0, 8, 8, 0, 2, faces(flat(PINE, 'b'), flat(PINE, 'b'), flat(PINE, 'c'))) + block(1, 1, 7, 7, 1, 3, faces(flat(MUST, 'c'), flat(MUST, 'c'), dots(MUST, 6, 6, 'c', 'd', 3))) + [Box(2, 2, 3, 3, 3, 4, solid(RED, 'c'))], col=(26, 20, 4))
item('DOG HOUSE', 'p', 180, C, block(1, 1, 7, 7, 0, 7, faces(M(RED, ['cccccc', 'ccbbcc', 'cbaabc', 'cbaabc', 'cbaabc', 'cbaabc', 'cccccc']), flat(RED, 'c'), flat(RED, 'c'))) + [Box(0, 0, 8, 8, 7, 9, solid(DARK, 'c')), Box(1, 1, 7, 7, 9, 11, solid(DARK, 'c')), Box(2, 2, 6, 6, 11, 12, solid(DARK, 'd'))], col=(26, 5, 5))
item('BIRD FEEDER', 'p', 60, C, [Box(3, 3, 5, 5, 0, 1, solid(P_WOOD, 'b')), Box(4, 4, 5, 5, 1, 14, solid(P_WOOD, 'c')), Box(2, 2, 6, 6, 14, 15, solid(P_WOOD, 'c')), Box(3, 3, 5, 5, 15, 19, faces(M(MUST, ['cdcc', 'cccd', 'dccc', 'ccdc']), flat(MUST, 'c'), flat(P_WOOD, 'c'))), Box(2, 2, 6, 6, 19, 21, faces(stripes(RED, 4, 2, 'c', 'b', True, 1), flat(RED, 'c'), flat(RED, 'd')))], col=(18, 11, 5))
item('WHEELBARROW', 'p', 70, C, [Box(1, 3, 3, 5, 0, 2, solid(DARK, 'b')), Box(0, 2, 7, 6, 2, 6, faces(flat(GREEN, 'c'), flat(GREEN, 'c'), M(P_SOIL, ['bbbbbbb', 'bcccccb', 'bccdccb', 'bbbbbbb']))), Box(7, 3, 8, 4, 0, 4, solid(P_WOOD, 'c')), Box(7, 4, 8, 5, 0, 4, solid(P_WOOD, 'c'))], 1, col=(6, 20, 8))
item('GARDEN SHED', '#', 700, C, block(0, 0, 8, 8, 0, 14, faces(M(PINE, ['bbbbbbbb', 'bcccccbb', 'bcbbbcbb', 'bcbbbcbb', 'bcbbbcbb', 'bcbbbcbb', 'bcccccbb', 'bbbbbbbb']), stripes(PINE, 8, 14, 'c', 'b', True, 2), flat(PINE, 'c'))) + [Box(0, 0, 8, 8, 14, 16, solid(DARK, 'c')), Box(1, 1, 7, 7, 16, 18, solid(DARK, 'c')), Box(2, 2, 6, 6, 18, 19, solid(DARK, 'd'))], col=(26, 20, 11))
item('COMPOST BIN', 'Y', 40, C, block(1, 1, 7, 7, 0, 9, faces(stripes(DARK, 6, 9, 'c', 'b', True, 3), stripes(DARK, 6, 9, 'c', 'b', True, 3), dots(P_SOIL, 6, 6, 'b', 'c', 2))), col=(9, 6, 4))
item('RAIN BARREL', 'O', 60, C, block(1, 1, 7, 7, 0, 12, faces(stripes(TERRA, 6, 12, 'c', 'b', True, 4), stripes(TERRA, 6, 12, 'c', 'b', True, 4), M(P_WATER, ['bbbbbb', 'bccccb', 'bcdccb', 'bccccb', 'bccdcb', 'bbbbbb']))), col=(22, 11, 5))
item('SCARECROW', 'p', 55, C, [Box(4, 4, 5, 5, 0, 12, solid(P_WOOD, 'c')), Box(1, 4, 8, 5, 9, 10, solid(P_WOOD, 'c')), Box(2, 3, 7, 6, 6, 12, faces(M(BLUE, ['ccbcc', 'cbcbc', 'ccbcc', 'cbcbc', 'ccbcc', 'ccccc']), flat(BLUE, 'c'), flat(BLUE, 'c'))), Box(3, 3, 6, 6, 12, 15, faces(M(MUST, ['cbc', 'ccc', 'cbc']), flat(MUST, 'c'), flat(MUST, 'c'))), Box(2, 2, 7, 7, 15, 16, solid(BROWN, 'c')), Box(3, 3, 6, 6, 16, 18, solid(BROWN, 'c'))], col=(26, 20, 4))
item('BOULDER', 'O', 30, C, block(1, 1, 7, 7, 0, 5, faces(dots(STONE, 6, 5, 'c', 'b', 3), dots(STONE, 6, 5, 'c', 'b', 3), dots(STONE, 6, 6, 'c', 'd', 2))) + block(2, 2, 6, 6, 5, 8, faces(flat(STONE, 'c'), flat(STONE, 'b'), flat(STONE, 'd'))), col=(17, 17, 16))
item('FLOWER POTS', 'p', 35, C, [Box(1, 1, 4, 4, 0, 3, solid(TERRA, 'c')), Box(4, 4, 7, 7, 0, 3, solid(TERRA, 'c')), Box(2, 2, 3, 3, 3, 6, solid(LEAF, 'c')), Box(5, 5, 6, 6, 3, 6, solid(LEAF, 'c')), Box(2, 2, 3, 3, 6, 7, solid(RED, 'd')), Box(5, 5, 6, 6, 6, 7, solid(YELLOW, 'd'))], col=(22, 11, 5))
item('TRELLIS', '#', 90, C, [Box(0, 3, 8, 4, 0, 1, solid(WHITEW, 'c'))] + [Box(i, 3, i + 1, 4, 1, 16, solid(WHITEW, 'c')) for i in (0, 3, 7)] + [Box(0, 3, 8, 4, z, z + 1, solid(WHITEW, 'c')) for z in (5, 10, 15)] + [Box(1 + (i * 3) % 6, 3, 2 + (i * 3) % 6, 4, 2 + i * 2, 4 + i * 2, solid(LEAF, 'c')) for i in range(6)], col=(28, 28, 29))

# ====================================================================== HAZARD: the barrels and canisters the BOREZIES arenas scatter about (also for sale in build mode)
C = cat('HAZARD')
item('TOXIC BARREL', 'O', 90, C, block(1, 1, 7, 7, 0, 5, faces(stripes(GREEN, 6, 5, 'c', 'e', True, 3), stripes(GREEN, 6, 5, 'c', 'e', True, 3), flat(GREEN, 'd'))) + block(1, 1, 7, 7, 5, 7, faces(stripes(YELLOW, 6, 2, 'c', 'e', False, 1), stripes(YELLOW, 6, 2, 'c', 'e', False, 1), flat(YELLOW, 'd'))) + block(1, 1, 7, 7, 7, 13, faces(stripes(GREEN, 6, 6, 'c', 'e', True, 3), stripes(GREEN, 6, 6, 'c', 'e', True, 3), M(GREEN, ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb']))), col=(6, 20, 8))
item('RED FUEL DRUM', 'O', 70, C, block(1, 1, 7, 7, 0, 13, faces(stripes(RED, 6, 13, 'c', 'b', True, 4), stripes(RED, 6, 13, 'c', 'b', True, 4), M(RED, ['bbbbbb', 'bccccb', 'bcddcb', 'bcddcb', 'bccccb', 'bbbbbb']))) + [Box(3, 3, 5, 5, 13, 14, solid(STEEL, 'c'))], col=(26, 5, 5))
item('GAS CANISTER', 'Y', 60, C, block(2, 2, 6, 6, 0, 12, faces(stripes(MUST, 4, 12, 'c', 'b', False, 2), stripes(MUST, 4, 12, 'c', 'b', False, 2), flat(MUST, 'd'))) + [Box(3, 3, 5, 5, 12, 15, solid(STEEL, 'c')), Box(2, 3, 6, 5, 15, 16, solid(STEEL, 'b'))], col=(26, 20, 4))
item('PROPANE TANK', 'Y', 80, C, block(1, 1, 7, 7, 0, 8, faces(stripes(WHITEW, 6, 8, 'c', 'b', True, 4), stripes(WHITEW, 6, 8, 'c', 'b', True, 4), flat(WHITEW, 'd'))) + [Box(2, 2, 6, 6, 8, 9, solid(STEEL, 'b')), Box(3, 3, 5, 5, 9, 12, solid(STEEL, 'c'))], col=(28, 28, 29))

# ====================================================================== COUNTERS: counters with a job (a sink to wash up at, a microwave to heat a snack, a fridge built in)
# R next to one of these (or the KITCHEN SINK / MICROWAVE STAND above) does it: counters.h. Appended LAST so no sprite number and no look byte that is already saved moves.
C = cat('COUNTERS')
def counterf(p, front, top, cap=None):   # a counter with its own front (and a plain cap over the cabinet)
    return block(0, 1, 8, 8, 0, 11, [front, flat(p, 'c'), front, flat(p, 'c'), top]) + block(0, 0, 8, 8, 11, 12, faces(flat(p, 'd'), flat(p, 'd'), cap or top))
def tap(a0=3, a1=5, h=17):   # a tall tap at the back of the top and its spout
    return [Box(a0, 1, a1, 2, 12, h, solid(STEEL, 'd')), Box(a0, 2, a1, 4, h - 1, h, solid(STEEL, 'c'))]
BASIN = ['cccccccc', 'cbbbbbbc', 'cbaaaabc', 'cbaaaabc', 'cbbbbbbc', 'cccccccc', 'cccddccc', 'cccccccc']
item('SINK COUNTER', 'y', 380, C, counter(OAK, M(STEEL, BASIN)) + tap(), col=(22, 14, 6), fn=1)
item('DOUBLE SINK', 'y', 520, C, counter(STEEL, M(STEEL, ['cccccccc', 'cbbccbbc', 'cbaccabc', 'cbaccabc', 'cbbccbbc', 'cccccccc', 'cdccccdc', 'cccccccc'])) + tap(3, 5, 18), col=(20, 22, 26), fn=1)
FARMF = M(WHITEW, ['bbbbbbbb', 'bccccccb', 'bcddddcb', 'bcddddcb', 'bccccccb', 'bbbbbbbb', 'bcccbccb', 'bcccbccb', 'bcccbccb', 'bcccbccb', 'bbbbbbbb'])
item('FARM SINK', 'y', 650, C, counterf(WHITEW, FARMF, M(STEEL, ['cccccccc', 'cbbbbbbc', 'cbaaaabc', 'cbaaaabc', 'cbaaaabc', 'cbbbbbbc', 'cccddccc', 'cccccccc'])) + tap(3, 5, 19), col=(28, 28, 29), fn=1)
MICROF = M(WHITEW, ['bbbbbb', 'baaaac', 'baaaad', 'bbbbbb'])
item('MICROWAVE COUNTER', 'y', 320, C, counter(WHITEW, grain(PINE, 8, 8)) + block(1, 2, 7, 6, 12, 17, faces(MICROF, flat(WHITEW, 'c'), flat(WHITEW, 'd'))), col=(28, 28, 29), fn=2)
item('STEEL MICROWAVE', 'y', 420, C, counter(STEEL, grain(STEEL, 8, 8)) + block(1, 2, 7, 6, 12, 17, faces(M(DARK, ['bbbbbb', 'baaacb', 'baaadb', 'bbbbbb']), flat(DARK, 'c'), flat(DARK, 'c'))), col=(20, 22, 26), fn=2)
item('SINK + MICROWAVE', 'y', 620, C, counter(WHITEW, M(STEEL, ['cccccccc', 'cbbbcccc', 'cbaabccc', 'cbaabccc', 'cbbbcccc', 'cccccccc', 'cdcccccc', 'cccccccc'])) + tap(1, 3, 17) + block(4, 2, 8, 6, 12, 16, faces(M(DARK, ['bbbb', 'baac', 'baad', 'bbbb']), flat(DARK, 'c'), flat(DARK, 'c'))), col=(20, 22, 26), fn=3)
item('BUTCHER BLOCK', 'y', 300, C, counter(PINE, grain(PINE, 8, 8)) + [Box(1, 2, 5, 7, 12, 13, solid(OAK, 'c')), Box(5, 3, 7, 5, 12, 16, solid(DARK, 'c'))], col=(26, 20, 11))
item('BREAKFAST BAR', 'y', 450, C, counter(DARK, grain(OAK, 8, 8)) + [Box(0, 0, 8, 2, 12, 16, faces(flat(OAK, 'c'), flat(OAK, 'b'), grain(OAK, 8, 8))), Box(2, 4, 4, 6, 12, 13, solid(WHITEW, 'd')), Box(5, 4, 7, 6, 12, 13, solid(WHITEW, 'd'))], col=(9, 6, 4))
item('NAVY ISLAND', 'y', 700, C, counter(BLUE, M(MARBLE, ['cccccccc', 'ccdccccc', 'cccccdcc', 'cdcccccc', 'cccccccc', 'cccdcccc', 'cccccccd', 'cccccccc'])) + [Box(3, 3, 5, 5, 12, 14, solid(MUST, 'c'))], col=(6, 10, 26))
FRIDGEF = M(STEEL, ['bbbbbbbb', 'bccccccb', 'bccccccb', 'bccccccb', 'bccccccb', 'bcccccdb', 'bcccccdb', 'bccccccb', 'bccccccb', 'bccccccb', 'bbbbbbbb'])
item('FRIDGE COUNTER', 'F', 850, C, counterf(STEEL, FRIDGEF, grain(STEEL, 8, 8)), col=(20, 22, 26))   # an F: it is a fridge to eat from, like the others

CAT_BUILD0 = 200   # the first 200 are the #76 batch; the rest the BUILD batch
assert len(ITEMS) == 296, len(ITEMS)
assert all(len([i for i in ITEMS if i['cat'] == c]) <= 12 for c in range(len(CATS))), 'a category holds at most 12 (the swatch row)'

# ====================================================================== emit
def cpal(n, c): return 'static const u16 %s[%d]={%s};' % (n, len(c), ','.join('RGB(%d,%d,%d)' % tuple(v) for v in c))
def emit():
    L = ['// catalogart.h - GENERATED by tools/make_catalog.py: the boxes and textures of the 200 catalog items (host-only; itembake.h bakes them into source/itemrom.h).']
    for n, c in PALS: L.append(cpal(n, c))
    for m in MATS: L.append('MAT(%s,%s,%d,%d,%s)' % (m.name, [n for n, c in PALS if c is m.pal][0], m.w, m.h, ','.join('"%s"' % r for r in m.rows)))
    for k, it in enumerate(ITEMS):
        b = it['boxes']; L.append('static const IBox bxCat%d[%d]={ // %s' % (k, len(b), it['name']))
        for i, x in enumerate(b): L.append(' {%d,%d,%d,%d,%d,%d,{%s}}%s' % (x.a0, x.b0, x.a1, x.b1, x.z0, x.z1, ','.join('&' + m.name for m in x.m), ',' if i < len(b) - 1 else ' };'))
    return '\n'.join(L) + '\n'
def emit_bake():
    L = ['// catalogbake.h - GENERATED by tools/make_catalog.py: included inside bakeAll() (itembake.h). One sprite an item, four for the ones that turn.']
    spr = 0
    for k, it in enumerate(ITEMS):
        if it['rot']: L.append('    for(int r=0;r<4;r++) bakeOne(V_CAT0+%d+r,bxCat%d,%d,r,%d);' % (spr, k, len(it['boxes']), it['nsh'])); spr += 4
        else: L.append('    bakeOne(V_CAT0+%d,bxCat%d,%d,0,%d);' % (spr, k, len(it['boxes']), it['nsh'])); spr += 1
    return '\n'.join(L) + '\n', spr
def emit_table(spr_n):
    L = ['// catalog.h - GENERATED by tools/make_catalog.py: THE CATALOG the room builder sells (200 looks for the tiles; see expand.h for what a look is).',
         '// catTab[id-1]: name, the map character the tile behaves as, price, first sprite (V_CAT0 + spr), turns to face away from the wall (4 sprites), category, swatch colour.',
         'typedef struct { const char*nm; char ch; u16 price, spr; u8 rot, cat; u16 col; u8 lk; } CatItem;   // lk: the look byte (wallMap) of this item: its number among the items of its character',
         '#define CAT_CATS %d' % len(CATS),
         'static const char* const catCatNm[CAT_CATS]={%s};' % ','.join('"%s"' % n for n, a in CATS),
         'static const u8 catCatAdult[CAT_CATS]={%s};   // (1: grown-ups only)' % ','.join(str(a) for n, a in CATS),
         'static const CatItem catTab[CAT_N]={']
    spr = 0
    for it in ITEMS:
        L.append('  {"%s",\'%s\',%d,%d,%d,%d,RGB(%d,%d,%d),%d},' % (it['name'], it['ch'], it['price'], spr, it['rot'], it['cat'], *it['col'], it['lk'])); spr += 4 if it['rot'] else 1
    L.append('};')
    first = []; n = 0
    for c in range(len(CATS)): first.append(n); n += len([i for i in ITEMS if i['cat'] == c])
    first.append(n)
    L.append('static const u8 catFn[CAT_N]={%s};   // what R does at the item (counters.h): bit 0 a sink, bit 1 a microwave' % ','.join(str(it['fn']) for it in ITEMS))
    L.append('static const u16 catCatFirst[CAT_CATS+1]={%s};   // the items of category c are ids catCatFirst[c]+1 .. catCatFirst[c+1]' % ','.join(str(v) for v in first))
    # the look byte -> id decoder: per map character, its items in order (catIdOf in main.c)
    chs = sorted(set(it['ch'] for it in ITEMS)); idx = [255] * 128
    for k, c in enumerate(chs): idx[ord(c)] = k
    ids = []; cf = [0]
    for c in chs: ids += [i + 1 for i, it in enumerate(ITEMS) if it['ch'] == c]; cf.append(len(ids))
    L.append('#define CAT_CH_N %d' % len(chs))
    L.append('static const u8 catChIdx[128]={%s};   // map character -> its row in catChFirst (255: no catalog item behaves as it)' % ','.join(str(v) for v in idx))
    L.append('static const u16 catChFirst[CAT_CH_N+1]={%s};' % ','.join(str(v) for v in cf))
    L.append('static const u16 catChIds[CAT_N]={%s};   // the ids, grouped by character: look byte b of character c is catChIds[catChFirst[row]+b-1]' % ','.join(str(v) for v in ids))
    for n, a in CATS: L.append('#define CAT_C_%s %d' % (n.replace(' ', '_').replace('-', '_'), CATS.index((n, a))))
    assert spr == spr_n
    return '\n'.join(L) + '\n'

def main():
    cnt = {}
    for it in ITEMS: cnt[it['ch']] = cnt.get(it['ch'], 0) + 1; it['lk'] = cnt[it['ch']]; assert it['lk'] <= 255
    for it in ITEMS:   # (the kitchen's own sink and microwave work too)
        if it['name'] == 'KITCHEN SINK': it['fn'] = 1
        if it['name'] == 'MICROWAVE STAND': it['fn'] = 2
    bake, spr_n = emit_bake()
    open(os.path.join(ROOT, 'source', 'catalogart.h'), 'w').write(emit())
    open(os.path.join(ROOT, 'source', 'catalogbake.h'), 'w').write(bake)
    open(os.path.join(ROOT, 'source', 'catalogids.h'), 'w').write('// catalogids.h - GENERATED by tools/make_catalog.py\n#define CAT_N %d\n#define CAT_SPR_N %d\n' % (len(ITEMS), spr_n))
    open(os.path.join(ROOT, 'source', 'catalog.h'), 'w').write(emit_table(spr_n))
    # the items are sorted by category in the table: the builder's rows rely on it
    assert all(ITEMS[i]['cat'] <= ITEMS[i + 1]['cat'] for i in range(len(ITEMS) - 1))
    print('items', len(ITEMS), 'sprites', spr_n, 'categories', len(CATS), 'materials', len(MATS))

if __name__ == '__main__': main()

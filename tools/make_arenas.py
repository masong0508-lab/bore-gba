#!/usr/bin/env python3
"""make_arenas.py - the BOREZIES arena maps (source/arenas.h, docs/arenas/*.txt|png).

Each arena is a 40 x 40 BITMAP: one cell per map tile, and every cell says three things - the tile (a wall, an item of the catalogue,
open floor), the floor under it and, for walls and catalogue items, the wallpaper / look byte - plus one flag: a barrel or a canister
may be set down here. The maps are drawn below with a few rectangle helpers, written out as a text bitmap (docs/arenas/NAME.txt, one
character per cell, the legend at its end) and a PNG preview, and packed for the game into arenas.h: a table of the distinct cells
(ArSym) and, per arena, 1600 bytes of cell numbers. The game stands the arena up in place of your lot while a run lasts (arena.h),
scatters its barrels and canisters at random and puts your own lot back afterwards.

Run:  python3 tools/make_arenas.py
"""
import os, re, sys
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
W = H = 40

# ------------------------------------------------------------------ the game's own tables (floors, wallpapers, the catalogue)
def rd(p): return open(os.path.join(ROOT, p)).read()
FL = {}; FLC = []
s = rd('source/floorart.h'); s = s[s.index('flTex[NFL]'):]
for i, m in enumerate(re.finditer(r'^\s*\{"([^"]+)",\{RGB\((\d+),(\d+),(\d+)\)', s, re.M)):
    FL.setdefault(m.group(1), i); FLC.append((int(m.group(2)), int(m.group(3)), int(m.group(4))))
s = rd('source/wallpapers.h'); i2 = s.index('wp2Tex['); i1 = s.index('wpTex[')
WP1 = [m.group(1) for m in re.finditer(r'^\s*\{"([^"]+)"', s[i1:i2], re.M)]
WP2 = [m.group(1) for m in re.finditer(r'^\s*\{"([^"]+)"', s[i2:], re.M)]
WPC = [tuple(map(int, m.groups())) for m in re.finditer(r'^\s*\{"[^"]+",\{RGB\((\d+),(\d+),(\d+)\)', s[i1:], re.M)]
NWP, NWX = 14, 89
WCOL = {n: WPC[i] for i, n in enumerate(WP1 + WP2)}
def wall_id(name):
    if name in WP1: return WP1.index(name)
    return NWP + NWX + WP2.index(name)
CAT = {}   # name -> (char, lk, colour)
for m in re.finditer(r'\{"([A-Z0-9 \-\']+)",\'(.)\',(\d+),\d+,\d+,\d+,RGB\((\d+),(\d+),(\d+)\),(\d+)\}', rd('source/catalog.h')):
    CAT.setdefault(m.group(1), (m.group(2), int(m.group(7)), (int(m.group(4)), int(m.group(5)), int(m.group(6)))))

class Arena:
    def __init__(s, name, blurb, ground, scatter, count, kinds):
        s.name, s.blurb, s.scatter_n, s.kinds = name, blurb, count, kinds
        s.t = [['.'] * W for _ in range(H)]; s.f = [[FL[ground]] * W for _ in range(H)]; s.w = [[0] * W for _ in range(H)]
        s.z = [[0] * W for _ in range(H)]; s.c = [[None] * W for _ in range(H)]   # scatter flag, a colour for the preview
        s.gates = []
    def floor(s, x0, y0, x1, y1, name):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): s.f[y][x] = FL[name]
    def wall(s, x, y, style, tall=True):
        s.t[y][x] = 'W' if tall else 'w'; s.w[y][x] = wall_id(style); s.z[y][x] = 0; s.c[y][x] = WCOL[style]
    def box(s, x0, y0, x1, y1, style, tall=True):   # the outline of a rectangle
        for x in range(x0, x1 + 1): s.wall(x, y0, style, tall); s.wall(x, y1, style, tall)
        for y in range(y0, y1 + 1): s.wall(x0, y, style, tall); s.wall(x1, y, style, tall)
    def hline(s, x0, x1, y, style, tall=True):
        for x in range(x0, x1 + 1): s.wall(x, y, style, tall)
    def vline(s, x, y0, y1, style, tall=True):
        for y in range(y0, y1 + 1): s.wall(x, y, style, tall)
    def gap(s, x0, y0, x1, y1, fl=None):   # open a doorway / a gate
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                s.t[y][x] = '.'; s.w[y][x] = 0; s.z[y][x] = 0; s.c[y][x] = None
                if fl: s.f[y][x] = FL[fl]
    def put(s, x, y, name):   # one catalogue item
        ch, lk, col = CAT[name]; s.t[y][x] = ch; s.w[y][x] = lk; s.c[y][x] = col; s.z[y][x] = 0
    def zone(s, x0, y0, x1, y1, on=1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): s.z[y][x] = on
    def spawn(s, x, y): s.t[y][x] = 'P'
    def border(s, style, tall=False):
        s.box(0, 0, W - 1, H - 1, style, tall)
    def finish(s):
        for y in range(H):
            for x in range(W):   # a scatter cell: open, not within two of the edge, not water / air
                if s.t[y][x] != '.' or x < 3 or y < 3 or x > W - 4 or y > H - 4: s.z[y][x] = 0
        return s

def disc(cx, cy, r): return lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= r * r

# (the outer wall is never a see-through fence: behind it is the empty edge of the map)
# ------------------------------------------------------------------ 1 CUL-DE-SAC: a suburban street, a round road, four houses
def culdesac():
    a = Arena('CUL-DE-SAC', 'A SUBURBAN ROUND  FOUR ROADS IN', 'GRASS', 0, 12, ['RAIN BARREL', 'BLUE OIL DRUM', 'COMPOST BIN', 'TRAFFIC CONE', 'CARDBOARD BOX'])
    road = lambda x, y: disc(19.5, 19.5, 9.5)(x, y) or (17 <= x <= 22) or (17 <= y <= 22)
    for y in range(H):
        for x in range(W):
            if road(x, y): a.f[y][x] = FL['OIL STAINED']
    for y in range(H):
        for x in range(W):
            if not road(x, y) and any(road(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)): a.f[y][x] = FL['CONCRETE']
    a.floor(18, 18, 21, 21, 'GRASS'); a.floor(17, 17, 22, 22, 'GRASS'); a.floor(16, 18, 23, 21, 'GRASS'); a.floor(18, 16, 21, 23, 'GRASS')
    a.put(19, 19, 'OAK TREE'); a.put(20, 20, 'FLOWER POTS'); a.put(18, 18, 'ROSE BUSH'); a.put(21, 21, 'GARDEN GNOME'); a.put(21, 18, 'PINK FLAMINGO')
    for (x0, y0, brick) in ((3, 3, 'RED BRICK WALL'), (28, 3, 'WHITE BRICK'), (3, 28, 'WHITE BRICK'), (28, 28, 'RED BRICK WALL')):   # four houses (shut: nobody gets in, they are scenery)
        a.box(x0, y0, x0 + 8, y0 + 8, brick); a.floor(x0 + 1, y0 + 1, x0 + 7, y0 + 7, 'OAK PARQUET')
    for (x, y) in ((12, 12), (27, 12), (12, 27), (27, 27)): a.put(x, y, 'STREET LAMP')
    for (x, y) in ((2, 13), (2, 26), (37, 13), (37, 26), (13, 2), (26, 2), (13, 37), (26, 37)): a.put(x, y, 'HEDGE')
    for (x, y) in ((14, 14), (25, 14), (14, 25), (25, 25)): a.put(x, y, 'PARK BENCH')
    for y in (36, 37): a.put(16, y, 'TRAFFIC CONE'); a.put(23, y, 'TRAFFIC CONE')
    a.put(16, 33, 'RED BARRIER'); a.put(23, 33, 'RED BARRIER')
    a.border('HEDGE ROW', tall=False)
    a.gap(17, 39, 22, 39); a.gap(17, 0, 22, 0); a.gap(0, 17, 0, 22); a.gap(39, 17, 39, 22)
    a.spawn(19, 24)
    a.zone(5, 12, 34, 27); a.zone(12, 5, 27, 34)
    return a.finish()

# ------------------------------------------------------------------ 2 THE WINCHESTER: a pub with a bar, tables, a back room of kegs
def winchester():
    a = Arena('THE WINCHESTER', 'A PUB  KEGS  A STUCK JUKEBOX', 'CONCRETE', 0, 14, ['BEER KEG', 'BLUE OIL DRUM', 'CARDBOARD BOX', 'COOLER', 'BAR STOOL'])
    a.floor(2, 2, 37, 37, 'GRAVEL')
    a.floor(4, 5, 35, 34, 'WOOD PLANKS')
    a.box(3, 4, 36, 35, 'RED BRICK WALL')
    a.gap(18, 35, 21, 35, 'WOOD PLANKS')   # the front doors
    a.gap(36, 19, 36, 21, 'WOOD PLANKS')   # the back door
    a.gap(3, 19, 3, 21, 'WOOD PLANKS')     # a side door
    a.vline(29, 5, 12, 'WOOD PANEL'); a.hline(29, 35, 12, 'WOOD PANEL'); a.gap(31, 12, 32, 12, 'WOOD PLANKS')   # the back room
    a.floor(30, 5, 35, 11, 'DARK WALNUT')
    for x in range(5, 28): a.put(x, 9, 'BAR COUNTER') if x not in (14, 15) else None   # the bar, a gap to walk behind it
    for x in range(6, 28, 3): a.put(x, 11, 'BAR STOOL')
    for x in (5, 6, 7, 8): a.put(x, 6, 'WINE FRIDGE') if x % 2 == 0 else a.put(x, 6, 'BEER KEG')
    a.put(10, 6, 'JUKEBOX'); a.put(12, 6, 'NEON BEER SIGN')
    for (x, y) in ((8, 17), (14, 17), (25, 17), (31, 17), (8, 25), (14, 25), (25, 25), (31, 25)):
        a.put(x, y, 'ROUND TABLE'); a.put(x - 1, y, 'OAK CHAIR'); a.put(x + 1, y, 'OAK CHAIR')
    a.put(20, 22, 'POOL TABLE'); a.put(5, 28, 'DARTBOARD'); a.put(5, 31, 'POKER TABLE')
    for (x, y) in ((31, 6), (33, 6), (31, 9), (33, 9), (34, 7)): a.put(x, y, 'BEER KEG')
    a.border('LOW BRICK WALL', tall=False)
    a.gap(17, 39, 22, 39); a.gap(17, 0, 22, 0); a.gap(0, 17, 0, 22); a.gap(39, 17, 39, 22)
    a.spawn(19, 29)
    a.zone(5, 13, 28, 33); a.zone(30, 13, 35, 33)
    return a.finish()

# ------------------------------------------------------------------ 3 THE LAB: corridors, side rooms, canisters (the white tank is somewhere in here)
def lab():
    a = Arena('THE LAB', 'CANISTERS EVERYWHERE  MIND THE YELLOW', 'CONCRETE', 0, 18, ['GAS CANISTER', 'TOXIC BARREL', 'RED FUEL DRUM', 'GAS CANISTER'])
    a.floor(3, 3, 36, 36, 'WHITE TILE'); a.floor(4, 17, 35, 22, 'STEEL PLATE'); a.floor(4, 24, 35, 35, 'GREY CARPET')
    a.box(3, 3, 36, 36, 'METAL PANEL')
    a.hline(4, 35, 16, 'WHITE TILE WALL'); a.hline(4, 35, 23, 'WHITE TILE WALL')
    for x in (13, 26): a.vline(x, 4, 15, 'WHITE TILE WALL'); a.vline(x, 24, 35, 'WHITE TILE WALL')
    for (x0, x1) in ((8, 9), (19, 20), (30, 31)): a.gap(x0, 16, x1, 16, 'WHITE TILE'); a.gap(x0, 23, x1, 23, 'WHITE TILE')
    a.gap(3, 19, 3, 20, 'STEEL PLATE'); a.gap(36, 19, 36, 20, 'STEEL PLATE'); a.gap(18, 36, 21, 36, 'GREY CARPET')
    for (x, y) in ((6, 6), (6, 8), (6, 10), (16, 6), (18, 6), (22, 6), (29, 6), (29, 8), (31, 8), (33, 6)): a.put(x, y, 'LAB BENCH')
    for (x, y) in ((7, 28), (9, 28), (7, 31), (30, 28), (32, 28), (30, 31)): a.put(x, y, 'LAB BENCH')
    for x in range(15, 25): a.put(x, 33, 'RECEPTION DESK') if x % 3 else None
    a.put(5, 5, 'POTTED PALM'); a.put(34, 34, 'POTTED PALM'); a.put(5, 34, 'FICUS'); a.put(34, 5, 'FICUS')
    a.border('LOW STONE WALL', tall=False)
    a.gap(17, 39, 22, 39); a.gap(17, 0, 22, 0); a.gap(0, 17, 0, 22); a.gap(39, 17, 39, 22)
    a.spawn(19, 19)
    a.zone(5, 17, 34, 22); a.zone(14, 4, 25, 15); a.zone(14, 24, 25, 32); a.zone(5, 4, 12, 15); a.zone(27, 4, 35, 15); a.zone(5, 24, 12, 35); a.zone(27, 24, 35, 35)
    return a.finish()

# ------------------------------------------------------------------ 4 THE ARCADE: rows of cabinets round a dance floor (Patient Zero's)
def arcade():
    a = Arena('THE ARCADE', 'CABINETS  NEON  ONE HIGH SCORE', 'CONCRETE', 0, 12, ['CARDBOARD BOX', 'COOLER', 'FIREWORKS BOX', 'CARDBOARD BOX'])
    a.floor(2, 2, 37, 37, 'NEON GRID'); a.floor(15, 14, 24, 25, 'DANCE FLOOR'); a.floor(4, 30, 11, 35, 'CASINO CARPET')
    a.box(2, 2, 37, 37, 'CIRCUIT')
    a.gap(18, 37, 21, 37, 'NEON GRID'); a.gap(2, 18, 2, 21, 'NEON GRID'); a.gap(37, 18, 37, 21, 'NEON GRID')
    for x in range(4, 36): a.put(x, 3, 'ARCADE CABINET') if x % 4 != 0 else a.put(x, 3, 'SLOT MACHINE')
    for (x0, x1, y) in ((7, 13, 10), (26, 32, 10), (7, 13, 29), (26, 32, 29)):
        for x in range(x0, x1 + 1): a.put(x, y, 'ARCADE CABINET'); a.put(x, y + 1, 'ARCADE CABINET')
    for y in range(7, 34): a.put(4, y, 'ARCADE CABINET') if y not in range(17, 23) and y % 3 else None
    for y in range(7, 34): a.put(35, y, 'ARCADE CABINET') if y not in range(17, 23) and y % 3 else None
    for x in range(5, 11): a.put(x, 31, 'RECEPTION DESK') if x % 2 else None
    a.put(19, 13, 'DJ BOOTH'); a.put(20, 13, 'JUKEBOX'); a.put(16, 26, 'BALL PIT')
    a.border('LOW STONE WALL', tall=False)
    a.gap(17, 39, 22, 39); a.gap(17, 0, 22, 0); a.gap(0, 17, 0, 22); a.gap(39, 17, 39, 22)
    a.gap(20, 38, 21, 38); a.gap(18, 38, 19, 38)
    a.spawn(19, 30)
    a.zone(6, 13, 33, 26); a.zone(14, 27, 25, 34)
    return a.finish()

# ------------------------------------------------------------------ 5 THE SCRAPYARD: dirt, a crate fort, barrels by the dozen
def scrapyard():
    a = Arena('THE SCRAPYARD', 'MUD  RUST  A CRATE FORT', 'DIRT', 0, 24, ['BLUE OIL DRUM', 'RED FUEL DRUM', 'TOXIC BARREL', 'CARDBOARD BOX', 'GAS CANISTER', 'RAIN BARREL'])
    for y in range(H):
        for x in range(W):
            if (x * 7 + y * 13) % 17 == 0 or disc(8, 30, 4)(x, y) or disc(31, 9, 3.5)(x, y): a.f[y][x] = FL['MUD']
            elif abs(x - 19.5) < 2.5 or abs(y - 19.5) < 2.5: a.f[y][x] = FL['GRAVEL']
    a.box(15, 15, 24, 24, 'CORRUGATED'); a.gap(19, 15, 20, 15); a.gap(19, 24, 20, 24); a.gap(15, 19, 15, 20); a.gap(24, 19, 24, 20)   # the crate fort
    a.floor(16, 16, 23, 23, 'STEEL PLATE')
    for (x, y) in ((17, 17), (22, 17), (17, 22), (22, 22)): a.put(x, y, 'CARDBOARD BOX')
    a.hline(5, 12, 8, 'CORRUGATED'); a.vline(5, 8, 12, 'CORRUGATED')   # two bunkers
    a.hline(27, 34, 31, 'CORRUGATED'); a.vline(34, 27, 31, 'CORRUGATED')
    for (x, y) in ((8, 20), (30, 20), (20, 7), (20, 33), (12, 12), (28, 28), (12, 28), (28, 12)): a.put(x, y, 'BOULDER')
    for (x, y) in ((3, 4), (35, 35), (4, 34), (35, 4)): a.put(x, y, 'DEAD TREE')
    a.border('CORRUGATED', tall=False)
    for (x0, y0, x1, y1) in ((17, 39, 22, 39), (17, 0, 22, 0), (0, 17, 0, 22), (39, 17, 39, 22), (0, 5, 0, 7), (39, 32, 39, 34)): a.gap(x0, y0, x1, y1)
    a.spawn(19, 27)
    a.zone(4, 4, 35, 35)
    return a.finish()

# ------------------------------------------------------------------ 6 THE CAR PARK: bays, lamp posts, the mall front
def carpark():
    a = Arena('THE CAR PARK', 'BAYS  LAMPS  THE MALL FRONT', 'OIL STAINED', 0, 16, ['RED FUEL DRUM', 'BLUE OIL DRUM', 'TRAFFIC CONE', 'PROPANE TANK', 'COOLER'])
    for y in range(H):
        for x in range(W):
            if (y // 2) % 3 == 1 and 9 <= y <= 36 and 3 <= x <= 36: a.f[y][x] = FL['CONCRETE']
    a.box(3, 2, 36, 8, 'WHITE BRICK'); a.floor(4, 3, 35, 7, 'MARBLE')
    a.gap(9, 8, 10, 8, 'MARBLE'); a.gap(19, 8, 20, 8, 'MARBLE'); a.gap(29, 8, 30, 8, 'MARBLE')
    for x in range(6, 34, 4): a.put(x, 4, 'POTTED PALM')
    for (x, y) in ((8, 14), (20, 14), (32, 14), (8, 26), (20, 26), (32, 26), (14, 20), (26, 20)): a.put(x, y, 'STREET LAMP')
    for x in range(4, 36, 2): a.put(x, 10, 'BOLLARD')
    for x in (12, 13, 26, 27): a.put(x, 31, 'RED BARRIER')
    a.border('LOW STONE WALL', tall=False)
    a.gap(17, 39, 22, 39); a.gap(0, 17, 0, 22); a.gap(39, 17, 39, 22); a.gap(5, 39, 6, 39); a.gap(33, 39, 34, 39)
    a.spawn(19, 30)
    a.zone(5, 11, 34, 35)
    return a.finish()

MAPS = [culdesac(), winchester(), lab(), arcade(), scrapyard(), carpark()]
for m in MAPS:   # the one 'P' of a map must have its eight neighbours open
    ps = [(x, y) for y in range(H) for x in range(W) if m.t[y][x] == 'P']
    assert len(ps) == 1, (m.name, ps)

# ------------------------------------------------------------------ the symbols: every distinct cell of every map
def cellcol(m, x, y):
    t = m.t[y][x]
    if m.c[y][x]: return m.c[y][x]
    if t == 'P': return (31, 31, 8)
    return FLC[m.f[y][x]]
SYM = []; CELLS = []
for m in MAPS:
    row = []
    for y in range(H):
        for x in range(W):
            key = (m.t[y][x], m.f[y][x], m.w[y][x], m.z[y][x], cellcol(m, x, y))
            if key not in SYM: SYM.append(key)
            row.append(SYM.index(key))
    CELLS.append(row)
assert len(SYM) <= 255, len(SYM)

# ------------------------------------------------------------------ the text bitmap and the PNG preview of each
def preview(m, path):
    try:
        from PIL import Image
    except ImportError:
        return
    sc = 8; im = Image.new('RGB', (W * sc, H * sc))
    for y in range(H):
        for x in range(W):
            r, g, b = cellcol(m, x, y); col = (r * 8, g * 8, b * 8)
            if m.z[y][x]: col = tuple(min(255, c + 24) for c in col)
            for dy in range(sc):
                for dx in range(sc):
                    e = dx == 0 or dy == 0
                    im.putpixel((x * sc + dx, y * sc + dy), tuple(int(c * 0.82) for c in col) if e and m.t[y][x] not in 'Ww' else col)
    im.save(path)
os.makedirs(os.path.join(ROOT, 'docs', 'arenas'), exist_ok=True)
for k, m in enumerate(MAPS):
    letters = {}
    lines = []
    for y in range(H):
        ln = ''
        for x in range(W):
            t = m.t[y][x]
            ln += t if t != '.' else (':' if m.z[y][x] else '.')
        lines.append(ln)
    fn = 'ARENA%d_%s' % (k + 1, m.name.replace(' ', '_').replace('-', '_'))
    open(os.path.join(ROOT, 'docs', 'arenas', fn + '.txt'), 'w').write(
        '%s - %s\n' % (m.name, m.blurb) + '\n'.join(lines) + '\n\n. open floor   : open floor where a barrel or canister may land   W wall   w fence   P you start here\n'
        'every other letter is a catalogue item (the game stands the real item there); the floor and wallpaper per cell are in arenas.h\n')
    preview(m, os.path.join(ROOT, 'docs', 'arenas', fn + '.png'))

# ------------------------------------------------------------------ arenas.h
out = ['// arenas.h - GENERATED by tools/make_arenas.py: the BOREZIES arena maps (arena.h stands one up). Do not edit.',
       '#define AR_N %d' % len(MAPS),
       '// one cell: the tile, the floor, the wallpaper / look byte, 1 = a barrel or canister may be set down here, the colour of its dot on the map picker',
       'typedef struct { char t; u8 fl, wl, sc; u16 col; } ArSym;',
       'static const ArSym arSym[%d]={' % len(SYM)]
for (t, fl, wl, z, c) in SYM:
    ch = "'\\''" if t == "'" else ("'\\\\'" if t == '\\' else "'%s'" % t)
    out.append(' {%s,%d,%d,%d,RGB(%d,%d,%d)},' % (ch, fl, wl, z, c[0], c[1], c[2]))
out.append('};')
out.append('static const char* const arNm[AR_N]={%s};' % ','.join('"%s"' % m.name for m in MAPS))
out.append('static const char* const arBl[AR_N]={%s};' % ','.join('"%s"' % m.blurb for m in MAPS))
out.append('static const u8 arCount[AR_N]={%s};   // barrels and canisters set down per run' % ','.join(str(m.scatter_n) for m in MAPS))
KINDS = []   # (tile, look)
for m in MAPS:
    for k in m.kinds:
        ch, lk, _ = CAT[k]
        if (ch, lk) not in KINDS: KINDS.append((ch, lk))
out.append('static const struct { char t; u8 lk; } arKind[%d]={%s};' % (len(KINDS), ','.join("{'%s',%d}" % kk for kk in KINDS)))
out.append('static const u8 arKinds[AR_N][6]={%s};   // which kinds each arena draws from (arKind indices; 255 ends the list)' % ','.join(
    '{' + ','.join(str(KINDS.index(CAT[k][:2])) for k in m.kinds[:6]) + ',255' * (6 - len(m.kinds[:6])) + '}' for m in MAPS))
tank = CAT['PROPANE TANK'][:2]
out.append('#define AR_TANK_T \'%s\'\n#define AR_TANK_LK %d   // the white tank of THE LAB' % tank)
out.append('static const u8 arCell[AR_N][%d]={' % (W * H))
for k, row in enumerate(CELLS):
    out.append(' {  // %s' % MAPS[k].name)
    for y in range(H): out.append('  ' + ','.join(str(v) for v in row[y * W:(y + 1) * W]) + ',')
    out.append(' },')
out.append('};')
open(os.path.join(ROOT, 'source', 'arenas.h'), 'w').write('\n'.join(out) + '\n')
print('arenas', len(MAPS), 'symbols', len(SYM), 'kinds', len(KINDS))

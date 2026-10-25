#!/usr/bin/env python3
"""make_king_art.py - THE LAND BOREFORE TIME's art: terrain tiles, buildings, the four giants, the Nubbins, icons.  Writes source/kingart.h (and preview sheets in docs/king_art/).
usage (from the repo root):  python3 tools/make_king_art.py          needs Pillow only for the previews (--no-preview skips them)
The map is drawn with the GBA's hardware tile background: one realm cell is 16 x 16 pixels, 2 x 2 tiles.  Palette banks of the BG: 0 LAND, 1 SEA, 2..14 the buildings, 15 the text.
Sprites (OBJ, 16 colour banks): 0 the giant, 1..4 the Nubbins' clothes, 5 goods icons, 6 smoke, 7 the cursor.   Everything here is deterministic."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, os.path.join(HERE, 'kingart'))
from pix import Cv, sheet
import terrain as T, bld as B, spr as S
def pack(c, x0=0, y0=0):   # an 8 x 8 tile of palette indices -> 32 bytes, low nibble first
    out = []
    for y in range(8):
        for x in range(0, 8, 2): out.append((c.p[y0 + y][x0 + x] & 15) | ((c.p[y0 + y][x0 + x + 1] & 15) << 4))
    return out
tiles = []; names = {}
def add(name, cv_list): names[name] = len(tiles); tiles.extend(cv_list)
def quads(c): return [pack(c, 0, 0), pack(c, 8, 0), pack(c, 0, 8), pack(c, 8, 8)]   # a 16 x 16 cell: TL TR BL BR
def blocks(c): return [pack(c, 8 * x, 8 * y) for y in range(c.h // 8) for x in range(c.w // 8)]   # row by row
T_ = []   # (name, [tile byte lists])
def addb(name, lists): names[name] = len(tiles); tiles.extend(lists)
addb('GRASS', [pack(T.grass_tile(i)) for i in range(8)])
addb('MEADOW', [pack(T.meadow_tile(i)) for i in range(8)])
addb('FOREST', sum([quads(f(100 + k)) for k, f in enumerate((T.tree_round, T.tree_pine, T.tree_twin, T.bush_trees))], []))
addb('ROCK', sum([quads(T.rock_cell(i)) for i in range(3)], []))
addb('CRYSTAL', sum([quads(T.crystal_cell(i)) for i in range(2)], []))
addb('DEEP', [pack(T.deep_tile(i)) for i in range(4)])
addb('WATER', [pack(T.water_tile(q & 1, q >> 1, m, 0)) for q in range(4) for m in range(8)])
water_b = [pack(T.water_tile(q & 1, q >> 1, m, 1)) for q in range(4) for m in range(8)]   # the second animation frame (copied over the first by the game)
addb('ROAD', [pack(T.road_tile(q & 1, q >> 1, hv, var)) for q in range(4) for hv in range(4) for var in range(2)])
addb('BRIDGE', sum([quads(T.bridge_cell(v)) for v in (0, 1)], []))
# the building site, in the LAND palette
def site_land():
    c = Cv(32, 32, 0); import random
    r = random.Random(14)
    for y in range(32):
        for x in range(32): c.p[y][x] = T.LG2 if r.random() < .06 else T.LG4 if r.random() > .93 else T.LG3
    c.ell(16, 19, 13.5, 9, lambda x, y, nx, ny: T.DI_M if (x * 7 + y * 13) % 6 else T.DI_L if (x + y) % 3 else T.DI_D, only={T.LG2, T.LG3, T.LG4})
    for (x, y) in ((6, 12), (25, 12), (6, 25), (25, 25)):
        c.rect(x, y - 5, 2, 8, T.TR_D); c.px(x, y - 5, T.TR_L); c.vline(x, y - 4, y + 2, T.TR_L)
    for y, col in ((8, T.TR_L), (9, T.TR_D), (21, T.TR_L), (22, T.TR_D)): c.hline(6, 26, y, col)
    c.line(6, 22, 14, 9, T.TR_D); c.line(25, 22, 17, 9, T.TR_D)
    for (x, y, w) in ((9, 26, 8), (19, 27, 7)): c.rect(x, y, w, 2, T.DI_L); c.hline(x, x + w - 1, y, T.WHITE); c.hline(x, x + w - 1, y + 2, T.DI_D)
    for (x, y) in ((11, 15), (17, 17)): c.rect(x, y, 5, 3, T.DI_L); c.hline(x, x + 4, y, T.WHITE); c.vline(x + 4, y, y + 2, T.DI_D)
    c.vline(16, 3, 8, T.TR_D); c.rect(17, 3, 4, 2, T.RED)
    return c
addb('SITE', blocks(site_land()))
order = ['hut', 'house', 'wcut', 'quarry', 'saw', 'mason', 'store', 'sheep', 'mine', 'weaver', 'cutter', 'well', 'tavern']   # (types 0..13 without the castle, 11)
bank_of = {'hut': 2, 'house': 3, 'wcut': 4, 'quarry': 5, 'saw': 6, 'mason': 7, 'cutter': 7, 'store': 8, 'sheep': 9, 'mine': 10, 'weaver': 11, 'castle': 12, 'well': 13, 'tavern': 14}
palkey = {'mason': 'mc', 'cutter': 'mc'}
for n in order: addb('B_' + n.upper(), blocks(B.BUILD[n]()))
for st in range(5): addb('CASTLE%d' % st, blocks(B.castle(st)))
NT = len(tiles)
banks = [list(T.LAND), list(T.SEA)] + [None] * 13 + [[0, 0x7FFF, 0x0421] + [0] * 13]
for n, b in bank_of.items(): banks[b] = B.PAL[palkey.get(n, n)]
assert all(b is not None for b in banks)
# ---- sprites ----
def obj_tiles(c): return [pack(c, 8 * x, 8 * y) for y in range(c.h // 8) for x in range(c.w // 8)]   # 1D mapping, row by row
giant_tiles = [sum([obj_tiles(f) for f in S.giant_frames(g)], []) for g in S.GIANTS]      # per giant: 9 frames x 32 tiles
nub_tiles = sum([obj_tiles(f) for f in S.NUBF], [])                                          # 6 frames x 2 tiles
good_tiles = sum([obj_tiles(g) for g in S.GOODS], [])
smoke_tiles = sum([obj_tiles(S.smoke(i)) for i in range(4)], [])
cur_tiles = obj_tiles(S.cursor())
crowd_tiles = sum([obj_tiles(S.CROWD[f][k]) for f in (0, 1) for k in range(3)], [])   # frame 0: group, crowd, throng (4 + 8 + 16 tiles), then frame 1
objpal = [S.GIANTS[0]['pal']] + S.NUB_LOOKS + [S.GOODPAL, S.SMOKEPAL, S.CURPAL, S.CROWDPAL]
def carr(name, rows, ctype='u8'):
    o = 'static const %s %s[%d][32]={\n' % (ctype, name, len(rows))
    o += ''.join(' {%s},\n' % ','.join('%d' % v for v in r) for r in rows) + '};\n'; return o
def parr(name, pals):
    return 'static const u16 %s[%d][16]={\n' % (name, len(pals)) + ''.join(' {%s},\n' % ','.join('0x%04X' % v for v in p) for p in pals) + '};\n'
h = ['// kingart.h - GENERATED by tools/make_king_art.py: the art of THE LAND BOREFORE TIME (EP6). Do not edit.',
     '// The map is the GBA hardware tile background: a cell is 16 x 16 pixels = 2 x 2 tiles (8 x 8, 4 bits a pixel, low nibble first).',
     '// BG palette banks: 0 LAND, 1 SEA, 2 hut, 3 house, 4 woodcutter, 5 quarry, 6 sawmill, 7 mason and cutter, 8 storehouse, 9 sheep pen, 10 mine, 11 weaver, 12 castle, 13 well, 14 tavern, 15 the text.',
     '// OBJ palette banks: 0 the giant, 1..4 the Nubbins\' clothes, 5 goods, 6 smoke, 7 the cursor, 8 the crowds (kaCrowd: 56 tiles: two frames of a group 16 x 16, a crowd 32 x 16, a throng 32 x 32).', '']
h.append('#define KA_NT %d     // map tiles in all (the text tiles come after them)' % NT)
for k, v in names.items(): h.append('#define KA_%s %d' % (k, v))
h.append('// a building type 0..13 -> its first tile (16 tiles: the 4 x 4 grid, row by row) and its palette bank; the castle (type 11) has five looks: KA_CASTLE0..4')
h.append('static const u16 kaBldTile[14]={%s};' % ','.join(str(names['B_' + {0: 'hut', 1: 'house', 2: 'wcut', 3: 'quarry', 4: 'saw', 5: 'mason', 6: 'store', 7: 'sheep', 8: 'mine', 9: 'weaver', 10: 'cutter', 11: 'hut', 12: 'well', 13: 'tavern'}[t].upper()]) for t in range(14)))
h.append('static const u8 kaBldBank[14]={%s};' % ','.join(str(bank_of[{0: 'hut', 1: 'house', 2: 'wcut', 3: 'quarry', 4: 'saw', 5: 'mason', 6: 'store', 7: 'sheep', 8: 'mine', 9: 'weaver', 10: 'cutter', 11: 'castle', 12: 'well', 13: 'tavern'}[t]]) for t in range(14)))
h.append(carr('kaTile', tiles)); h.append(carr('kaWaterB', water_b)); h.append(parr('kaPal', banks))
h.append('#define KA_GIANT_FRAMES 9   // 32 x 64 sprites: stand, step, step facing down (0-2), right (3-5), up (6-8); 32 tiles each')
h.append(carr('kaGiant0', giant_tiles[0]).replace('[32]={', '[32]={', 1)) if False else None
for gi, gt in enumerate(giant_tiles): h.append(carr('kaGiant%d' % gi, gt))
h.append('static const u8* const kaGiants[4]={kaGiant0[0],kaGiant1[0],kaGiant2[0],kaGiant3[0]};')
h.append(carr('kaNub', nub_tiles)); h.append(carr('kaGood', good_tiles)); h.append(carr('kaSmoke', smoke_tiles)); h.append(carr('kaCursor', cur_tiles)); h.append(carr('kaCrowd', crowd_tiles))
h.append(parr('kaObjPal', objpal))
h.append(parr('kaGiantPal', [g['pal'] for g in S.GIANTS]))   # the four giants' own OBJ palette (bank 0)
out = '\n'.join(x for x in h if x is not None) + '\n'
open(os.path.join(HERE, '..', 'source', 'kingart.h'), 'w').write(out)
print('kingart.h: %d map tiles (%d B), %d bytes of source' % (NT, NT * 32, len(out)))
if '--no-preview' not in sys.argv:
    os.makedirs(os.path.join(HERE, '..', 'docs', 'king_art'), exist_ok=True)
    pr = os.path.join(HERE, '..', 'docs', 'king_art')
    for n in order: sheet([B.BUILD[n]()], B.PAL[palkey.get(n, n)], os.path.join(pr, 'b_%s.png' % n), 4, 1)
    for st in range(5): sheet([B.castle(st)], B.PAL['castle'], os.path.join(pr, 'castle%d.png' % st), 4, 1)
    for k, nm in enumerate(('group', 'crowd', 'throng')): sheet([S.CROWD[0][k], S.CROWD[1][k]], S.CROWDPAL, os.path.join(pr, 'crowd_%s.png' % nm), 6, 2)
    sheet([n for n in S.NUBF], S.NUB_LOOKS[0], os.path.join(pr, 'nubbins.png'), 6, 6)

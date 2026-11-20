# generates source/icons.h: 10x10 power icons (by power kind) and 8x8 stat icons; characters map to UI palette indices (see ui.h)
PAL = {'.': 0, 'w': 6, 'e': 7, 'g': 8, 'n': 9, 'r': 10, 'b': 11, 'p': 12, 'o': 13, 'y': 24, 'c': 25, 's': 26, 'l': 27, 'd': 28, 'm': 29, 'k': 15, 'i': 30}
P10 = {  # by power kind (see PK_ enum order in main.c)
 'INSPECT': ["..wwww....", ".w....w...", "w......w..", "w......w..", "w......w..", ".w....w...", "..wwwwww..", ".......ww.", "........ww", ".........w"],
 'SPAWN':   ["..........", "...nn.....", "..nnnn.n..", "...nn.nn..", "....nnn...", "...nn.....", "...n......", "..sss.....", ".ssssss...", ".........."],
 'RAIN':    ["...llll...", "..llllll..", ".llllllll.", ".llllllll.", "..........", "..c..c..c.", "..c..c..c.", ".c..c..c..", ".c..c..c..", "..........", ],
 'FIRE':    ["....o.....", "....oo....", "...ooo.o..", "..oooooo..", "..ooyooo..", ".ooyyyoo..", ".ooyyyoo..", ".oooyyoo..", "..oooooo..", "...oooo..."],
 'BOLT':    ["....yyyy..", "...yyy....", "..yyy.....", "..yyyyyy..", "....yyy...", "....yy....", "...yy.....", "...y......", "..y.......", ".........."],
 'METEOR':  [".......oo.", "......ooyo", ".....oyyoo", "..ssssoo..", ".sssssso..", ".ssdsss...", ".sssssd...", "..ssss....", "o.........", ".o........"],
 'SMITE':   ["...wwww...", "..wwwwww..", "..wk..kw..", "..wwwwww..", "...wwww...", "..w.ww.w..", "..........", "..r.r.r...", "...r.r....", "..........", ],
 'HEAL':    ["..rr..rr..", ".rrrrrrrr.", ".rrwwrrrr.", ".rrrrrrrr.", "..rrrrrr..", "...rrrr...", "....rr....", "..........", "..........", ".........."],
 'FEED':    ["......n...", ".....nn...", "..rrrrrr..", ".rrwrrrrr.", ".rwrrrrrr.", ".rrrrrrrr.", ".rrrrrrrr.", "..rrrrrr..", "...rr.rr..", ".........."],
 'SETTLE':  ["....r.....", "...rrr....", "..rrrrr...", ".rrrrrrr..", "rrrrrrrrr.", ".wwwwwww..", ".ww.ss.ww.", ".ww.ss.ww.", ".wwwwwwww.", ".........."],
 'WAR':     ["w.......w.", ".w.....w..", "..w...w...", "...w.w....", "....w.....", "...w.w....", "..w...w...", ".yw.....wy", "y.........", ".........."],
 'PEACE':   ["......ww..", ".....wwww.", "...wwwwww.", "..wwwwwww.", ".wwwwww...", "ywwww.....", ".yww......", "..n.n.....", ".nn.nn....", "..n..n...."],
 'BLESS':   ["....yy....", "....yy....", "..yyyyyy..", "yyyyyyyyyy", ".yyyyyyyy.", "..yyyyyy..", "..yyyyyy..", ".yyy..yyy.", ".yy....yy.", ".y......y."],
 'GIFT':    ["..rr..rr..", "...rrrr...", "..gggggg..", ".gggrrggg.", ".gggrrggg.", ".gggrrggg.", ".ggrrrrgg.", ".gggrrggg.", ".gggggggg.", ".........."],
 'QUAKE':   ["..........", "...s...s..", "..ss..ss..", "..s..ds...", "sss.dss.ss", "...dd.s...", "..s.ds.ss.", ".ss..s....", "ss....ss..", ".........."],
 'FLOOD':   ["..........", "..cc......", ".cccc..cc.", "cc..cccccc", "..........", ".cc...cc..", "cccc.cccc.", "..cccc..cc", "..........", "cccccccccc"],
 'BLOOM':   ["...rr.....", "..rrrr....", "...rr.yy..", "....n.yyy.", "...nn..y..", "..nnnn.n..", ".nn.nn.n..", "....n.nn..", "....n.....", "..........",],
 'VIBE':    ["..........", "...pp.....", "..p..p....", "...pp.p...", "....pp....", "...p..p...", "..pp.pp...", "...pppp...", "....pp....", ".........."],
 'MUTATE':  ["..p....p..", "..pp..pp..", "...pppp...", "....pp....", "...pppp...", "..pp..pp..", "..p....p..", "..pp..pp..", "...pppp...", "....pp...."],
 # the Calamity pack (v0.14): see calamity.h
 'TORNADO': ["lllllllll.", ".sslllsss.", "..lssssl..", "...lllsl..", "..sslll...", "...slls...", "....lss...", ".....ls...", "....sl....", "...s......"],
 'ACID':    ["..nnnn....", ".nnnnnnn..", "nnnnnnnnn.", ".nnnnnnn..", "..........", "..y..y..y.", "..n..n..n.", ".n..n..n..", ".y..y..y..", ".........."],
 'HOLE':    ["...wwww...", "..wddddw..", ".wddppddw.", ".wdpddpdw.", ".wdpddpdw.", ".wddppddw.", "..wddddw..", "...wwww...", "..........", ".........."],
 'NUKE':    ["...oooo...", "..oyyyyo..", ".ooyyyyoo.", "..oooooo..", "....oo....", "....oo....", "...oooo...", "..oooooo..", ".sssssssss", ".........."],
 'PLAGUE':  [".n..nn..n.", "..nnnnnn..", ".nnnnnnnn.", "nnnwnnwnnn", ".nnnnnnnn.", "nnnnwwnnnn", ".nnnnnnnn.", "..nnnnnn..", ".n..nn..n.", ".........."],
 'CURSE':   ["..pppppp..", ".pppppppp.", "ppdppppdpp", "ppdppppdpp", "pppppppppp", ".pppppppp.", "..pp..pp..", "..pppppp..", "..p.pp.p..", ".........."],
}
# Redrawn silhouettes for the most-used tools and distinct field-work sprites.
# Colors are shared with the HUD, so these also read well at DS scale.
P10.update({
 'INSPECT': "..illli... .iccccli.. icw..ccli. ic....cli. ic....cli. .ic..cli.. ..illii... .....isi.. ......isi. .......is.",
 'SPAWN':   "...yy..... ..ylls.... ..lwwd.... ...ll....i ..ibbi..in .ibccbi..i .ibbbbi... ...bb..... ..bd.db... ..dd.dd...",
 'RAIN':    "....l..... ..leel.... .lelllel.. wlellllld. .lllllld.. ..c..c..c. ..b..c..c. .c..b..b.. .b..b..... ..........",
 'FIRE':    ".....r.... ....ro.... ...ro..r.. ...ro.rr.. ..rooory.. .roooyor.. .rooyyor.. .roywwor.. ..rooor... ...rrr....",
 'BOLT':    ".....wyyo. ....wyyo.. ...wyyo... ..wyyyyo.. .oyyyyo... ...oyyo... ..oyyo.... ..oyo..... .oy....... o.........",
 'HEAL':    "..rr..rr.. .rmmrrmmr. rmwmrrmmmr rmmrwwmmmr rrrwwwwrrr .rrrwwrrr. ..rrrrrr.. ...rrrr... ....rr.... .....r....",
 'FEED':    "......n... .....ni... ..gogig... .goyyooog. goyysoooog gooysoooog gooyooooog .goooosog. ..gosssg.. ...gggg...",
 'SETTLE':  ".....g.... ....ogo... ...oggo... ..oggoos.. .oggoooos. oggoooooss .ollslls.. .olcslos.. .ollslos.. iiiiiiiiii",
 'WAR':     "w....d...w .w..d...l. ..wl...l.. ...wldl... ....ww.... ...lwll... .gll..llg. ..g....g.. og......go o........o",
 'BLOOM':   "...mwm.... ..mwwrm... ..mrorm... ...rr.m... ..iinii... .nnininn.. ..niini... ....in.... ...nni.... ..iiiinn..",
 'FIELD':   "....ww.... ...wllw... .issllssi. .syyyyyys. .sylllyys. .sylssyys. .syllllls. .syyyylys. .ssssssss. ..........",
 'SURVEY':  "....ll.... ..ssiiss.. .syywwyys. .syddyys.. .sysysys.. .syyysys.. .sycysys.. .syyyysl.. .sssssssi. ..........",
 'RATIONS': "...sggs... ....os.... ...sggs... ..sgygos.. .sgyyogos. .sgyygos.. .sgoygos.. .soogoss.. ..ssssss.. ..........",
 'CLINIC':  "...lwwl... ...lddl... .lrrrrrrl. lrrmrwmrrl lrmmwwwmrl lrwwwwwwrl lrrrwrrrrl .lrrrrrrl. ..llllll.. ..........",
 'RESTORE': "...nii.... ..nwnii... .nwnnni... ..nini..w. ...ii..wc. ...ni.wcd. ..nio.ocd. .ssoosdd.. ssssss.... iiiiiiii..",
})
P10 = {name: rows.split() if isinstance(rows, str) else rows for name, rows in P10.items()}


def detailed_icon(rows):
    """Draw a larger pixel contour and bevel offline for the 3DS toolbar."""
    def at(x, y):
        return rows[y][x] if 0 <= x < 10 and 0 <= y < 10 else '.'

    pixels = [['.'] * 20 for _ in range(20)]
    for y in range(10):
        for x in range(10):
            up, left, center, right, down = (at(x, y-1), at(x-1, y), at(x, y),
                                              at(x+1, y), at(x, y+1))
            for dx, dy, side, vertical in ((0,0,left,up), (1,0,right,up),
                                            (0,1,left,down), (1,1,right,down)):
                color = side if side == vertical and up != down and left != right else center
                pixels[y*2+dy][x*2+dx] = color
    highlight = {'s':'g', 'o':'y', 'r':'m', 'n':'i', 'i':'c', 'b':'c', 'p':'m', 'd':'l'}
    shade = {'c':'b', 'y':'o', 'g':'s', 'l':'d', 'w':'l', 'm':'r', 'n':'i', 's':'d'}
    def large(x, y):
        return pixels[y][x] if 0 <= x < 20 and 0 <= y < 20 else '.'

    result = []
    for y in range(20):
        row = []
        for x in range(20):
            color = pixels[y][x]
            if color != '.':
                if large(x,y+1) == '.' and large(x-1,y) != '.':
                    color = shade.get(color, color)
                elif large(x,y-1) == '.' and large(x+1,y) != '.':
                    color = highlight.get(color, color)
            row.append(color)
        result.append(''.join(row))
    return result


P8 = {
 'LIFE':  [".rr..rr.", "rrrrrrrr", "rrrrrrrr", "rrrrrrrr", ".rrrrrr.", "..rrrr..", "...rr...", "........"],
 'HOME':  ["...oo...", "..oooo..", ".oooooo.", "oooooooo", ".wwwwww.", ".ww..ww.", ".ww..ww.", "........"],
 'FLAG':  ["sgggg...", "sgggg...", "sgggg...", "s.......", "s.......", "s.......", "s.......", "ss......"],
 'FOLK':  ["..ww....", "..ww....", ".wwww...", "wwwwww..", ".w..w...", ".w..w...", "........", "........"],
 'SKULL': ["..wwww..", ".wwwwww.", "wk.ww.kw", "wwwwwwww", ".wwwwww.", "..w..w..", "..wwww..", "........"],
 'STAR':  ["...yy...", "...yy...", "yyyyyyyy", ".yyyyyy.", "..yyyy..", ".yy..yy.", "yy....yy", "........"],
 'NOTE':  ["...cccc.", "...c..c.", "...c..c.", "...c..c.", "ccc.ccc.", "ccc.ccc.", "........", "........"],
 'SUN':   ["y..yy..y", ".yyyyyy.", ".yyyyyy.", "yyyyyyyy", "yyyyyyyy", ".yyyyyy.", ".yyyyyy.", "y..yy..y"],
 'MOON':  ["..wwww..", ".ww.....", "ww......", "ww......", "ww......", ".ww.....", "..wwww..", "........"],
 'LEFT':  ["....w...", "...ww...", "..www...", ".wwww...", "..www...", "...ww...", "....w...", "........"],
 'UP':    ["...ww...", "..wwww..", ".wwwwww.", "wwwwwwww", "...ww...", "...ww...", "........", "........"],
 'DOWN':  ["........", "........", "...ww...", "...ww...", "wwwwwwww", ".wwwwww.", "..wwww..", "...ww..."],
 'RIGHT': ["...w....", "...ww...", "...www..", "...wwww.", "...www..", "...ww...", "...w....", "........"],
}
out = ['// icons.h - GENERATED by compat/gen_icons.py: power icons (10x10, by power kind) and stat icons (8x8); entries are UI palette indices', '']
def emit(name, d, n):
    keys = list(d.keys()); out.append('static const unsigned char %s[%d][%d]={' % (name, len(keys), n * n))
    for k in keys:
        rows = d[k]; assert len(rows) == n, (k, len(rows))
        vals = []
        for r in rows:
            assert len(r) == n, (k, r, len(r)); vals += [PAL[c] for c in r]
        out.append('{%s}, /* %s */' % (','.join(map(str, vals)), k))
    out.append('};'); return keys
k10 = emit('icon10', P10, 10)
out.append('#ifdef __3DS__')
emit('icon20', {name: detailed_icon(rows) for name, rows in P10.items()}, 20)
out.append('#endif')
k8 = emit('icon8', P8, 8)
for i, k in enumerate(k10): out.append('#define IC10_%s %d' % (k, i))
for i, k in enumerate(k8): out.append('#define IC8_%s %d' % (k, i))
from pathlib import Path
Path(__file__).resolve().parent.parent.joinpath('source/icons.h').write_text('\n'.join(out) + '\n'); print('ok', len(k10), len(k8))

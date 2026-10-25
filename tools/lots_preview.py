#!/usr/bin/env python3
"""lots_preview.py - draws the community lots (venues.h venueBuild) from the REAL source as top-down pictures, on a PC, in a second.
Pulls the room-building code out of source/venues.h and the g* helpers out of source/main.c, compiles them with a small C stub, builds every venue at a lot size and
paints the map: floors, walls, doors, and every item in its catalog colour with its name. Run from the project root:
    python3 tools/lots_preview.py [out.png] [lots...] [WxH...]       e.g.  python3 tools/lots_preview.py /tmp/lots.png GYM CAFE 4x4 6x5
A lot is W x H in 4-tile units (4x4 .. 8x5: the town's lot shapes). Without arguments: every venue at 4x4 and at 6x5."""
import re, subprocess, sys, os, json
os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
V = open('source/venues.h', encoding='utf-8').read(); M = open('source/main.c', encoding='utf-8').read(); N = open('source/neighborhood.h', encoding='utf-8').read()
a = V.index('typedef struct { u8 ct, wp, fl;'); b = V.index('static const Venue* venueHere')
c = V.index('// ---- the room ----'); d = V.index('// ---- the counter, drawn by code ----')
g = M[M.index('static void gBox('):M.index('static void mapGen(void){')]
ct = re.search(r'enum \{ CT_PARK.*?CT_N \};', N, re.S).group(0)
cat = open('source/catalog.h', encoding='utf-8').read(); cat = cat[cat.index('typedef struct { const char*nm;'):cat.index('static const u8 catFn')]
ncat = len(re.findall(r'^\s*\{"[^"]+",\'.\',', cat, re.M))
src = r'''#include <stdio.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
#define MW 40
#define MH 40
#define CAT_N %d
#define CAT_CATS 28
#define EWRAM_BSS
#define FL_COPING 71
#define FL_W0 72
#define NWP 14
static u16 bfsQ[MH*MW]; typedef signed char s8; static char lifeMap[MH][MW+1]; static u8 wallMap[MH][MW], floorMap[MH][MW];
%s
%s

%s
%s
%s
int main(int argc,char**argv){
    for(int a=1;a<argc;a++){ int ct_=atoi(argv[a]); int w=atoi(argv[a+1]), h=atoi(argv[a+2]); a+=2;
        for(int y=0;y<MH;y++){ for(int x=0;x<MW;x++){ lifeMap[y][x]=(x==0||y==0||x==MW-1||y==MH-1)?'w':'.'; wallMap[y][x]=0; floorMap[y][x]=11; } lifeMap[y][MW]=0; }
        int tw=w*4, th=h*4, x0=(MW-tw)/2, y0=(MH-th)/2, x1=x0+tw-1, y1=y0+th-1; venueBuild(x0,y0,x1,y1,ct_);
        printf("LOT %%d %%d %%d %%d %%d %%d %%d %%d\n",ct_,w,h,x0,y0,x1,y1,vRemoved);
        for(int y=0;y<MH;y++){ for(int x=0;x<MW;x++){ const char*nm="-"; if(wallMap[y][x]&&lifeMap[y][x]!='W'&&lifeMap[y][x]!='D'){ for(int i=0;i<CAT_N;i++) if(catTab[i].ch==lifeMap[y][x]&&catTab[i].lk==wallMap[y][x]){ nm=catTab[i].nm; break; } }
            printf("%%d %%c %%d %%s\n",floorMap[y][x],lifeMap[y][x],wallMap[y][x],nm); } } }
    return 0; }
''' % (ncat, ct, cat, g, V[a:b], V[c:d])
src = src.replace('#include <string.h>', '#include <string.h>\n#include <stdlib.h>')
open('/tmp/lots_harness.c', 'w').write(src)
r = subprocess.run(['cc', '-O0', '-w', '-o', '/tmp/lots_harness', '/tmp/lots_harness.c'], capture_output=True, text=True)
if r.returncode: print(r.stderr[:3000]); sys.exit(1)
names = re.search(r'ctNm\[CT_N\]=\{(.*?)\};', N, re.S).group(1); names = re.findall(r'"([^"]*)"', names)
cts = re.search(r'enum \{ CT_PARK.*?CT_N \};', N, re.S).group(0); cts = re.findall(r'CT_([A-Z]+)', cts)
VEN = ['LOUNGE', 'PLAZA', 'PARK'] + [x for x in re.findall(r'\{CT_([A-Z]+),', V[V.index('static const Venue venues[]'):V.index('#define NVEN')])]
args = sys.argv[1:]; out = args[0] if args and args[0].endswith('.png') else '/tmp/lots.png'; args = [x for x in args if not x.endswith('.png')]
want = [x for x in args if not re.fullmatch(r'\d+x\d+', x)] or VEN; sizes = [tuple(map(int, x.split('x'))) for x in args if re.fullmatch(r'\d+x\d+', x)] or [(5, 5), (6, 5)]
call = []
for v in want:
    for (w, h) in sizes: call += [str(cts.index(v)), str(w), str(h)]
o = subprocess.run(['/tmp/lots_harness'] + call, capture_output=True, text=True).stdout.split('\n')
from PIL import Image, ImageDraw
catcol = {}
for m in re.finditer(r'\{"([^"]+)",\'(.)\',\d+,\d+,\d,\d+,RGB\((\d+),(\d+),(\d+)\)', cat): catcol[m.group(1)] = tuple(int(m.group(i)) * 8 for i in (3, 4, 5))
CELL = int(os.environ.get('CELL', 12)); lots = []; i = 0
while i < len(o) and o[i].startswith('LOT'):
    h = list(map(int, o[i].split()[1:])); rem = h.pop(); i += 1; cells = [o[i + k] for k in range(1600)]; i += 1600; lots.append((h + [rem], cells))
W = max(h[5] - h[3] + 3 for h, _ in lots); H = max(h[6] - h[4] + 3 for h, _ in lots)
cols = 4 if len(lots) > 3 else len(lots); rows = (len(lots) + cols - 1) // cols
img = Image.new('RGB', (cols * (W * CELL + 10), rows * (H * CELL + 22)), (20, 24, 30)); dr = ImageDraw.Draw(img)
FLC = {0: (150, 120, 90), 1: (120, 90, 70), 2: (60, 60, 70), 3: (200, 190, 170), 4: (110, 70, 60), 5: (170, 190, 210), 6: (70, 150, 150), 7: (170, 170, 170), 11: (70, 120, 60), 12: (60, 60, 64), 13: (190, 150, 110)}
def check(h, cells):   # what can be walked to from the door: every item needs a free tile beside it (rows behind the counter are for show)
    ct_, w, hh, x0, y0, x1, y1, rem = h; cx = (x0 + x1) // 2; ry1 = y1 - 3; ry0 = y0 + 1
    g = lambda x, y: cells[y * 40 + x].split(' ', 3)[1]
    seen = {(cx, ry1)}; q = [(cx, ry1)]
    while q:
        x, y = q.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in seen and 0 <= n[0] < 40 and 0 <= n[1] < 40 and g(*n) in '.PBD':
                seen.add(n); q.append(n)
    orphans = []; dead = []
    if cts[ct_] in ('PARK', 'PLAZA'): return seen, orphans, dead, True   # (open ground: nothing to walk through)
    for y in range(ry0 + 1, ry1):
        for x in range(x0 + 2, x1 - 1):
            c = g(x, y)
            if c == '.' and (x, y) not in seen and y > ry0 + 3: dead.append((x, y))
            elif c not in '.WDEefw@' and y > ry0 + 3 and not any(((x + dx, y + dy) in seen) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))): orphans.append((x, y))
    front = (cx, ry0 + 4) in seen or cts[ct_] in ('PARK', 'PLAZA')   # (a park and a plaza have no room: no door, no counter)
    return seen, orphans, dead, front
bad = 0
for k, (h, cells) in enumerate(lots):
    ct_, w, hh, x0, y0, x1, y1, rem = h; seen, orph, dead, front = check(h, cells)
    if rem: print('%-9s %dx%d  repaired: %d pieces taken away' % (cts[ct_], w, hh, rem))
    if orph or dead or not front: bad += 1; print('%-9s %dx%d  orphan items %d, unreachable floor %d%s' % (cts[ct_], w, hh, len(orph), len(dead), '' if front else ', COUNTER NOT REACHABLE'))
print('lots with problems:', bad, 'of', len(lots))
BAD = bad
for k, (h, cells) in enumerate(lots):
    ct_, w, hh, x0, y0, x1, y1, rem = h; seen, orph, dead, front = check(h, cells); ox = (k % cols) * (W * CELL + 10) + 5; oy = (k // cols) * (H * CELL + 22) + 18
    dr.text((ox, oy - 14), '%s %dx%d' % (cts[ct_], w, hh), fill=(255, 255, 255))
    for y in range(y0 - 1, y1 + 2):
        for x in range(x0 - 1, x1 + 2):
            fl, ch, wp, nm = cells[y * 40 + x].split(' ', 3); fl = int(fl); px = ox + (x - x0 + 1) * CELL; py = oy + (y - y0 + 1) * CELL
            col = FLC.get(fl, (90 + fl * 3 % 60, 100, 110))
            dr.rectangle([px, py, px + CELL - 1, py + CELL - 1], fill=col)
            if ch in 'Eef': dr.rectangle([px, py, px + CELL - 1, py + CELL - 1], fill=(120, 190, 230))
            elif ch == 'W': dr.rectangle([px, py, px + CELL - 1, py + CELL - 1], fill=(60, 50, 80))
            elif ch == 'D': dr.rectangle([px + 2, py, px + CELL - 3, py + CELL - 1], fill=(190, 140, 60))
            elif ch == '@': dr.rectangle([px + 1, py + 1, px + CELL - 2, py + CELL - 2], fill=(230, 200, 60))
            elif ch not in '.w' and nm != '-': c2 = catcol.get(nm, (255, 0, 255)); dr.rectangle([px + 1, py + 1, px + CELL - 2, py + CELL - 2], fill=c2); dr.text((px + 2, py + 1), nm[0] + (nm.split()[-1][0] if ' ' in nm else ''), fill=(0, 0, 0))
            elif ch not in '.w': dr.rectangle([px + 3, py + 3, px + CELL - 4, py + CELL - 4], fill=(255, 0, 255)); dr.text((px + 3, py + 1), ch, fill=(0, 0, 0))
            if (x, y) in orph: dr.rectangle([px, py, px + CELL - 1, py + CELL - 1], outline=(255, 0, 0), width=2)
            if (x, y) in dead: dr.rectangle([px + 4, py + 4, px + CELL - 5, py + CELL - 5], fill=(255, 0, 0))
img.save(out); print('wrote', out, img.size, len(lots), 'lots')
sys.exit(1 if BAD else 0)

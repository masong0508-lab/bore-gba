"""Shot layouts and animation for the GodGuy campaign."""
from math import sin, cos, sqrt
from random import Random

from PIL import ImageDraw

from story_art import (
    VOID, INK, INDIGO, SLATE, PURPLE, WINE, RUST, RED, AMBER, SUN, CREAM,
    WHITE, SEA, TEAL, JADE, MINT, PINE, LEAF, OLIVE, STONE, ASH,
    CLOUD, ICE, WOOD, CLAY, TAN, SKIN, LILAC, PLUM, BLUE,
    WISP, MIRA, GRUUL, ELOWEN, THORGRIM, HOLLOW, DAVE, MIRA_ELDER,
    canvas, cast, actor, spark,
)

WIDTH, HEIGHT = 128, 72
STARS = [(rng.randrange(WIDTH), rng.randrange(46)) for rng in (Random(i+37) for i in range(68))]
RIDGES = (
    ((-15, 53), (9, 44), (15, 47), (28, 38), (36, 45), (43, 43), (58, 53),
     (72, 47), (86, 39), (96, 48), (108, 36), (123, 48), (160, 42)),
    ((-20, 57), (4, 49), (12, 46), (21, 48), (34, 45), (42, 51), (64, 58),
     (83, 55), (95, 49), (104, 52), (120, 46), (133, 48), (150, 44)),
)
COTTAGES = ((6, 62, 4), (21, 58, 5), (36, 61, 4), (47, 66, 2),
            (106, 64, 4), (122, 59, 5), (136, 56, 7))


def sky(d, colors, step=8):
    for i, color in enumerate(colors):
        d.rectangle((0, i*step, 127, (i+1)*step-1), fill=color)
        if i:
            for x in range((i % 2)*2, 128, 4):
                d.point((x, i*step), fill=colors[i-1])


def stars(d, t, count=68):
    for i, (x, y) in enumerate(STARS[:count]):
        d.point((x, y), fill=CLOUD if (i + t//8) % 13 == 0 else SLATE)
    spark(d, 106, 11, SLATE, ICE)


def orbit(frame, d, t, lost=False):
    sky(d, (VOID, VOID, VOID, INK, INK, INDIGO, INDIGO, SLATE, PURPLE))
    stars(d, t)
    cx, cy, radius = 67, 42 + t//20, 19 + t//8
    d.ellipse((cx-radius-1, cy-radius-1, cx+radius+1, cy+radius+1),
              fill=WINE if lost else TEAL)
    pixels = frame.load()
    angle = (t//5)*0.090
    ca, sa = cos(angle), sin(angle)
    for y in range(cy-radius, min(HEIGHT, cy+radius+1)):
        v = (y-cy)/radius
        for x in range(cx-radius, cx+radius+1):
            u = (x-cx)/radius
            mag = u*u+v*v
            if mag > 1:
                continue
            z = sqrt(max(0, 1-mag))
            light = -0.68*u - 0.5*v + 0.4*z
            a, b = u*ca+z*sa, z*ca-u*sa
            land = sin(a*8+v*3)+sin(v*10-b*4)+0.6*sin(a*18+v*11)
            cloud = sin(v*29+3*sin(a*7)+b*4) > 0.94 and z > 0.48
            if light < -0.13:
                color = VOID if light < -0.37 else INK
            elif lost:
                color = (RED if light > 0.55 else RUST) if land > 0.5 else WINE
            elif cloud and light > 0.45:
                color = ICE if light > 0.69 else MINT
            elif land > 0.38:
                color = OLIVE if light > 0.55 else LEAF if light > 0.25 else PINE
            else:
                color = JADE if light > 0.6 else TEAL if light > 0.23 else SEA
            if z < 0.18 and light > 0.45:
                color = AMBER if lost else MINT
            pixels[x, y] = color
    # A near star drifts while the distant globe turns more slowly.
    d.point((14+(t//2)%24, 9+t%5), fill=ICE)
    if lost:
        for i in range(10):
            x = 13+(i*31+3*(t//3))%104
            y = 5+(i*37+t)%62
            d.point((x, y), fill=AMBER if i%5 == 0 else RUST)


# A flat map cut for DS/DSi. The outline stays fixed as ripples travel
# along the bays and a trail of lights points to the far bank.
def atlas(d, t, lost=False):
    edge = ((0, 54), (14, 52), (26, 43), (36, 43), (49, 34),
            (65, 32), (76, 37), (91, 35), (99, 26), (111, 19), (127, 20))
    sea = WINE if lost else SEA
    shore = CLAY if lost else TAN
    land = RUST if lost else LEAF
    forest = WINE if lost else PINE
    d.rectangle((0, 0, 127, 71), fill=sea)
    d.polygon(((0, 0), (127, 0), *reversed(edge)), fill=land)
    d.line(edge, fill=shore, width=3)
    d.line(tuple((x, y+3) for x, y in edge), fill=RUST if lost else TEAL)
    d.polygon(((0, 10), (13, 7), (25, 11), (33, 23), (27, 30),
               (13, 28), (0, 35)), fill=forest)
    d.polygon(((67, 0), (99, 0), (104, 9), (91, 16), (80, 13),
               (72, 21), (62, 13)), fill=forest)
    d.polygon(((59, 55), (68, 50), (78, 51), (85, 58), (79, 63),
               (66, 60)), fill=shore)
    d.polygon(((63, 55), (69, 52), (77, 53), (81, 57), (75, 60),
               (67, 58)), fill=land)
    d.line((4, 44, 20, 40, 34, 31, 46, 31, 56, 24, 72, 25, 88, 20),
           fill=WOOD if lost else OLIVE, width=2)
    for i in range(14):
        x = (i*29+3*(t//2))%128
        y = 58+(i*5)%13 if x < 90 else 35+(i*5)%33
        if not (57 <= x <= 86 and 48 <= y <= 64):
            d.line((x, y, x+2+i%4, y), fill=RUST if lost else JADE)
    for i, (x, y) in enumerate(((19, 40), (44, 31), (56, 24), (87, 20))):
        if not lost or i == 0 and (t//6)&1:
            d.point((x, y), fill=SUN if (i+t//9)%5 else CREAM)


def ridge(d, index, offset, color):
    d.polygon([(x-offset, y) for x, y in RIDGES[index]] +
              [(160, 59), (-30, 59)], fill=color)


def house(d, x, y, s, lit, wall=STONE, roof=WOOD):
    d.rectangle((x, y-s, x+2*s, y), fill=wall, outline=INK)
    d.polygon(((x-2, y-s), (x+s, y-2*s-1), (x+2*s+2, y-s)),
              fill=roof, outline=INK)
    d.line((x+s, y-2*s, x+2*s, y-s-1), fill=CLAY)
    d.rectangle((x+s-1, y-3, x+s, y-2), fill=lit)
    if s >= 5:
        d.rectangle((x+2*s-2, y-3, x+2*s-1, y), fill=INK)


def pine(d, x, y, s):
    d.line((x, y-s, x, y), fill=WOOD)
    for j in range(3):
        d.polygon(((x, y-s+j*2), (x-j-2, y-s+4+j*2), (x+j+2, y-s+4+j*2)),
                  fill=PINE, outline=INK)


def coast(d, t, settled=False, storm=False, night=False):
    dawn = t//12
    if storm:
        colors = (INK, INDIGO, SLATE, SLATE, PURPLE, SLATE, BLUE, TEAL, SEA)
    elif night and dawn < 3:
        colors = (VOID, INK, INDIGO, INDIGO, PURPLE, WINE, RUST, PURPLE, SEA)
    else:
        colors = (INDIGO, SLATE, PURPLE, WINE, RUST, RED, AMBER, SUN, CREAM)
    sky(d, colors)
    if night and dawn < 3:
        stars(d, t, 35)
    if not storm:
        sy = 54 - min(t//5, 14)
        d.ellipse((51, sy-10, 72, sy+10), fill=CREAM)
    for i in range(5):
        x = (i*37+t//4)%153-13
        y = 12+i*7
        d.line((x, y, x+11+i*3, y), fill=SLATE if storm else colors[max(0, y//8-1)])
    ridge(d, 0, 0, PURPLE if storm else WINE)
    ridge(d, 1, 2, SLATE if storm else TEAL)
    d.rectangle((0, 56, 127, 71), fill=SEA)
    for i in range(7):
        y = 56+(i*7+t//5)%16
        x = 61+(i*11%17-8)
        d.line((x-i-2, y, x+i+2, y), fill=JADE if storm else AMBER if i%2 else SUN)
        x = (i*31+t//3)%127
        d.line((x, y, x+i+3, y), fill=TEAL)
    d.polygon(((-10, 56), (6, 54), (28, 56), (40, 60), (46, 65),
               (42, 67), (57, 71), (-10, 71)), fill=INK)
    d.polygon(((92, 72), (96, 65), (104, 60), (118, 57), (138, 53),
               (149, 72)), fill=INK)
    if settled:
        for i, (x, y, size) in enumerate(COTTAGES):
            house(d, x, y, size, SUN if (t//5+i)%7 else AMBER)
            if size == 5:
                p = (t//3+i)%7
                d.point((x+size+p//2, y-size*2-2-p), fill=ASH)
    else:
        for x, y, size in ((5, 62, 9), (21, 58, 10), (29, 60, 5), (123, 63, 13)):
            pine(d, x, y, size)
    if storm:
        rain(d, t)
    elif t > 18:
        for i in range(3):
            x, y = 43+t//2+i*9, 30-(i%2)*4
            wing = 1 if (t//2+i)%4 < 2 else -1
            d.line((x-2, y-wing, x, y, x+2, y-wing), fill=INK)


def rain(d, t, count=16):
    t = t//2*2
    for i in range(count):
        x, y = (i*43-t)%131, (i*37+t*3)%75
        d.line((x, y, x-1, y+4), fill=BLUE if i%4 else CLOUD)


def cup(d, x, y, t):
    d.ellipse((x+14, y+5, x+23, y+15), outline=AMBER, width=2)
    d.polygon(((x, y), (x+17, y), (x+15, y+15), (x+4, y+15)),
              fill=CLAY, outline=INK)
    d.ellipse((x, y-2, x+17, y+3), fill=SUN, outline=WOOD)
    d.ellipse((x+3, y, x+14, y+2), fill=JADE)
    d.line((x+5, y+6, x+6, y+12), fill=TAN)
    if t//3%4 < 2:
        d.ellipse((x+5, y+9+t//3%4, x+12, y+11+t//3%4), outline=AMBER)


def fire(d, x, y, t, size=15):
    d.ellipse((x-size, y-3, x+size, y+4), fill=WOOD)
    d.line((x-8, y+2, x+9, y-1), fill=CLAY, width=2)
    for i in range(4):
        dx = i*5-8
        rise = (i*7+t//2)%8
        d.polygon(((x+dx-5, y), (x+dx-3, y-rise-4), (x+dx, y-rise-9),
                   (x+dx+3, y-rise-3), (x+dx+5, y)), fill=RUST if i%2 else AMBER)
    d.polygon(((x-4, y), (x-1, y-6-t//3%3), (x+3, y)), fill=SUN)
    for i in range(9):
        dx = (i*31+t//4)%29-14
        dy = (i*19+t)%35
        d.point((x+dx, y-dy), fill=SUN if i%3 else AMBER)


def masonry(d, box):
    x, y, right, bottom = box
    d.rectangle(box, fill=STONE, outline=INK)
    for row in range(y+3, bottom, 6):
        d.line((x+1, row, right-1, row), fill=SLATE)
        for seam in range(x+5+(row//6%2)*4, right-1, 11):
            d.line((seam, row, seam, min(bottom-1, row+5)), fill=SLATE)


def landscape(d, t, mode):
    colors = (INK, INDIGO, SLATE, PURPLE, WINE, RUST, AMBER, SUN, SEA)
    sky(d, colors)
    if mode == "night":
        sky(d, (VOID, INK, INK, INDIGO, INDIGO, SLATE, PURPLE, WINE, SEA))
        stars(d, t, 36)
    d.ellipse((89, 11, 107, 30), fill=ICE if mode == "night" else SUN)
    ridge(d, 0, 0, SLATE if mode == "night" else WINE)
    ridge(d, 1, 2, SEA)
    d.rectangle((0, 55, 127, 71), fill=PINE if mode != "night" else INK)
    for i in range(38):
        x, y = i*23%128, 58+i*17%14
        d.line((x, y, x+2, y), fill=LEAF if mode != "night" else INDIGO)


def interior(d, t, forge=False, library=False):
    sky(d, (INK, INK, INDIGO, INDIGO, SLATE, SLATE, SLATE, WOOD, WOOD))
    for x in (7, 44, 85, 120):
        d.line((x, 0, x, 60), fill=VOID, width=3)
        d.line((x+2, 0, x+2, 60), fill=STONE)
    for y in (57, 63, 70):
        d.line((0, y, 127, y), fill=INK)
    if library:
        for x in (7, 86):
            for y in (16, 32, 48):
                d.rectangle((x, y, x+34, y+3), fill=CLAY)
                for i in range(7):
                    top = y-10-i*3%6
                    d.rectangle((x+1+4*i, top, x+3+4*i, y-1),
                                fill=(WINE, BLUE, WOOD, TEAL, PLUM)[(i+y)%5])
                    d.point((x+2+4*i, top+2), fill=TAN)
    else:
        d.rectangle((48, 6, 81, 42), fill=VOID, outline=WOOD, width=2)
        d.ellipse((53, 13, 74, 33), fill=ICE if not forge else RED)
        d.line((65, 7, 65, 41), fill=WOOD, width=2)
        d.line((49, 25, 80, 25), fill=WOOD, width=2)
    if forge:
        masonry(d, (82, 21, 126, 62))
        d.pieslice((90, 32, 120, 68), 180, 360, fill=INK)
        d.rectangle((90, 48, 120, 60), fill=INK)
        fire(d, 105, 58, t)


def river(d, t):
    landscape(d, t, "day")
    d.polygon(((59, 43), (67, 43), (64, 53), (81, 59), (93, 72),
               (32, 72), (45, 60), (52, 52)), fill=SEA)
    for i in range(11):
        y = 49+(i*7+t//3)%24
        x = 62+i%5-2
        d.line((x-(y-46)//2, y, x+i%7, y), fill=JADE if i%4 == 0 else TEAL)
    for x, y in ((4, 59), (14, 63), (115, 59), (126, 56)):
        pine(d, x, y, 13)


def creature(d, x, y, t, color=INK):
    stride = (0, 1, 2, 1)[t//2 % 4]
    d.polygon(((x-8, y-3), (x-7, y-9), (x-10, y-15-stride),
               (x-3, y-12), (x+4, y-12), (x+9, y-17+stride),
               (x+7, y-8), (x+9, y-3)), fill=color)
    d.line((x-5, y-6, x-4-stride, y), fill=color, width=3)
    d.line((x+5, y-6, x+4+stride, y-stride), fill=color, width=3)
    d.line((x-4, y-7, x-2, y-7), fill=AMBER)
    d.line((x+2, y-7, x+4, y-7), fill=AMBER)
    d.point((x, y-3+(t//6)%2), fill=RUST)


def gate(d, t, siege=False):
    landscape(d, t, "night" if siege else "day")
    masonry(d, (10, 29, 118, 61))
    for x in range(13, 116, 12):
        masonry(d, (x, 23, x+7, 32))
    d.pieslice((46, 32, 81, 69), 180, 360, fill=INK)
    d.rectangle((46, 49, 81, 63), fill=INK)
    if siege:
        for x in range(49, 82, 6):
            d.line((x, 45, x, 63), fill=WOOD, width=2)
    for x in (33, 94):
        d.line((x, 41, x, 58), fill=WOOD, width=2)
        d.ellipse((x-2, 40-t//3%3, x+2, 45), fill=SUN)
    if siege:
        for i in range(4):
            x = (8, 31, 101, 122)[i] + (1 if i < 2 else -1)*min(8, t//4)
            creature(d, x, 72-i%2*4, t)
        for i in range(8):
            d.point(((i*31+t//4)%128, (i*17-t)%52), fill=RUST)


def book(d, t, illuminated=False):
    d.polygon(((13, 37), (54, 37), (64, 41), (73, 36), (112, 35),
               (116, 65), (72, 66), (63, 69), (51, 66), (11, 65)),
              fill=WOOD, outline=INK)
    d.polygon(((15, 35), (52, 34), (62, 39), (62, 66), (50, 62), (13, 62)),
              fill=CREAM)
    d.polygon(((64, 39), (74, 34), (111, 33), (113, 62), (75, 62), (64, 66)),
              fill=TAN)
    for y in range(41, 61, 4):
        d.line((18, y, 49-(y//4%3), y), fill=CLAY)
    creature(d, 88, 56, t, WOOD)
    if illuminated:
        x = 93-t//3%22
        d.line((x, 63, x+13, 45), fill=WHITE, width=2)
        d.line((x+4, 58, x+14, 49), fill=ICE, width=2)
        spark(d, x-2, 62, AMBER, WHITE)


def throne(d, t, crown=True):
    t = t//3*3
    sky(d, (VOID, INK, INK, INDIGO, INDIGO, PURPLE, PURPLE, WINE, INK))
    d.ellipse((47, 7, 81, 47), fill=SLATE, outline=PURPLE, width=2)
    for x in (9, 28, 100, 119):
        d.rectangle((x-2, 5, x+2, 57), fill=INK)
        d.line((x+2, 6, x+2, 53), fill=STONE)
    d.polygon(((52, 54), (77, 54), (99, 71), (25, 71)), fill=WINE)
    for y in (56, 62, 68):
        d.line((31, y, 96, y), fill=INK, width=2)
    if crown:
        d.polygon(((56, 63), (53, 56), (58, 58), (62, 54), (66, 58),
                   (72, 56), (69, 63)), fill=AMBER, outline=WOOD)
    for i in range(10):
        d.point(((i*37+t//5)%128, (i*19-t//4)%65), fill=PLUM)


def rafters(d, t):
    sky(d, (INK, INDIGO, SLATE, SLATE, PURPLE, SLATE, BLUE, TEAL, SEA))
    rain(d, t)
    d.rectangle((0, 58, 127, 71), fill=PINE)
    masonry(d, (18, 38, 111, 67))
    d.rectangle((57, 52, 71, 67), fill=INK)
    d.rectangle((27, 50, 38, 59), fill=AMBER)
    d.rectangle((88, 50, 98, 59), fill=AMBER)
    settle = min(13, t//2)
    left = [(12, 41), (64, 16), (67, 20), (13, 44)]
    right = [(64, 16), (117, 41), (115, 44), (62, 20)]
    for plank in (left, right):
        d.polygon([(x, y+settle-13) for x, y in plank], fill=WOOD, outline=INK)
    for slat in range(min(7, max(0, (t-20)//2))):
        y = 21+slat*3
        d.line((63-slat*7, y, 64+slat*7, y), fill=CLAY, width=2)
    for x in (4, 117):
        d.line((x, 62, x, 47), fill=INK, width=2)
        d.ellipse((x-1, 43, x+1, 47), fill=SUN)


def forge_close(d, t):
    sky(d, (INK, INK, WINE, RUST, WINE, WOOD, WOOD, INK, INK))
    fire(d, 100, 54, t, 29)
    d.polygon(((7, 54), (110, 54), (96, 62), (86, 65), (85, 71),
               (35, 71), (31, 63), (19, 63)), fill=STONE, outline=INK)
    d.line((13, 55, 104, 55), fill=ASH)
    d.polygon(((24, 49), (48, 49), (51, 42), (71, 45), (76, 52),
               (71, 56), (52, 57), (48, 53), (24, 53)), fill=RUST, outline=WOOD)
    d.line((51, 46, 69, 49, 72, 51), fill=SUN, width=2)
    swing = t//2%12
    swing = min(swing, 12-swing)
    d.line((20, 24, 60, 25+3*swing), fill=CLAY, width=4)
    d.rectangle((52, 20+3*swing, 66, 28+3*swing), fill=ASH, outline=INK)
    if swing>=5:
        for dx, dy in ((-14, -7), (-5, -9), (9, -5), (19, -2), (23, 5)):
            spark(d, 64+dx, 46+dy, AMBER, CREAM, 1)


def hall(d, t):
    sky(d, (INK, STONE, SLATE, STONE, SLATE, STONE, STONE, INK, INK))
    for x in (10, 43, 76, 109):
        d.pieslice((x-6, 4, x+10, 33), 180, 360, fill=INK)
        d.rectangle((x-6, 18, x+10, 44), fill=INK)
        d.pieslice((x-4, 6, x+8, 30), 180, 360, fill=SEA)
        d.rectangle((x-4, 18, x+8, 39), fill=SEA)
        d.polygon(((x+2, 11), (x+6, 24), (x+2, 35), (x-2, 24)), fill=AMBER)
        d.line((x+2, 10, x+2, 38), fill=CREAM)
    d.polygon(((51, 55), (77, 55), (104, 71), (24, 71)), fill=WINE)
    for i in range(12):
        x = (i*37+t//6)%128
        y = 28+(i*19+t//3)%38
        d.point((x, y), fill=AMBER if i%4 else CREAM)


def field_notes(d, t):
    river(d, t)
    for i in range(5):
        x, y = 64+i*8, 66-i%2*4
        for j in range(3):
            d.line((x+j, y, x+j-2, y-4-j), fill=LEAF)
    x = 82-min(9, t//4)
    creature(d, x, 68, t, TEAL)
    creature(d, 101, 71, t, PINE)
    d.polygon(((78, 65), (85, 65), (82, 70)), fill=OLIVE)
    for i in range(4):
        spark(d, 57+i*7, 33+(i*11+t//4)%12, JADE, MINT, 1)


def cup_close(d, t, dry=False):
    """Return to the same small prop as the village grows up around it."""
    interior(d, t)
    d.rectangle((0, 49, 127, 71), fill=WOOD)
    d.line((0, 51, 127, 51), fill=CLAY)
    d.ellipse((45, 58, 87, 65), fill=INK)
    cup(d, 51, 42, t)
    if dry:
        d.ellipse((58, 65, 71, 70), fill=SEA)
        d.line((64, 70, 65, 60), fill=LEAF)
        d.ellipse((62, 61, 65, 64), fill=MINT)
        d.ellipse((65, 60, 68, 62), fill=JADE)
        d.point((64, 57+t%7), fill=MINT)
    else:
        d.line((60, 6+t%30, 60, 9+t%30), fill=ICE)
        radius = (t//2)%6
        d.ellipse((60-radius, 43, 60+radius, 47), outline=MINT)


def lift_roof(d, t):
    masonry(d, (0, 45, 127, 75))
    fall = min(16, t)
    d.polygon(((10, 36+fall), (95, 11+fall), (107, 14+fall), (21, 41+fall)),
              fill=WOOD, outline=INK)
    d.line((17, 38+fall, 100, 13+fall), fill=TAN)
    nail = 36+max(0, fall-12)
    swing = max(0, (10-t%13)*2)
    d.line((99, 18, 71, nail-8-swing), fill=WOOD, width=4)
    d.rectangle((61, nail-13-swing, 74, nail-7-swing), fill=ASH, outline=INK)
    d.line((66, nail-5, 66, nail), fill=ASH, width=2)
    d.line((65, nail-5, 68, nail-5), fill=ICE)
    if t%13 >= 10:
        spark(d, 67, 43, AMBER, WHITE)


def quill_close(d, t):
    interior(d, t, library=True)
    book(d, t)
    edge = 62 + min(51, max(0, t-4)*5)
    d.polygon(((64, 39), (edge, 33), (edge, 61), (64, 66)), fill=CREAM)
    for j in range(4):
        d.line((68, 45+j*4, min(edge, 103-2*j), 43+j*4), fill=CLAY)
    x = 70+t*2 % 26
    d.line((x, 58, x+11, 32), fill=INK)
    d.polygon(((x+2, 54), (x+4, 35), (x+14, 30), (x+11, 42)), fill=ICE)
    d.point((x, 59), fill=INK)


def render(name, t, flat=False):
    f = canvas((WIDTH, HEIGHT))
    d = ImageDraw.Draw(f)
    if name == "opening":
        if t < 38:
            if flat:
                atlas(d, t)
            else:
                orbit(f, d, t)
        elif t < 74:
            coast(d, t-38)
            actor(f, WISP, 49+(t-38)//2, 50-min(20, (t-38)//2), t, 17)
        else:
            coast(d, t-56)
            actor(f, WISP, 64, 51, t, 37, pose="talk")
            for i in range(5):
                x, y = 57+i*6, 67-i%2*4
                rise = min(7, max(0, (t-75-i*3)//2))
                if rise:
                    d.line((x, y, x, y-rise), fill=MINT)
                    d.point((x+1, y-rise), fill=CREAM)
    elif name == "first_folk":
        if t < 30:
            coast(d, t, storm=True)
            actor(f, MIRA, 17+t//2, 71, t, 32, pose="walk", prop="cup")
        elif t < 55:
            landscape(d, t, "night")
            actor(f, MIRA, 42, 74, t, 62, pose="reach", prop="cup")
            actor(f, WISP, 101-(t-30)//3, 48, t, 25)
            rain(d, t, 12)
        else:
            cup_close(d, t-55)
    elif name == "roof":
        if t < 32:
            rafters(d, t)
            actor(f, MIRA, 17+t//3, 75, t, 36, pose="walk")
            actor(f, THORGRIM, 99, 72, t, 34, flip=True, pose="lift", prop="hammer")
        elif t < 54:
            lift_roof(d, t-32)
        else:
            coast(d, t-20, settled=True, night=True)
            actor(f, MIRA, 104-(t-54)//2, 73, t, 28, flip=True, pose="walk")
            fire(d, 87, 71, t, 10)
    elif name == "river_oath":
        river(d, t)
        for x, y in ((54, 69), (63, 67), (72, 70)):
            d.ellipse((x-5, y-1, x+5, y+2), fill=STONE)
            d.line((x-3, y-1, x+3, y-1), fill=ASH)
        move = min(t, 38)
        actor(f, GRUUL, 26+2*move//3, 71, t, 43,
              pose="walk" if t<38 else "reach", prop="cup" if t>=52 else None)
        actor(f, MIRA, 102-2*move//3, 72, t, 42, flip=True,
              pose="walk" if t<38 else "reach", prop="cup" if t<52 else None)
        if t >= 61:
            actor(f, WISP, 62, 45-min(9, (t-61)//2), t, 19)
    elif name == "first_darkness":
        if t < 29:
            coast(d, t, storm=True)
            for i in range(4):
                creature(d, 4+i*11+t//2, 69-i%2, t+i*7)
        elif t < 53:
            throne(d, t, crown=False)
            actor(f, HOLLOW, 39+(t-29), 67, t, 54, pose="walk")
        else:
            throne(d, t, crown=False)
            cast(f, HOLLOW, 64, 68, t, 57, speaking=True)
    elif name == "iron":
        if t >= 39:
            forge_close(d, t-39)
        else:
            interior(d, t, forge=True)
            actor(f, THORGRIM, 36, 72, t, 57, pose="lift" if t%12<5 else "bow")
            d.polygon(((53, 58), (77, 58), (71, 61), (70, 68), (60, 68),
                       (60, 62)), fill=ASH, outline=INK)
            angle = (t//3)%6
            d.line((47, 55, 65, 47+2*angle), fill=WOOD, width=3)
            d.rectangle((60, 42+2*angle, 71, 47+2*angle), fill=STONE, outline=INK)
            if angle >= 4:
                spark(d, 67, 55, AMBER, WHITE, 6-angle)
    elif name == "stone":
        if t >= 34:
            hall(d, t)
            actor(f, MIRA, 38+min(9,(t-34)//3), 73, t, 42,
                  pose="walk" if t < 59 else "talk")
            actor(f, THORGRIM, 95-min(9,(t-34)//3), 73, t, 44, flip=True,
                  pose="walk" if t < 59 else "bow")
        else:
            gate(d, t)
            for x in (18, 105):
                h = t//7%4
                d.line((x, 26, x, 42), fill=WOOD, width=2)
                d.polygon(((x,31), (x+8,31+h), (x+8,39+h), (x,37)), fill=TEAL)
            actor(f, MIRA, 31+t//2, 72, t, 29, pose="walk")
            actor(f, THORGRIM, 43+t//2, 72, t+4, 31, pose="walk")
    elif name == "ink":
        if t < 47:
            interior(d, t, library=True)
            actor(f, ELOWEN, 63, 58, t, 55, pose="write", prop="quill")
            book(d, t, illuminated=True)
        else:
            quill_close(d, t-47)
    elif name == "library":
        if t < 32:
            field_notes(d, t)
            actor(f, ELOWEN, 18+t//3, 70, t, 37, pose="walk", prop="quill")
        else:
            interior(d, t, library=True)
            actor(f, ELOWEN, 79, 57, t, 46, flip=True, pose="write", prop="quill")
            book(d, t, illuminated=True)
            creature(d, 35, 55, t, TEAL)
    elif name == "one_fire":
        landscape(d, t, "night")
        house(d, 14, 59, 5, SUN)
        house(d, 100, 59, 6, AMBER)
        actor(f, GRUUL, 39, 72, t, 41, pose="reach" if t<29 else "talk")
        actor(f, MIRA, 87-min(t//3, 11), 72, t, 41, flip=True,
              pose="walk" if t<25 else "reach", prop="cup" if t<38 else None)
        fire(d, 63, 67, t, 18)
        if t > 58:
            cup(d, 29, 55, t)
            d.line((34, 53-t%7, 36, 51-t%7), fill=CLOUD)
    elif name == "siege":
        if t < 42:
            gate(d, t, siege=True)
            actor(f, THORGRIM, 38+t//2, 54, t, 24, pose="walk", prop="hammer")
            for i in range(3):
                x, y = 13+(t*4+i*36)%108, 51-((t*3+i*11)%34)
                d.line((x-7, y+2, x, y), fill=AMBER)
                d.point((x, y), fill=SUN)
            if 22<=t<28:
                spark(d, 70, 45, AMBER, WHITE, 6-(t-22))
        else:
            throne(d, t, crown=False)
            cast(f, HOLLOW, 55, 61, t, 40, speaking=True)
            actor(f, WISP, 105-min(18,(t-42)//2), 45, t, 22)
    elif name == "city":
        coast(d, t, settled=True, night=True)
        for x, y in ((12, 51), (27, 53), (111, 54)):
            house(d, x, y, 4, AMBER)
        if t >= 66:
            cup_close(d, t-66, dry=True)
        elif t >= 32:
            d.ellipse((2, 53, 51, 78), fill=INK)
            cast(f, MIRA_ELDER, 29, 74, t, 39, speaking=True)
            cup(d, 49, 54, t)
            actor(f, WISP, 85, 47, t, 17)
    elif name == "old_god":
        if t < 31:
            if flat:
                atlas(d, t, lost=True)
            else:
                orbit(f, d, t, lost=True)
        else:
            landscape(d, t, "night")
            d.rounded_rectangle((12, 47, 73, 70), radius=5, fill=PLUM, outline=INK, width=2)
            d.line((17, 61, 67, 61), fill=LILAC, width=2)
            cast(f, DAVE, 44, 66, t, 36, speaking=True)
            actor(f, WISP, 89, 47, t, 23)
            # Dave tosses a puff and the couch gets it first.
            if 58 <= t < 80:
                arc = t-58
                x, y = 58+arc//2, 53-arc*(22-arc)//13
                d.ellipse((x-1, y-1, x+1, y+1), fill=SUN)
            if t >= 80:
                d.ellipse((69, 58, 72, 59), fill=AMBER)
    elif name == "godhood":
        throne(d, t, crown=False)
        if t >= 40:
            span = min(13, 2*((t-40)//4))
            d.rectangle((63-span, 6, 64+span, 49), fill=AMBER)
            d.rectangle((64-span//2, 6, 64+span//2, 49), fill=CREAM)
            d.polygon(((63-span, 49), (64+span, 49), (82, 65),
                       (50, 65)), fill=RUST)
        actor(f, HOLLOW, 42+min(19,t//3), 66, t, 51,
              pose="walk" if t<38 else "bow")
        actor(f, WISP, 96-min(17,t//5), 45, t, 24)
        if t>=54:
            spark(d, 59, 56+(t-54)//10, SUN, WHITE, 1)
    elif name == "finale":
        if t < 36:
            coast(d, t, settled=True, storm=True)
            actor(f, HOLLOW, 21+t//2, 71, t, 39, pose="walk")
            actor(f, WISP, 78, 46, t, 19)
        elif t < 69:
            coast(d, t-23, settled=True, night=True)
            actor(f, MIRA_ELDER, 100-(t-36)//2, 72, t, 28, flip=True, pose="walk", prop="cup")
            actor(f, HOLLOW, 36+(t-36)//2, 70, t+3, 34, pose="walk")
        elif t < 87:
            cup_close(d, t-69, dry=True)
        else:
            if flat:
                atlas(d, t-87)
                spark(d, 44, 31, MINT, WHITE, 2)
                spark(d, 87, 20, AMBER, WHITE, 2)
            else:
                orbit(f, d, t-87)
                spark(d, 45, 23, MINT, WHITE, 2)
                spark(d, 80, 33, AMBER, WHITE, 2)
    else:
        raise ValueError(name)
    return f


# Chapter order. The finale follows the epilogue dialogue.
SHOTS = (
    ("opening", "BEFORE THE FIRST LIGHT", 96),
    ("first_folk", "THE FIRST RAIN", 75),
    ("roof", "A ROOF AGAINST THE NIGHT", 84),
    ("river_oath", "TWO PEOPLES, ONE RIVER", 84),
    ("first_darkness", "THE KING IN THE DARK", 77),
    ("iron", "THE FIRST FORGE", 72),
    ("stone", "WHAT STONE REMEMBERS", 77),
    ("ink", "GIVING THE DARK A NAME", 72),
    ("one_fire", "ONE FIRE", 70),
    ("siege", "HOLD THE WALLS", 92),
    ("city", "THE CITY THAT NEVER SLEEPS", 82),
    ("library", "THE OTHER SIDE OF THE PAGE", 72),
    ("old_god", "THE WORLDS HE LOST", 88),
    ("godhood", "SHOW ME", 84),
    ("finale", "THERE IS ROOM FOR TWO", 117),
)

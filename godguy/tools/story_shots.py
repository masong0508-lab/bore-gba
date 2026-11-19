"""Shot layouts and animation for the GodGuy campaign."""
from math import sin, cos, sqrt
from random import Random

from PIL import ImageDraw

from story_art import (
    VOID, INK, INDIGO, SLATE, PURPLE, WINE, RUST, RED, AMBER, SUN, CREAM,
    WHITE, SEA, TEAL, JADE, MINT, PINE, LEAF, OLIVE, STONE, ASH,
    CLOUD, ICE, WOOD, CLAY, TAN, SKIN, LILAC, PLUM, BLUE,
    WISP, MIRA, GRUUL, ELOWEN, THORGRIM, HOLLOW, DAVE, MIRA_ELDER,
    canvas, cast, spark,
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
    angle = (t//3)*0.055
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
    if lost:
        for i in range(14):
            x = 13+(i*31+3*(t//3))%104
            y = 5+(i*37+t)%62
            d.point((x, y), fill=AMBER if i%5 == 0 else RUST)


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
    for i in range(9):
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


def rain(d, t, count=44):
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
        y = 49+(i*7+t//5)%24
        x = 62+i%5-2
        d.line((x-(y-46)//2, y, x+i%7, y), fill=JADE if i%4 == 0 else TEAL)
    for x, y in ((4, 59), (14, 63), (115, 59), (126, 56)):
        pine(d, x, y, 13)


def creature(d, x, y, t, color=INK):
    d.polygon(((x-8, y), (x-7, y-9), (x-10, y-15), (x-3, y-12),
               (x+4, y-12), (x+9, y-17), (x+7, y-8), (x+9, y)), fill=color)
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
        for i in range(12):
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
        y = 28+(i*19+t//4)%38
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


def render(name, t):
    f = canvas((WIDTH, HEIGHT))
    d = ImageDraw.Draw(f)
    if name == "opening":
        if t < 38:
            orbit(f, d, t)
        elif t < 74:
            coast(d, t-38)
            cast(f, WISP, 69, 50-min(20, (t-38)//2), t, 17)
        else:
            coast(d, t-56)
            cast(f, WISP, 64, 51, t, 39)
    elif name == "first_folk":
        if t < 30:
            coast(d, t, storm=True)
            cast(f, MIRA, 27, 70, t, 24)
        else:
            landscape(d, t, "night")
            cast(f, MIRA, 40, 73, t, 49, speaking=True)
            cup(d, 60, 52, t)
            cast(f, WISP, 94, 51, t, 23)
            rain(d, t, 26)
    elif name == "roof":
        if t < 38:
            rafters(d, t)
            cast(f, MIRA, 30, 75, t, 25)
        else:
            coast(d, t, settled=True, night=True)
            cast(f, MIRA, 91, 73, t, 24, flip=True)
            fire(d, 87, 71, t, 10)
    elif name == "river_oath":
        river(d, t)
        cast(f, GRUUL, 28+min(t//8, 5), 72, t, 36, speaking=t < 35)
        cast(f, MIRA, 101-min(t//8, 5), 72, t, 35, flip=True, speaking=t >= 35)
        cup(d, 82, 62, t)
        if t > 40:
            cast(f, WISP, 63, 44, t, 17)
    elif name == "first_darkness":
        if t < 25:
            coast(d, t, storm=True)
            for i in range(4):
                creature(d, 11+i*10, 69, t)
        else:
            throne(d, t, crown=False)
            cast(f, HOLLOW, 64, 65, t, min(54, 38+(t-25)//2), speaking=True)
    elif name == "iron":
        if t >= 39:
            forge_close(d, t-39)
        else:
            interior(d, t, forge=True)
            cast(f, THORGRIM, 38, 69, t, 39, speaking=True)
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
            cast(f, MIRA, 44, 72, t, 32, speaking=True)
            cast(f, THORGRIM, 84, 72, t, 35, flip=True)
        else:
            gate(d, t)
            for x in (18, 105):
                h = t//7%4
                d.line((x, 26, x, 42), fill=WOOD, width=2)
                d.polygon(((x,31), (x+8,31+h), (x+8,39+h), (x,37)), fill=TEAL)
            cast(f, MIRA, 53, 72, t, 22)
            cast(f, THORGRIM, 73, 72, t, 23, flip=True)
    elif name == "ink":
        interior(d, t, library=True)
        cast(f, ELOWEN, 63, 51, t, 42, speaking=True)
        book(d, t, illuminated=True)
    elif name == "library":
        if t < 32:
            field_notes(d, t)
            cast(f, ELOWEN, 23, 68, t, 28)
        else:
            interior(d, t, library=True)
            cast(f, ELOWEN, 79, 55, t, 30, flip=True)
            book(d, t, illuminated=True)
            creature(d, 35, 55, t, TEAL)
    elif name == "one_fire":
        landscape(d, t, "night")
        house(d, 14, 59, 5, SUN)
        house(d, 100, 59, 6, AMBER)
        cast(f, GRUUL, 44, 69, t, 28, speaking=True)
        cast(f, MIRA, 85, 69, t, 27, flip=True)
        fire(d, 63, 67, t, 18)
    elif name == "siege":
        if t < 42:
            gate(d, t, siege=True)
            cast(f, THORGRIM, 66, 57, t, 18)
        else:
            throne(d, t, crown=False)
            cast(f, HOLLOW, 55, 61, t, 40, speaking=True)
            cast(f, WISP, 91, 43, t, 21)
    elif name == "city":
        coast(d, t, settled=True, night=True)
        for x, y in ((12, 51), (27, 53), (111, 54)):
            house(d, x, y, 4, AMBER)
        if t >= 32:
            d.ellipse((2, 53, 51, 78), fill=INK)
            cast(f, MIRA_ELDER, 29, 74, t, 39, speaking=True)
            cup(d, 49, 54, t)
            cast(f, WISP, 85, 47, t, 17)
    elif name == "old_god":
        if t < 31:
            orbit(f, d, t, lost=True)
        else:
            landscape(d, t, "night")
            d.rounded_rectangle((12, 47, 73, 70), radius=5, fill=PLUM, outline=INK, width=2)
            d.line((17, 61, 67, 61), fill=LILAC, width=2)
            cast(f, DAVE, 44, 66, t, 36, speaking=True)
            cast(f, WISP, 89, 47, t, 23)
    elif name == "godhood":
        throne(d, t)
        if t >= 40:
            span = min(13, (t-40)//2)
            d.rectangle((63-span, 6, 64+span, 49), fill=AMBER)
            d.rectangle((64-span//2, 6, 64+span//2, 49), fill=CREAM)
            d.polygon(((63-span, 49), (64+span, 49), (82, 65),
                       (50, 65)), fill=RUST)
        cast(f, HOLLOW, 49+min(12,t//6), 54+(t<44)*4, t, 38, speaking=t<40)
        cast(f, WISP, 95-min(16,t//8), 44, t, 22)
    elif name == "finale":
        if t < 36:
            coast(d, t, settled=True, storm=True)
            cast(f, HOLLOW, 38, 67, t, 28)
            cast(f, WISP, 78, 45, t, 19)
        elif t < 78:
            coast(d, t-23, settled=True, night=True)
            cup(d, 26, 56, t)
            for i in range(2):
                x = 101 + i*11 - (t-36)//8
                d.ellipse((x-2, 57+i, x+2, 61+i), fill=SKIN)
                d.line((x, 61+i, x, 66+i), fill=MINT if i else AMBER, width=3)
                step = (t//4+i)%2
                d.line((x-2, 69+i-step, x, 66+i, x+2, 68+i+step), fill=ASH)
        else:
            orbit(f, d, t-78)
            spark(d, 45, 23, MINT, WHITE, 2)
            spark(d, 80, 33, AMBER, WHITE, 2)
    else:
        raise ValueError(name)
    return f


# Chapter order. The finale follows the epilogue dialogue.
SHOTS = (
    ("opening", "BEFORE THE FIRST LIGHT", 96),
    ("first_folk", "THE FIRST RAIN", 69),
    ("roof", "A ROOF AGAINST THE NIGHT", 72),
    ("river_oath", "TWO PEOPLES, ONE RIVER", 72),
    ("first_darkness", "THE KING IN THE DARK", 72),
    ("iron", "THE FIRST FORGE", 72),
    ("stone", "WHAT STONE REMEMBERS", 72),
    ("ink", "GIVING THE DARK A NAME", 60),
    ("one_fire", "ONE FIRE", 60),
    ("siege", "HOLD THE WALLS", 84),
    ("city", "THE CITY THAT NEVER SLEEPS", 72),
    ("library", "THE OTHER SIDE OF THE PAGE", 72),
    ("old_god", "THE WORLDS HE LOST", 78),
    ("godhood", "SHOW ME", 72),
    ("finale", "THERE IS ROOM FOR TWO", 108),
)

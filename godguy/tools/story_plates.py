"""Illustrated sets and small animation cels for the chapter-film timelines."""
from PIL import ImageDraw

from story_art import (
    AMBER, ASH, CLAY, CLOUD, CREAM, ICE, INK, JADE, LEAF, MINT, PLUM,
    RUST, SEA, SLATE, STONE, SUN, TAN, TEAL, WHITE, WOOD,
    WISP, HOLLOW, MIRA_ELDER, actor, canvas,
)
from story_shots import (
    HEIGHT, WIDTH, atlas, coast, fire, gate, hall, house, interior, landscape,
    masonry, orbit, river, throne,
)

# Keep this order in step with the set and prop IDs in story_reels.h.
SETS = (
    "orbit", "shore", "storm", "village", "ford", "gate", "forge",
    "hall", "archive", "hearth", "throne", "couch", "porch", "ruins",
)
PROPS = (
    "cup", "goat", "hammer", "spoon", "quill", "lamp", "crown", "leaf",
    "bread", "mask", "crumb", "seed",
)
# DS keeps compact 28x34 2D pixels while gaining real dialogue poses.
# Six compressed keys per character remain vastly smaller than the 3DS
# twelve-pose 40x48 sheets and require no runtime animation buffers.
CEL_WIDTH, CEL_HEIGHT, CEL_N = 28, 34, 6
PROP_SIZE = 20


def plate(name, flat=False):
    image = canvas((WIDTH, HEIGHT))
    d = ImageDraw.Draw(image)
    if name == "orbit":
        if flat:
            atlas(d, 27)
        else:
            orbit(image, d, 27)
    elif name in ("shore", "storm", "village"):
        coast(d, 32, settled=name == "village", storm=name == "storm")
    elif name == "ford":
        river(d, 10)
        for x, y in ((48, 67), (62, 65), (77, 68)):
            d.ellipse((x-7, y-1, x+7, y+2), fill=STONE)
            d.line((x-4, y-1, x+4, y-1), fill=ASH)
    elif name == "gate":
        gate(d, 18)
    elif name in ("forge", "archive"):
        interior(d, 12, forge=name == "forge", library=name == "archive")
        if name == "forge":
            d.polygon(((45, 54), (82, 54), (77, 61), (72, 70), (55, 70),
                       (56, 64), (49, 62)), fill=STONE, outline=INK)
            d.line((47, 55, 80, 55), fill=ASH)
    elif name == "porch":
        # A recognisable home, not the generic hall/forge room reused in
        # every dialogue shot. This compact 128x72 plate is shared by DS,
        # DSi, and the wider 3DS version; no additional set allocation.
        d.rectangle((0, 0, 127, 49), fill=STONE)
        d.polygon(((0, 47), (127, 48), (127, 71), (0, 71)),
                  fill=WOOD)
        d.polygon(((0, 48), (56, 45), (91, 45), (127, 48)),
                  fill=CLAY)
        # Rafter beams, brackets and pitched perspective ceiling.
        d.rectangle((0, 0, 127, 7), fill=WOOD)
        d.line((0, 7, 127, 7), fill=TAN, width=2)
        for x in (6, 49, 85, 126):
            d.line((x, 8, x, 45), fill=WOOD, width=3)
            d.line((x+2, 9, x+2, 41), fill=TAN)
        d.line((0, 12, 64, 0), fill=ASH, width=2)
        d.line((64, 0, 127, 12), fill=ASH, width=2)
        # Door at left, frame and threshold fixed across cuts.
        d.rectangle((11, 15, 37, 55), fill=WOOD, outline=INK)
        d.rectangle((14, 18, 34, 51), fill=SLATE)
        d.polygon(((15, 19), (30, 20), (29, 50), (15, 50)),
                  fill=CLAY, outline=WOOD)
        d.ellipse((27, 36, 29, 38), fill=SUN)
        d.line((10, 54, 40, 54), fill=TAN, width=2)
        # Exactly the same window whose rain layer and sill are updated
        # by filmWorldContinuity at screen x=211..241, y=56..94.
        d.rectangle((101, 13, 124, 39), fill=WOOD, outline=INK)
        d.rectangle((105, 16, 121, 35), fill=SEA)
        d.polygon(((106, 17), (120, 17), (118, 24), (106, 29)),
                  fill=TEAL)
        d.line((113, 16, 113, 35), fill=ASH)
        d.line((105, 26, 121, 26), fill=ASH)
        d.line((99, 39, 125, 39), fill=TAN, width=3)
        d.line((101, 14, 101, 38), fill=CLAY, width=2)
        # A low bench, woven blanket and table with four planted legs.
        d.polygon(((43, 46), (78, 46), (86, 52), (38, 52)),
                  fill=CLAY, outline=WOOD)
        d.rectangle((39, 52, 85, 58), fill=WOOD)
        d.line((44, 58, 44, 66), fill=STONE, width=3)
        d.line((79, 58, 79, 66), fill=STONE, width=3)
        d.polygon(((43, 49), (63, 49), (73, 52), (43, 52)),
                  fill=PLUM)
        d.line((48, 51, 65, 51), fill=RUST, width=2)
        d.polygon(((50, 59), (91, 59), (101, 64), (42, 64)),
                  fill=TAN, outline=WOOD)
        for x in (48, 94):
            d.line((x, 64, x-2, 71), fill=WOOD, width=3)
        # Floorboard junctions converge toward the door, not a grid.
        for x in (-18, 23, 81, 137):
            d.line((61, 47, x, 71), fill=CLAY)
        d.line((0, 60, 127, 61), fill=CLAY)
        # One practical lantern mounted to a bracket, no floating lights.
        d.line((89, 18, 98, 18), fill=WOOD, width=2)
        d.rectangle((93, 21, 100, 30), fill=WOOD, outline=INK)
        d.rectangle((95, 23, 98, 27), fill=AMBER)
        d.point((96, 25), fill=CREAM)
    elif name == "hall":
        hall(d, 1)
    elif name == "throne":
        throne(d, 0, crown=False)
    elif name == "ruins":
        landscape(d, 3, "night")
        masonry(d, (5, 39, 38, 63))
        masonry(d, (90, 34, 123, 63))
        for x, y in ((17, 38), (97, 33), (100, 37), (24, 40)):
            d.polygon(((x-4, y-4), (x+4, y-3), (x+1, y+2), (x-2, y+7)), fill=SLATE)
        d.line((45, 62, 57, 54, 62, 64, 82, 70), fill=STONE, width=2)
    else:
        landscape(d, 2, "night")
        if name == "hearth":
            house(d, 7, 56, 5, SUN)
            house(d, 105, 59, 6, AMBER)
            fire(d, 65, 68, 11, 14)
        else:
            d.rounded_rectangle((3, 48, 124, 71), radius=9, fill=PLUM, outline=INK, width=2)
            d.line((10, 60, 117, 60), fill=TAN, width=2)
            d.line((12, 63, 116, 63), fill=CLAY)
    return image.tobytes()


def cast_cel(who, index):
    image = canvas((CEL_WIDTH, CEL_HEIGHT), 255)
    pose, phase = (
        ("idle", 0), ("walk", 6), ("walk", 10),
        ("reach", 0), ("talk", 0), ("listen", 0),
    )[index]
    if who == HOLLOW and index == 3:
        pose = "bow"
    actor(image, who, CEL_WIDTH//2, CEL_HEIGHT, phase, CEL_HEIGHT, pose=pose)
    return image.tobytes()


def prop(name):
    image = canvas((PROP_SIZE, PROP_SIZE), 255)
    d = ImageDraw.Draw(image)
    if name == "goat":
        d.ellipse((2, 8, 14, 15), fill=CLOUD, outline=INK)
        d.line((5, 13, 4, 18), fill=WOOD, width=2)
        d.line((11, 13, 12, 18), fill=WOOD, width=2)
        d.polygon(((10, 10), (14, 5), (17, 6), (19, 9), (14, 12)), fill=CREAM, outline=INK)
        d.line((13, 6, 12, 2, 14, 0), fill=TAN, width=2)
        d.line((16, 6, 16, 2, 18, 1), fill=TAN)
        d.point((16, 8), fill=INK)
        d.line((3, 10, 1, 7), fill=WHITE)
        d.point((13, 12), fill=AMBER)
    elif name == "cup":
        d.ellipse((12, 7, 19, 14), outline=TAN, width=2)
        d.polygon(((2, 4), (14, 4), (13, 16), (5, 16)), fill=CLAY, outline=WOOD)
        d.ellipse((2, 3, 14, 6), fill=SUN, outline=AMBER)
        d.line((4, 6, 6, 14), fill=TAN)
    elif name == "hammer":
        d.line((5, 17, 13, 5), fill=WOOD, width=4)
        d.line((5, 16, 13, 6), fill=TAN)
        d.rounded_rectangle((7, 3, 18, 8), radius=1, fill=ASH, outline=STONE)
        d.line((8, 4, 17, 4), fill=ICE)
    elif name in ("spoon", "quill"):
        color = ICE if name == "spoon" else CREAM
        d.line((3, 17, 13, 7), fill=ASH, width=3)
        if name == "spoon":
            d.ellipse((10, 2, 18, 10), fill=color, outline=ASH)
        else:
            d.polygon(((7, 13), (10, 4), (19, 1), (15, 8)), fill=color, outline=TAN)
    elif name == "lamp":
        d.arc((6, 0, 14, 8), 180, 360, fill=TAN, width=2)
        d.rectangle((4, 5, 15, 17), fill=WOOD, outline=INK)
        d.rectangle((7, 7, 12, 14), fill=AMBER)
        d.ellipse((8, 9, 11, 13), fill=CREAM)
    elif name == "crown":
        d.polygon(((2, 14), (1, 5), (5, 9), (10, 3), (13, 9), (18, 5), (17, 14)),
                  fill=AMBER, outline=WOOD)
        d.line((3, 13, 16, 13), fill=SUN)
        d.point((10, 10), fill=TEAL)
    elif name in ("leaf", "seed"):
        d.line((9, 18, 10, 6), fill=LEAF, width=2)
        d.ellipse((4, 6, 10, 11), fill=JADE)
        d.ellipse((10, 2, 17, 8), fill=MINT)
        if name == "seed":
            d.ellipse((4, 14, 16, 19), fill=WOOD)
    elif name in ("bread", "crumb"):
        d.rounded_rectangle((3, 7, 17, 15), radius=3, fill=AMBER, outline=WOOD)
        for x in (7, 10, 13):
            d.line((x, 8, x-2, 11), fill=CREAM)
    elif name == "mask":
        d.polygon(((3, 4), (15, 3), (17, 9), (13, 17), (6, 15), (2, 9)),
                  fill=ASH, outline=INK)
        d.line((5, 8, 8, 8), fill=INK)
        d.line((11, 7, 14, 7), fill=INK)
        d.line((10, 5, 9, 10, 12, 13), fill=CLOUD)
    return image.tobytes()


def plate_hd(name):
    """Add native-resolution details for the 3DS camera to move through."""
    matte = canvas((WIDTH, HEIGHT))
    matte.frombytes(plate(name))
    image = matte.resize((WIDTH*2, HEIGHT*2))
    d = ImageDraw.Draw(image)
    if name == "orbit":
        for i in range(37):
            x, y = (i*73+25)%254, (i*29+10)%113
            if (x-134)**2+(y-86)**2 > 49**2:
                d.point((x, y), fill=ASH if i%3 else CLOUD)
        d.arc((83, 34, 184, 136), 204, 313, fill=TEAL)
        d.arc((80, 32, 187, 139), 219, 268, fill=SEA)
        for x, y in ((36, 33), (213, 26), (208, 90)):
            d.line((x-3, y, x+3, y), fill=ASH)
            d.line((x, y-2, x, y+2), fill=CLOUD)
            d.point((x,y), fill=WHITE)
    elif name in ("shore", "storm", "village", "ford"):
        light = TEAL if name in ("storm", "ford") else TAN
        for i in range(17):
            x, y = (i*37+19)%246, 21+(i*19)%56
            d.line((x, y, x+3+i%7, y), fill=light if i%4==0 else SLATE)
        for i in range(17):
            x, y = 86+(i*23)%83, 116+(i*7)%22
            d.line((x, y, x+2+i%8, y), fill=JADE if i%4==0 else TEAL)
        if name == "village":
            for x, y in ((22, 113), (49, 106), (81, 111), (221, 115), (249, 108)):
                d.line((x-3, y, x+3, y), fill=AMBER)
                d.point((x, y-2), fill=CREAM)
    elif name in ("porch", "forge", "archive"):
        for x in (17, 92, 174, 244):
            d.line((x, 6, x, 104), fill=TAN if name == "porch" else ASH)
        for i in range(20):
            x, y = 10+(i*47)%234, 116+(i*8)%25
            d.line((x, y, x+2+i%9, y), fill=CLAY)
        if name == "archive":
            for x in (14, 172):
                for y in (29, 61, 93):
                    d.line((x, y, x+63, y), fill=ASH)
                    for i in range(8):
                        d.point((x+3+8*i, y-8-i%3), fill=CREAM if i%3==0 else TAN)
        if name == "forge":
            for i in range(15):
                d.point((204+i*17%33, 78+i*13%22), fill=AMBER)
        if name == "porch":
            d.line((83, 124, 177, 124), fill=TAN)
    elif name in ("hall", "throne", "ruins", "gate"):
        for i in range(23):
            x, y = 11+(i*41)%231, 65+(i*17)%56
            d.line((x, y, x+2+i%4, y), fill=ASH if i%4==0 else STONE)
        if name == "hall":
            for x in (24, 90, 156, 222):
                d.line((x-1, 23, x-1, 73), fill=CREAM)
                d.line((x-7, 48, x+7, 48), fill=ASH)
        if name == "throne":
            d.arc((96, 15, 160, 91), 205, 332, fill=ASH)
    else:
        for i in range(25):
            x, y = 5+(i*67)%244, 13+(i*47)%75
            d.point((x, y), fill=CLOUD if i%5==0 else ASH)
        if name == "couch":
            for x in range(29, 234, 28):
                d.line((x, 102, x+2, 117), fill=TAN)
                d.point((x+1, 103), fill=CREAM)
    return image.tobytes()

"""Indexed pixel art shared by GodGuy's story movies and dialogue portraits."""
from PIL import Image, ImageDraw

(
    VOID, INK, INDIGO, SLATE, PURPLE, WINE, RUST, RED,
    AMBER, SUN, CREAM, WHITE, SEA, TEAL, JADE, MINT,
    PINE, LEAF, OLIVE, LIME, STONE, ASH, CLOUD, ICE,
    WOOD, CLAY, TAN, SKIN, ROSE, LILAC, PLUM, BLUE,
) = range(32)

PALETTE = (
    (10, 8, 21), (22, 18, 39), (35, 36, 69), (51, 55, 91),
    (77, 55, 101), (102, 50, 75), (154, 64, 75), (210, 88, 81),
    (243, 144, 83), (255, 195, 110), (255, 226, 160), (255, 245, 218),
    (24, 55, 76), (35, 86, 103), (47, 136, 140), (127, 206, 187),
    (29, 62, 58), (53, 99, 73), (102, 147, 88), (171, 190, 101),
    (65, 71, 84), (109, 115, 125), (165, 183, 184), (211, 230, 217),
    (80, 47, 48), (130, 76, 61), (189, 125, 83), (236, 173, 124),
    (213, 131, 141), (177, 144, 202), (113, 76, 140), (72, 117, 166),
)
# Nessa is the bridgewright; Serin once drew siege maps for the Hollow King.
# Their full-body acting cels are authored for the 3DS film. DS portraits and
# compressed assets intentionally retain the original eight-person cast.
WISP, MIRA, GRUUL, ELOWEN, THORGRIM, HOLLOW, DAVE, MIRA_ELDER, NESSA, SERIN = range(10)


def canvas(size, color=VOID):
    image = Image.new("P", size, color)
    image.putpalette([channel for rgb in PALETTE for channel in rgb])
    return image


def spark(draw, x, y, color, core, radius=2):
    draw.line((x-radius, y, x+radius, y), fill=color)
    draw.line((x, y-radius, x, y+radius), fill=color)
    draw.point((x, y), fill=core)


def cast_sprite(who, talking=False, blink=False, react=False):
    # The transparency key is outside the 32-color palette. Portraits use
    # these same silhouettes at 2x and movies can stage them at either scale.
    image = canvas((32, 32), 255)
    d = ImageDraw.Draw(image)
    if who == WISP:
        d.arc((3, 4, 28, 28), 190, 355, fill=TEAL, width=2)
        d.arc((6, 6, 27, 25), 15, 120, fill=JADE)
        d.polygon([(16, 2), (19, 12), (28, 16), (20, 19), (16, 30),
                   (12, 21), (4, 18), (12, 14)], fill=TEAL, outline=INK)
        d.polygon([(16, 6), (19, 14), (23, 17), (18, 19), (16, 25),
                   (12, 19), (9, 17), (14, 14)], fill=MINT)
        d.ellipse((13, 15 if blink else 13, 20 if react else 19, 18 if blink else 20), fill=WHITE)
        d.point((20 if react else 19, 16 if talking else 17), fill=TEAL)
        d.point((7, 9), fill=MINT)
        d.point((25, 24), fill=JADE)
    elif who == HOLLOW:
        d.polygon([(5, 30), (7, 21), (11, 17), (10, 8), (14, 4), (23, 6),
                   (25, 14), (23, 20), (28, 26), (30, 31)], fill=INK, outline=VOID)
        d.polygon([(7, 30), (11, 22), (14, 21), (15, 26), (24, 24), (28, 31)],
                  fill=PLUM)
        d.polygon([(13, 11), (10, 3), (15, 6), (17, 1), (20, 6), (24, 4),
                   (23, 10)], fill=AMBER, outline=VOID)
        d.polygon([(13, 12), (22, 10), (23, 16), (20, 23), (15, 21), (12, 17)],
                  fill=ASH)
        d.polygon([(13, 12), (18, 11), (17, 20), (14, 18)], fill=ICE)
        d.line((20, 11, 18, 15, 20, 17, 18, 21), fill=INK)
        d.line((13, 15, 15, 15), fill=VOID)
        d.line((20, 15, 22, 15), fill=VOID)
        if not blink:
            d.point((14, 15), fill=MINT)
            d.point((21, 15), fill=MINT)
        d.point((17, 19 + talking), fill=INK)
        if react:
            d.line((12, 13, 15, 12), fill=SLATE)
            d.line((20, 12, 23, 13), fill=SLATE)
        d.line((10, 24, 15, 29), fill=LILAC)
        d.point((17, 27), fill=AMBER)
    else:
        coat, shade, trim, skin, hair = {
            MIRA: (TEAL, SEA, MINT, SKIN, WOOD),
            MIRA_ELDER: (TEAL, SEA, MINT, TAN, CLOUD),
            GRUUL: (RUST, WINE, AMBER, OLIVE, PINE),
            ELOWEN: (PLUM, PURPLE, LILAC, SKIN, TAN),
            THORGRIM: (CLAY, WOOD, AMBER, TAN, RUST),
            DAVE: (PURPLE, INDIGO, LILAC, SKIN, ASH),
            NESSA: (JADE, PINE, ICE, TAN, INK),
            SERIN: (SLATE, INDIGO, CLOUD, SKIN, ASH),
        }[who]
        d.polygon([(3, 31), (6, 25), (12, 21), (20, 21), (26, 24),
                   (29, 31)], fill=shade, outline=INK)
        d.polygon([(6, 31), (10, 24), (15, 22), (23, 24), (26, 31)], fill=coat)
        d.line((11, 23, 16, 26, 22, 23), fill=trim, width=2)
        d.line((9, 28, 7, 31), fill=trim)
        d.polygon([(8, 11), (11, 6), (21, 6), (25, 12), (24, 19), (20, 24),
                   (14, 24), (9, 20)], fill=skin, outline=INK)
        d.line((21, 12, 22, 17, 19, 21), fill=TAN if who != GRUUL else LEAF, width=2)
        d.line((12, 15, 14, 15), fill=INK)
        d.line((19, 15, 21, 15), fill=INK)
        if not blink:
            d.point((13, 15), fill=WHITE)
            d.point((20, 15), fill=WHITE)
        d.line((16, 16, 15, 18, 17, 18), fill=CLAY if who != GRUUL else LEAF)
        d.line((15, 21, 18, 21), fill=WOOD)
        if talking:
            d.point((17, 22), fill=INK)
        if react:
            d.line((12, 13, 14, 12), fill=hair)
            d.line((19, 12, 21, 13), fill=hair)
            d.line((15, 20, 17, 22, 19, 20), fill=WOOD)
        if who in (MIRA, MIRA_ELDER):
            d.polygon([(7, 11), (9, 6), (15, 3), (22, 5), (25, 10),
                       (22, 12), (17, 8), (12, 14), (9, 22), (5, 25)], fill=hair, outline=INK)
            d.line((8, 11, 11, 7, 16, 5), fill=TAN if who == MIRA else ICE)
            d.line((8, 17, 7, 23, 5, 26), fill=hair, width=3)
            d.point((8, 21), fill=SUN)
            d.rectangle((19, 27, 22, 29), fill=AMBER)
            d.point((20, 28), fill=SEA)
        elif who == GRUUL:
            d.polygon([(6, 13), (3, 11), (4, 17), (10, 19), (8, 8),
                       (15, 4), (23, 6), (27, 10), (30, 10), (27, 17),
                       (24, 18), (23, 10), (12, 10), (10, 16)], fill=hair, outline=INK)
            d.line((10, 9, 16, 6, 23, 8), fill=LEAF)
            d.polygon([(11, 22), (12, 18), (14, 23)], fill=CREAM)
            d.polygon([(21, 22), (22, 18), (19, 23)], fill=CREAM)
            d.line((24, 24, 16, 31), fill=RED, width=3)
        elif who == ELOWEN:
            d.polygon([(10, 16), (3, 9), (5, 16), (10, 20),
                       (10, 11), (20, 8), (23, 13), (25, 19), (29, 8),
                       (24, 10), (19, 4), (11, 5), (8, 8)], fill=hair, outline=INK)
            d.line((9, 9, 15, 6, 20, 6), fill=SUN)
            d.line((12, 8, 16, 10, 20, 8), fill=JADE)
            d.rectangle((19, 26, 25, 31), fill=INK)
            d.rectangle((20, 26, 23, 30), fill=LILAC)
            d.line((24, 27, 24, 30), fill=CREAM)
        elif who == THORGRIM:
            d.polygon([(6, 13), (7, 7), (14, 4), (22, 5), (26, 10),
                       (25, 14), (19, 11), (9, 14)], fill=STONE, outline=INK)
            d.line((8, 11, 15, 7, 23, 8), fill=ASH)
            d.polygon([(9, 18), (13, 20), (16, 19), (21, 20), (24, 18),
                       (23, 25), (17, 29), (11, 25)], fill=hair, outline=WOOD)
            d.line((13, 23, 16, 27), fill=AMBER)
            d.point((18, 26), fill=SUN)
            d.line((26, 24, 22, 31), fill=ASH, width=2)
            d.rectangle((23, 22, 28, 25), fill=STONE)
        elif who == NESSA:
            # Long dark scarf, braided hair and copper clasp distinguish
            # the river engineer from the existing village characters.
            d.polygon(((6, 13), (10, 4), (22, 4), (26, 13), (23, 24),
                       (21, 12), (13, 11), (8, 27)),fill=INK,outline=PINE)
            d.line((10, 5, 19, 4, 23, 9),fill=ASH,width=2)
            d.line((9, 25, 7, 30, 10, 31),fill=WOOD,width=3)
            d.line((11, 23, 19, 29, 25, 25),fill=ICE,width=2)
            d.rectangle((18, 26, 22, 29),fill=AMBER)
        elif who == SERIN:
            # White-streaked hair, narrow ceremonial mantle and a damaged
            # bronze pin: a former royal surveyor, not another masked guard.
            d.polygon(((7, 12),(9, 4),(15, 6),(21, 2),(26, 9),
                       (24, 15),(18, 10),(11, 12)),fill=ASH,outline=INK)
            d.line((11, 6, 20, 4, 24, 9),fill=WHITE,width=2)
            d.polygon(((7, 26),(14, 22),(18, 27),(24, 23),(28, 31),
                       (8, 31)),fill=INDIGO)
            d.line((12, 26, 17, 28, 22, 26),fill=CLOUD,width=2)
            d.point((18, 28),fill=AMBER)
            d.line((12, 20, 19, 20),fill=ASH)
        elif who == DAVE:
            d.polygon([(4, 13), (9, 10), (11, 2), (16, 1), (22, 5),
                       (18, 5), (23, 11), (29, 13), (22, 15), (9, 15)],
                      fill=coat, outline=INK)
            d.line((10, 10, 22, 10), fill=LILAC, width=2)
            d.point((16, 10), fill=SUN)
            d.polygon([(9, 19), (14, 22), (20, 21), (24, 18),
                       (21, 25), (16, 29), (11, 26)], fill=hair)
            d.line((12, 22, 16, 25, 21, 22), fill=CLOUD)
            d.line((14, 20, 19, 20), fill=WOOD)
            if talking:
                d.point((17, 21), fill=INK)
            d.polygon([(22, 27), (26, 28), (24, 31)], fill=SUN)
    return image


def cast(frame, who, x, y, time, size=32, flip=False, speaking=False):
    # One-pixel breathing and a short blink, plus a talking cel where staged.
    sway = (time // 8) % 4 == 2
    sprite = cast_sprite(who, speaking and time // 3 % 2 == 0,
                         (time + who * 9) % 68 in (0, 1))
    if size != 32:
        sprite = sprite.resize((size, size), Image.Resampling.NEAREST)
    if flip:
        sprite = sprite.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    frame.paste(sprite, (x-size//2, y-size-int(sway)), sprite.point(lambda i: 255 if i != 255 else 0, "1"))


def actor(frame, who, x, y, time, height, *, flip=False, pose="idle", prop=None):
    """Stage whole-body movement with separate limbs, props and expressions."""
    if who == WISP:
        # Wisp never grows human hands. Animate its actual spark silhouette:
        # eye-line, wing arc, light pulse and a deliberate forward lean.
        drift = ((0, 1, 2, 1)[time//2 % 4] if pose == "walk"
                 else 2 if pose == "bow" else 0)
        draw = ImageDraw.Draw(frame)
        for i, color in enumerate((MINT, JADE, TEAL, SEA)):
            tail = i*3+2
            draw.point((x-tail*(-1 if flip else 1), y-height//2+tail//2),
                       fill=color)
        sprite = cast_sprite(who,
                             pose == "talk" and time//4 % 2 == 0,
                             pose in ("listen", "bow"),
                             pose == "react")
        size = max(1, height*5//6)
        sprite = sprite.resize((size, size), Image.Resampling.NEAREST)
        if flip:
            sprite = sprite.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        frame.paste(sprite, (x-size//2, y-size+drift),
                    sprite.point(lambda i: 255 if i != 255 else 0, "1"))
        if pose == "reach":
            draw.line((x+4,y-12,x+10,y-7), fill=MINT, width=2)
        elif pose == "lift":
            draw.line((x+1,y-31,x+2,y-37), fill=ICE, width=2)
        elif pose == "brace":
            draw.arc((x-13,y-29,x+10,y-6), 195, 325, fill=JADE, width=2)
        elif pose == "work":
            draw.line((x-12,y-17,x-17,y-18), fill=TEAL, width=2)
        return

    shade, cloth, trim, skin = {
        MIRA: (SEA, TEAL, MINT, SKIN), MIRA_ELDER: (SEA, TEAL, MINT, TAN),
        GRUUL: (WINE, RUST, AMBER, OLIVE), ELOWEN: (PURPLE, PLUM, LILAC, SKIN),
        THORGRIM: (WOOD, CLAY, AMBER, TAN), HOLLOW: (INK, INDIGO, PLUM, ASH),
        DAVE: (INDIGO, PURPLE, LILAC, SKIN),
        NESSA: (PINE, JADE, ICE, TAN),
        SERIN: (SLATE, INDIGO, CLOUD, SKIN),
    }[who]
    image = canvas((40, 48), 255)
    d = ImageDraw.Draw(image)
    # Directed acting keys: anticipation, contact, lift, hold and recovery.
    step = (-4, -2, 0, 3, 5, 2, 0, -2)[time//2 % 8] if pose == "walk" else 0
    bob = abs(step)//3
    lean = 2 if pose in ("reach", "bow", "work", "brace") else (-1 if pose == "react" else 0)
    for foot in (15+step, 24-step):
        heel = 45-abs(step)//3
        d.line((19, 32-bob, foot, heel), fill=INK, width=5)
        d.line((19, 32-bob, foot, heel-1), fill=shade, width=3)
        d.line((foot-1, heel, foot+3, heel), fill=WOOD, width=2)
    if who == HOLLOW:
        wind = (0,1,2,1,0,-1,-2,-1)[time//2 % 8]
        d.polygon(((12, 18), (22, 20), (25, 30), (31+wind, 44),
                   (13+wind, 41), (7+wind, 45)), fill=shade, outline=VOID)
        d.line((13, 25, 12+wind, 42), fill=trim, width=2)
    d.polygon(((10, 23-bob), (25, 22-bob), (27, 32-bob),
               (22, 37-bob), (12, 36-bob), (9, 28-bob)),
              fill=cloth, outline=INK)
    d.line((11, 33-bob, 25, 33-bob), fill=shade, width=3)
    d.rectangle((18, 32-bob, 20, 34-bob), fill=trim)
    # Clear shoulder seams and fitted collar: the neck actually joins the
    # torso rather than a large disembodied portrait hovering above it.
    # Only the 3DS 40x48 acting keys use this proportional full-body face;
    # readable 32x32 DS/dialogue portraits remain completely unchanged.
    d.polygon(((12, 23-bob), (16, 21-bob), (20, 21-bob), (25, 23-bob),
               (22, 27-bob), (15, 27-bob)),
              fill=shade, outline=INK)
    d.polygon(((16+lean, 18-bob), (21+lean, 18-bob),
               (21+lean, 24-bob), (16+lean, 24-bob)),
              fill=skin, outline=INK)
    d.line((13, 24-bob, 17, 27-bob), fill=trim, width=2)
    d.line((23, 24-bob, 20, 27-bob), fill=trim, width=2)
    head = cast_sprite(who, pose == "talk" and time//4 % 2 == 0,
                       pose == "listen" or (time+who*9)%79 < 3,
                       pose == "react")
    # Previous 23x21 portrait dominated nearly half a 48-pixel body. A
    # narrower, reshaped 18x16 face leaves room for the shoulders, neck,
    # waist, knees and ankles in a clean silhouette. Retain the original
    # detailed face artwork and resample only once during asset generation.
    head_width = 19 if who in (GRUUL, THORGRIM) else 18
    top = head.crop((3, 0, 31, 25)).resize((head_width, 16),
                                           Image.Resampling.NEAREST)
    nod = 2 if pose == "bow" else (1 if pose == "brace" else 0)
    face_x = 19-head_width//2+lean
    image.paste(top, (face_x, 5+nod-bob),
                top.point(lambda i: 255 if i != 255 else 0, "1"))
    # A tiny jaw shadow, forehead plane and one-pixel neckline help the
    # character read as a torso with a face rather than a spherical head.
    d.line((face_x+4, 20+nod-bob, face_x+head_width-5, 20+nod-bob),
           fill=shade)
    d.point((19+lean, 22+nod-bob), fill=trim)
    left_hand = {
        "brace": (4, 26), "react": (7, 20), "work": (8, 29),
        "talk": (5, 29), "listen": (8, 34),
    }.get(pose, (9-step//2, 34-bob))
    d.line((11, 24-bob, 7-step//2, 29-bob, *left_hand),
           fill=shade, width=4)
    hx, hy = {
        "reach": (35, 25), "lift": (28, 15), "bow": (30, 32),
        "work": (34, 29), "brace": (35, 37), "react": (30, 23),
        "talk": (30, 27), "listen": (26, 34),
        "write": (31+(time//2)%3, 32+(time//2)%2),
    }.get(pose, (26+step//2, 34-bob))
    d.line((24, 24-bob, 27, hy+2, hx, hy), fill=shade, width=5)
    d.line((25, 24-bob, 27, hy+1, hx, hy), fill=cloth, width=3)
    d.rectangle((hx-1, hy-1, hx+1, hy+1), fill=skin)
    if prop == "cup":
        d.rectangle((hx-1, hy-5, hx+3, hy-2), fill=CLAY, outline=AMBER)
        d.arc((hx+2, hy-6, hx+6, hy-2), 260, 100, fill=SUN)
    elif prop == "hammer":
        d.line((hx, hy, hx+4, hy-5), fill=WOOD, width=2)
        d.rectangle((hx+1, hy-8, hx+7, hy-5), fill=ASH, outline=STONE)
    elif prop == "quill":
        d.line((hx-2, hy+3, hx+4, hy-6), fill=CREAM)
        d.polygon(((hx, hy), (hx+1, hy-6), (hx+5, hy-8), (hx+3, hy-2)), fill=ICE)
    if flip:
        image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    width = max(1, height*40//48)
    if height != 48:
        image = image.resize((width, height), Image.Resampling.NEAREST)
    frame.paste(image, (x-width//2, y-height), image.point(lambda i: 255 if i != 255 else 0, "1"))

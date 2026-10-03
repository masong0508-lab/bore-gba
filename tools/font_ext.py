#!/usr/bin/env python3
"""EXTENDED FONT for the BORE font tool (run it with:  python3 tools/make_font.py --extend).

What it does to assets/font/bore_font.png / .json (the plain A-Z, 0-9, punctuation and a-z glyphs stay pixel-identical):
  * every size gets a few extra rows ABOVE the capitals ("top"), so accents on capitals have room. The game draws text with its top
    moved up by that much (FTOP_x in source/fontdata.h), so nothing on screen moves; glyphs without ink up there skip the rows.
  * ASCII { } | ` \\ , and the Latin letters with marks:  Latin-1 (A-grave ... y-diaeresis), Latin Extended-A (Polish, Czech, Slovak,
    Hungarian, Romanian, Turkish, Croatian, Baltic ...), s-sharp, ae oe, o-slash, l-stroke, d-stroke, inverted ? !, guillemets,
    degree, plus-minus, multiply, divide, pound, euro, yen, cent, section, bullet, ellipsis, en and em dash.
  * accents are drawn over the plain letter by a small set of mark shapes (acute grave circumflex diaeresis tilde ring caron breve dot
    double-acute macron, ogonek / cedilla / comma below, an apostrophe for d' t' l'), so they are easy to retune here.
Glyphs found by Unicode code point (FNT_EXTRA); anything the font still lacks falls back to its plain letter (FNT_FALLBACK).
Idempotent: it rebuilds all the non-ASCII glyphs from the plain ones every time."""
import json, unicodedata
from PIL import Image, ImageDraw, ImageFont

# extra rows above the capitals per size, accent height, gap between accent and letter, pen for strokes, tail height below the baseline
TOPS = {"s": dict(top=2, mark=2, gap=0, pen=1, tail=2, apos=(2, 1, 2), below_mark=2),
        "m": dict(top=4, mark=3, gap=1, pen=2, tail=3, apos=(4, 2, 4), below_mark=3),
        "l": dict(top=8, mark=6, gap=2, pen=3, tail=6, apos=(9, 4, 6), below_mark=5)}   # apos = (height, bar width, extra width)

PAT = {"acute": ["..#", ".#."], "grave": ["#..", ".#."], "circ": [".#.", "#.#"], "diaer": ["#.#", "..."], "tilde": ["##.", ".##"],
       "ring": ["###", "#.#"], "caron": ["#.#", ".#."], "breve": ["#.#", "###"], "dot": [".#.", "..."], "dacute": ["#.#", "#.#"],
       "macron": ["###", "..."], "cedilla": [".#.", "##."], "comma": [".#.", "#.."], "ogonek": ["..#", ".#."]}
BELOW = {"cedilla", "comma", "ogonek"}

# char base mark   (mark: a PAT name, apos, nodot, slash, lslash, bar, dbar)
MARKED = """
À A grave|Á A acute|Â A circ|Ã A tilde|Ä A diaer|Å A ring|Ç C cedilla|È E grave|É E acute|Ê E circ|Ë E diaer|Ì I grave|Í I acute|Î I circ|Ï I diaer
Ñ N tilde|Ò O grave|Ó O acute|Ô O circ|Õ O tilde|Ö O diaer|Ù U grave|Ú U acute|Û U circ|Ü U diaer|Ý Y acute
á a acute|à a grave|â a circ|ä a diaer|ã a tilde|å a ring|ç c cedilla|é e acute|è e grave|ê e circ|ë e diaer|í i acute|ì i grave|î i circ|ï i diaer
ñ n tilde|ó o acute|ò o grave|ô o circ|ö o diaer|õ o tilde|ú u acute|ù u grave|û u circ|ü u diaer|ý y acute|ÿ y diaer
Ā A macron|ā a macron|Ă A breve|ă a breve|Ą A ogonek|ą a ogonek|Ć C acute|ć c acute|Č C caron|č c caron|Ď D caron|ď d apos|Đ D dbar|đ d bar
Ē E macron|ē e macron|Ė E dot|ė e dot|Ę E ogonek|ę e ogonek|Ě E caron|ě e caron|Ğ G breve|ğ g breve|Ī I macron|ī i macron|Į I ogonek|į i ogonek
İ I dot|ı i nodot|Ĺ L acute|ĺ l acute|Ľ L apos|ľ l apos|Ł L lslash|ł l lslash|Ń N acute|ń n acute|Ň N caron|ň n caron|Ō O macron|ō o macron
Ő O dacute|ő o dacute|Ŕ R acute|ŕ r acute|Ř R caron|ř r caron|Ś S acute|ś s acute|Ş S cedilla|ş s cedilla|Š S caron|š s caron
Ţ T cedilla|ţ t cedilla|Ť T caron|ť t apos|Ū U macron|ū u macron|Ů U ring|ů u ring|Ű U dacute|ű u dacute|Ų U ogonek|ų u ogonek
Ź Z acute|ź z acute|Ż Z dot|ż z dot|Ž Z caron|ž z caron|Ș S comma|ș s comma|Ț T comma|ț t comma|Ø O slash|ø o slash
"""
MARKED = [(e.split()[0], e.split()[1], e.split()[2]) for line in MARKED.strip().splitlines() for e in line.split("|")]
LIGS = {"æ": "ae", "Æ": "AE", "œ": "oe", "Œ": "OE"}
NEW_ASCII = "{}|`\\"
SPECIAL_TTF = "ß¿¡«»‹›°±×÷£€¥¢§•…–—"          # drawn from a TTF at the medium and large size, by hand at the small size
SMALL = {   # rows 0..4 above the baseline, 5..6 below; '#' solid
 '{': [".##", ".#.", "##.", ".#.", ".##"], '}': ["##.", ".#.", ".##", ".#.", "##."], '|': ["#"] * 7, '`': ["#.", ".#"],
 '\\': ["#..", "#..", ".#.", "..#", "..#"], '¿': [".#.", "", ".#.", "#..", ".##"], '¡': ["#", "#", "#", "", "#"],
 'ß': ["##.", "#.#", "##.", "#.#", "#.#"], '«': ["", ".#.#", "#.#.", ".#.#"], '»': ["", "#.#.", ".#.#", "#.#."], '‹': ["", ".#", "#.", ".#"],
 '›': ["", "#.", ".#", "#."], '°': [".#.", "#.#", ".#."], '±': [".#.", "###", ".#.", "", "###"], '×': ["", "#.#", ".#.", "#.#"],
 '÷': [".#.", "", "###", "", ".#."], '£': [".##", ".#.", "###", ".#.", "###"], '€': [".##", "###", "#..", "###", ".##"],
 '¥': ["#.#", "#.#", ".#.", "###", ".#."], '¢': [".#.", ".##", "#..", ".##", ".#."], '§': [".##", "#..", ".#.", "..#", "##."],
 '•': ["", "##", "##"], '…': ["", "", "", "", "#.#.#"], '–': ["", "", "###"], '—': ["", "", "#####"],
}

# characters (beyond the ones above) that fall back to a look-alike when a string holds them
MANUAL_FALLBACK = {0x2018: "'", 0x2019: "'", 0x201A: ",", 0x201C: '"', 0x201D: '"', 0x2212: "-", 0xB7: ".", 0xA0: " ",
                   0xD0: "D", 0xF0: "d", 0xDE: "P", 0xFE: "p", 0xA9: "c", 0xAE: "r", 0x2122: "t", 0xBC: "1", 0xBD: "1", 0xBE: "3"}

def fallback_table(extra_chars):
    """(code point, ASCII char) for every Latin letter with a mark that has no glyph of its own, plus the manual ones"""
    have = {ord(c) for c in extra_chars}; out = {}
    for cp in range(0xC0, 0x250):
        if cp in have: continue
        d = unicodedata.normalize("NFD", chr(cp))
        if len(d) > 1 and d[0].isascii() and d[0].isalpha(): out[cp] = d[0]
    for cp, a in MANUAL_FALLBACK.items():
        if cp not in have: out[cp] = a
    return sorted(out.items())

def _bitmap(rows, W, H, top):
    w = max(len(r) for r in rows) or 1
    im = Image.new("L", (w, H), 0)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "#": im.putpixel((x, top + y), 255)
    return im

def _pat(name, pw, ph):
    m = Image.new("L", (3, 2), 0)
    for y, r in enumerate(PAT[name]):
        for x, c in enumerate(r):
            if c == "#": m.putpixel((x, y), 255)
    return m.resize((pw, ph), Image.NEAREST)

def _compose(sz, cells, base, mark):
    T = TOPS[sz]; top = T["top"]; im = cells[base].copy(); H = im.height
    xt = cells["n"].getbbox()[1]                                  # first row of the x-height
    if base == "i":                                               # accents replace the dot
        for y in range(xt):
            for x in range(im.width): im.putpixel((x, y), 0)
        if mark == "nodot": return im
    def widen(im, add_left, add_right):
        n = Image.new("L", (im.width + add_left + add_right, H), 0); n.paste(im, (add_left, 0)); return n
    bb = im.getbbox(); ink_top = bb[1]
    if mark in ("slash", "lslash", "bar", "dbar"):
        if mark == "lslash": im = widen(im, 0, 1)
        if mark == "dbar": im = widen(im, 1, 0)
        W = im.width; bb = im.getbbox(); x0, y0, x1, y1 = bb; d = ImageDraw.Draw(im); pen = T["pen"]
        if mark == "slash": d.line([(x0, y1 - 1), (x1 - 1, y0)], fill=255, width=pen)
        elif mark == "lslash": d.line([(0, y0 + (y1 - y0) * 0.72), (W - 1, y0 + (y1 - y0) * 0.28)], fill=255, width=pen)
        elif mark == "bar": ym = y0 + max(1, (y1 - y0) // 4); d.line([(x0, ym), (x1 - 1, ym)], fill=255, width=max(1, pen - 1) if sz != "s" else 1)
        else: ym = (y0 + y1) // 2; d.line([(0, ym), (x0 + (x1 - x0) * 0.55, ym)], fill=255, width=max(1, pen - 1) if sz != "s" else 1)
        return im
    if mark == "apos":
        ah, bw, ex = T["apos"]; w = im.width; im = widen(im, 0, ex); d = ImageDraw.Draw(im)
        d.rectangle([w + (ex - bw), ink_top, w + (ex - bw) + bw - 1, ink_top + ah - 1], fill=255)
        return im
    W = im.width; pw = max(3, min(max(W, 3), round(W * 0.6)))
    if W < pw: im = widen(im, (pw - W) // 2, pw - W - (pw - W) // 2); W = im.width
    if mark in BELOW:
        ph = T["below_mark"]; row = top + _cap(sz); x0 = (W - pw) if mark == "ogonek" else (W - pw) // 2
        if row + ph > H: ph = H - row
    else:
        ph = T["mark"]; row = ink_top - T["gap"] - ph; x0 = (W - pw) // 2
    if ph < 1: return im
    m = _pat(mark, pw, ph)
    for y in range(ph):
        for x in range(pw):
            if m.getpixel((x, y)) and 0 <= row + y < H: im.putpixel((x0 + x, row + y), 255)
    return im

_CAP = {}
def _cap(sz): return _CAP[sz]

def extend(M):
    meta = json.load(open(M.META)); atlas = Image.open(M.ATLAS).convert("L")
    keep = [c for c in meta["chars"] if ord(c) < 128]
    cells = {}; info = {}
    for name, s in meta["sizes"].items():
        T = TOPS[name]["top"]; old_top = s.get("top", 0); base_h = s["h"] - old_top; _CAP[name] = s["cap"]; info[name] = (base_h, s)
        d = {}
        for ch in keep:
            g = s["glyphs"][ch]; im = atlas.crop((g["x"], s["y"] + old_top, g["x"] + g["w"], s["y"] + s["h"]))
            c = Image.new("L", (g["w"], base_h + T), 0); c.paste(im, (0, T)); d[ch] = c
        cells[name] = d
    new_ascii = [c for c in NEW_ASCII if c not in keep]
    extras = [c for c, _, _ in MARKED] + list(LIGS) + list(SPECIAL_TTF)
    extras = [c for i, c in enumerate(extras) if c not in extras[:i] and c not in keep and c not in NEW_ASCII]
    order = keep + new_ascii + extras
    for name, (base_h, s) in info.items():
        T = TOPS[name]["top"]; H = base_h + T; cap = s["cap"]
        font = ImageFont.truetype(M.ADD_TTF, M.fit_px(M.ADD_TTF, cap))
        def special(ch):
            if name == "s": return _bitmap(SMALL[ch], 0, H, T)
            return _paste(M.quant(M.raster_glyph(font, ch, cap, base_h - cap)), H, T)
        for ch in new_ascii: cells[name][ch] = special(ch)
        for ch, base, mark in MARKED: cells[name][ch] = _compose(name, cells[name], base, mark)
        for ch, pair in LIGS.items():
            a, b = cells[name][pair[0]], cells[name][pair[1]]; c = Image.new("L", (a.width + b.width, H), 0); c.paste(a, (0, 0)); c.paste(b, (a.width, 0)); cells[name][ch] = c
        for ch in SPECIAL_TTF: cells[name][ch] = special(ch)
    # pack: one strip per size, 2 px gutters, strips stacked with a 2 px gap (like the original atlas)
    width = max(sum(cells[n][c].width + 2 for c in order) for n in cells) + 2
    height = sum(info[n][0] + TOPS[n]["top"] + 2 for n in cells) + 2
    out = Image.new("L", (width, height), 0); y = 2; sizes = {}
    for name in meta["sizes"]:
        base_h, s = info[name]; H = base_h + TOPS[name]["top"]; x = 2; table = {}
        for ch in order:
            c = cells[name][ch]; out.paste(c, (x, y)); table[ch] = {"x": x, "w": c.width}; x += c.width + 2
        sizes[name] = {"y": y, "h": H, "top": TOPS[name]["top"], "cap": s["cap"], "gap": s["gap"], "space": s["space"], "ttf_px": s.get("ttf_px"),
                       "used_by": s["used_by"], "glyphs": table}
        y += H + 2
    meta = {"chars": "".join(order), "levels": meta["levels"], "sizes": sizes}
    out.save(M.ATLAS, optimize=True); json.dump(meta, open(M.META, "w"), indent=1)
    print("extended: %d glyphs (%d ASCII, %d more), atlas %s" % (len(order), len(keep) + len(new_ascii), len(extras), out.size))

def _paste(g, H, top):
    c = Image.new("L", (g.width, H), 0); c.paste(g, (0, top)); return c

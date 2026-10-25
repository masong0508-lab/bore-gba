"""pix.py - a tiny pixel painting kit for tools/make_king_art.py: canvases of palette indices, shapes, light-aware fills and previews."""
import math, random
class Cv:
    def __init__(s, w, h, fill=0): s.w, s.h = w, h; s.p = [[fill] * w for _ in range(h)]
    def get(s, x, y): return s.p[y][x] if 0 <= x < s.w and 0 <= y < s.h else None
    def px(s, x, y, c):
        if c is not None and 0 <= x < s.w and 0 <= y < s.h: s.p[y][x] = c
    def rect(s, x, y, w, h, c):
        for j in range(h):
            for i in range(w): s.px(x + i, y + j, c)
    def hline(s, x0, x1, y, c):
        for x in range(min(x0, x1), max(x0, x1) + 1): s.px(x, y, c)
    def vline(s, x, y0, y1, c):
        for y in range(min(y0, y1), max(y0, y1) + 1): s.px(x, y, c)
    def line(s, x0, y0, x1, y1, c):
        dx, dy = abs(x1 - x0), -abs(y1 - y0); sx = 1 if x0 < x1 else -1; sy = 1 if y0 < y1 else -1; e = dx + dy
        while True:
            s.px(x0, y0, c)
            if x0 == x1 and y0 == y1: break
            e2 = 2 * e
            if e2 >= dy: e += dy; x0 += sx
            if e2 <= dx: e += dx; y0 += sy
    def ell(s, cx, cy, rx, ry, c, only=None):   # filled ellipse; `c` may be a function (x, y, nx, ny) -> colour | None
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                nx, ny = (x + .5 - cx) / rx, (y + .5 - cy) / ry
                if nx * nx + ny * ny <= 1.0:
                    col = c(x, y, nx, ny) if callable(c) else c
                    if col is not None and (only is None or s.get(x, y) in only): s.px(x, y, col)
    def paste(s, o, x, y, key=None):
        for j in range(o.h):
            for i in range(o.w):
                v = o.p[j][i]
                if v != key: s.px(x + i, y + j, v)
    def crop(s, x, y, w, h):
        o = Cv(w, h)
        for j in range(h):
            for i in range(w): o.p[j][i] = s.p[y + j][x + i] if 0 <= y + j < s.h and 0 <= x + i < s.w else 0
        return o
    def flip(s):
        o = Cv(s.w, s.h)
        for j in range(s.h): o.p[j] = list(reversed(s.p[j]))
        return o
    def outline(s, c, bg, only_fill=None):   # a ring of colour c on every `bg` pixel that touches a drawn one (4-neighbours)
        o = [row[:] for row in s.p]
        for y in range(s.h):
            for x in range(s.w):
                if s.p[y][x] == bg:
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        v = s.get(x + dx, y + dy)
                        if v is not None and v != bg and v != c: o[y][x] = c; break
        s.p = o
def lit(nx, ny, ramp, lx=-.55, ly=-.65):   # a colour from a dark->light ramp for a surface point with normal (nx, ny): light from the top left
    d = (-nx * lx - ny * ly) * -1; v = (nx * lx + ny * ly)   # v in about -1..1, 1 = facing the light
    t = (v * .5 + .5); i = int(t * (len(ramp) - .001)); return ramp[max(0, min(len(ramp) - 1, i))]
def rgb15(r, g, b): return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10)
def to_rgb(c): return ((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31)
def sheet(items, pal, path, scale=4, cols=16, pad=1, bgc=(40, 40, 60)):
    from PIL import Image
    rows = (len(items) + cols - 1) // cols; cw = max(c.w for c in items) + pad; ch = max(c.h for c in items) + pad
    im = Image.new('RGB', (cols * cw, rows * ch), bgc)
    for n, c in enumerate(items):
        ox, oy = (n % cols) * cw, (n // cols) * ch
        for y in range(c.h):
            for x in range(c.w):
                v = c.p[y][x]
                if v is not None and v >= 0: im.putpixel((ox + x, oy + y), to_rgb(pal[v]))
    im = im.resize((im.width * scale, im.height * scale), Image.NEAREST); im.save(path)

# Tiny LZ for the music streams. Flag byte (bit set = literal, LSB first), a match is two bytes: offset-1 in 12 bits, length-3 in 4 bits (15 = 18 + one extra byte).
def encode(data):
    n = len(data); out = bytearray(); i = 0; heads = {}; items = []
    def match(i):
        best = (0, 0)
        if i + 3 > n: return best
        for j in reversed(heads.get(data[i:i + 3], [])[-48:]):
            off = i - j
            if off > 4096: break
            ln = 0; mx = min(273, n - i)
            while ln < mx and data[j + ln] == data[i + ln]: ln += 1
            if ln > best[0]: best = (ln, off)
            if ln >= 273: break
        return best
    def add(i):
        if i + 3 <= n: heads.setdefault(data[i:i + 3], []).append(i)
    while i < n:
        ln, off = match(i)
        if ln >= 3: items.append((1, ln, off)); [add(k) for k in range(i, i + ln)]; i += ln
        else: items.append((0, data[i], 0)); add(i); i += 1
    for g in range(0, len(items), 8):
        grp = items[g:g + 8]; fl = 0; body = bytearray()
        for k, (m, a, b) in enumerate(grp):
            if m == 0: fl |= 1 << k; body.append(a)
            else:
                o = b - 1; f = min(a - 3, 15); body += bytes([o >> 4, ((o & 15) << 4) | f])
                if f == 15: body.append(a - 18)
        out.append(fl); out += body
    return bytes(out)
def decode(src, n):
    out = bytearray(); p = 0
    while len(out) < n:
        fl = src[p] | 0x100; p += 1
        while fl > 1 and len(out) < n:
            if fl & 1: out.append(src[p]); p += 1
            else:
                a, b = src[p], src[p + 1]; p += 2; off = ((a << 4) | (b >> 4)) + 1; ln = (b & 15) + 3
                if ln == 18: ln += src[p]; p += 1
                for _ in range(ln): out.append(out[-off])
            fl >>= 1
    return bytes(out)

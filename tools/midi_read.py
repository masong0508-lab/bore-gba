"""A small standard MIDI file reader: parse(path) -> (ticks per quarter, tracks); a track is a list of
(tick, 'ev', status, data1, data2) for channel events and (tick, 'meta', type, bytes) for meta events.
notes(track) -> sorted (start tick, length in ticks, pitch, velocity, channel)."""
import struct
def _vlq(d, p):
    v = 0
    while True:
        b = d[p]; p += 1; v = (v << 7) | (b & 127)
        if b < 128: return v, p
def parse(path):
    d = open(path, 'rb').read(); assert d[:4] == b'MThd'
    fmt, ntr, div = struct.unpack('>HHH', d[8:14]); p = 8 + struct.unpack('>I', d[4:8])[0]; tracks = []
    for _ in range(ntr):
        assert d[p:p + 4] == b'MTrk'; ln = struct.unpack('>I', d[p + 4:p + 8])[0]; q = p + 8; end = q + ln; t = 0; run = 0; ev = []
        while q < end:
            dt, q = _vlq(d, q); t += dt; s = d[q]
            if s == 0xFF:
                ty = d[q + 1]; n, q = _vlq(d, q + 2); ev.append((t, 'meta', ty, d[q:q + n])); q += n
            elif s in (0xF0, 0xF7):
                n, q = _vlq(d, q + 1); q += n
            else:
                if s & 0x80: run = s; q += 1
                k = run >> 4; a = d[q]; b = d[q + 1] if k not in (0xC, 0xD) else 0; q += 1 if k in (0xC, 0xD) else 2
                ev.append((t, 'ev', run, a, b))
        tracks.append(ev); p = end
    return div, tracks
def notes(track):
    on = {}; out = []
    for e in track:
        if e[1] != 'ev': continue
        k = e[2] >> 4; key = (e[2] & 15, e[3])
        if k == 9 and e[4] > 0: on[key] = (e[0], e[4])
        elif k == 8 or (k == 9 and e[4] == 0):
            if key in on: t0, v = on.pop(key); out.append((t0, e[0] - t0, e[3], v, e[2] & 15))
    return sorted(out)

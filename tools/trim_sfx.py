#!/usr/bin/env python3
"""trim_sfx.py - cut the silence off the start and end of the effect and voice clips (source/sfx/*.adp, 4-bit IMA ADPCM, u32 sample count first).
A clip that starts late reacts late (v_disagree waited half a second before saying anything): the start is cut to 24 samples before the sound
begins and the clip is encoded again from there (the decoder starts from zero). A long quiet tail is cut 160 samples after the last sound, as is
(no new encoding needed at the end). Run it again any time: a clip already trimmed is left as it is.
usage: python3 tools/trim_sfx.py [--dry]"""
import glob, os, struct, sys
import numpy as np

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,
        408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,
        6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1,-1,-1,-1,2,4,6,8]

def dstep(v, p, i):   # the game's effect decoder (main.c sfxMix)
    st = STEP[i]; df = st >> 3
    if v & 1: df += st >> 2
    if v & 2: df += st >> 1
    if v & 4: df += st
    p += -df if v & 8 else df; p = max(-32768, min(32767, p)); i = max(0, min(88, i + IDX[v & 7]))
    return p, i

def dec(b):
    n = struct.unpack('<I', b[:4])[0]; d = b[4:]; p = i = 0; out = np.zeros(n, np.int32)
    for k in range(n):
        v = d[k >> 1]; v = (v >> 4) if k & 1 else v & 15; p, i = dstep(v, p, i); out[k] = p
    return out

def enc(x):
    p = i = 0; nib = []
    for s in x:
        st = STEP[i]; d = int(s) - p; v = 0
        if d < 0: v = 8; d = -d
        if d >= st: v |= 4; d -= st
        if d >= st >> 1: v |= 2; d -= st >> 1
        if d >= st >> 2: v |= 1
        p, i = dstep(v, p, i); nib.append(v)
    if len(nib) & 1: nib.append(0)
    return struct.pack('<I', len(x)) + bytes([nib[k] | (nib[k + 1] << 4) for k in range(0, len(nib), 2)])

def main():
    d = os.path.join(os.path.dirname(__file__), '..', 'source', 'sfx'); before = after = 0
    for f in sorted(glob.glob(os.path.join(d, '*.adp'))):
        b = open(f, 'rb').read(); x = dec(b); a = np.abs(x); thr = max(300, a.max()*0.02); on = np.where(a > thr)[0]
        before += len(b)
        if len(on) == 0: after += len(b); continue
        lead, last = on[0], on[-1]; nb = b
        if lead > 64:   # starts late: encode again from just before the sound
            s0 = lead - 24; y = x[s0:min(len(x), last + 160)].astype(float)
            y[:16] *= np.linspace(0, 1, 16)
            nb = enc(y.astype(np.int32))
        elif len(x) - last - 1 > 160:   # only a long tail: shorten it as it is
            n = last + 160; nb = struct.pack('<I', n) + b[4:4 + (n + 1)//2]
        if nb is not b:
            print(os.path.basename(f), len(b), '->', len(nb), '(started %d ms late)' % (lead*1000//6554) if lead > 64 else '')
            if '--dry' not in sys.argv: open(f, 'wb').write(nb)
        after += len(nb)
    print('effects and voices: %d -> %d bytes' % (before, after))

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""encode_ambience.py - the nature beds (source/sfx/amb_<name>.raw, 8-bit stereo PCM from tools/make_ambience.py) into BLOCK DPCM:
source/sfx/amb_<name>.abd, about half the size, decoded by the game as it mixes for a few instructions a sample (main.c ambChan).
BLOCK DPCM (like the SNES's BRR): blocks of 16 frames. A block starts with one header byte: low nibble for L, high nibble for R, each
(shift 0..5) | (delta << 3). Then 16 bytes, one a frame: low nibble L, high nibble R, a signed 4-bit n. A sample is
    delta block:  s = previous sample + (n << shift)        plain block:  s = n << shift
The encoder tries every shift and both kinds for each block and channel, keeps the one with the least error, and never lets a sample leave 8 bits.
Layout (little endian): u32 frames, u32 keys, then keys x 2 bytes (s8 previous L, s8 previous R: the decoder's state at frame k*AMB_KEY, so a
bed can start on any key), padded to 4, then the blocks (the last one padded with zeros).
usage: python3 tools/encode_ambience.py   (--check prints the error against the 8-bit original)"""
import sys, os, struct
import numpy as np

B = 16
KEY = 2048   # frames between snapshots (main.c AMB_KEY); a multiple of B

def enc_chan(x):
    n = len(x); nb = (n + B - 1)//B; hdr = []; nib = np.zeros(nb*B, np.int8); out = np.zeros(n, np.int32); prev = 0; keys = []
    for b in range(nb):
        if (b*B) % KEY == 0: keys.append(prev)
        seg = [int(v) for v in x[b*B:(b+1)*B]]; best = None
        for f in (1, 0):
            for sh in range(6):
                p = prev; err = 0; ns = []; outs = []
                for v in seg:
                    base = p if f else 0
                    q = int(round((v - base)/(1 << sh))); q = max(-8, min(7, q)); s = base + (q << sh)
                    while s > 127: q -= 1; s -= 1 << sh
                    while s < -128: q += 1; s += 1 << sh
                    if q < -8 or q > 7: err = 1 << 60; break
                    err += (s - v)**2; ns.append(q); outs.append(s); p = s
                if best is None or err < best[0]: best = (err, f, sh, ns, outs, p)
                if best[0] == 0: break
            if best[0] == 0: break
        _, f, sh, ns, outs, p = best
        hdr.append(sh | (f << 3)); nib[b*B:b*B + len(ns)] = ns; out[b*B:b*B + len(outs)] = outs; prev = p
    return hdr, nib, out, keys

def main():
    d = os.path.join(os.path.dirname(__file__), '..', 'source', 'sfx')
    for nm in ('day', 'night', 'rain', 'wind'):
        raw = open(os.path.join(d, 'amb_%s.raw' % nm), 'rb').read()
        n = struct.unpack('<I', raw[:4])[0]
        a = np.frombuffer(raw[4:4 + 2*n], dtype=np.int8).astype(np.int32)
        L = a[0::2]; R = a[1::2]
        hl, nl, ol, kl = enc_chan(L); hr, nr, orr, kr = enc_chan(R)
        nb = len(hl)
        with open(os.path.join(d, 'amb_%s.abd' % nm), 'wb') as f:
            f.write(struct.pack('<II', n, len(kl)))
            for i in range(len(kl)): f.write(struct.pack('<bb', kl[i], kr[i]))
            while f.tell() % 4: f.write(b'\0')
            for b in range(nb):
                f.write(bytes([hl[b] | (hr[b] << 4)]))
                f.write(bytes([(int(nl[b*B + j]) & 15) | ((int(nr[b*B + j]) & 15) << 4) for j in range(B)]))
            while f.tell() % 4: f.write(b'\0')
        if '--check' in sys.argv:
            print(nm, 'rms error (8-bit steps): L %.2f R %.2f, max %d' % (np.sqrt(((ol - L)**2).mean()), np.sqrt(((orr - R)**2).mean()), max(abs(ol - L).max(), abs(orr - R).max())))
        print(nm, len(raw), '->', os.path.getsize(os.path.join(d, 'amb_%s.abd' % nm)), file=sys.stderr)

if __name__ == '__main__':
    main()

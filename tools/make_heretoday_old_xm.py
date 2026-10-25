#!/usr/bin/env python3
"""HERE TODAY (ORIGINAL), SEQUENCED: the short chiptune placeholder (1:13) as a tracker module (tools/here_today_old.xm) instead of 664 KB of streamed ADPCM.
tools/make_heretoday.py still has the song as a recipe: the hummed melody as timed notes (PHRASES), chords found by a small search under it, a triangle bass, a quiet
arpeggio, a held pad, a swell over the whole song. This script plays the same recipe, note for note: it imports PHRASES, the chord search and the swell from there, renders
every voice with the same formulas (the lead's 25 % + 12.5 % pulse pair with the slide into each note and the late vibrato, the 50 % pulse pad with its tremolo, the triangle
bass with its decay, the 12.5 % pluck) as a handful of small samples, and writes the notes on a 0.02 s grid (speed 1, 125 BPM), so every note starts within 10 ms of where the
original has it.  What the tracker cannot do is baked in or approximated:
  * a sample has one length: each voice comes in length classes and a note takes the class nearest its own length (pitch shifts change a sample's length: that is counted);
  * the lead's three-tap echo is real notes, 0.23 / 0.46 / 0.69 s later at 0.22 / 0.10 / 0.045 of the lead's level, on a pool of channels (a tap that would need a channel
    nobody has free, and is the quietest around, is dropped);
  * the swell over the song (0.5 .. 1.05) is the volume column, set from the time each accompaniment note starts.
Run from the project root:   python3 tools/make_heretoday_old_xm.py     then   python3 tools/xm2gba.py
Check it against the original with   python3 tools/make_heretoday.py /tmp/ht_old.wav   and   python3 tools/preview_xm.py here_today_old tools/here_today_old.xm /tmp/ht_xm.wav"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import make_heretoday as H
from make_heretoday import PHRASES, midi_hz, pulse, swell, slots, harmonise, TAIL

OUT = os.environ.get('OUT_XM', 'tools/here_today_old.xm')
SR = 44100                       # the recipe's own rate: every sample is rendered here, then band-limited down to the tracker rate
SPEED, BPM = 1, 125              # a row is 2.5 / 125 = 0.02 s
ROWSEC = SPEED * 2.5 / BPM
REL = 13                         # samples are stored at 8363 * 2^(13/12) = 17.7 kHz (the game's own rate is 18.157 kHz)
RATE = int(round(8363 * 2 ** (REL / 12)))
NCH = 16
LEAD_CH = (0, 1); ECHO_CH = (2, 3, 4, 5, 6, 7); PAD_CH = (8, 9, 10); BASS_CH = (11, 12); ARP_CH = (13, 14)
BASS_TRIM, ARP_TRIM = 0.85, 0.72   # (measured against the original's stems: the tracker's whole-number volumes and its interpolation leave these two about 1.5 / 3 dB loud)
SWMAX = 1.05                     # the loudest the swell gets: column volume 64 is this
PAD_LP = 3000.0
TAPS = ((0.23, 0.22), (0.46, 0.10), (0.69, 0.045))

# ------------------------------------------------------------------------------------------------------------- the voices, one note at the sample's root pitch (the recipe's formulas)
def lead_note(root, L):          # a lead note of L seconds at MIDI pitch `root` (the tune's pitch + 12): 25 % pulse (a slow wobble in the width) + a detuned 12.5 % pulse, a slide up into the note, vibrato on long ones
    n = int(max(L, .12) * SR); t = np.arange(n) / SR
    vib = 1 + (0.007 * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - 0.22) * 4, 0, 1) if L > 0.4 else 0)
    slide = 2 ** ((-0.7 * np.exp(-t / 0.025)) / 12.0)
    f = midi_hz(root) * vib * slide; ph = np.cumsum(f) / SR
    duty = 0.25 + 0.03 * np.sin(2 * np.pi * 2.2 * t)
    lead = pulse(ph, duty); dbl = pulse(np.cumsum(f * 1.004) / SR, 0.125)
    env = np.minimum(t / 0.006, 1.0) * (np.exp(-t * 1.5) * 0.65 + 0.35); env *= np.clip((n / SR - t) / 0.07, 0, 1)
    return (lead * 0.26 + dbl * 0.08) * env
def pad_tone(root, L, last):     # one tone of the pad: a 50 % pulse (x 0.034), a slow tremolo, a short crossfade at each end (the last chord rings out over two seconds)
    n = int(L * SR); t = np.arange(n) / SR
    padenv = np.minimum(t / 0.12, 1.0) * np.clip((L - t) / (0.20 if not last else 2.0), 0, 1) * (0.8 + 0.2 * np.sin(2 * np.pi * 0.7 * t))
    x = pulse(np.cumsum(np.full(n, midi_hz(root))) / SR, 0.5) * 0.034 * padenv
    x = x * (np.clip((L - t) / 2.0, 0, 1) if last else 1.0)
    return signal.sosfiltfilt(signal.butter(6, PAD_LP, 'low', fs=SR, output='sos'), x)      # (the pad is the quietest voice: its harmonics above 3 kHz are 30 dB under the lead, and without them the converter stores it at half the rate)
def bass_note(root, ln):         # a triangle: soft attack, an exponential decay, a short release
    m = int(ln * SR); tt = np.arange(m) / SR; ph = np.cumsum(np.full(m, midi_hz(root))) / SR
    e = np.minimum(tt / 0.012, 1.0) * np.exp(-tt * 0.9) * np.clip((ln - tt) / 0.08, 0, 1)
    return (4 * np.abs((ph % 1.0) - 0.5) - 1) * e * 0.30
def pluck(root):                 # the arpeggio's 12.5 % pulse pluck
    m = int(0.28 * SR); tt = np.arange(m) / SR; ph = np.cumsum(np.full(m, midi_hz(root))) / SR
    return pulse(ph, 0.125) * np.minimum(tt / 0.004, 1.0) * np.exp(-tt * 11.0) * 0.065

# ------------------------------------------------------------------------------------------------------------- the events of the recipe, as (time, voice, pitch, length, level)
def cluster(lengths, k):         # k length classes: 1-D k-means on log length
    v = np.sort(np.log(np.array(lengths))); k = min(k, len(set(np.round(v, 3))))
    c = np.quantile(v, (np.arange(k) + .5) / k)
    for _ in range(40):
        a = np.argmin(abs(v[:, None] - c[None, :]), axis=1); c = np.array([v[a == j].mean() if (a == j).any() else c[j] for j in range(k)])
    return sorted(float(np.exp(x)) for x in c)
def events():
    lead = []; pad = []; bass = []; arp = []
    for ps, pe, notes in PHRASES:
        for ns, nl, m in notes: lead.append((ns, m, max(nl, .12)))
    sl = slots(); ch = harmonise(sl)
    for (s0, s1, notes), c in zip(sl, ch):
        r, q = c; third = 3 if q else 4; L = s1 - s0; a = lambda p, lo: lo + ((p - lo) % 12)
        last = sl[-1][0] == s0
        for p in (a(r, 50), a(r + third, 50), a(r + 7, 50)): pad.append((s0, p, L, last, swell(s0 + L / 2)))
        bl = min(L * (0.55 if L > 2.0 else 0.95), 2.6) if not last else 4.0
        bass.append((s0, a(r, 43), bl, 0.30, swell(s0)))
        if L > 2.0 and not last: bass.append((s0 + L * 0.55, a(r + 7, 43), min(L * 0.4, 1.6), 0.26, swell(s0 + L * 0.55)))
        if L > 3.4 and not last: bass.append((s0 + L - 0.5, a(r, 43) + 12, 0.45, 0.18, swell(s1)))
        pat = [0, 1, 2, 3, 2, 1]; step = 0.25
        for k in range(int(L / step) if not last else 8):
            tt0 = k * step; p = [a(r, 60), a(r + third, 60), a(r + 7, 60), a(r, 60) + 12][pat[k % 6]]
            arp.append((s0 + tt0, p, swell(s0 + tt0) * (1.0 if not last else 0.7)))
    return lead, pad, bass, arp

# ------------------------------------------------------------------------------------------------------------- instruments and the grid
IN = []                          # dicts: key, x (floats, peak 1), peak (the recipe's own peak level of the sample before it was normalised), root, L (seconds as recorded)
def to_rate(x):                  # band-limited to the tracker rate
    n1 = int(round(len(x) * RATE / SR)); return signal.resample(x, n1) if n1 != len(x) else x
def new_inst(key, x, root, L):
    pk = float(np.abs(x).max()); y = to_rate(x); y = y / max(1e-9, np.abs(y).max())
    IN.append(dict(key=key, x=y, peak=pk, root=root, L=L, rel=REL)); return len(IN) - 1
GRID = {}                        # row -> {channel: (note, instrument 1-based, volume column)}
def put(t, ch, ii, pitch, level, root):
    row = int(round(t / ROWSEC)); note = 49 + int(round(pitch)) - root; assert 1 <= note <= 96, (ii, pitch, root)
    if level < .015: return                                  # (too quiet to hear)
    GRID.setdefault(row, {})[ch] = (note, ii + 1, 0x10 + max(1, min(64, int(round(64 * level)))))      # (the volume column: 0x10 + volume 0..64)
def play_len(Lc, root, p): return Lc * 2 ** ((root - p) / 12.0)       # a sample played at another pitch runs shorter / longer
def pick(classes, need, root, p):     # the class whose playing length is nearest the wanted one (a little prefered shorter: a sample that is still sounding gets cut by the next note)
    best = None
    for j, (Lc, ii) in enumerate(classes):
        pl = play_len(Lc, root, p); e = abs(np.log(pl / need)) * (1.0 if pl <= need else 1.25)
        if best is None or e < best[0]: best = (e, j)
    return classes[best[1]]

def build():
    lead, pad, bass, arp = events()
    # ---- classes
    LREG = ((64, 'lo'), (78, 'hi'))                                       # the lead's two registers (sample roots), sounding pitches 58..86
    lc = cluster([n[2] for n in lead], 7)
    lead_cls = {}
    for root, nm in LREG: lead_cls[root] = [(L, new_inst('ld_%s_%d' % (nm, int(round(L * 100))), lead_note(root, L), root, L)) for L in lc]
    pl = cluster([p[2] for p in pad if not p[3]], 6); padL = [L for (_, _, L, last, _) in pad if last][0]
    PADROOT = 55
    pad_cls = [(L, new_inst('pd_%d' % int(round(L * 100)), pad_tone(PADROOT, L, False), PADROOT, L)) for L in pl]
    pad_last = (padL, new_inst('pd_last', pad_tone(PADROOT, padL, True), PADROOT, padL))
    bl = cluster([b[2] for b in bass if b[2] < 3.9], 4); BROOT = 48
    bass_cls = [(L, new_inst('bs_%d' % int(round(L * 100)), bass_note(BROOT, L), BROOT, L)) for L in bl]
    bass_last = (4.0, new_inst('bs_last', bass_note(BROOT, 4.0), BROOT, 4.0))
    PROOT = 66; arp_i = new_inst('arp', pluck(PROOT), PROOT, .28)
    pmax = max(d['peak'] for d in IN)
    # volume: svol_inst = enough for the loudest note of the voice (peak x its highest level), the volume column then carries the swell and the lead's echo level
    SV = {}
    def svol_for(i, top): d = IN[i]; d['svol'] = int(min(64, max(1, np.ceil(64 * d['peak'] * top / pmax)))); SV[i] = top
    for root in lead_cls:
        for _, i in lead_cls[root]: svol_for(i, 1.0)
    for _, i in pad_cls + [pad_last]: svol_for(i, SWMAX)
    for _, i in bass_cls + [bass_last]: svol_for(i, SWMAX)
    svol_for(arp_i, SWMAX)
    def lvl(i, amp):   # the column volume (0..1 of 64) that makes instrument i sound at `amp` times its own recorded level
        d = IN[i]; return min(1.0, amp * (64 * d['peak'] / pmax) / d['svol'])
    # ---- the lead and its echo
    echoes = []
    for k, (ns, m, L) in enumerate(lead):
        root = 64 if m + 12 < 71 else 78; Lc, ii = pick(lead_cls[root], L, root, m + 12)
        put(ns, LEAD_CH[k % 2], ii, m + 12, lvl(ii, 1.0), root)
        for d, g in TAPS: echoes.append((ns + d, g, ii, m + 12, root, play_len(Lc, root, m + 12)))
    echoes.sort(); busy = {c: -1.0 for c in ECHO_CH}; dropped = 0
    for k, (t, g, ii, p, root, ln) in enumerate(echoes):
        free = [c for c in ECHO_CH if busy[c] <= t + 1e-6]
        if not free:
            # nobody free: the quietest sounding tap (or this one) goes; here a new tap is dropped when it is no louder than the quietest still running
            dropped += 1; continue
        c = min(free, key=lambda c: busy[c]); put(t, c, ii, p, lvl(ii, g), root); busy[c] = t + ln
    # ---- the pad: three tones a chord
    for k, (s0, p, L, last, sw) in enumerate(pad):
        Lc, ii = pad_last if last else pick(pad_cls, L, PADROOT, p)
        put(s0, PAD_CH[k % 3], ii, p, lvl(ii, sw), PADROOT)
    # ---- the bass
    for k, (t, p, ln, vel, sw) in enumerate(bass):
        Lc, ii = bass_last if ln > 3.9 else pick(bass_cls, ln, BROOT, p)
        put(t, BASS_CH[k % 2], ii, p, lvl(ii, sw * vel / 0.30 * BASS_TRIM), BROOT)
    # ---- the arpeggio
    for k, (t, p, sw) in enumerate(arp): put(t, ARP_CH[k % 2], arp_i, p, lvl(arp_i, sw * ARP_TRIM), PROOT)
    end = PHRASES[-1][1] + TAIL + 0.7
    return end, dropped

def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(d):
    q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int); delta = np.diff(np.concatenate([[0], q])); dl = bytes((int(v) & 255) for v in delta)
    h = struct.pack('<I', 263) + d['key'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, d['svol'], 0, 0, 128, d['rel'], 0) + b'\0' * 22
    return h + sh + dl
def main():
    end, dropped = build()
    keep = os.environ.get('KEEP_CH')                                           # (debugging: only these channels, e.g. KEEP_CH=8,9,10)
    if keep:
        ks = {int(x) for x in keep.split(',')}
        for r in list(GRID): GRID[r] = {c: v for c, v in GRID[r].items() if c in ks}
    used = sorted({c[1] for cells in GRID.values() for c in cells.values()})
    remap = {old: new + 1 for new, old in enumerate(used)}                     # only the instruments that are played
    INU = [IN[o - 1] for o in used]
    nrows = int(end / ROWSEC) + 8; PL = 256; pats = []; nrows = (nrows + PL - 1) // PL * PL
    for p0 in range(0, nrows, PL):
        P = []
        for r in range(p0, p0 + PL):
            cells = GRID.get(r, {}); P.append([(cells[c][0], remap[cells[c][1]], cells[c][2]) if c in cells else (0, 0, 0) for c in range(NCH)])
        pats.append(P)
    order = list(range(len(pats)))
    hdr = b'Extended Module: ' + b'HERE TODAY (ORIGINAL)'.ljust(20)[:20] + b'\x1a' + b'make_heretoday_old_xm'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(INU), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    open(OUT, 'wb').write(hdr + b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(d) for d in INU))
    notes = sum(len(c) for c in GRID.values())
    print('wrote %s: %d patterns of %d rows (%.1f s), %d instruments, %d notes, %d echo taps dropped, samples %.0f KB, file %.0f KB' % (
        OUT, len(pats), PL, nrows * ROWSEC, len(INU), notes, dropped, sum(len(d['x']) for d in INU) / 1024, os.path.getsize(OUT) / 1024))
    for d in INU: print('  %-12s root %2d  %5.2f s  %6d bytes  svol %2d' % (d['key'], d['root'], d['L'], len(d['x']), d['svol']))
if __name__ == '__main__': main()

#!/usr/bin/env python3
"""HERE TODAY GONE TOMORROW (by Singhs), SEQUENCED: the 12 minute suite as a tracker module for the GBA player (tools/here_today_gone_tomorrow.xm).
The old version was a 6.5 MB streamed recording; this is notes plus about half a megabyte of small synthesised samples (every instrument is rendered here, once).

The song is the HERE TODAY MIDI (tools/here_today.mid) played by a very early (1970) band: 12-string acoustic, mellotron strings and flute, Hammond swell, a felt piano,
fuzzed bass, bells, timpani. Same key and tempo as the song, no key change. The suite has FOUR ACTS and the melody is kept back until the last two minutes:
  I   THE FIELD       wind, 12-string alone, verses that thicken, a bridge, a first modest chorus
  II  WEATHER         hush, builds, bigger choruses, a false ending, a pastoral return
  III THE STORM       tremolo strings, timpani rolls, the biggest chorus, then the unwinding
  IV  HOMECOMING      the 12-string alone again, a tender bridge, then the melody for the first time, on a solo cello over 12-string and a soft Hammond;
                      a final chorus that is warm rather than loud, and an ending that slows, lands on G major and rings out.
Player limits kept: 16 channels, 32 instruments (one 8-bit sample each, no loops, no note-off), fixed speed/BPM (speed 6, 78 BPM = 16th-note rows of 0.192 s),
so every sustained instrument comes in a few LENGTHS (S / M / L) and a note picks the one that ends about when it does.
Run from the project root:  python3 tools/make_heretoday_xm.py      (needs numpy, scipy; a few seconds)
Then:  python3 tools/xm2gba.py   and audition with   python3 tools/preview_xm.py here_today_gone_tomorrow tools/here_today_gone_tomorrow.xm out.wav"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from heretoday_kit import *
OUT = os.environ.get('OUT_XM', 'tools/here_today_gone_tomorrow.xm')

ALL = dict(v1=1.0, v2=.9, va=1.0, vc=1.1)
def S(**k): return {n: v * k.get(n, 1.0) for n, v in ALL.items()}
def SC(w): return {n: v * w for n, v in ALL.items()}
SECS = [
 # ---- I. THE FIELD
 ('wind',        OUTR, dict(lv=1.33, ramp=(.4, .9), pad=.7, fl=.9, pn=.5, pnp='arp8', bl=0.5, blp='bell4')),
 ('prelude',     V1R,  dict(lv=0.88, ramp=(.55, .9), gt=1.0, pad=.3, pn=.55, pnp='arp8b', bl=0.9, blp='bell4')),
 ('intro',       V1R,  dict(fl=.8, lv=0.53, ramp=(.7, 1.0), gt=1.0, pn=.85, pnp='arp8', st={'vc': .8, 'va': .5}, pad=.45, bl=0.9, blp='bell4')),
 ('verse 1',     V1R,  dict(lv=0.6, gt=1.0, pad=.55, st=S(v1=.8, v2=.8), fl=.6, pn=.6, pnp='arp8', bs=.5, bsp='pedal', bl=0.9, blp='bell4', bfade=1)),
 ('verse 2',     V2R,  dict(lv=0.61, gt=1.0, pad=.6, st=SC(.95), fl=.65, pn=.85, pnp='arp8b', bs=.6, bsp='rf', bl=0.3, blp='bell4')),
 ('bridge',      BRR,  dict(lv=0.67, gt=.9, pad=.7, st=SC(1.1), pn=.85, pnp='arp8', bs=.75, bsp='rf', fl=.7, bl=0.35, blp='bell4')),
 ('chorus',      CHR,  dict(lv=0.69, gt=.9, gs=.5, pad=.75, st=SC(1.15), fl=.75, pn=1.0, pnp='arp8', bs=.8, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 # ---- II. WEATHER
 ('interlude 1', V2R,  dict(lv=0.67, gt=.9, st=S(v1=.7, v2=1.1, va=1.1), pn=1.0, pnp='arp8c', bs=.65, bsp='rf', pad=.55, fl=.7, bl=0.45, blp='bell4')),
 ('hush',        V1R,  dict(lv=1.18, ramp=(.7, .4), gt=.8, pad=.5, st={'vc': .5}, pn=.6, pnp='arp8b', gap=10, bl=0.7, blp='bell4')),
 ('build 1',     BRR,  dict(lv=0.85, ramp=(.45, 1.1), gt=.8, pad=.7, st=SC(1.1), fl=.7, pn=.85, pnp='arp8', bs=.75, bsp='pulse', pc='roll', rollbars=2, sw=.9, bl=0.3, blp='bell4')),
 ('chorus 2',    CHR,  dict(lv=0.74, gt=.9, gs=.6, pad=.8, st=SC(1.25), fl=.85, pn=1.1, pnp='arp8b', bs=.9, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('verse 3',     V3R,  dict(lv=0.65, gt=1.0, pad=.7, st=S(v1=.9), fl=.65, pn=.95, pnp='arp8', bs=.65, bsp='rf', bl=0.35, blp='bell4')),
 ('first ending', OUTR, dict(lv=0.71, ramp=(1, .5), gt=.9, pad=.8, st=SC(1.0), fl=.7, pn=.8, pnp='arp8c', gap=8, bl=0.4, blp='bell4')),
 ('pastoral',    V2R,  dict(lv=0.79, ramp=(.6, .9), gt=1.0, fl=.9, pn=.9, pnp='arp8', pad=.6, bl=0.6, blp='bell4')),
 ('second verse', V1R, dict(lv=0.58, gt=.9, st=S(v1=.8, v2=.9), fl=.6, pad=.6, pn=.8, pnp='arp8b', bs=.55, bsp='pedal', bl=0.5, blp='bell4')),
 # ---- III. THE STORM
 ('second bridge', BRR, dict(lv=0.78, ramp=(.5, 1.15), gt=.8, pad=.7, st=SC(1.15), fl=.7, pn=1.0, pnp='arp8c', bs=.8, bsp='walk', pc='roll', rollbars=2, sw=1.0, bl=0.35, blp='bell4')),
 ('interlude 2', V3R,  dict(lv=0.72, gt=.9, st=S(v1=.8, va=1.1), fl=.65, pn=1.0, pnp='arp8', pad=.6, bs=.65, bsp='octave', bl=0.4, blp='bell4')),
 ('chorus 3',    CHR,  dict(lv=0.78, ramp=(.9, 1.1), gt=.9, gs=.7, pad=.85, st=SC(1.3), fl=.9, pn=1.1, pnp='arp8', bs=.95, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('storm',       V2R,  dict(lv=0.99, ramp=(1.0, 1.2), gt=.9, pad=.8, st=SC(1.2), tr=1, fl=.7, pn=1.0, pnp='arp8c', bs=.9, bsp='pulse', pc='bar', bl=0.4, blp='bell4')),
 ('interlude 3', V2R,  dict(lv=0.71, ramp=(.9, .45), gt=.9, pad=.7, st=S(v1=.7, va=.9), fl=.65, pn=.95, pnp='arp8b', bs=.5, bsp='pedal', bl=0.55, blp='bell4')),
 ('build 2',     BRR,  dict(lv=0.97, ramp=(.4, 1.3), gt=.7, pad=.7, st=SC(1.2), tr=1, fl=.7, pn=.9, pnp='arp8', bs=.85, bsp='pulse', pc='roll', rollbars=3, sw=1.0, bl=0.3, blp='bell4')),
 ('chorus 4',    CHR,  dict(lv=1.16, ramp=(1.05, 1.25), gt=.9, gs=.8, pad=.9, st=SC(1.4), fl=.95, pn=1.2, pnp='arp8b', bs=1.0, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('verse 4',     V3R,  dict(lv=0.88, ramp=(1.0, .6), gt=.9, pad=.7, st=S(v1=.8, v2=.9, va=.9), pn=.9, pnp='arp8c', bs=.6, bsp='rf', fl=.55, bl=0.35, blp='bell4')),
 # ---- IV. HOMECOMING  (the arpeggios stay, softer; the cello has the tune)
 ('ending 1',    OUTR, dict(lv=0.74, ramp=(.9, .5), gt=.8, pad=.8, st=SC(.9), fl=.55, pn=.8, pnp='arp8', gap=10, bl=0.4, blp='bell4')),
 ('quiet again', V1R,  dict(lv=1.01, ramp=(.7, .55), gt=.9, pad=.3, pn=.55, pnp='arp8b', gap=6, bl=0.8, blp='bell4')),
 ('third bridge', BRR, dict(lv=0.86, ramp=(.6, .8), gt=.7, pad=.45, st={'vc': .7, 'va': .55, 'v2': .4}, pn=.8, pnp='arp8', bs=.45, bsp='pedal', bl=0.35, blp='bell4')),
 ('last verse',  V3R,  dict(lv=0.91, ramp=(.7, .85), ld=.85, gt=.8, pad=.4, st={'vc': .55, 'va': .3}, pn=.5, pnp='arp8', bl=0.35, blp='bell4')),
 ('final chorus', CHR, dict(lv=1.0, ramp=(.7, .95), ld=.95, gt=.8, gs=.35, pad=.55, st=S(v1=.45, v2=.5, va=.6, vc=.7), fl=.3, pn=.65, pnp='arp8b', bs=.45, bsp='pedal', pc='heart', bl=0.3, blp='bell4')),
 ('coda',        (672, 726), dict(lv=1.06, ramp=(.9, .45), rit=.5, ld=.9, gt=.7, pad=.6, st=S(v1=.3, v2=.4, va=.5, vc=.6), pn=.45, pnp='arp8', bs=.4, bsp='pedal', bl=0.5, blp='bell4')),
]
LVF = json.load(open(os.environ['LVFILE'])) if os.environ.get('LVFILE') else {}
def compose():
    D = 0; log = []
    for name, src, L in SECS:
        L = dict(L); L['lv'] = L.get('lv', 1.0) * LVF.get(name, 1.0)
        d0 = D; D = section(name, D, src, **L); log.append((name, d0, D))
    # the last G chord rings and fades in four slow Hammond swells; the first four notes of the tune come back once on bells, slower and softer, and the piece is done
    for k, (lvl, p) in enumerate(((.5, 0), (.4, 1), (.3, 0), (.2, 1))):
        for j, t in enumerate((43, 50)): put(D + 10 * k + j, PADCH[j], 'pad_lo' if t < 52 else 'pad_hi', t, lvl * 1.3)
    for i, p in enumerate(MOTTO): put(D + 14 + i * 5 + (i == 3) * 3, PCCH, 'bell', p + 24, .30 - .05 * i)
    D += 16
    end = D + 48
    return log, end


def main():
    log, end = compose()
    IN, nrows = write_xm(OUT, 'HERE TODAY GONE TOMORROW', end, 'make_heretoday_xm')
    for d in IN: print('  %-8s root %2d  %5.2f s  %6d bytes  svol %2d' % (d['key'], d['root'], d['L'], len(d['x']), d['svol']))
    json.dump([(n, round(a * ROW, 1), round(b * ROW, 1)) for n, a, b in log], open(os.environ.get('TMPDIR', '.') + '/ht_sections.json', 'w'))
    for n, a, b in log: print('  %-14s %5.1f - %5.1f s' % (n, a * ROW, b * ROW))
if __name__ == '__main__': main()

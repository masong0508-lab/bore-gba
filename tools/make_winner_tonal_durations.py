#!/usr/bin/env python3
"""Rebuild the exact BORE 1 Winner note-length map from the original Koala file.

Usage: python3 tools/make_winner_tonal_durations.py Winner.koala
Matches source event/voice allocation in import_winner_koala.py. Does not
require extracting audio or changing the original Koala project.
"""
import argparse
import base64
from collections import Counter, defaultdict
import hashlib
import json
import lzma
import math
from pathlib import Path
import struct
import zipfile

SOURCE_SHA = '98ab503f5e2ff1c0712c5c8a7ffd67bbf56e9f6f79e65b75a045685afbc62b5b'
ATTACK_SHA = 'ccdeda8c73119e7fca2d5597a2bd8090edf008917ee9351ba012f02c0de0c726'
DURATION_SHA = '01b7dde4b1d65b539ffd714e805b0dc5bafd71d19eff2a4d16aa60887bb98767'
TONAL = frozenset((4, 5, 8))
ROWS_PER_BEAT = 16


def make(project):
    data = Path(project).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SOURCE_SHA:
        raise ValueError('Not the expected original Koala file: ' + digest)
    with zipfile.ZipFile(project) as z:
        sq = json.loads(z.read('sequence.json'))
        sp = json.loads(z.read('sampler/sampler.json'))
    pads = {int(p['pad']): p for p in sp['pads']}
    swing = max(.5, min(.75, float(sq['swing'])))

    def beat_to_row(beat):
        whole = math.floor(beat)
        frac = beat - whole
        phase = frac * (swing / .5) if frac < .5 else swing + (frac - .5) * (1 - swing) / .5
        return int(round((whole + phase) * ROWS_PER_BEAT))

    events = []
    used = Counter()
    bar = 0
    for part in sq['sequences']:
        pattern = part.get('noteSequence') and part['noteSequence'].get('pattern')
        if not pattern or not pattern.get('notes'):
            continue
        for note in pattern['notes']:
            pn = int(note['num'])
            used[pn] += 1
            start = bar * 4 + float(note['timeOffset']) / 4096
            length_beat = float(note['length']) / 4096
            events.append((beat_to_row(start), pn, int(round(float(note.get('pitch', 0)))),
                           float(note.get('vel', 110)), length_beat * ROWS_PER_BEAT,
                           start, length_beat))
        bar += int(pattern['numBars'])
    if len(events) != 8813 or bar != 206 or len(used) != 23:
        raise ValueError('Unexpected source song structure')

    instruments = {pn: i + 1 for i, pn in enumerate(sorted(used))}
    events.sort(key=lambda e: (e[0], -int(e[1] in (5, 6, 4, 8)), -e[3], e[1]))
    byrow = defaultdict(list)
    for event in events:
        byrow[event[0]].append(event)
    free_until = [-1] * 16
    channel_pad = [None] * 16
    attacks, lengths = [], []
    for row in sorted(byrow):
        if row >= bar * 64:
            continue
        occupied = []
        for event in byrow[row]:
            row, pn, pitch, vel, length_rows, start_beat, length_beat = event
            choices = [ch for ch in range(16) if ch not in occupied]
            if not choices:
                raise ValueError('A Koala note would be lost at row ' + str(row))
            def score(ch):
                return (free_until[ch] > row, 0 if channel_pad[ch] == pn else 1,
                        max(0, free_until[ch] - row), ch)
            channel = min(choices, key=score)
            occupied.append(channel)
            pad = pads[pn]
            rate = 2 ** ((float(pad.get('pitch', 0)) + pitch) / 12)
            sample_seconds = (int(pad['end']) - int(pad['start'])) / 48000
            lifespan = sample_seconds / rate * sq['bpm'] * ROWS_PER_BEAT / 60
            free_until[channel] = row + max(1, int(min(lifespan, length_rows + max(3, .5 * ROWS_PER_BEAT))))
            channel_pad[channel] = pn
            attacks.append((row, channel, instruments[pn], max(1, min(96, 49 + pitch))))
            if pn in TONAL:
                lengths.append((row, channel, max(1, min(255, beat_to_row(start_beat + length_beat) - row))))
    attacks.sort()
    fingerprint = hashlib.sha256(b''.join(struct.pack('<HBBB', *e) for e in attacks)).hexdigest()
    bypos = {(row, ch): duration for row, ch, duration in lengths}
    durations = bytes(bypos[(row, ch)] for row, ch, inst, note in attacks if inst in (4, 5, 8))
    if fingerprint != ATTACK_SHA or hashlib.sha256(durations).hexdigest() != DURATION_SHA:
        raise ValueError('Source note timing/pitch/channel fingerprint changed; re-audit before update')
    return base64.b64encode(lzma.compress(durations, preset=9)).decode('ascii') + '\n'


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('project')
    ap.add_argument('--output', default='tools/winner_tonal_durations.b64')
    args = ap.parse_args()
    output = make(args.project)
    Path(args.output).write_text(output, encoding='ascii')
    print('Source-accurate 4053 bass/Rhodes note lengths regenerated; %d encoded bytes' % len(output))

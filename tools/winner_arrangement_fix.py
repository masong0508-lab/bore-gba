#!/usr/bin/env python3
"""Improve BORE 1's Winner arrangement using the original Koala note lengths.

Source: Winner.koala SHA256 98ab503f5e2ff1c0712c5c8a7ffd67bbf56e9f6f79e65b75a045685afbc62b5b.
The user-uploaded source has 8,813 attacks, 206 bars, and 23 used pads.
This pass restores Koala bass (pad 4) and Rhodes (pads 5, 8) releases while
retaining every original note-on, instrument, pitch and time position.
Koala pad 6's note-offs are installed separately by winner_note_gate.py.

The full 43 MB Koala project is not needed at build time: a 1.8 KB encoded
source-duration map, bound to an exact event-position checksum, is versioned.
"""
import base64
from bisect import bisect_right
import hashlib
import lzma
import struct
from pathlib import Path

from winner_note_gate import decode_patterns

ROOT = Path(__file__).resolve().parent.parent
DURATIONS = ROOT / 'tools/winner_tonal_durations.b64'
SOURCE_SHA256 = '98ab503f5e2ff1c0712c5c8a7ffd67bbf56e9f6f79e65b75a045685afbc62b5b'
NOTE_ATTACK_SHA256 = 'ccdeda8c73119e7fca2d5597a2bd8090edf008917ee9351ba012f02c0de0c726'
DURATION_SHA256 = '01b7dde4b1d65b539ffd714e805b0dc5bafd71d19eff2a4d16aa60887bb98767'
TONAL_INSTRUMENTS = frozenset((4, 5, 8))  # Koala pads 4 bass, 5/8 Rhodes
TOTAL_TONAL = 4053
TOTAL_ATTACKS = 8813


def _attack_log(patterns):
    events = []
    for bar, pat in enumerate(patterns):
        for step, cells in enumerate(pat):
            for channel, (note, instrument, _vol, _fx, _param) in enumerate(cells):
                if 1 <= note <= 96:
                    events.append((bar * 64 + step, channel, instrument, note))
    return sorted(events)


def verify_source_events(patterns):
    entries = _attack_log(patterns)
    if len(entries) != TOTAL_ATTACKS:
        raise ValueError('Winner source note-on count drift: %d' % len(entries))
    packed = b''.join(struct.pack('<HBBB', *e) for e in entries)
    digest = hashlib.sha256(packed).hexdigest()
    if digest != NOTE_ATTACK_SHA256:
        raise ValueError('Winner source note position/pitch/instrument drift: ' + digest)
    return entries


def load_durations():
    text = DURATIONS.read_text(encoding='ascii').strip()
    packed = lzma.decompress(base64.b64decode(text, validate=True))
    if len(packed) != TOTAL_TONAL or hashlib.sha256(packed).hexdigest() != DURATION_SHA256:
        raise ValueError('Winner source note-duration map checksum/count mismatch')
    return packed


def transform(original):
    patterns, tail = decode_patterns(original)
    entries = verify_source_events(patterns)
    durations = load_durations()
    grid = [row for pat in patterns for row in pat]
    occupied = [[] for _ in range(16)]
    for step, cells in enumerate(grid):
        for ch, cell in enumerate(cells):
            if cell[0]:
                occupied[ch].append(step)

    inserted = preempted = beyond_song = 0
    bass_accents = bass_passing = 0
    releases = []
    index = 0
    for row, ch, instrument, note in entries:
        if instrument not in TONAL_INSTRUMENTS:
            continue
        duration = durations[index]
        index += 1
        if duration < 1:
            raise ValueError('Zero-length Koala source note')
        if instrument == 4:
            # All original pitches and swing attacks remain untouched. Only
            # mild backbeat/anticipation dynamics differentiate the bass line.
            vol = grid[row][ch][2]
            if 0x10 <= vol <= 0x50:
                if row % 16 == 0:
                    grid[row][ch][2] = min(0x50, vol + 4)
                    bass_accents += 1
                elif row % 16 in (5, 13):
                    grid[row][ch][2] = max(0x11, vol - 4)
                    bass_passing += 1
        due = row + duration
        if due >= len(grid):
            beyond_song += 1
            continue
        future = occupied[ch]
        pos = bisect_right(future, row)
        if pos < len(future) and future[pos] <= due:
            preempted += 1
            continue
        releases.append((due, ch, instrument))
    if index != TOTAL_TONAL:
        raise ValueError('Winner source duration/event count drift')
    for due, ch, instrument in releases:
        if grid[due][ch][0]:
            preempted += 1
            continue
        grid[due][ch] = [97, instrument, 0, 0, 0]
        inserted += 1

    out = bytearray(original[:336])
    for pattern in patterns:
        data = bytearray()
        for cells in pattern:
            for cell in cells:
                if cell[0] == 0 and not any(cell[1:]):
                    data.append(0x80)
                else:
                    data.extend(bytes(cell))
        out.extend(struct.pack('<IBHH', 9, 0, 64, len(data)))
        out.extend(data)
    out.extend(tail)
    corrected = bytes(out)
    reread, final_tail = decode_patterns(corrected)
    if verify_source_events(reread) != entries or final_tail != tail:
        raise ValueError('Winner patch altered original attacks or samples')
    return corrected, {
        'source_koala_sha256': SOURCE_SHA256,
        'notes_verified': len(entries),
        'melodic_note_lengths_checked': index,
        'melodic_releases_added': inserted,
        'preempted_by_existing_event': preempted,
        'beyond_song': beyond_song,
        'bass_downbeats_accented': bass_accents,
        'bass_passing_notes_softened': bass_passing,
    }


if __name__ == '__main__':
    import sys
    source = Path(sys.argv[1]).read_bytes()
    result, report = transform(source)
    Path(sys.argv[2]).write_bytes(result)
    print(report)

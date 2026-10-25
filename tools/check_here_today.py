#!/usr/bin/env python3
"""check_here_today.py - HERE TODAY (MISSY'S SONG) is the tracker module tools/here_today.xm (made by tools/make_heretoday_here_xm.py); the scene-5 lyric beats are timed to it.
Fails when the song is not the whole song (the old streamed render ran 152 s; the tracker's fixed 78 BPM makes it 148 s)."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from xm import parse
S = parse(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'here_today.xm'))
rows = sum(len(S['pats'][o]) for o in S['order']); secs = rows * S['tempo'] * 2.5 / S['bpm']
print('### HERE TODAY (Missy\'s song)'); print('%d s of notes in tools/here_today.xm (the whole song is about 148 s)' % secs)
if secs < 140: print('ERROR: here_today.xm is only %d s long: not the whole song' % secs); sys.exit(1)

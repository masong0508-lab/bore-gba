#!/usr/bin/env python3
"""check_helptext.py - every hand-wrapped help line must fit its screen, in the game's own font (source/fontdata.h, the same widths tw() uses).
Run from the project root:  python3 tools/check_helptext.py        (tools/test_helptext.sh calls it)

What it checks
  * the HOW TO PLAY tabs (lifeHelp, creatureHelp, mapHelp, nbHelp, packHelp, jbHelp, slotHelp, optHelp): a line is drawn at x=16 in a well that ends at x=222, so at most
    206 px; a '>' heading sits in a green band (x=17 .. 220), so at most 200 px; plus: the font has the characters (no curly quotes, no em dash)
  * the full-screen help boxes (helpScreen: x=18 in a box that ends at x=236): at most 214 px
  * the tutorial cards (tutorial.h: x=14 in a card that ends at x=230): lines and heading at most 214 px, the goal hint in the HUD bar at most 200 px
  * the kingdom's HOW TO PLAY pages and the advisor tips (kingplay.h, kingadv.h): the pages are wrapped by vsLines (tools/test_vslines.sh), the tips are one line of at most 205 px
Anything over is listed with its width. Exit status 1 when a line does not fit."""
import re, sys, os
os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
fd = open('source/fontdata.h').read()
CH = re.search(r'#define FNT_CHARS "((?:[^"\\]|\\.)*)"', fd).group(1).encode().decode('unicode_escape')
FA = [int(v) for v in re.search(r'fa_s\[\d+\]=\{([^}]*)\}', fd).group(1).replace('\n', '').split(',') if v.strip()]
SP = int(re.search(r'#define FSP_s (\d+)', fd).group(1))
def tw(s):
    w = 0
    for c in s:
        if c == ' ': w += SP; continue
        i = CH.find(c)
        if i < 0: raise ValueError('character %r is not in the font' % c)
        w += FA[i]
    return w
def lits(body):
    out = []
    for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', body): out.append(m.group(1).encode().decode('unicode_escape'))
    return out
def array(path, name):
    s = open(path, encoding='utf-8').read()
    m = re.search(r'static const char\* const %s\[\]\s*=\s*\{(.*?)\};' % re.escape(name), s, re.S)
    if not m: raise SystemExit('could not find %s in %s' % (name, path))
    return lits(m.group(1))
bad = 0; worst = {}
def chk(label, s, lim):
    global bad
    w = tw(s)
    worst[label] = max(worst.get(label, 0), w)
    if w > lim: bad += 1; print('TOO WIDE  %-12s %3d > %3d  %s' % (label, w, lim, s))
for nm, path in (('lifeHelp', 'source/main.c'), ('creatureHelp', 'source/main.c'), ('mapHelp', 'source/main.c'), ('nbHelp', 'source/neighborhood.h'),
                 ('packHelp', 'source/main.c'), ('jbHelp', 'source/main.c'), ('slotHelp', 'source/slots.h'), ('optHelp', 'source/optscreen.h')):
    for l in array(path, nm):
        if l.startswith('>'): chk(nm + ' >', l[1:], 200)
        else: chk(nm, l, 206)
s = open('source/ccdemo.h', encoding='utf-8').read()
m = re.search(r'static void ccAbout\(void\)\{(.*?)helpScreen', s, re.S)
for l in lits(m.group(1)):
    chk('ccAbout', l[1:] if l.startswith('>') else l, 214)
t = open('source/tutorial.h', encoding='utf-8').read()
m = re.search(r'static const TutStep tutSteps\[\]=\{(.*?)\n\};', t, re.S)
for row in re.findall(r'^ \{[^\n]*\},$', m.group(1), re.M):
    L = lits(row)                                   # heading, up to four lines, then the hint
    chk('tut heading', L[0], 214)
    for l in L[1:-1]: chk('tut line', l, 214)
    chk('tut hint', L[-1], 200)
a = open('source/kingadv.h', encoding='utf-8').read()
for l in re.findall(r'kaCat\([be],"([^"]*)"\)', a): chk('advisor', l + ' CASTLE', 205)
print('widest per list:', ', '.join('%s %d' % (k, v) for k, v in sorted(worst.items())))
print('FAILED: %d line(s) too wide' % bad if bad else 'ok: every help line fits')
sys.exit(1 if bad else 0)

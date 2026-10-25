#!/bin/sh
# test_storytitles.sh - every story chapter title must fit the 56 byte line the journal page builds ("CHAPTER n  " + the title, story.h stTkPage).
# (Before it was 44 bytes: chapter 5 of the TV story, "WHILE SHE'S DANCING WITH THE STARS", wrote two bytes past it.) No sanitizer needed: it measures the real tables.
cd "$(dirname "$0")/.." || exit 1
python3 - <<'PYX'
import re, sys
s = open('source/story.h').read()
m = re.search(r'char t\[(\d+)\]; char\*e=slCat\(t,"CHAPTER "\);', s)
if not m: print("FAILED: cannot find the CHAPTER line in story.h"); sys.exit(1)
cap = int(m.group(1)); bad = []; n = 0
for a in re.finditer(r'static const StCh (\w+)\[\]=\{(.*?)\};', s, re.S):
    for i, t in enumerate(re.findall(r'\{"((?:[^"\\]|\\.)*)",\s*SG_', a.group(2))):
        n += 1
        if len("CHAPTER %d  " % (i + 1)) + len(t) + 1 > cap: bad.append((a.group(1), i + 1, t))
if n < 100: print("FAILED: found only %d chapter titles" % n); sys.exit(1)
print("storytitles: %d chapter titles, buffer %d bytes: %s" % (n, cap, "FAILED %s" % bad if bad else "ALL PASSED"))
sys.exit(1 if bad else 0)
PYX

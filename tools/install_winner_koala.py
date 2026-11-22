#!/usr/bin/env python3
"""Install the exact original-sample Winner tracker into BORE 1's XM data bank.

Requires only the vendored tools/xm2gba.py, tools/xm.py, numpy and scipy.
The 331-KiB tools/winner_koala.xm.xz is the tracked build asset: NO 43-MB
source Koala project is needed by CI. The original source may be re-imported
using tools/import_winner_koala.py Winner.koala and re-compressed with lzma.

On a clean checkout, append this one song to the existing XM bank, preserving
all old-song offsets. A marker makes repeated 'make' calls idempotent and
allows an updated source to replace its own previous appended bank section.
"""
import hashlib
import lzma
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parent.parent
ARCHIVE=ROOT/'tools/winner_koala.xm.xz'
XM=ROOT/'tools/winner_koala.xm'
HEADER=ROOT/'source/musicdata.h'
BANK=ROOT/'source/music/xmdata.bin'
MARK='// BEGIN ORIGINAL KOALA WINNER '
END='// END ORIGINAL KOALA WINNER\n'
EXPECTED_XM='b3d5d2cda538f1987c34292f2e94d2218f217c3811bfe0321cd0ab33e61279a4'

def install():
    if not ARCHIVE.exists():
        raise SystemExit('Winner original sample archive missing: '+str(ARCHIVE))
    source=lzma.decompress(ARCHIVE.read_bytes())
    digest=hashlib.sha256(source).hexdigest()
    if digest!=EXPECTED_XM:
        raise SystemExit('Winner source checksum mismatch. Re-import and update EXPECTED_XM: '+digest)
    if not XM.exists() or XM.read_bytes()!=source:
        XM.write_bytes(source)
    old=HEADER.read_text(encoding='utf8')
    end=old.find(MARK)
    if end>=0:
        line=old[end:old.find('\n',end)].strip()
        parts=line.split(' ')
        offset=int(next(s.split('=')[1] for s in parts if s.startswith('offset=')))
        old_digest=next(s.split('=')[1] for s in parts if s.startswith('sha256='))
        if old_digest==digest and BANK.stat().st_size>=offset:
            print('Winner Koala XM bank is already prepared: '+digest[:12]); return
        old=old[:end]
        prior=BANK.read_bytes()
        if offset>len(prior):raise SystemExit('original bank offset exceeds binary')
        BANK.write_bytes(prior[:offset])
    if 'xm_winner_koala_' in old:
        raise SystemExit('Winner symbols appear in an unexpected part of musicdata.h')

    # Import the *one* Koala XM with the repo's existing GBA converter, avoiding
    # a costly recompression of every other song on every CI build.
    sys.path.insert(0,str(ROOT/'tools'))
    import xm2gba as C
    C.BLOB.clear(); C.LABELS.clear(); C.SHARED.clear()
    C.GAIN['winner_koala']=1.45
    song=C.convert('winner_koala',str(XM))
    base=BANK.stat().st_size
    blob=bytes(C.BLOB)
    if not blob:raise RuntimeError('Winner converter wrote an empty bank')
    decl=['\n'+MARK+'offset='+str(base)+' sha256='+digest+'\n',
          '// Uses the original Koala project note events and 23 embedded WAV pad samples.\n',
          '__asm__(".pushsection .rodata\\n"\n']
    for name,typ,pos,size in C.LABELS:
        decl.append(f'  ".global {name}\\n{name}:\\n.incbin \\"source/music/xmdata.bin\\",{base+pos},{size}\\n"\n')
    decl.append('  ".popsection\\n");\n')
    for name,typ,pos,size in C.LABELS:
        decl.append(f'extern const {typ} {name}[];\n')
    decl.append(song)
    decl.append(END)
    # Do not publish half-written header/binary on failure; prepare first.
    output=old+''.join(decl)
    BANK.write_bytes(BANK.read_bytes()+blob)
    HEADER.write_text(output,encoding='utf8')
    print('Installed Koala Winner: %d sample/event bytes at XM bank offset %d, sha256=%s' % (len(blob),base,digest[:16]))

if __name__=='__main__':install()

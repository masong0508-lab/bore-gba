#!/usr/bin/env python3
"""Reversible bass recomposition for Winner, applied after exact Koala note-gates.

Preserves all 8,813 note-on positions, samples, section boundaries and other
instruments. Revoices the repeated 16-bar bass riff around E/A/B roots, removes
large octave jumps/chromatic runs, and adds alternating turnaround responses.
The original imported Koala module remains unchanged in the .xz archive.
This is a *candidate* arrangement pending listening approval.
"""
import struct
from pathlib import Path
from winner_note_gate import decode_patterns

BASS=4
RIFF=(
 (-8,-8,-6,-4,-6,-1,-6),(-8,-8,-6,-4,-6,-1,-3),
 (-8,-8,-6,-4,-6,1,-1),(-8,-8,-6,-4,-6,3,1),
 (-3,-3,-1,1,-1,4,1),(-3,-3,-1,1,-1,6,4),
 (-3,1,-3,-1,1,-1,4,1),(-1,-1,-1,3,-1,6,1,-1),
 (-8,-8,-6,-4,-6,-1,-6),(-8,-8,-6,-4,-6,-1,-3),
 (-8,-8,-6,-4,-6,1,-1),(-8,-8,-6,-4,-6,3,1),
 (-3,-3,-1,1,-1,4,1),(-3,-3,-1,1,-1,6,4),
 (-3,1,-3,-1,1,-1,4,1),(-1,-1,-1,3,-1,6,1,-1)
)

def bass_target(bar):
    if 38<=bar<54:
        return RIFF[bar-38],0
    if 78<=bar<206:
        cycle=(bar-78)//16
        motif=(bar-78)%16
        target=list(RIFF[motif])
        if cycle%2 and motif in (3,11): target[-2:]=[-1,-3]
        elif cycle%2 and motif in (5,13):target[-2:]=[4,1]
        elif cycle%2 and motif in (7,15):target[-2:]=[3,1]
        return tuple(target),cycle
    return None,None

def attacks(patterns):
    return [(bar*64+r,ch,cell[1],cell[0]) for bar,p in enumerate(patterns)
            for r,row in enumerate(p) for ch,cell in enumerate(row)
            if 1<=cell[0]<=96]

def transform(source):
    patterns,tail=decode_patterns(source)
    if len(patterns)!=206:raise ValueError('Not the 206-bar Winner source')
    baseline=attacks(patterns)
    if len(baseline)!=8813:raise ValueError('Original Winner attacks changed')
    nonbass=[e for e in baseline if e[2]!=BASS]
    changed_pitches=changed_velocities=bars=0
    for bar,pat in enumerate(patterns):
        notes=[(r,ch,cell) for r,row in enumerate(pat)
                for ch,cell in enumerate(row)
                if 1<=cell[0]<=96 and cell[1]==BASS]
        if not notes:continue
        target,cycle=bass_target(bar)
        if target is not None:
            if len(target)!=len(notes):
                raise ValueError('Unexpected bass pattern at bar %d'%bar)
            bars+=1
        for k,(row,ch,cell) in enumerate(notes):
            pitch=cell[0]-49
            if target is not None: new=target[k]
            elif 18<=bar<70 and pitch==-13: new=-8
            else:new=pitch
            if new!=pitch:
                cell[0]=49+new;changed_pitches+=1
            vol=cell[2]
            if 0x11<=vol<=0x50:
                delta=2 if row%16==0 else -3 if row%16 in (5,10,13) else 0
                if target is not None and cycle is not None and cycle%2 and k==len(notes)-1:
                    delta=-4
                updated=max(0x11,min(0x50,vol+delta))
                if updated!=vol:cell[2]=updated;changed_velocities+=1
    output=bytearray(source[:336])
    for pat in patterns:
        data=bytearray()
        for row in pat:
            for cell in row:
                if not any(cell): data.append(0x80)
                else:data.extend(bytes(cell))
        output.extend(struct.pack('<IBHH',9,0,64,len(data)))
        output.extend(data)
    output.extend(tail)
    check,checktail=decode_patterns(bytes(output))
    revised=attacks(check)
    if len(revised)!=8813 or [x[:3] for x in revised]!=[x[:3] for x in baseline]:
        raise ValueError('An original note attack or its timing changed')
    if [x for x in revised if x[2]!=BASS]!=nonbass:
        raise ValueError('A nonbass note changed')
    if tail!=checktail:raise ValueError('Original samples changed')
    return bytes(output),{
        'original_attacks':len(revised),
        'bass_notes':sum(e[2]==BASS for e in revised),
        'bass_pitches_changed':changed_pitches,
        'bass_velocities_adjusted':changed_velocities,
        'bars_reworked':bars
    }

if __name__=='__main__':
    import sys
    updated,report=transform(Path(sys.argv[1]).read_bytes())
    Path(sys.argv[2]).write_bytes(updated)
    print(report)

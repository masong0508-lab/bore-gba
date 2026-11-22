#!/usr/bin/env python3
"""Add exact Koala flute note endings to the existing Winner XM at build time.

Only note-off events are added. The original compressed XM and its 8,813
note-ons remain untouched, and later retriggers cancel stale releases.
"""
import base64
import hashlib
import lzma
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
FLUTE_INSTRUMENT=6
GATE_SHA="ebbae974ec2b2479380cd266b97dee0941c26952bf0e7a3d1c2db16cc1accb79"

def decode_patterns(raw):
    songlen, restart, nch, npats, ninst, flags, speed, bpm=struct.unpack_from("<8H",raw,64)
    if (songlen,nch,npats,ninst,speed,bpm)!=(206,16,206,23,2,170):
        raise ValueError("Not the original Koala Winner XM")
    offset=60+struct.unpack_from("<I",raw,60)[0]
    patterns=[]
    for _ in range(npats):
        hlen,packing,rows,packed=struct.unpack_from("<IBHH",raw,offset)
        if (hlen,packing,rows)!=(9,0,64):raise ValueError("Unexpected Koala XM pattern")
        pos=offset+hlen;end=pos+packed;pat=[]
        for _row in range(rows):
            cells=[]
            for _ch in range(nch):
                key=raw[pos];pos+=1
                if key&128:
                    mask=key&31;cell=[0,0,0,0,0]
                    for field in range(5):
                        if mask&(1<<field):cell[field]=raw[pos];pos+=1
                else:
                    cell=[key,raw[pos],raw[pos+1],raw[pos+2],raw[pos+3]];pos+=4
                cells.append(cell)
            pat.append(cells)
        if pos!=end:raise ValueError("Corrupt XM pattern length")
        offset=end;patterns.append(pat)
    return patterns,raw[offset:]

def add_gates(original,compressed_b64):
    packed=lzma.decompress(base64.b64decode(compressed_b64))
    if hashlib.sha256(packed).hexdigest()!=GATE_SHA:
        raise ValueError("Original source-note gate map checksum mismatch")
    entries=list(struct.iter_unpack("<HBB",packed))
    if len(entries)!=716:raise ValueError("Missing Koala flute note releases")
    patterns,tail=decode_patterns(original)
    grid=[row for pat in patterns for row in pat]
    inserted=preempted=0
    for row,ch,duration in entries:
        if not (0<=row<len(grid) and 0<=ch<16 and 1<=duration<128):
            raise ValueError("Bad Koala note duration")
        cell=grid[row][ch]
        if not cell[0] or cell[1]!=FLUTE_INSTRUMENT:
            raise ValueError("Release is not attached to its original flute note")
        due=min(len(grid),row+duration)
        if any(grid[r][ch][0] for r in range(row+1,due+1) if r<len(grid)):
            preempted+=1
            continue  # newer note on same voice already stopped the old one
        if due<len(grid) and not grid[due][ch][0]:
            grid[due][ch]=[97,FLUTE_INSTRUMENT,0,0,0]
            inserted+=1
    out=bytearray(original[:336])
    for pattern in patterns:
        buf=bytearray()
        for row in pattern:
            for cell in row:
                if cell[0]==0 and not any(cell[1:]):buf.append(0x80)
                else:buf.extend(bytes(cell))
        out.extend(struct.pack("<IBHH",9,0,64,len(buf)))
        out.extend(buf)
    out.extend(tail)
    return bytes(out),dict(original_notes=len(entries),inserted_releases=inserted,preempted_by_retrigger=preempted)

def install(source):
    return add_gates(source,(ROOT/"tools/winner_flute_gates.b64").read_text().strip())

if __name__=="__main__":
    raw=lzma.decompress((ROOT/"tools/winner_koala.xm.xz").read_bytes())
    out,report=install(raw)
    (ROOT/"tools/winner_koala.xm").write_bytes(out)
    print(report)

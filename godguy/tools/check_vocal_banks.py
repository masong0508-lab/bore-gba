#!/usr/bin/env python3
"""Prove all CORPORATIONS and DUCK & COVER vocal bytes are resident in
the two 4MiB-DS ROM banks; missing clips fail the release before build."""
import re
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'source'
s=(root/'gmvox.h').read_text()
def con(n):
    v=re.search(r'^#define '+re.escape(n)+r'\s+(\d+)',s,re.M)
    if not v: raise AssertionError(n)
    return int(v.group(1))
def arr(n):
    v=re.search(r'\b'+re.escape(n)+r'\[(\d+)\]=\{([^}]+)\}',s)
    if not v: raise AssertionError(n)
    xs=[int(k) for k in v.group(2).split(',')]
    assert len(xs)==int(v.group(1)),n
    return xs
off=arr('gmVoxOff');lengths=arr('gmVoxLen');rates=arr('gmVoxRate')
low=(root/'gmvox.bin').read_bytes()
hi=(root/'gmvox_hi.bin').read_bytes()
split=con('GM_VOX_SPLIT')
assert len(low)==split and len(hi)==con('GM_VOX_HIBYTES')
assert len(low)<=0x5B000 and len(hi)<=0x80000
assert len(low)+len(hi)==con('GM_VOX_BLOBSIZE')
assert (root/'gmvox_compact.h').exists()
n=con('GM_VOX_N');p=con('GM_VOX_CLYR0');count=con('GM_VOX_CLYRN')
d=con('GM_VOX_LYR0');duck=con('GM_VOX_LYRN')
assert n==len(off)==len(lengths)==len(rates)
assert count==56 and duck>=10
assert all(lengths[p+i]>0 for i in range(count))
assert all(lengths[d+i]>0 for i in range(duck))
assert max(lengths)<=0x9800, "DS audio decode scratch buffer overflow"
assert off==sorted(off)
for i,(o,size) in enumerate(zip(off,lengths)):
    nxt=off[i+1] if i+1<n else len(low)+len(hi)
    assert nxt-o>=(size+1)//2,(i,o,size,nxt)
    assert not (o<split<nxt),(i,o,nxt)
    assert 0<rates[i]<=65535
duration=sum(lengths[p+i]/rates[p+i] for i in range(count))
print(f"4MiB DS: {count}/56 CORPORATIONS vocal phrases "
      f"({duration:.1f} seconds), {duck} DUCK vocal pads, "
      f"ARM7 bank {len(low)} bytes, ARM9 bank {len(hi)} bytes.")

#!/usr/bin/env python3
"""Verify *packaged* normal DS and DSi-DS ROM images, not just source
tables, contain the complete two-bank 4-bit ADPCM voice archive."""
import struct,sys
from pathlib import Path
if len(sys.argv)!=2:raise SystemExit("usage: check_nds_vocal_image.py game.nds")
rom=Path(sys.argv[1]).read_bytes()
source=Path(__file__).resolve().parent.parent/'source'
low=(source/'gmvox.bin').read_bytes()
high=(source/'gmvox_hi.bin').read_bytes()
if len(rom)<0x200:raise SystemExit("ROM is truncated")
def cpu(off):
    ro,entry,ram,size=struct.unpack_from('<4I',rom,off)
    assert size>0 and ro+size<=len(rom),(ro,ram,size)
    return ro,entry,ram,size
arm9=cpu(0x20)
arm7=cpu(0x30)
def bank(cpu,mem,blob,name):
    ro,_,ram,size=cpu
    offset=mem-ram
    assert 0<=offset and offset+len(blob)<=size,(name,hex(mem),hex(ram),size)
    data=rom[ro+offset:ro+offset+len(blob)]
    assert data==blob,(name,'wrong ROM bank bytes',len(blob))
bank(arm9,0x02300000,high,'ARM9 full-voice overflow')
bank(arm7,0x02382000,low,'ARM7 first voice bank')
assert arm9[2]==0x02000000 and arm7[2]==0x02380000
print(f"ROM VOICES VERIFIED: {sys.argv[1]}, "
      f"ARM9 {len(high)} + ARM7 {len(low)} compressed bytes; "
      "all vocal streams resident on standard 4MiB DS.")

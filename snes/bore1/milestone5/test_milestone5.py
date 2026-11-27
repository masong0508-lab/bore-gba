#!/usr/bin/env python3
"""Test emitted 65816 code with an IPL protocol model; libgme tests SPC separately."""
import hashlib,struct,json
from pathlib import Path
import test_milestone4 as t
import build_milestone5 as b
from test_milestone4 import PX,PY,FOOD,REST,WASH,FUN,FAMILY,DAY,PACK,CASH,SCENE,STATUS,PROG,CHAP,TASK,HP,ENX,ENY,ENHP,DEFEATS,COOLDOWN,GROUPS,LIMITS
ROOT=Path(__file__).resolve().parent
PAYLOADS=[(ROOT/'track_0.bin').read_bytes(),(ROOT/'track_31.bin').read_bytes()]
class Machine(t.Machine):
 def __init__(self,*args,**kw):
  super().__init__(*args,**kw);self.completed=[];self.upload_frame_sizes=[];self.ack_fail=False;self.max_frame_steps=0
 def wa(self,p,v):
  if not 0x2140<=p<=0x2143 or not self.apu:return super().wa(p,v)
  v&=255;self.io[p]=v;self.apu_ports[p-0x2140]=v
  if p==0x2141 and v==0x80 and self.apu_state=='playing':
   self.apu_state='ipl';self.acks=0xaa;self.apu_upload=bytearray();return
  if p!=0x2140:return
  if self.apu_state=='ipl':
   assert v==0xcc and self.apu_ports[1]==1;self.apu_start=self.apu_ports[2]|self.apu_ports[3]<<8;self.apu_state='transfer';self.acks=v
  elif self.apu_state=='transfer':
   expected=PAYLOADS[self.mem[21]-1]
   if len(self.apu_upload)==len(expected):
    assert self.apu_ports[1]==0 and v==(len(expected)+2)&255 and self.apu_start==512
    assert bytes(self.apu_upload)==expected
    self.completed.append(self.mem[21]);self.apu_state='playing';self.acks=0x42
   else:
    assert v==len(self.apu_upload)&255
    self.apu_upload.append(self.apu_ports[1]);self.acks=v if not self.ack_fail else v^255
 def step(self):
  op=self.rom[self.pc-0x8000]
  if op not in (0xa6,0x86):return super().step()
  self.ip=self.pc;self.byte();self.instructions+=1;p=self.byte()
  if op==0xa6:self.x=self.nz(self.ra(p)|(self.ra(p+1)<<8),16)
  else:self.wa(p,self.x&255);self.wa(p+1,self.x>>8)
 def frame(self,pad=0):
  before=len(self.apu_upload);n=self.instructions;super().frame(pad);d=len(self.apu_upload)-before
  if d>0:self.upload_frame_sizes.append(d);assert d<=b.CHUNK
  self.max_frame_steps=max(self.max_frame_steps,self.instructions-n)
 def ready(self,track,limit=600):
  for n in range(limit):
   if self.mem[33] and self.mem[21]==track and self.mem[b.STATE]==0:return n
   self.frame()
  raise AssertionError(('audio did not become ready',self.mem[b.STATE],self.mem[21],self.mem[33],len(self.apu_upload)))

def main():
 rom,labels=b.build();c=Machine(rom,labels);c.run_to_frame();assert c.io[0x2100]==15 and c.mem[b.STATE]==1
 # Keep playing and navigating while the APU is uploading.
 c.walk(7,10);c.taps(0x80);assert c.mem[FOOD]==8
 c.taps(0x4000,12);assert c.mem[FAMILY]==8;c.taps(0x8000,12);assert c.mem[FAMILY]==1
 c.ready(1);assert c.completed==[1] and c.apu_upload==PAYLOADS[0]
 c.taps(0x10);assert c.mem[21]==2 and c.mem[b.STATE]!=0
 c.walk(15,19);c.taps(0x80);assert c.mem[SCENE]==1
 c.ready(2);assert c.completed==[1,2] and c.apu_upload==PAYLOADS[1]
 c.taps(0x20);c.ready(1);assert c.completed==[1,2,1]
 # Group and sub-campaign changes remain separate, with all eight ledgers.
 groups=[];campaigns=[]
 for _ in range(5):
  groups.append(GROUPS[c.mem[PACK]]);campaigns.append(c.mem[PACK]);c.taps(0x40);campaigns.append(c.mem[PACK]);c.taps(0x40);c.taps(0x2000)
 assert groups==[0,1,2,3,4] and set(campaigns)==set(range(8)) and c.mem[PACK]==0
 c.walk(24,10);cash=c.mem[CASH];c.taps(0x80);assert c.mem[CASH]==cash-3 and c.mem[FOOD]==8
 c.walk(23,19);c.taps(0x80);assert c.mem[CHAP]==1 and c.mem[TASK]==0
 c.taps(0x2000,2);assert c.mem[PACK]==3;c.mem[REST]=8;c.taps(0x80);assert 2<=c.mem[SCENE]<=5 and c.mem[HP]==6
 start=c.mem[ENHP];c.taps(0x80);assert c.mem[ENHP]==start
 for _ in range(1000):
  if c.mem[SCENE]<2:break
  dx=c.mem[ENX]-c.mem[PX];dy=c.mem[ENY]-c.mem[PY]
  if abs(dx)+abs(dy)<=2:
   if c.mem[COOLDOWN]==0:c.taps(0x80)
   else:c.frame()
  else:c.frame(0x100 if dx>0 else 0x200 if dx<0 else 0x400 if dy>0 else 0x800)
 assert c.mem[SCENE]==1 and c.mem[TASK+3]==1 and c.mem[DEFEATS]==3
 c.taps(0x80);assert c.mem[CHAP+3]==1 and c.mem[TASK+3]==0 and c.mem[CHAP]==1
 maps=[]
 for chapter in range(4):
  c.mem[CHAP+3]=chapter;c.mem[PROG+3]=0;c.mem[REST]=8;c.taps(0x80);assert c.mem[SCENE]==2+chapter
  maps.append(tuple(c.vram[9*32:20*32]));c.taps(0x8000);assert c.mem[SCENE]==1 and c.mem[FAMILY]==1
 assert len(set(maps))==4
 c.mem[CHAP+3]=3;c.mem[TASK+3]=1;c.mem[CASH]=30;c.taps(0x80);assert c.mem[PROG+3]==1 and c.mem[CHAP+3]==0 and c.mem[CASH]==34
 c.taps(0x1000);c.mem[CASH]=51;c.taps(0x1000);s2=bytes(c.sram)
 d=Machine(rom,labels,s2);d.run_to_frame();assert d.mem[CASH]==51 and d.mem[PROG+3]==1 and d.mem[CHAP]==1
 bad=bytearray(s2);idx=bad[0x100]&1;bad[0x110+idx*64+1]^=0x80
 e=Machine(rom,labels,bad);e.run_to_frame();assert e.mem[CASH]==34 and e.mem[PROG+3]==1
 # Optional MSU-1 never overwrites the active transfer; absent media falls back.
 m=Machine(rom,labels,msu=True);m.run_to_frame();m.ready(1);assert m.io[0x2007]==3 and m.apu_ports[1]==1
 f=Machine(rom,labels,msu=True,media_error=True);f.run_to_frame();f.ready(1);assert f.apu_ports[1]==0 and f.mem[33]==1
 no=Machine(rom,labels,apu=False);no.run_to_frame();[no.frame() for _ in range(245)];assert no.mem[b.STATE]==0 and no.mem[33]==0 and no.io[0x2100]==15
 fail=Machine(rom,labels);fail.run_to_frame();fail.frame();fail.ack_fail=True;fail.frame();assert fail.mem[b.STATE]==0 and fail.mem[33]==0
 assert sum(rom)&65535==struct.unpack_from('<H',rom,0x7fde)[0]
 report=dict(rom_sha256=hashlib.sha256(rom).hexdigest(),rom_bytes=len(rom),tracks=[len(x) for x in PAYLOADS],native_upload_sequences=c.completed,max_upload_bytes_per_frame=max(c.upload_frame_sizes),modeled_frames=sum(q.frames for q in (c,d,e,m,f,no,fail)),cpu_instructions=sum(q.instructions for q in (c,d,e,m,f,no,fail)),passed=['native upload across banks','switch to track 2 and back','movement and actions during upload','no audio hardware timeout','byte acknowledgment timeout','MSU success/failure fallback','eight campaign ledgers and four pack groups','eight-member cap','town travel and shop','chapter isolation','three-rival arena challenge','four arena maps','mission reward','two-record SRAM and fallback','ROM checksum'],limitations='Instruction-level modeled peripherals, not hardware or full-system emulator. Resident SPC programs tested separately in libgme; no integrated real APU/CPU test yet.')
 (ROOT/'m5_test.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()

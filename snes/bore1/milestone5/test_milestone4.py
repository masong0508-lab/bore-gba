#!/usr/bin/env python3
"""Smoke-test actual emitted 65816 instructions, SRAM, modeled DMA/PPU and IPL.
This is a deterministic instruction-level harness, not a cycle-accurate SNES.
"""
import struct,json,hashlib
from pathlib import Path
from build_milestone4 import build,ROOT,PROG,CHAP,TASK,HP,ENX,ENY,ENHP,DEFEATS,COOLDOWN,ATICK,LIMITS,GROUPS
from build_milestone2 import PX,PY,FOOD,REST,WASH,FUN,FAMILY,DAY,TICK,CLOCK,PACK,CASH,SCENE,STATUS
class Machine:
 def __init__(self,rom,labels,sram=None,apu=True,msu=False,media_error=False):
  self.rom=rom;self.labels=labels;self.pc=struct.unpack_from('<H',rom,0x7ffc)[0];self.mem=bytearray(8192);self.vram=[0]*32768;self.io={};self.stack=[];self.a=self.x=self.y=0;self.m=8;self.z=self.c=self.n=0;self.va=0;self.pad=0;self.msu=msu;self.media_error=media_error;self.status=0;self.frames=0;self.instructions=0;self.sram=bytearray(2048) if sram is None else bytearray(sram);self.oam=bytearray(544);self.opos=0;self.apu=apu;self.apu_state='ipl';self.acks=0xaa if apu else 0;self.apu_ports=[0]*4;self.apu_upload=bytearray();self.apu_start=None;self.dma_count=0
 def byte(self):
  assert 0x8000<=self.pc<=0xffff,hex(self.pc)
  v=self.rom[self.pc-0x8000];self.pc+=1;return v
 def word(self):return self.byte()|self.byte()<<8
 def long(self):return self.word()|self.byte()<<16
 def val(self):return self.byte() if self.m==8 else self.word()
 def nz(self,v,bits=0):
  bits=bits or self.m;v&=(1<<bits)-1;self.z=int(v==0);self.n=v>>(bits-1);return v
 def ra(self,p):
  if p>=0x8000:return self.rom[p-0x8000]
  if p==0x4212:
   if self.ip==self.labels['leave_blank']:return 0
   return 0x80
  if p==0x4218:return self.pad&255
  if p==0x4219:return self.pad>>8
  if p==0x2140:return self.acks
  if p==0x2141:return 0xbb if self.apu_state=='ipl' and self.apu else 0
  if self.msu and 0x2002<=p<=0x2007:return b'S-MSU1'[p-0x2002]
  if self.msu and p==0x2000:return self.status
  if p in self.io:return self.io[p]
  return self.mem[p] if p<len(self.mem) else 0
 def rd(self,p):return self.ra(p) if self.m==8 else self.ra(p)|(self.ra((p+1)&0xffff)<<8)
 def ra_long(self,p):
  if 0x700000<=p<0x700000+len(self.sram):return self.sram[p-0x700000]
  if p&0x8000:return self.rom[((p>>16)&0x7f)*0x8000+(p&0x7fff)]
  raise AssertionError(('long read',hex(p),hex(self.ip)))
 def wa_long(self,p,v):
  assert 0x700000<=p<0x700000+len(self.sram),hex(p);self.sram[p-0x700000]=v&255
 def wa(self,p,v):
  v&=255
  if 0x2000<=p<=0x43ff:
   self.io[p]=v
   if p==0x2116:self.va=(self.va&0x7f00)|v
   elif p==0x2117:self.va=(self.va&255)|((v&127)<<8)
   elif p==0x2118:self.vram[self.va]=(self.vram[self.va]&0xff00)|v
   elif p==0x2119:self.vram[self.va]=(self.vram[self.va]&255)|(v<<8);self.va=(self.va+1)&32767
   elif p in (0x2102,0x2103):self.opos=2*(self.io.get(0x2102,0)+256*(self.io.get(0x2103,0)&1))
   elif p==0x2104:self.oam[self.opos%544]=v;self.opos=(self.opos+1)%544
   elif p==0x420b and v&1:
    assert self.io[0x4300]==1 and self.io[0x4301]==0x18
    p0=self.io[0x4302]|(self.io[0x4303]<<8)|(self.io[0x4304]<<16);n=self.io[0x4305]|(self.io[0x4306]<<8)
    assert 0<n<8192 and n%2==0 and self.io[0x2100]&128
    for j in range(0,n,2):self.wa(0x2118,self.ra_long(p0+j));self.wa(0x2119,self.ra_long(p0+j+1))
    self.dma_count+=1
   elif 0x2140<=p<=0x2143 and self.apu:
    self.apu_ports[p-0x2140]=v
    if p==0x2140:
     if self.apu_state=='ipl':
      assert v==0xcc and self.apu_ports[1]==1
      self.apu_start=self.apu_ports[2]|self.apu_ports[3]<<8;self.apu_state='transfer';self.acks=v
     elif self.apu_state=='transfer':
      if len(self.apu_upload)==len((ROOT/'native_title.bin').read_bytes()):
       assert self.apu_ports[1]==0 and v==((len(self.apu_upload)+2)&255)
       self.apu_state='playing';self.acks=0x42
      else:
       assert v==len(self.apu_upload)&255,(v,len(self.apu_upload));self.apu_upload.append(self.apu_ports[1]);self.acks=v
   elif p==0x2005 and self.msu:self.status=8 if self.media_error else 0
   elif p==0x2007 and self.msu:self.status=(8 if self.media_error else 0x10) if v&1 else 0
  elif p<0x2000:self.mem[p]=v
  else:raise AssertionError(('write',hex(p)))
 def wr(self,p,v):
  self.wa(p,v)
  if self.m==16:self.wa((p+1)&0xffff,v>>8)
 def rdlong(self,p):return self.ra_long(p) if self.m==8 else self.ra_long(p)|(self.ra_long(p+1)<<8)
 def wrlong(self,p,v):
  self.wa_long(p,v)
  if self.m==16:self.wa_long(p+1,v>>8)
 def step(self):
  self.ip=self.pc;op=self.byte();self.instructions+=1
  if op in (0x78,0xfb,0xd8,0x9a,0x5b,0x4b,0xab):return
  if op==0x18:self.c=0
  elif op==0x38:self.c=1
  elif op==0xc2:
   if self.byte()&32:self.m=16
  elif op==0xe2:
   if self.byte()&32:self.m=8;self.a&=255
  elif op==0xa9:self.a=self.nz(self.val())
  elif op==0xa2:self.x=self.nz(self.word(),16)
  elif op==0xa0:self.y=self.nz(self.word(),16)
  elif op in (0xa5,0xb5):self.a=self.nz(self.rd((self.byte()+(self.x if op==0xb5 else 0))&65535))
  elif op in (0xad,0xbd):self.a=self.nz(self.rd((self.word()+(self.x if op==0xbd else 0))&65535))
  elif op in (0xaf,0xbf):self.a=self.nz(self.rdlong((self.long()+(self.x if op==0xbf else 0))&0xffffff))
  elif op in (0x85,0x95):self.wr((self.byte()+(self.x if op==0x95 else 0))&65535,self.a)
  elif op==0x8d:self.wr(self.word(),self.a)
  elif op in (0x8f,0x9f):self.wrlong((self.long()+(self.x if op==0x9f else 0))&0xffffff,self.a)
  elif op in (0x64,0x74):self.wr((self.byte()+(self.x if op==0x74 else 0))&65535,0)
  elif op==0x9c:self.wr(self.word(),0)
  elif op in (0xe6,0xc6,0xf6):
   p=(self.byte()+(self.x if op==0xf6 else 0))&65535;self.wr(p,self.nz(self.rd(p)+(1 if op!=0xc6 else -1)))
  elif op in (0xe8,0x88,0xca):
   if op==0xe8:self.x=self.nz(self.x+1,16)
   elif op==0xca:self.x=self.nz(self.x-1,16)
   else:self.y=self.nz(self.y-1,16)
  elif op==0x8a:self.a=self.nz(self.x)
  elif op==0xaa:self.x=self.nz(self.a,16)
  elif op==0x0a:self.c=self.a>>(self.m-1);self.a=self.nz(self.a*2)
  elif op==0x1a:self.a=self.nz(self.a+1)
  elif op in (0x29,0x25,0x49,0x45,0x09):
   v=self.val() if op in (0x29,0x49,0x09) else self.rd(self.byte());self.a=self.nz(self.a&v if op in (0x29,0x25) else self.a|v if op==0x09 else self.a^v)
  elif op in (0x65,0x69,0x75):
   v=self.val() if op==0x69 else self.rd((self.byte()+(self.x if op==0x75 else 0))&65535);v+=self.a+self.c;self.c=int(v>=(1<<self.m));self.a=self.nz(v)
  elif op in (0xe5,0xe9):
   v=self.rd(self.byte()) if op==0xe5 else self.val();r=self.a-v-(1-self.c);self.c=int(r>=0);self.a=self.nz(r)
  elif op in (0xc9,0xc5,0xe0,0xdd):
   v=self.val() if op==0xc9 else self.rd(self.byte()) if op==0xc5 else self.rd((self.word()+self.x)&65535) if op==0xdd else self.word();r=self.x if op==0xe0 else self.a;self.c=int(r>=v);self.nz(r-v,16 if op==0xe0 else self.m)
  elif op in (0xf0,0xd0,0x90,0xb0,0x80):
   d=self.byte();d=d if d<128 else d-256
   if {0xf0:self.z,0xd0:not self.z,0x90:not self.c,0xb0:self.c,0x80:True}[op]:self.pc+=d
  elif op==0x20:
   q=self.word();self.stack.append(self.pc);self.pc=q
  elif op==0x60:self.pc=self.stack.pop()
  elif op==0x4c:self.pc=self.word()
  else:raise AssertionError(('unsupported',hex(op),hex(self.ip)))
 def run_to_frame(self):
  for i in range(2000000):
   self.step()
   if self.pc==self.labels['frame']:
    self.frames+=1;assert not self.stack;return
  raise AssertionError('frame not reached')
 def frame(self,pad=0):self.pad=pad;self.run_to_frame()
 def taps(self,pad,n=1):
  for _ in range(n):self.frame(pad);self.frame(0)
 def walk(self,x,y):
  while self.mem[PX]!=x or self.mem[PY]!=y:
   dx=x-self.mem[PX];dy=y-self.mem[PY];self.frame(0x100 if dx>0 else 0x200 if dx<0 else 0x400 if dy>0 else 0x800)
  self.frame(0)
 def text(self,x,y,n):
  from build_prototype import chars
  return ''.join(chars[t&1023] if (t&1023)<len(chars) else '?' for t in self.vram[y*32+x:y*32+x+n])

def main():
 rom,labels=build();payload=(ROOT/'native_title.bin').read_bytes();c=Machine(rom,labels);c.run_to_frame()
 assert c.io[0x2100]==15 and c.io[0x2105]==1 and c.mem[33]==1 and c.apu_start==512 and c.apu_upload==payload
 assert c.oam[:4]==bytes([120,120,75,0x30]);assert c.vram[10*32+7]==67
 assert c.text(1,0,19).startswith('BORE 1 / YOUR HOME')
 c.taps(0x4000,16);assert c.mem[FAMILY]==8;c.taps(0x8000,16);assert c.mem[FAMILY]==1
 # SELECT cycles exactly 5 groups; X selects the secondary campaign.
 groups=[];campaigns=[]
 for _ in range(5):
  groups.append(GROUPS[c.mem[PACK]]);campaigns.append(c.mem[PACK]);c.taps(0x0040);campaigns.append(c.mem[PACK]);c.taps(0x0040);c.taps(0x2000)
 assert groups==[0,1,2,3,4] and set(campaigns)==set(range(8)) and c.mem[PACK]==0
 # Movement bounds and valid interactions, town travel, currency.
 for _ in range(120):c.frame(0x200)
 assert c.mem[PX]==4
 c.walk(7,10);c.taps(0x0080);assert c.mem[FOOD]==8 and c.mem[TASK]==1
 c.walk(15,19);c.taps(0x0080);assert c.mem[SCENE]==1 and c.text(1,0,20).startswith('BORE 1 / TOWN CENTRE')
 c.walk(24,10);cash=c.mem[CASH];c.taps(0x0080);assert c.mem[CASH]==cash-3 and c.mem[FOOD]==8
 c.walk(23,19);c.taps(0x0080);assert c.mem[CHAP]==1 and c.mem[TASK]==0
 # Combat campaign entry via actual buttons, then exercise strike range and pursuit.
 c.taps(0x2000,2);assert c.mem[PACK]==3
 c.mem[REST]=8;c.taps(0x0080);assert 2<=c.mem[SCENE]<=5 and c.mem[HP]==6
 initial=c.mem[ENHP];c.taps(0x0080);assert c.mem[ENHP]==initial
 old=(c.mem[ENX],c.mem[ENY]);
 for _ in range(18):c.frame()
 assert (c.mem[ENX],c.mem[ENY])!=old
 # Beat three rivals by approaching and striking. When close, attack no more
 # than once every 12 frames; execute all mechanics via machine input.
 for _ in range(1000):
  if c.mem[SCENE]<2:break
  dx=c.mem[ENX]-c.mem[PX];dy=c.mem[ENY]-c.mem[PY]
  if abs(dx)+abs(dy)<=2:
   if c.mem[COOLDOWN]==0:c.taps(0x0080)
   else:c.frame()
  else:c.frame(0x100 if dx>0 else 0x200 if dx<0 else 0x400 if dy>0 else 0x800)
 assert c.mem[SCENE]==1 and c.mem[TASK+3]==1 and c.mem[DEFEATS]==3,(c.mem[SCENE],c.mem[HP],c.mem[DEFEATS],c.mem[STATUS])
 c.taps(0x0080);assert c.mem[CHAP+3]==1 and c.mem[TASK+3]==0 and c.mem[CHAP]==1
 # Every arena map loaded, with a different map ID, on consecutive progress.
 maps=[]
 for chapter in range(4):
  c.mem[CHAP+3]=chapter;c.mem[PROG+3]=0;c.mem[REST]=8;c.taps(0x0080);assert c.mem[SCENE]==2+chapter
  maps.append(tuple(c.vram[9*32:20*32]));c.taps(0x8000);assert c.mem[SCENE]==1 and c.mem[FAMILY]==1
 assert len(set(maps))==4
 # Complete mission and collect reward, then test SRAM transaction and fallback.
 c.mem[CHAP+3]=3;c.mem[TASK+3]=1;c.mem[CASH]=30;c.taps(0x0080);assert c.mem[PROG+3]==1 and c.mem[CHAP+3]==0 and c.mem[CASH]==34
 c.taps(0x1000);s1=bytes(c.sram);c.mem[CASH]=51;c.taps(0x1000);s2=bytes(c.sram);assert s1!=s2 and not any(s2[:256]);
 d=Machine(rom,labels,s2);d.run_to_frame();assert d.mem[CASH]==51 and d.mem[PROG+3]==1 and d.mem[CHAP]==1
 bad=bytearray(s2);idx=bad[0x100]&1;bad[0x110+idx*64+1]^=0x80
 e=Machine(rom,labels,bad);e.run_to_frame();assert e.mem[CASH]==34 and e.mem[PROG+3]==1
 # MSU-1 success and failure choose native mute only after successful request.
 m=Machine(rom,labels,msu=True);m.run_to_frame();m.frame();assert m.io[0x2007]==3 and m.apu_ports[1]==1;m.taps(0x10);assert m.mem[21]==2
 f=Machine(rom,labels,msu=True,media_error=True);f.run_to_frame();f.frame();assert f.apu_ports[1]==0 and f.mem[33]==1
 noapu=Machine(rom,labels,apu=False);noapu.run_to_frame();assert noapu.mem[33]==0 and noapu.io[0x2100]==15
 assert sum(rom)&65535==struct.unpack_from('<H',rom,0x7fde)[0]
 report={'rom_sha256':hashlib.sha256(rom).hexdigest(),'rom_bytes':len(rom),'apu_payload_bytes':len(payload),'modeled_frames':sum(x.frames for x in (c,d,e,m,f,noapu)),'cpu_instructions':sum(x.instructions for x in (c,d,e,m,f,noapu)),'passed':['reset and tile/OAM upload','full byte-by-byte modeled IPL transfer','APU unavailable timeout','all four packs/eight campaigns','eight member cap','home and town movement/interactions','currency and needs','chapter progress isolation','four distinct arena maps','out-of-range strike','rival pursuit','three-rival fight and reward','arena exit','two-slot save and corrupt newest fallback','MSU present/missing media fallback','ROM header checksum'],'limitations':'Instruction-level harness with modeled peripherals, not real console or full-system emulator; SPC audio independently emulated by libgme. M4 saves use separate record area from M2/M3, no migration yet. Full BORE game not ported.'}
 (ROOT/'m4_test.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':main()

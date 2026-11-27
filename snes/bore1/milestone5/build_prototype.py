#!/usr/bin/env python3
"""BORE 1 native SNES home prototype. Dependency-free LoROM builder.
Mode 1 BG1, 4bpp tiles, bounded vblank writes, auto joypad, optional MSU-1.
This is a new port foundation, not a conversion of the ARM executable.
"""
import struct,sys,hashlib,json
from pathlib import Path

class ASM:
    def __init__(self): self.b=bytearray();self.labels={};self.fix=[]
    def emit(self,*b):self.b.extend(b)
    def label(self,n):assert n not in self.labels;self.labels[n]=0x8000+len(self.b)
    def word(self,op,n):
        self.emit(op)
        if isinstance(n,str):self.fix.append((len(self.b),n,False));self.emit(0,0)
        else:self.emit(n&255,n>>8&255)
    def rel(self,op,n):self.emit(op);self.fix.append((len(self.b),n,True));self.emit(0)
    def finish(self):
        for p,n,r in self.fix:
            a=self.labels[n]
            if r:
                d=a-(0x8000+p+1);assert -128<=d<=127,(n,d);self.b[p]=d&255
            else:struct.pack_into('<H',self.b,p,a)
        return self.b

FONT='''A:0E11111F111111 B:1E11111E11111E C:0F10101010100F D:1E11111111111E E:1F10101E10101F F:1F10101E101010 G:0F10101711110F H:1111111F111111 I:0E04040404040E J:0702020212120C K:11121418141211 L:1010101010101F M:111B1515111111 N:11191513111111 O:0E11111111110E P:1E11111E101010 Q:0E11111115120D R:1E11111E141211 S:0F10100E01011E T:1F040404040404 U:1111111111110E V:11111111110A04 W:11111115151B11 X:11110A040A1111 Y:11110A04040404 Z:1F01020408101F 0:0E11131519110E 1:040C040404040E 2:0E11010204081F 3:1E01010601011E 4:02060A121F0202 5:1F10101E01011E 6:0E10101E11110E 7:1F010204080808 8:0E11110E11110E 9:0E11110F01010E &:0C12140C15120D -:0000001F000000 /:01010204081010 ::00040000040000'''
# ASCII tile IDs, 0 is blank; artwork tile IDs start at 64.
chars=' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789&-/:'
fonts={q[0]:bytes.fromhex(q[2:]) for q in FONT.split()}
# 4bpp tile data, low planes first, then high planes.
def planar(p):
    return bytes(sum((p[y][x]>>bit&1)<<(7-x) for x in range(8)) for base in (0,2) for y in range(8) for bit in (base,base+1))
def blank(c=0):return [[c]*8 for _ in range(8)]
def picture(rows):return [[int(c,16) for c in r] for r in rows.split()]
def tileset():
    t=[blank() for _ in range(80)]
    for i,c in enumerate(chars):
        for y,b in enumerate(fonts.get(c,b'')):
            for x in range(5):t[i][y][x+1]=1 if b&(16>>x) else 0
    t[64]=[[3 if (x+y*2)%8==0 else 2 for x in range(8)] for y in range(8)]
    t[65]=[[5 if y%4==0 or (x+(y//4)*4)%8==0 else 4 for x in range(8)] for y in range(8)]
    t[66]=picture('00000000 00666600 06777660 06787760 06777760 00666600 00600600 00000000')
    t[67]=picture('55555555 59999995 59555595 59599995 59599995 59999995 55555555 55555555')
    t[68]=picture('AAAAAAA5 ABBBBBA5 A11111A5 ABBBBBA5 ABBBBBA5 ABBBBBA5 AAAAAAA5 55555555')
    t[69]=picture('55555555 5CCCCCC5 5C1111C5 5C1111C5 5CCCCCC5 5C1111C5 55555555 55555555')
    t[70]=picture('00000000 000D0D00 00DDDDD0 0DDEDED0 0DDDDDD0 00DDDD00 00D00D00 00000000')
    t[71]=picture('00000000 01111110 01111110 01111110 01111110 01111110 01111110 00000000')
    t[72]=picture('00000000 03333330 03000030 03000030 03000030 03000030 03333330 00000000')
    t[73]=[[14 if (x*3+y)%11==0 else 15 for x in range(8)] for y in range(8)]
    # The avatar and pet use an opaque floor-colored background.
    for n in (66,70):t[n]=[[c or 2 for c in row] for row in t[n]]
    return b''.join(planar(p) for p in t)

def tile(c):return chars.index(c) if c in chars else 0
def screen():
    a=[0]*1024
    def put(x,y,s):a[y*32+x:y*32+x+len(s)]=[tile(c) for c in s]
    put(1,0,'BORE 1 / SNES HOME PROTOTYPE')
    put(1,2,'FOOD');put(17,2,'REST');put(1,3,'WASH');put(17,3,'FUN')
    put(1,5,'FAMILY');put(17,5,'DAY')
    for y in range(7,23):
        for x in range(3,29):a[y*32+x]=65 if x in (3,28) or y in (7,22) else 64
    for x,y,n in ((7,10,67),(24,10,68),(7,20,69),(23,20,70)):a[y*32+x]=n
    put(5,8,'FRIDGE');put(22,8,'BED');put(5,21,'BATH');put(22,21,'PET')
    put(1,23,'MOVE / A USE / Y ADD B REMOVE')
    put(1,24,'SELECT PACK / L R MUSIC')
    put(1,26,'PACK:')
    return struct.pack('<1024H',*a)

PACKS=('BASE GAME / 11 MISSIONS','LIFE & PREHISTORY','BORE IN CRISIS','BORTUA FIGHTER','LIFE LEGACY & BEYOND')
MISSIONS=(11,8+6,4+6,4,15+4)
# DP bytes, positions are 16-bit values; other variables are independent bytes.
PX,PY,FOOD,REST,WASH,FUN,FAMILY,DAY,TICK,CLOCK,JH,JL,PH,PL,EH,EL,PACK,TMP,MSU,TRACK=0,2,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21

def build():
    a=ASM();E=a.emit;W=a.word;R=a.rel;L=a.label
    def imm(v):E(0xa9,v)
    def ld(p):E(0xa5,p)
    def st(p):E(0x85,p)
    def setdp(p,v):imm(v);st(p)
    def sta(p):W(0x8d,p)
    def reg(p,v):imm(v);sta(p)
    def addr(n):E(0xc2,0x20);W(0xa9,n);sta(0x2116);E(0xe2,0x20)
    def call(n):W(0x20,n)
    def jmp(n):W(0x4c,n)
    def cmp(v):E(0xc9,v)
    def textcell(p,v):addr(p);imm(v);sta(0x2118);W(0x9c,0x2119)
    L('reset');E(0x78,0x18,0xfb,0xd8,0xc2,0x30);W(0xa2,0x1fff);E(0x9a);W(0xa9,0);E(0x5b,0x4b,0xab,0xe2,0x20)
    reg(0x2100,0x80);reg(0x4200,0);reg(0x420b,0);reg(0x420c,0);reg(0x2105,1);reg(0x2107,0);reg(0x210b,2);reg(0x2115,0x80);reg(0x212c,1);reg(0x212d,0);reg(0x2130,0);reg(0x2131,0);reg(0x2133,0)
    for p in (0x210d,0x210e):reg(p,0);reg(p,0)
    # upload art and map with forced blank, 16-bit VRAM addressing
    for name,where,data in [('tiles',0x2000,tileset()),('map',0,screen())]:
        addr(where);W(0xa2,0);L('upload_'+name);W(0xbd,name);sta(0x2118);E(0xe8);W(0xbd,name);sta(0x2119);E(0xe8);W(0xe0,len(data));R(0xd0,'upload_'+name)
    reg(0x2121,0);W(0xa2,0);L('palette_loop');W(0xbd,'palette');sta(0x2122);E(0xe8);W(0xe0,32);R(0xd0,'palette_loop')
    for p in range(22):E(0x64,p)
    for p,v in [(PX,15),(PY,15),(FOOD,8),(REST,8),(WASH,8),(FUN,8),(FAMILY,1),(DAY,1),(TRACK,1)]:setdp(p,v)
    # Optional MSU-1 signature; unenhanced SNES stays silent at this stage.
    for i,c in enumerate(b'S-MSU1'):
        W(0xad,0x2002+i);cmp(c);R(0xd0,'no_msu')
    setdp(MSU,1);call('music');L('no_msu')
    call('avatar');call('hud');reg(0x4200,1);reg(0x2100,15)
    L('frame');L('leave_blank');W(0xad,0x4212);E(0x29,0x80);R(0xd0,'leave_blank');L('enter_blank');W(0xad,0x4212);E(0x29,0x80);R(0xf0,'enter_blank');L('joy_wait');W(0xad,0x4212);E(0x29,1);R(0xd0,'joy_wait')
    for port,now,prev,edge in [(0x4219,JH,PH,EH),(0x4218,JL,PL,EL)]:
        W(0xad,port);st(now);E(0x45,prev,0x25,now);st(edge);ld(now);st(prev)
    call('erase_avatar');call('move');call('actions');call('advance');call('avatar');call('hud');jmp('frame')
    L('index');E(0xc2,0x20);ld(PY);E(0x0a,0x0a,0x0a,0x0a,0x0a,0x18,0x65,PX);sta(0x2116);E(0x0a,0xaa,0xe2,0x20,0x60)
    L('erase_avatar');call('index');W(0xbd,'map');sta(0x2118);E(0xe8);W(0xbd,'map');sta(0x2119);E(0x60)
    L('avatar');call('index');reg(0x2118,66);W(0x9c,0x2119);E(0x60)
    L('move');E(0xe6,TICK);ld(TICK);cmp(5);R(0x90,'move_done');E(0x64,TICK)
    for i,(mask,pos,bound,op) in enumerate([(2,PX,4,0xc6),(1,PX,27,0xe6),(8,PY,9,0xc6),(4,PY,20,0xe6)]):
        ld(JH);E(0x29,mask);R(0xf0,'dir'+str(i));ld(pos);cmp(bound);R(0xf0,'move_done');E(op,pos,0x60);L('dir'+str(i))
    L('move_done');E(0x60)
    L('actions');ld(EL);E(0x29,0x80);R(0xf0,'no_use')
    for i,(x,y,p) in enumerate([(7,10,FOOD),(24,10,REST),(7,20,WASH),(23,20,FUN)]):
        ld(PX);cmp(x);R(0xd0,'object'+str(i));ld(PY);cmp(y);R(0xd0,'object'+str(i));setdp(p,8);L('object'+str(i))
    L('no_use');ld(EH);E(0x29,0x40);R(0xf0,'no_add');ld(FAMILY);cmp(8);R(0xb0,'no_add');E(0xe6,FAMILY);L('no_add')
    ld(EH);E(0x29,0x80);R(0xf0,'no_remove');ld(FAMILY);cmp(1);R(0xf0,'no_remove');E(0xc6,FAMILY);L('no_remove')
    ld(EH);E(0x29,0x20);R(0xf0,'no_pack');E(0xe6,PACK);ld(PACK);cmp(5);R(0x90,'no_pack');E(0x64,PACK);L('no_pack')
    ld(EL);E(0x29,0x30);R(0xf0,'no_track');ld(MSU);R(0xf0,'no_track');ld(TRACK);E(0x49,3);st(TRACK);call('music');L('no_track');E(0x60)
    L('music');reg(0x2007,0);ld(TRACK);sta(0x2004);reg(0x2005,0);reg(0x2006,255);E(0x60)
    # Start audio when ready, without busy-waiting inside the frame's VRAM work.
    L('advance');ld(MSU);R(0xf0,'clock');W(0xad,0x2000);E(0x29,0x58);R(0xd0,'clock');reg(0x2007,3)
    L('clock');E(0xe6,CLOCK);ld(CLOCK);cmp(240);R(0x90,'advance_done');E(0x64,CLOCK)
    for i,p in enumerate((FOOD,REST,WASH,FUN)):
        ld(p);R(0xf0,'need'+str(i));E(0xc6,p);L('need'+str(i))
    E(0xe6,DAY);ld(DAY);cmp(10);R(0x90,'advance_done');setdp(DAY,1);L('advance_done');E(0x60)
    L('hud')
    for row,col,p in [(2,7,FOOD),(2,23,REST),(3,7,WASH),(3,23,FUN)]:addr(row*32+col);ld(p);call('bar')
    for p,where in [(FAMILY,5*32+8),(DAY,5*32+22)]:
        addr(where);ld(p);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119)
    addr(26*32+7);ld(PACK);E(0xc2,0x20);W(0x29,255);E(0x0a,0x0a,0x0a,0x0a,0x0a,0xaa,0xe2,0x20);W(0xa0,24);L('pack_line');W(0xbd,'packs');sta(0x2118);W(0x9c,0x2119);E(0xe8,0x88);R(0xd0,'pack_line');E(0x60)
    L('bar');st(TMP);W(0xa2,0);L('bar_loop');E(0x8a,0xc5,TMP);R(0xb0,'empty');imm(71);R(0x80,'bar_store');L('empty');imm(72);L('bar_store');sta(0x2118);W(0x9c,0x2119);E(0xe8);W(0xe0,8);R(0xd0,'bar_loop');E(0x60)
    L('irq');E(0x40)
    L('tiles');a.b.extend(tileset());L('map');a.b.extend(screen())
    L('palette');a.b.extend(struct.pack('<16H',0x1483,0x7fff,0x2189,0x2dad,0x3dce,0x2528,0x1c09,0x3ddf,0x001f,0x56b5,0x442c,0x7ad6,0x5f16,0x2a9e,0x2a69,0x1e06))
    L('packs');a.b.extend(bytes(tile(c) for s in PACKS for c in s.ljust(32)))
    program=a.finish();assert len(program)<0x7fc0
    rom=bytearray(0x20000);rom[:len(program)]=program
    # Standard LoROM, no enhancement chip claimed for this milestone.
    rom[0x7fc0:0x7fd5]=b'BORE 1 HOME PROTOTYPE'.ljust(21,b' ')
    rom[0x7fd5:0x7fdc]=bytes([0x20,0,7,0,1,0x33,0])
    for p in [0x7fe4,0x7fe6,0x7fe8,0x7fea,0x7fee,0x7ff4,0x7ff8,0x7ffa,0x7ffe]:struct.pack_into('<H',rom,p,a.labels['irq'])
    struct.pack_into('<H',rom,0x7ffc,a.labels['reset']);struct.pack_into('<HH',rom,0x7fdc,0xffff,0)
    cs=sum(rom)&65535;struct.pack_into('<HH',rom,0x7fdc,cs^65535,cs);assert sum(rom)&65535==cs
    return rom,a.labels

if __name__=='__main__':
    out=Path(sys.argv[1] if len(sys.argv)>1 else 'bore1-home.sfc');rom,labels=build();out.write_bytes(rom)
    print(json.dumps({'bytes':len(rom),'sha256':hashlib.sha256(rom).hexdigest(),'reset':hex(labels['reset']),'scope':'Native home prototype; not full game; MSU-1 optional; no native SPC playback yet'},indent=2))

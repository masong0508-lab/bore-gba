#!/usr/bin/env python3
"""BORE 1 SNES milestone 2: native home/town, bounded life sim, mission loop,
transactional dual-record SRAM and optional full-quality MSU-1 playback.
All instructions are emitted as 65816 machine code. No external assembler.
"""
import json, hashlib, struct, sys
from pathlib import Path
from build_prototype import ASM, tile, tileset as old_tileset, planar, picture, PACKS, MISSIONS

# Runtime and persistent state; position words have zero high bytes.
PX,PY,FOOD,REST,WASH,FUN,FAMILY,DAY,TICK,CLOCK,JH,JL,PH,PL,EH,EL,PACK,TMP,MSU,TRACK=0,2,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21
SCENE,CASH,DIRTY,MOVED,OLDX,OLDY,STATUS,PENDING,SLOT,SUM,XOR=22,23,24,25,26,27,28,29,30,31,32
PROG=40; CHAP=45; TASK=50
SAVE=[(FOOD,0,8),(REST,0,8),(WASH,0,8),(FUN,0,8),(FAMILY,1,8),(DAY,1,99),(CASH,0,99),(PACK,0,4)]+[(PROG+i,0,n) for i,n in enumerate(MISSIONS)]+[(CHAP+i,0,4) for i in range(5)]+[(TASK+i,0,1) for i in range(5)]
# All five columns are independent progress ledgers; these are prototype
# activity loops, not the finished narrative campaigns.
CHAPTERS=(3,4,4,3,5)
TASK_NAMES=('CARE AT HOME','FEED THE PET','WORK IN TOWN','VISIT THE PARK','WORK IN TOWN')
STATUSES=('WELCOME TO BORE','FOOD RESTORED','REST RESTORED','FRESH AND CLEAN','PET CARED FOR','WELCOME TO TOWN','WELCOME HOME','SHIFT COMPLETE','NEED REST OR FUN','BOUGHT GROCERIES','NOT ENOUGH JENES','PARK VISIT','COMPLETE THE TASK','CHAPTER COMPLETE','MISSION COMPLETE','CAMPAIGN COMPLETE','GAME SAVED','SAVE INVALID / NEW GAME')

def tileset():
    data=bytearray(old_tileset())
    art={74:'00000000 00066000 00677600 00687600 00677600 00066000 00600600 00000000',75:'00000000 00066000 00677600 00687600 00677600 00066000 00066000 00000000',76:'55555555 59999995 59111995 59111995 59111995 59111995 59999995 55555555',77:'55555555 5AAAAAA5 5AB1B1A5 5ABBBBA5 5AB1B1A5 5AAAAAA5 55555555 55555555'.replace('BBBB A','BBBBA'),78:'000D0000 00DDD000 0DDDDD00 00DDD000 00DDD000 00090000 00090000 00000000',79:'55555555 5CCCCCC5 5C1111C5 5C1CC1C5 5C1111C5 5CCCCCC5 55555555 55555555'}
    for i,p in art.items():
        rows=p.split(); assert len(rows)==8 and all(len(r)==8 for r in rows),(i,rows)
        data[i*32:(i+1)*32]=planar(picture(p))
    return bytes(data)

def screen(scene):
    a=[0]*1024
    def put(x,y,s):
        assert x+len(s)<=32,(x,s)
        a[y*32+x:y*32+x+len(s)]=[tile(c) for c in s]
    put(1,0,'BORE 1 / '+('YOUR HOME' if scene==0 else 'TOWN CENTRE'))
    put(1,2,'FOOD');put(17,2,'REST');put(1,3,'WASH');put(17,3,'FUN')
    put(1,5,'FAM');put(8,5,'DAY');put(17,5,'JENES')
    for y in range(7,22):
        for x in range(3,29):a[y*32+x]=65 if x in (3,28) or y in (7,21) else (64 if scene==0 else 73)
    # Interaction cells are deliberately walkable in this milestone; sprite
    # is an actual OBJ over the tile rather than overwriting furniture.
    for x,y,n in ([(7,10,67),(24,10,68),(7,19,69),(23,19,70),(15,19,76)] if not scene else [(7,10,77),(24,10,67),(7,19,78),(23,19,79),(15,19,76)]):a[y*32+x]=n
    for x,y,s in ([(5,8,'FRIDGE'),(22,8,'BED'),(5,17,'BATH'),(22,17,'PET'),(13,20,'TOWN')] if not scene else [(5,8,'WORK'),(22,8,'SHOP'),(5,17,'PARK'),(21,17,'MISSION'),(13,20,'HOME')]):put(x,y,s)
    put(1,22,'MOVE / A USE / START SAVE')
    put(1,23,'SELECT PACK / Y ADD B REMOVE')
    put(1,24,'L R MUSIC / PACK:')
    put(1,25,'MISSION');put(17,25,'CHAPTER')
    put(1,26,'TASK:')
    return struct.pack('<1024H',*a)

class Code(ASM):
    def long(self,op,n):self.emit(op,n&255,(n>>8)&255,(n>>16)&255)

def build():
    a=Code();E=a.emit;W=a.word;R=a.rel;L=a.label
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
    def nz(p):setdp(p,1)
    def status(v):setdp(STATUS,v);nz(DIRTY)
    def longif(op,target,tag):
        inv={0xf0:0xd0,0xd0:0xf0,0x90:0xb0,0xb0:0x90}[op];R(inv,tag);jmp(target);L(tag)
    def ascii_at(p,where):addr(where);ld(p);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119)
    def num_at(p,where):addr(where);ld(p);call('number')
    def dirty():nz(DIRTY)
    # Reset, initialize every register used by this port, and clear OAM.
    L('reset');E(0x78,0x18,0xfb,0xd8,0xc2,0x30);W(0xa2,0x1fff);E(0x9a);W(0xa9,0);E(0x5b,0x4b,0xab,0xe2,0x20)
    for p,v in [(0x2100,0x80),(0x4200,0),(0x420b,0),(0x420c,0),(0x2101,1),(0x2105,1),(0x2107,0),(0x210b,2),(0x2115,0x80),(0x2123,0),(0x2124,0),(0x2125,0),(0x212c,0x11),(0x212d,0),(0x212e,0),(0x212f,0),(0x2130,0),(0x2131,0),(0x2133,0)]:reg(p,v)
    for p in (0x210d,0x210e):reg(p,0);reg(p,0)
    # 128 hidden sprites; high table zero, small 8x8 objects.
    reg(0x2102,0);reg(0x2103,0);W(0xa2,128);L('oam_hide');reg(0x2104,0);reg(0x2104,240);reg(0x2104,0);reg(0x2104,0);E(0xca);R(0xd0,'oam_hide')
    W(0xa2,32);L('oam_high');reg(0x2104,0);E(0xca);R(0xd0,'oam_high')
    # DMA mode 1, paired VRAM data ports; bounded transfers while blank.
    reg(0x4300,1);reg(0x4301,0x18);reg(0x4304,0)
    addr(0x2000);E(0xc2,0x20);W(0xa9,'tiles');sta(0x4302);W(0xa9,len(tileset()));sta(0x4305);E(0xe2,0x20);reg(0x420b,1)
    for color_index in (0,128):
        reg(0x2121,color_index);W(0xa2,0);L('palette_'+str(color_index));W(0xbd,'palette');sta(0x2122);E(0xe8);W(0xe0,32);R(0xd0,'palette_'+str(color_index))
    for p in range(55):E(0x64,p)
    for p,v in [(PX,15),(PY,15),(FOOD,8),(REST,8),(WASH,8),(FUN,8),(FAMILY,1),(DAY,1),(TRACK,1),(CASH,12),(DIRTY,1),(MOVED,1)]:setdp(p,v)
    call('load_game')
    # Optional MSU-1 signature, never busy wait for nonexistent media.
    for i,c in enumerate(b'S-MSU1'):
        W(0xad,0x2002+i);cmp(c);R(0xd0,'no_msu')
    nz(MSU);call('music');L('no_msu')
    call('draw_scene');call('sprite');call('hud');reg(0x4200,1);reg(0x2100,15)
    L('frame');L('leave_blank');W(0xad,0x4212);E(0x29,0x80);R(0xd0,'leave_blank');L('enter_blank');W(0xad,0x4212);E(0x29,0x80);R(0xf0,'enter_blank');L('joy_wait');W(0xad,0x4212);E(0x29,1);R(0xd0,'joy_wait')
    for port,now,prev,edge in [(0x4219,JH,PH,EH),(0x4218,JL,PL,EL)]:
        W(0xad,port);st(now);E(0x45,prev,0x25,now);st(edge);ld(now);st(prev)
    E(0x64,MOVED);call('move');call('actions');call('advance')
    ld(MOVED);R(0xf0,'no_sprite');call('sprite');L('no_sprite');ld(DIRTY);R(0xf0,'no_hud');call('hud');L('no_hud');jmp('frame')
    L('draw_scene');reg(0x2100,0x80);addr(0);ld(SCENE);R(0xd0,'town_map');E(0xc2,0x20);W(0xa9,'home');R(0x80,'map_addr');L('town_map');E(0xc2,0x20);W(0xa9,'town');L('map_addr');sta(0x4302);W(0xa9,2048);sta(0x4305);E(0xe2,0x20);reg(0x420b,1);nz(MOVED);dirty();reg(0x2100,15);E(0x60)
    L('sprite');reg(0x2102,0);reg(0x2103,0);ld(PX);E(0x0a,0x0a,0x0a);sta(0x2104);ld(PY);E(0x0a,0x0a,0x0a);sta(0x2104);ld(PX);E(0x29,1,0x18,0x69,74);sta(0x2104);reg(0x2104,0x30);E(0x60)
    L('move');E(0xe6,TICK);ld(TICK);cmp(5);R(0x90,'move_done');E(0x64,TICK)
    for i,(mask,pos,bound,op) in enumerate([(2,PX,4,0xc6),(1,PX,27,0xe6),(8,PY,9,0xc6),(4,PY,19,0xe6)]):
        ld(JH);E(0x29,mask);R(0xf0,'dir'+str(i));ld(pos);cmp(bound);R(0xf0,'move_done');E(op,pos);nz(MOVED);E(0x60);L('dir'+str(i))
    L('move_done');E(0x60)
    # Input is edge-triggered, so holding A cannot repeatedly earn money.
    L('actions');ld(EL);E(0x29,0x80);R(0xf0,'no_use');call('use');L('no_use')
    ld(EH);E(0x29,0x40);R(0xf0,'no_add');ld(FAMILY);cmp(8);R(0xb0,'no_add');E(0xe6,FAMILY);dirty();L('no_add')
    ld(EH);E(0x29,0x80);R(0xf0,'no_remove');ld(FAMILY);cmp(1);R(0xf0,'no_remove');E(0xc6,FAMILY);dirty();L('no_remove')
    ld(EH);E(0x29,0x20);R(0xf0,'no_pack');E(0xe6,PACK);ld(PACK);cmp(5);R(0x90,'pack_ok');E(0x64,PACK);L('pack_ok');dirty();L('no_pack')
    ld(EH);E(0x29,0x10);R(0xf0,'no_save');call('save_game');status(16);L('no_save')
    ld(EL);E(0x29,0x30);R(0xf0,'no_track');ld(MSU);R(0xf0,'no_track');ld(TRACK);E(0x49,3);st(TRACK);call('music');L('no_track');E(0x60)
    # Home and town share the same travel door and life state.
    L('use');ld(PX);cmp(15);R(0xd0,'not_door');ld(PY);cmp(19);R(0xd0,'not_door');ld(SCENE);E(0x49,1);st(SCENE);R(0xf0,'back_home');status(5);R(0x80,'travel');L('back_home');status(6);L('travel');setdp(PY,15);call('draw_scene');E(0x60);L('not_door')
    ld(SCENE);longif(0xd0,'town_use','is_home')
    for i,(x,y,p,stat,task) in enumerate([(7,10,FOOD,1,0),(24,10,REST,2,0),(7,19,WASH,3,0),(23,19,FUN,4,1)]):
        ld(PX);cmp(x);R(0xd0,'obj'+str(i));ld(PY);cmp(y);R(0xd0,'obj'+str(i));setdp(p,8);status(stat);setdp(TASK+task,1);E(0x60);L('obj'+str(i))
    E(0x60)
    L('town_use')
    for i,(x,y,dest) in enumerate([(7,10,'work'),(24,10,'shop'),(7,19,'park'),(23,19,'mission')]):
        ld(PX);cmp(x);R(0xd0,'tobj'+str(i));ld(PY);cmp(y);R(0xd0,'tobj'+str(i));jmp(dest);L('tobj'+str(i))
    E(0x60)
    L('work');ld(REST);cmp(3);R(0x90,'cant_work');ld(FUN);cmp(1);R(0x90,'cant_work');ld(REST);E(0x38,0xe9,3);st(REST);E(0xc6,FUN);ld(CASH);E(0x18,0x69,8);cmp(100);R(0x90,'work_cash');imm(99);L('work_cash');st(CASH);nz(TASK+2);nz(TASK+4);status(7);E(0x60);L('cant_work');status(8);E(0x60)
    L('shop');ld(CASH);cmp(3);R(0x90,'cant_buy');E(0x38,0xe9,3);st(CASH);setdp(FOOD,8);nz(TASK);status(9);E(0x60);L('cant_buy');status(10);E(0x60)
    L('park');setdp(FUN,8);nz(TASK+3);status(11);E(0x60)
    L('mission');ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20);E(0xb5,PROG);W(0xdd,'mission_limits');R(0xb0,'campaign_done');E(0xb5,TASK);R(0xf0,'need_task');E(0x74,TASK);E(0xf6,CHAP,0xb5,CHAP);W(0xdd,'chapter_limits');R(0x90,'chapter_done');E(0x74,CHAP,0xf6,PROG);status(14);E(0x60);L('chapter_done');status(13);E(0x60);L('need_task');status(12);E(0x60);L('campaign_done');status(15);E(0x60)
    L('music');reg(0x2007,0);ld(TRACK);sta(0x2004);reg(0x2005,0);reg(0x2006,235);nz(PENDING);E(0x60)
    L('advance');ld(PENDING);R(0xf0,'clock');W(0xad,0x2000);E(0x29,0x40);R(0xd0,'clock');W(0xad,0x2000);E(0x29,8);R(0xd0,'media_error');reg(0x2007,3);L('media_error');E(0x64,PENDING)
    L('clock');E(0xe6,CLOCK);ld(CLOCK);cmp(240);R(0x90,'advance_done');E(0x64,CLOCK)
    for i,p in enumerate((FOOD,REST,WASH,FUN)):
        ld(p);R(0xf0,'need'+str(i));E(0xc6,p);L('need'+str(i))
    E(0xe6,DAY);ld(DAY);cmp(100);R(0x90,'day_ok');setdp(DAY,1);L('day_ok');dirty();L('advance_done');E(0x60)
    # Write all HUD fields only when their values change.
    L('hud');E(0x64,DIRTY)
    for row,col,p in [(2,7,FOOD),(2,23,REST),(3,7,WASH),(3,23,FUN)]:addr(row*32+col);ld(p);call('bar')
    ascii_at(FAMILY,5*32+5);num_at(DAY,5*32+12);num_at(CASH,5*32+23)
    # selected pack's mission/chapter and name. A two-digit counter can show all 19.
    addr(24*32+18);ld(PACK);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119)
    addr(25*32+9);ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20,0xb5,PROG);call('number')
    addr(25*32+26);ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20,0xb5,CHAP);call('number')
    for p,where,label,tag,count in [(PACK,26*32+6,'tasks','taskline',24),(STATUS,27*32+1,'messages','statusline',30)]:
        addr(where);ld(p);E(0xc2,0x20);W(0x29,255);E(0x0a,0x0a,0x0a,0x0a,0x0a,0xaa,0xe2,0x20);W(0xa0,count);L(tag);W(0xbd,label);sta(0x2118);W(0x9c,0x2119);E(0xe8,0x88);R(0xd0,tag)
    E(0x60)
    L('number');W(0xa2,0);L('tens_loop');cmp(10);R(0x90,'digits');E(0x38,0xe9,10,0xe8);R(0x80,'tens_loop');L('digits');st(TMP);E(0x8a,0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119);ld(TMP);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119);E(0x60)
    L('bar');st(TMP);W(0xa2,0);L('bar_loop');E(0x8a,0xc5,TMP);R(0xb0,'empty');imm(71);R(0x80,'bar_store');L('empty');imm(72);L('bar_store');sta(0x2118);W(0x9c,0x2119);E(0xe8);W(0xe0,8);R(0xd0,'bar_loop');E(0x60)
    # Two independent save records. Selector is committed LAST; if a write
    # tears, an intact old record remains available. Validate both checksums
    # plus every individual range before loading any state.
    base=0x700010; select=0x700000
    L('slot_x');ld(SLOT);R(0xf0,'slot0');W(0xa2,64);E(0x60);L('slot0');W(0xa2,0);E(0x60)
    L('save_game');a.long(0xaf,select);cmp(0xa1);R(0xf0,'write0');setdp(SLOT,1);R(0x80,'write_slot');L('write0');E(0x64,SLOT);L('write_slot');call('slot_x');imm(0);a.long(0x9f,base);setdp(SUM,0x55);setdp(XOR,0xa7)
    for j,(p,_,_) in enumerate(SAVE):
        ld(p);a.long(0x9f,base+1+j);E(0x18,0x65,SUM);st(SUM);ld(p);E(0x45,XOR);st(XOR)
    ld(SUM);a.long(0x9f,base+1+len(SAVE));ld(XOR);a.long(0x9f,base+2+len(SAVE));imm(0xb2);a.long(0x9f,base);ld(SLOT);E(0x09,0xa0);a.long(0x8f,select);E(0x60)
    L('load_game');a.long(0xaf,select);cmp(0xa1);R(0xd0,'load0');setdp(SLOT,1);R(0x80,'load_first');L('load0');E(0x64,SLOT);L('load_first');call('load_slot');R(0xb0,'load_ok');ld(SLOT);E(0x49,1);st(SLOT);call('load_slot');R(0xb0,'load_ok');E(0x60);L('load_ok');status(16);E(0x60)
    L('load_slot');call('slot_x');a.long(0xbf,base);cmp(0xb2);R(0xf0,'valid_magic');E(0x18,0x60);L('valid_magic');setdp(SUM,0x55);setdp(XOR,0xa7)
    for j,(p,lo,hi) in enumerate(SAVE):
        a.long(0xbf,base+1+j);cmp(lo);R(0xb0,'min'+str(j));E(0x18,0x60);L('min'+str(j));cmp(hi+1);R(0x90,'max'+str(j));E(0x18,0x60);L('max'+str(j));E(0x18,0x65,SUM);st(SUM);a.long(0xbf,base+1+j);E(0x45,XOR);st(XOR)
    for j,p in enumerate((SUM,XOR)):
        a.long(0xbf,base+1+len(SAVE)+j);E(0xc5,p);R(0xf0,'check'+str(j));E(0x18,0x60);L('check'+str(j))
    # Chapter count and completed-campaign bounds are also validated.
    for i,n in enumerate(CHAPTERS):
        j=next(j for j,(p,_,_) in enumerate(SAVE) if p==CHAP+i);a.long(0xbf,base+1+j);cmp(n);R(0x90,'chapter_valid'+str(i));E(0x18,0x60);L('chapter_valid'+str(i))
    for j,(p,_,_) in enumerate(SAVE):a.long(0xbf,base+1+j);st(p)
    E(0x38,0x60)
    L('irq');E(0x40)
    L('tiles');a.b.extend(tileset());L('home');a.b.extend(screen(0));L('town');a.b.extend(screen(1))
    L('palette');a.b.extend(struct.pack('<16H',0x1483,0x7fff,0x2189,0x2dad,0x3dce,0x2528,0x1c09,0x3ddf,0x001f,0x56b5,0x442c,0x7ad6,0x5f16,0x2a9e,0x2a69,0x1e06))
    L('tasks');a.b.extend(bytes(tile(c) for s in TASK_NAMES for c in s.ljust(32)));L('messages');a.b.extend(bytes(tile(c) for s in STATUSES for c in s.ljust(32)))
    L('mission_limits');a.b.extend(bytes(MISSIONS));L('chapter_limits');a.b.extend(bytes(CHAPTERS))
    program=a.finish();assert len(program)<0x7fc0,len(program)
    rom=bytearray(0x20000);rom[:len(program)]=program
    rom[0x7fc0:0x7fd5]=b'BORE 1 TOWN M2'.ljust(21,b' ');rom[0x7fd5:0x7fdc]=bytes([0x20,2,7,1,1,0x33,1])
    for p in [0x7fe4,0x7fe6,0x7fe8,0x7fea,0x7fee,0x7ff4,0x7ff8,0x7ffa,0x7ffe]:struct.pack_into('<H',rom,p,a.labels['irq'])
    struct.pack_into('<H',rom,0x7ffc,a.labels['reset']);struct.pack_into('<HH',rom,0x7fdc,0xffff,0);cs=sum(rom)&65535;struct.pack_into('<HH',rom,0x7fdc,cs^65535,cs);assert sum(rom)&65535==cs
    return rom,a.labels

if __name__=='__main__':
    out=Path(sys.argv[1] if len(sys.argv)>1 else 'bore1-town.sfc');rom,labels=build();out.write_bytes(rom);print(json.dumps({'bytes':len(rom),'sha256':hashlib.sha256(rom).hexdigest(),'reset':hex(labels['reset']),'scope':'SNES gameplay slice; not full campaign or native SPC soundtrack'},indent=2))

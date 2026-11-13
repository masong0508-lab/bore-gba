# Runs the real DS build (ARM9 and ARM7 images) in two Unicorn CPUs sharing main RAM, with a fake cartridge save chip (flash or EEPROM, any size, optionally pre-filled) on the ARM7's SPI port.
# Checks what the slot code does on the real chip commands: finds the size, lays out slots, saves into them, refuses a save that does not fit, changes the layout, keeps an old single save.
# Needs images built with:  GG_FLAGS="-fkeep-static-functions -fno-inline-functions-called-once -fno-inline-small-functions" ./build_hw.sh   (copy build_hw/arm9.bin, arm9.elf, arm7.bin to a directory and `nm arm9.elf > syms.txt` there)
import sys, ctypes, struct
from unicorn import *
from unicorn.arm_const import *
D = sys.argv[1] if len(sys.argv)>1 else '/tmp/hwtest'
syms = {}
for l in open(D+'/syms.txt'):
    p = l.split()
    if len(p)==3: syms[p[2].split('.')[0]] = int(p[0],16)
IPC = 0x023F0000; SVTYPE = IPC+32; SVBUSY = IPC+36; SVOK = IPC+40; SVSIZE = IPC+44
class Chip:
    def __init__(self, kind, cap, jedec=None, data=None):
        self.kind=kind; self.cap=cap; self.jedec=jedec or (0x20,0x40,cap.bit_length()-1); self.data=bytearray(data if data is not None else b'\xff'*cap); self.cmd=None; self.n=0; self.addr=0; self.writes=0
    def byte(self, b, hold):
        out=0
        if self.cmd is None:
            self.cmd=b; self.n=0; self.addr=0
        else:
            c=self.cmd; fl=(self.kind=='flash'); na=3 if fl else 2
            if c==0x9F: out=(self.jedec[self.n] if (self.kind=='flash' and self.n<3) else 0)
            elif c==0x05: out=0 if self.kind=='flash' or True else 0
            elif c in (0x03,0x0A,0x02):
                if self.n<na: self.addr=(self.addr<<8)|b
                else:
                    i=self.n-na
                    if c==0x03: out=self.data[(self.addr+i)%self.cap]
                    else:
                        page=256 if fl else 128; base=self.addr-(self.addr%page); pos=base+((self.addr%page)+i)%page; self.data[pos%self.cap]=b; self.writes+=1
            self.n+=1
        if not hold: self.cmd=None
        return out
def make(kind, cap, **kw):
    chip=Chip(kind,cap,**kw)
    host=ctypes.create_string_buffer(4*1024*1024)
    u9=Uc(UC_ARCH_ARM,UC_MODE_ARM); u9.ctl_set_cpu_model(UC_CPU_ARM_946)
    u7=Uc(UC_ARCH_ARM,UC_MODE_ARM); u7.ctl_set_cpu_model(UC_CPU_ARM_946)
    for u in (u9,u7): u.mem_map_ptr(0x02000000,4*1024*1024,UC_PROT_ALL,ctypes.addressof(host))
    for base,size in ((0x04000000,0x100000),(0x05000000,0x10000),(0x06000000,0x800000),(0x07000000,0x10000)): u9.mem_map(base,size)
    u7.mem_map(0x04000000,0x100000); u7.mem_map(0x03800000,0x10000)
    u9.mem_write(0x02000000,open(D+'/arm9.bin','rb').read()); u7.mem_write(0x02380000,open(D+'/arm7.bin','rb').read())
    vc=[0]
    def vcount(uc,acc,a,s,v,u): vc[0]=(vc[0]+1)%263; uc.mem_write(0x04000006,struct.pack('<H',vc[0]))
    u9.hook_add(UC_HOOK_MEM_READ,vcount,None,0x04000006,0x04000007); u7.hook_add(UC_HOOK_MEM_READ,vcount,None,0x04000006,0x04000007)
    st={'auxcnt':0,'auxdat':0}
    def w_auxcnt(uc,acc,a,s,v,u): st['auxcnt']=v
    def r_auxcnt(uc,acc,a,s,v,u): uc.mem_write(0x040001A0,struct.pack('<H',st['auxcnt']&~0x80))
    def w_auxdat(uc,acc,a,s,v,u): st['auxdat']=chip.byte(v&0xFF,bool(st['auxcnt']&0x40))
    def r_auxdat(uc,acc,a,s,v,u): uc.mem_write(0x040001A2,struct.pack('<H',st['auxdat']))
    u7.hook_add(UC_HOOK_MEM_WRITE,w_auxcnt,None,0x040001A0,0x040001A1); u7.hook_add(UC_HOOK_MEM_READ,r_auxcnt,None,0x040001A0,0x040001A1)
    u7.hook_add(UC_HOOK_MEM_WRITE,w_auxdat,None,0x040001A2,0x040001A3); u7.hook_add(UC_HOOK_MEM_READ,r_auxdat,None,0x040001A2,0x040001A3)
    sc=[0]
    def r_spi(uc,acc,a,s,v,u): uc.mem_write(0x040001C0,struct.pack('<H',0))
    u7.hook_add(UC_HOOK_MEM_READ,r_spi,None,0x040001C0,0x040001C1)
    return chip,u9,u7,host
class Rig:
    def __init__(self, kind, cap, **kw):
        self.chip,self.u9,self.u7,self.host=make(kind,cap,**kw); self.pc9=0x02000800; self.pc7=0x02380000; self.hud=0
        def onhud(uc,a,s,u):
            self.hud+=1
            if self.hud==2: uc.emu_stop()
        self.u9.hook_add(UC_HOOK_CODE,onhud,None,syms['hud'],syms['hud'])
    def slice(self,cpu,n):
        u,pc=(self.u9,self.pc9) if cpu==9 else (self.u7,self.pc7)
        try: u.emu_start(pc,0,count=n)
        except UcError as e: print("emulation error",cpu,e); raise
        npc=u.reg_read(UC_ARM_REG_PC)
        if cpu==9: self.pc9=npc
        else: self.pc7=npc
    def boot(self):
        for i in range(4000):
            self.slice(7,60000); self.slice(9,300000)
            if self.hud>=2: break
        assert self.hud>=2, "the game did not reach its main loop"
    def call(self,name,*args,maxcalls=8000):
        u=self.u9; TRAP=0x02000400; done=[False]
        def trap(uc,a,s,ud): done[0]=True; uc.emu_stop()
        h=u.hook_add(UC_HOOK_CODE,trap,None,TRAP,TRAP)
        for i,a in enumerate(args): u.reg_write(UC_ARM_REG_R0+i,a)
        u.reg_write(UC_ARM_REG_LR,TRAP); pc=syms[name]
        for k in range(maxcalls):
            self.slice7=self.slice(7,60000)
            try: u.emu_start(pc,0,count=300000)
            except UcError as e: print("emulation error",e); raise
            if done[0]: break
            pc=u.reg_read(UC_ARM_REG_PC)
        u.hook_del(h); return u.reg_read(UC_ARM_REG_R0)
    def rd(self,name,size=4):
        return struct.unpack('<I' if size==4 else '<B',bytes(self.u9.mem_read(syms[name],size)))[0]
    def wr(self,name,val): self.u9.mem_write(syms[name],struct.pack('<I',val))
ok=True
def check(c,what):
    global ok; print("  %s: %s"%(what,"yes" if c else "NO")); ok&=bool(c)
def meta(r,k):
    return bytes(r.u9.mem_read(syms['slotMeta']+k*16,1))[0]      # SlotMeta is 16 bytes, `used` first
def scenario(title,kind,cap,**kw):
    print(title); r=Rig(kind,cap,**kw); r.boot(); return r
# A: 512 KB flash, empty -> 2 slots
r=scenario("512 KB flash (JEDEC capacity code 0x13), empty","flash",512*1024)
check(r.rd('hwCap')==512*1024,"size found: 512 KB"); check(r.rd('nSlots')==2,"two slots"); sz=r.rd('hwSlotSz'); check(sz==((512*1024-256)//2)&~255,"slot size %d bytes"%sz)
check(bytes(r.chip.data[512*1024-256:512*1024-256+4])==b'GGL1' and r.chip.data[512*1024-256+8]==2,"layout record written at the end of the chip")
used0=meta(r,0); check(used0==0,"no slot holds a save yet")
r.wr('curSlot',1); r.wr('hwSlotCur',1); r.call('doSave'); r.call('slotsRefresh'); check(meta(r,1)==1 and meta(r,0)==0,"saving with slot 2 selected fills slot 2 only")
check(bytes(r.chip.data[sz:sz+3])==b'GGS',"the save sits at the start of slot 2 (offset %d)"%sz); check(bytes(r.chip.data[0:3])!=b'GGS',"slot 1 untouched")
# too big for the slot
saved=r.rd('hwSlotSz'); r.wr('hwSlotSz',2048); r.wr('curSlot',0); r.wr('hwSlotCur',0); r.call('doSave'); tb=r.rd('svTooBig'); check(tb>0,"a save bigger than its slot is refused (needs %d KB)"%tb); check(bytes(r.chip.data[0:3])!=b'GGS',"and nothing was written"); r.wr('hwSlotSz',saved)
r.call('doSave'); r.call('slotsRefresh'); check(meta(r,0)==1,"slot 1 saves normally again")
check(r.call('hwLayoutOk',4)==1,"4 slots are allowed on 512 KB (each %d KB)"%((((512*1024-256)//4)&~255)//1024))
res=r.call('hwLayoutSet',4); check(res==1,"layout changed to 4 slots"); check(r.rd('hwSlots')==4,"the game now has 4 slots"); check(bytes(r.chip.data[0:3])==b'GGS',"slot 1 kept its save"); check(bytes(r.chip.data[sz:sz+3])!=b'GGS',"slot 2's old save is gone (its offset moved)")
# B: 1 MB -> 4 slots
r=scenario("1 MB flash, empty","flash",1024*1024); check(r.rd('nSlots')==4,"four slots")
# C: 256 KB -> 1 slot
r=scenario("256 KB flash","flash",256*1024); check(r.rd('hwCap')==256*1024 and r.rd('nSlots')==1,"one slot of the whole chip (minus the 256-byte record page)")
# D: EEPROMs
r=scenario("8 KB EEPROM, empty","eeprom",8192); check(r.rd('hwCap')==8192,"size found by the write test: 8 KB"); check(r.rd('nSlots')==1 and r.rd('hwSlotSz')==8192,"one slot of 8 KB"); check(r.chip.data[:16]==b'\xff'*16,"the test pattern was put back")
r=scenario("64 KB EEPROM, empty","eeprom",65536); check(r.rd('hwCap')==65536,"size found: 64 KB")
r.call('doSave'); r.call('slotsRefresh'); check(meta(r,0)==1,"a 128 x 128 save fits and is listed")
r=scenario("8 KB EEPROM holding data","eeprom",8192,data=bytes((i*7+3)&255 for i in range(8192))); check(r.rd('hwCap')==8192,"size found by the wrap read: 8 KB")
# F: unknown flash id
r=scenario("flash with an unknown capacity code","flash",512*1024,jedec=(0x20,0x40,0x00)); check(r.rd('hwCap')==0 and r.rd('nSlots')==1 and r.rd('hwSlotSz')==0,"unknown size: one slot, no limit (as before slots existed)")
r.call('doSave'); r.call('slotsRefresh'); check(meta(r,0)==1,"it still saves"); legacy=bytes(r.chip.data)
# G: an old single save on a flash chip whose size is now known
r=scenario("512 KB flash holding an old single-slot save","flash",512*1024,data=legacy); check(r.rd('nSlots')==1,"the old save keeps the one slot it had"); check(meta(r,0)==1,"and is listed in slot 1")
print("SLOTS-HW OK" if ok else "SLOTS-HW WEAK")

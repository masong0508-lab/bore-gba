import sys, ctypes, struct
from unicorn import *
from unicorn.arm_const import *
syms = {}
for l in open('syms.txt'):
    p = l.split()
    if len(p)==3: syms[p[2]] = int(p[0],16)
img = open('/home/user/bore-gba/godguy/build_dsibig/arm9.bin','rb').read()
uc = Uc(UC_ARCH_ARM, UC_MODE_ARM); uc.ctl_set_cpu_model(UC_CPU_ARM_946)
RAM = 16*1024*1024; host = ctypes.create_string_buffer(RAM); uc.mem_map_ptr(0x02000000, RAM, UC_PROT_ALL, ctypes.addressof(host))
for base,size in ((0x04000000,0x100000),(0x05000000,0x10000),(0x06000000,0x800000),(0x07000000,0x10000),(0x03000000,0x10000)): uc.mem_map(base,size)
uc.mem_write(0x02000000, img)
IPC=0x023F0000; SVTYPE=IPC+32; SVBUSY=IPC+36; SVOK=IPC+40
vc=[0]; flashed=[]
def vcount(uc,acc,a,s,v,u):
    vc[0]=(vc[0]+1)%263; uc.mem_write(0x04000006,struct.pack('<H',vc[0])); uc.mem_write(SVTYPE,struct.pack('<I',1))      # a flash chip is "present"
uc.hook_add(UC_HOOK_MEM_READ,vcount,None,0x04000006,0x04000007)
def svbusy(uc,acc,a,s,v,u):      # the ARM7 would do the save and clear busy: do it at the moment the ARM9 looks
    uc.mem_write(SVBUSY,struct.pack('<I',0)); uc.mem_write(SVOK,struct.pack('<I',1))
uc.hook_add(UC_HOOK_MEM_READ,svbusy,None,SVBUSY,SVBUSY+3)
writes={'low':0,'upper':0,'svcmp':0,'top':0}; 
def wr(uc,acc,a,s,v,u):
    if a<0x02300000: writes['low']+=1
    elif a<0x02400000: writes['upper' if False else 'low']+=0
    elif a<0x02C00000: writes['upper']+=1
    elif a<0x02FFC000: writes['svcmp']+=1
    else: writes['top']+=1
uc.hook_add(UC_HOOK_MEM_WRITE,wr)
hudn=[0]
def onhud(uc,a,s,u):
    hudn[0]+=1
    if hudn[0]==2: uc.emu_stop()
uc.hook_add(UC_HOOK_CODE,onhud,None,syms['hud'],syms['hud'])
uc.emu_start(0x02000800,0,count=800_000_000)
print("game reached its main loop; calling doSave (the real save path of the DSi-mode build)")
for k in writes: writes[k]=0
uc.mem_write(SVOK,struct.pack('<I',1))
TRAP=0x02000400
uc.reg_write(UC_ARM_REG_LR,TRAP); 
def trap(uc,a,s,u): uc.emu_stop()
uc.hook_add(UC_HOOK_CODE,trap,None,TRAP,TRAP)
uc.reg_write(UC_ARM_REG_PC,syms['doSave'])
uc.emu_start(syms['doSave'],TRAP,count=800_000_000)
hdr=bytes(uc.mem_read(0x02C00000,12))
print("packed save header at 0x02C00000:",hdr[:3],hdr[3],"packed bytes",struct.unpack('<I',hdr[4:8])[0],"raw bytes",struct.unpack('<I',hdr[8:12])[0])
print("writes during the save: below 4 MB",writes['low'],"| big arrays region",writes['upper'],"| save block 0x02C00000+",writes['svcmp'],"| system area at the top",writes['top'])
# decode the packed data like the game does and check the planet header magic
clen,raw=struct.unpack('<II',hdr[4:12]); data=bytes(uc.mem_read(0x02C0000C,clen)); out=bytearray(); i=0
while i<len(data):
    c=data[i]; i+=1
    if c<128: out+=data[i:i+c+1]; i+=c+1
    else: r=c-126; out+=bytes([data[i]])*r; i+=1
print("decoded %d bytes (header says %d): %s; save magic %s"%(len(out),raw,"MATCH" if len(out)==raw else "MISMATCH",out[:4]))

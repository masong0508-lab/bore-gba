import sys, ctypes, struct
from unicorn import *
from unicorn.arm_const import *
mode = sys.argv[1]            # "dsi" (16 MB of RAM) or "ds" (4 MB mirrored)
budget = int(sys.argv[2]) if len(sys.argv)>2 else 400_000_000
syms = {}
for l in open('syms.txt'):
    p = l.split()
    if len(p)==3: syms[p[2]] = int(p[0],16)
img = open('/home/user/bore-gba/godguy/build_dsibig/arm9.bin','rb').read()
uc = Uc(UC_ARCH_ARM, UC_MODE_ARM)
try: uc.ctl_set_cpu_model(UC_CPU_ARM_946)
except Exception as e: print("cpu model:", e)
RAM = 16*1024*1024
if mode == "dsi":
    host = ctypes.create_string_buffer(RAM); uc.mem_map_ptr(0x02000000, RAM, UC_PROT_ALL, ctypes.addressof(host))
else:
    host = ctypes.create_string_buffer(4*1024*1024)
    for k in range(4): uc.mem_map_ptr(0x02000000+k*0x400000, 0x400000, UC_PROT_ALL, ctypes.addressof(host))     # 4 MB mirrored 4 times
mm = {}
for base,size in ((0x04000000,0x100000),(0x05000000,0x10000),(0x06000000,0x800000),(0x07000000,0x10000),(0x03000000,0x10000)):
    uc.mem_map(base,size)
uc.mem_write(0x02000000, img)
vc = [0]
def vcount(uc, access, address, size, value, ud):
    vc[0] = (vc[0]+1) % 263
    uc.mem_write(0x04000006, struct.pack('<H', vc[0]))
uc.hook_add(UC_HOOK_MEM_READ, vcount, None, 0x04000006, 0x04000007)
seen = {}
def mark(name):
    def f(uc, addr, size, ud):
        seen[name] = seen.get(name,0)+1
    return f
for n in ("newWorld","genWorld","hud","uiFlush","game_main","hwBoot","main"):
    a = syms[n]; uc.hook_add(UC_HOOK_CODE, mark(n), None, a, a)
errs = []
try:
    uc.emu_start(0x02000800, 0, count=budget)
except UcError as e:
    errs.append(str(e)); print("stopped:", e, "pc=%08x" % uc.reg_read(UC_ARM_REG_PC))
print("mode", mode, "executed marks:", seen)
alias = struct.unpack('<I', uc.mem_read(syms['dsiAlias'],4))[0]
print("dsiAlias =", alias)
w = bytes(uc.mem_read(syms['world'], 128*768))
print("world bytes in upper RAM written:", sum(1 for b in w if b) , "nonzero (a generated planet has many)")

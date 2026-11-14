#!/usr/bin/env python3
"""Boot probe under Unicorn (pip install unicorn): runs build_dsi16/arm9.elf's crt0 on an ARM946 with either 16 MB of RAM (DSi mode) or 4 MB mirrored (DS mode).
DSi mode must get past the probe, switch the caches on (region 3 = save block) and reach hwBoot; DS mode must stop on the red screen without touching the program.
usage: probe.py arm9.elf arm9.bin"""
import sys, subprocess
from unicorn import *
from unicorn.arm_const import *
elf, binf = sys.argv[1], sys.argv[2]
nm = subprocess.check_output(['arm-none-eabi-nm', elf]).decode()
sym = {l.split()[2]: int(l.split()[0], 16) for l in nm.splitlines() if len(l.split()) == 3}
img = open(binf, 'rb').read()
def run(ds_mode):
    uc = Uc(UC_ARCH_ARM, UC_MODE_ARM); uc.ctl_set_cpu_model(UC_CPU_ARM_946) if hasattr(uc, 'ctl_set_cpu_model') else None
    if ds_mode:
        uc.mem_map(0x02000000, 0x400000)
        def rd(u, off, sz, ud): return int.from_bytes(u.mem_read(0x02000000 + ((0x400000 + off) & 0x3FFFFF), sz), 'little')
        def wr(u, off, sz, val, ud): u.mem_write(0x02000000 + ((0x400000 + off) & 0x3FFFFF), (val & ((1 << 8 * sz) - 1)).to_bytes(sz, 'little'))
        uc.mmio_map(0x02400000, 0xC00000, rd, None, wr, None)
    else: uc.mem_map(0x02000000, 0x1000000)
    for a, n in ((0x04000000, 0x100000), (0x05000000, 0x1000), (0x06000000, 0x1000000), (0x07000000, 0x1000)): uc.mem_map(a, n)
    uc.mem_write(0x02000000, img)
    uc.reg_write(UC_ARM_REG_CPSR, 0x1D3)
    stop = sym['hwBoot']; red = 0x06800000
    hit = {}
    uc.hook_add(UC_HOOK_CODE, lambda u, a, s, d: (hit.setdefault('boot', 1), u.emu_stop()), begin=stop, end=stop)
    try: uc.emu_start(0x02000800, 0, count=30000000 if not ds_mode else 400000)
    except UcError as e: hit['err'] = str(e)
    red_px = int.from_bytes(uc.mem_read(red + 0x1000, 4), 'little')
    code_ok = bytes(uc.mem_read(0x02000800, 4096)) == img[0x800:0x1800]
    return hit, red_px, code_ok, uc
hit, px, ok, uc = run(False)
print('DSi mode (16 MB): reached hwBoot' if hit.get('boot') else 'DSi mode: FAIL %s' % hit, '| red screen' if px == 0x801F801F else '| no red screen')
bad = not hit.get('boot') or px == 0x801F801F or not ok
hit2, px2, ok2, _ = run(True)
print('DS mode (4 MB): red screen, program intact' if px2 == 0x801F801F and ok2 and not hit2.get('boot') else 'DS mode: FAIL %s %x %s' % (hit2, px2, ok2))
bad |= not (px2 == 0x801F801F and ok2 and not hit2.get('boot'))
print('probe: FAIL' if bad else 'probe: ok'); sys.exit(1 if bad else 0)

#!/usr/bin/env python3
"""Host verification of compiled option table values and pacing source wiring."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
 p=Path(d);c=p/'test.c';c.write_text('''#include <stdint.h>\n#include <assert.h>\ntypedef uint8_t u8; typedef uint16_t u16;\n#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))\n#define K_LEFT 1\n#define K_RIGHT 2\n#define K_UP 4\n#define K_DOWN 8\n#include "opts.h"\nint main(void){optsDefaults(); assert(oStepsMin()==30 && oNeedPct()==50 && oFoodEvery()==360 && oWcEvery()==300); int day[]={15,30,60,120,0},need[]={0,25,50,75,100}; for(int i=0;i<5;i++){xo[XO_DAY]=xo[XO_NEED]=i; assert(oStepsMin()==day[i] && oNeedPct()==need[i]);} xo[XO_HUNGER]=0;assert(!oFoodEvery()&&!oWcEvery());return 0;}\n''');subprocess.run(['cc','-std=c11','-I',str(root/'source'),str(c),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
assert 'hhNeedEvery&&' in (root/'source/house.h').read_text()
assert 'lifeNeedRate' in (root/'source/sims.h').read_text()
print('GBA life pacing: compiled options, frozen clock, needs off, household wiring PASS')

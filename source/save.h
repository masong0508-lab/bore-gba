// save.h - the save chip: 128 KB of flash (FLASH1M; a 64 KB flash chip works too), or 32 KB of battery SRAM when the cart has no flash.
// Needs before it: u8 / u16 / u32, EWRAM_BSS, IWRAM_THUMB, REG_VCOUNT.
//
// Flash is not written like RAM: a byte can only be PROGRAMMED from the erased value 0xFF (bits go 1 -> 0), and only whole 4 KB
// sectors can be ERASED. So the save memory is used like this:
//   0     .. 4095    sector 0: the head of the room being played (mapSave rewrites the sector when the room changed)
//   4096  .. 8191    sector 1: the small blocks (settings, options, persona, jukebox, life, household ...). They live in RAM (svLow) and
//                    old code reaches them through SRAM_BASE+offset as before. svCommit writes them back when they changed; svTick (in
//                    vsync) compares a slice every frame, so a change reaches the chip within half a second even with no svCommit call.
//   8192  ..         the slots (slots.h): a slot is erased first (svErase), then written byte by byte (svWr). Reads go through svPtr().
//   the last sector  scratch: when only half of a sector must be erased, svErase parks the other half in there and copies it back.
// On SRAM everything is plain bytes: SRAM_BASE is the chip itself, svWr writes, svErase fills with 0xFF.
//
// Which chip: the first touch of the save memory must be a flash command (mGBA picks the save type from the first access), so svInit
// asks for the flash ID first. A known ID = flash; anything else = SRAM. Asking writes two bytes on an SRAM chip (0x2AAA, 0x5555, inside
// slots 2 and 7): svWr keeps a copy of those two bytes (SV_BK) and svInit puts them back. A 64 KB ID is asked to switch to bank 1 and back,
// which turns mGBA's 64 KB flash into 128 KB (a real 64 KB chip ignores it). Atmel 64 KB chips (page writes) are not supported.
enum { SV_SRAM=1, SV_FLASH=2 };
#define SV_SEC   4096u
#define SV_LOW0  4096u      // the RAM copy covers SV_LOW0 .. SV_LOW0+SV_LOWN-1 (flash only)
#define SV_LOWN  4096u
#define SV_BK    4840u      // SRAM only: 'S' 'K', then copies of the bytes at 0x2AAA and 0x5555 (svInit writes over them)
static u8 svType=SV_SRAM, svErr;   // svErr: a write or an erase failed since it was last cleared
static u32 svSize=32768, svScr;    // bytes on the chip; the scratch sector (flash)
static int svBank;                 // the 64 KB bank the chip shows (128 KB flash)
static u16 svScan;                 // svTick: where the next comparison starts
static u8 svLow[SV_LOWN] EWRAM_BSS;
static u32 svShad[SV_LOWN/4] EWRAM_BSS;   // flash: what sector 1 of the chip holds right now (read back after every commit), so svTick compares RAM with RAM, not with the slow chip
static volatile u8* svBase=(volatile u8*)0x0E000000;
#define SRAM_BASE svBase           // every old SRAM_BASE+offset with offset 4096..8191 lands in svLow on flash
#define SVB ((volatile u8*)0x0E000000)

// ---- the flash commands (in IWRAM, so nothing on the cartridge bus runs between the steps) ----
IWRAM_THUMB static void flCmd(int c){ SVB[0x5555]=0xAA; SVB[0x2AAA]=0x55; SVB[0x5555]=(u8)c; }
IWRAM_THUMB static int flWait(int a,int want,int tmo){ while(tmo-->0) if(SVB[a]==(u8)want) return 1; SVB[0x5555]=0xF0; return 0; }
IWRAM_THUMB static int flProgRaw(int a,int v){ flCmd(0xA0); SVB[a]=(u8)v; return flWait(a,v,60000); }
IWRAM_THUMB static int flEraseRaw(int a){ flCmd(0x80); SVB[0x5555]=0xAA; SVB[0x2AAA]=0x55; SVB[a]=0x30; return flWait(a,0xFF,4000000); }
IWRAM_THUMB static void flBankRaw(int b){ flCmd(0xB0); SVB[0]=(u8)b; }
static void svWaitFrames(int n){ while(n-->0){ while(REG_VCOUNT>=160); while(REG_VCOUNT<160); } }

static void svBankTo(int b){ if(svSize>65536&&b!=svBank){ flBankRaw(b); svBank=b; } }
static u8 svChip(u32 off){ if(svType!=SV_FLASH) return SVB[off&0x7FFF]; svBankTo((int)(off>>16)); return SVB[off&0xFFFF]; }
// A pointer to read save bytes at off (valid until another bank is used; a multi-slot save never crosses a bank, see slFits)
static volatile u8* svPtr(u32 off){ if(svType!=SV_FLASH) return SVB+off; if(off-SV_LOW0<SV_LOWN) return svLow+(off-SV_LOW0); svBankTo((int)(off>>16)); return SVB+(off&0xFFFF); }
static u8 svRd(u32 off){ if(svType==SV_FLASH&&off-SV_LOW0<SV_LOWN) return svLow[off-SV_LOW0]; return svChip(off); }
static void svBk(u32 off,int v){ if(off==0x2AAA) SVB[SV_BK+2]=(u8)v; else if(off==0x5555) SVB[SV_BK+3]=(u8)v; }
static int svWr(u32 off,int v){   // 1 = written. Flash: off must be erased, or v may only clear bits
    v&=255;
    if(svType!=SV_FLASH){ if(off<32768){ SVB[off]=(u8)v; svBk(off,v); } return 1; }
    if(off-SV_LOW0<SV_LOWN){ svLow[off-SV_LOW0]=(u8)v; return 1; }
    if(off>=svSize){ svErr=1; return 0; }
    svBankTo((int)(off>>16)); int a=(int)(off&0xFFFF), c=SVB[a];
    if(c==v) return 1;
    if((c&v)!=v||!flProgRaw(a,v)){ svErr=1; return 0; }
    return 1;
}
static int svEraseSec(u32 off){ svBankTo((int)(off>>16)); if(!flEraseRaw((int)(off&0xF000))){ svErr=1; return 0; } return 1; }
// Make off..off+len-1 read 0xFF. Whatever else shares a sector with it is kept (copied to the scratch sector and back).
static void svErase(u32 off,u32 len){
    if(svType!=SV_FLASH){ for(u32 i=0;i<len&&off+i<32768;i++) svWr(off+i,0xFF); return; }
    u32 end=off+len;
    for(u32 s=off&~(SV_SEC-1);s<end;s+=SV_SEC){
        u32 a=off>s?off:s, b=end<s+SV_SEC?end:s+SV_SEC; int dirty=0, keep=0;
        for(u32 i=a;i<b;i++) if(svChip(i)!=0xFF){ dirty=1; break; }
        if(!dirty) continue;                                   // already blank
        for(u32 i=s;i<s+SV_SEC;i++) if((i<a||i>=b)&&svChip(i)!=0xFF){ keep=1; break; }
        if(keep){ svEraseSec(svScr); for(u32 i=0;i<SV_SEC;i++){ u32 p=s+i; if(p>=a&&p<b) continue; int v=svChip(p); if(v!=0xFF) svWr(svScr+i,v); } }
        svEraseSec(s);
        if(keep) for(u32 i=0;i<SV_SEC;i++){ u32 p=s+i; if(p>=a&&p<b) continue; int v=svChip(svScr+i); if(v!=0xFF) svWr(p,v); }
    }
}
static void svShadLoad(void){ int ob=svBank; svBankTo(0); u8*d=(u8*)svShad; for(u32 i=0;i<SV_LOWN;i++) d[i]=SVB[SV_LOW0+i]; svBankTo(ob); }
// Write the RAM copy of sector 1 back if it differs (programs only when no bit has to go back to 1, else erase + program)
static void svCommit(void){
    if(svType!=SV_FLASH) return;
    int ob=svBank, diff=0, rise=0; svBankTo(0);
    for(u32 i=0;i<SV_LOWN;i++){ u8 c=SVB[SV_LOW0+i], v=svLow[i]; if(c!=v){ diff=1; if((c&v)!=v){ rise=1; break; } } }
    if(diff){
        if(rise&&!flEraseRaw((int)SV_LOW0)) svErr=1;
        for(u32 i=0;i<SV_LOWN;i++){ u8 v=svLow[i]; if(SVB[SV_LOW0+i]!=v&&!flProgRaw((int)(SV_LOW0+i),v)){ svErr=1; break; } }
    }
    svBankTo(ob); svScan=0; svShadLoad();
}
// A commit stops the game for a few frames (a sector erase and up to 4096 byte writes: longer on a real cart than in an emulator), so svTick never
// commits right after a button press: it waits for a lull (SV_QUIET frames with no new press; keyRaw sets svQuiet), or SV_LATE frames at most.
#define SV_QUIET 20
#define SV_LATE  600
static u8 svQuiet; static u16 svLate;
static void svTick(void){   // from vsync: compare 128 bytes of the RAM copy with the chip, commit on a difference
    if(svType!=SV_FLASH) return;
    if(svQuiet) svQuiet--;
    const u32*l=(const u32*)(svLow+svScan), *c=svShad+svScan/4; int d=0;   // (the chip's copy: the same answer as reading the chip, 32 words of RAM instead of 128 flash reads)
    for(int i=0;i<32;i++) if(c[i]!=l[i]){ d=1; break; }
    if(!d){ svScan=(u16)((svScan+128)&(SV_LOWN-1)); return; }
    if(svQuiet&&++svLate<SV_LATE) return;   // (someone is pressing buttons: later, the same slice is looked at again)
    svLate=0; svCommit();
}
static void svEraseAll(void){   // ERASE EVERYTHING: the whole chip (flash) or 32 KB of zeros (SRAM)
    if(svType!=SV_FLASH){ for(u32 i=0;i<32768;i++) SVB[i]=0; SVB[SV_BK]='S'; SVB[SV_BK+1]='K'; return; }
    flCmd(0x80); flCmd(0x10); svBank=-1; svBankTo(0); if(!flWait(0,0xFF,8000000)) svErr=1; svShadLoad();
    for(u32 i=0;i<SV_LOWN;i++) svLow[i]=0xFF;
}
static u32 svSlotEnd(void){ return svType==SV_FLASH?svScr:32768; }   // the slots run up to here

static void svInit(void){
#ifdef SV_FORCE_SRAM
    if(SVB[0x7FFF]|1){ svType=SV_SRAM; svSize=32768; svBase=SVB; return; }   // (test build: read first, so an emulator picks plain SRAM)
#endif
    static const u8 ids[][3]={ {0xC2,0x09,2},{0x62,0x13,2},{0xC2,0x1C,1},{0x32,0x1B,1},{0xBF,0xD4,1} };   // maker, device, 1 = 64 KB 2 = 128 KB
    int kind=0;
    for(int pass=0;pass<2&&kind!=2;pass++){
        flCmd(0x90); svWaitFrames(2); u8 m=SVB[0], d=SVB[1];
        flCmd(0xF0); SVB[0x5555]=0xF0; svWaitFrames(2);
        kind=0; for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);i++) if(ids[i][0]==m&&ids[i][1]==d) kind=ids[i][2];
        if(kind!=1) break;
        flBankRaw(1); flBankRaw(0); flCmd(0xF0); svWaitFrames(1);   // a 64 KB answer: try a switch to bank 1 and back (mGBA then gives the full 128 KB)
    }
    if(!kind){   // SRAM: put back the two bytes the question wrote over
        svType=SV_SRAM; svSize=32768; svBase=SVB;
        if(SVB[SV_BK]=='S'&&SVB[SV_BK+1]=='K'){ SVB[0x2AAA]=SVB[SV_BK+2]; SVB[0x5555]=SVB[SV_BK+3]; }
        else { SVB[SV_BK]='S'; SVB[SV_BK+1]='K'; SVB[SV_BK+2]=SVB[0x2AAA]; SVB[SV_BK+3]=SVB[0x5555]; }
        return;
    }
    svType=SV_FLASH; svSize=kind==2?131072u:65536u; svScr=svSize-SV_SEC;
    svBank=-1; if(svSize>65536) svBankTo(0); else svBank=0;
    for(u32 i=0;i<SV_LOWN;i++) svLow[i]=SVB[SV_LOW0+i];
    svShadLoad();
    svBase=svLow-SV_LOW0;
}
static const char* svName(void){ return svType!=SV_FLASH?"32 KB SRAM":svSize>65536?"128 KB FLASH":"64 KB FLASH"; }

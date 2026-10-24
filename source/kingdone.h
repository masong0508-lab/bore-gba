// kingdone.h - EP6's own "missions done" block. The shared block (EXD_OFF, expand.h) holds 64 bits and the older packs use 0 to 59, so EP6
// keeps its own: 3 bytes in SRAM after the game totals (LG_OFF block ends at 7606) and before the life stats (LS_OFF 7610).
//   KD_OFF+0 done bits 0..7   KD_OFF+1 done bits 8..15   KD_OFF+2 check byte: 0x4B ^ lo ^ (3*hi)   (blank 0x00 and erased 0xFF read as invalid)
// A damaged block reads as nothing done; the next kdSet starts it again. Needs before it: u8, SRAM_BASE.
#define KD_OFF 7607
#define KD_LEN 3
#define KD_MAX 16
static u8 kdSumOf(u8 lo,u8 hi){ return (u8)(0x4B^lo^(u8)(hi*3)); }
static int kdBitsOf(volatile const u8*m){ if(m[2]!=kdSumOf(m[0],m[1])) return 0; return m[0]|(m[1]<<8); }
static int kdGet(int b){ if(b<0||b>=KD_MAX) return 0; return (kdBitsOf(SRAM_BASE+KD_OFF)>>b)&1; }
static void kdSet(int b){ if(b<0||b>=KD_MAX) return; volatile u8*m=SRAM_BASE+KD_OFF; int v=kdBitsOf(m)|(1<<b);
    u8 lo=(u8)v, hi=(u8)(v>>8); m[0]=lo; m[1]=hi; m[2]=kdSumOf(lo,hi); }

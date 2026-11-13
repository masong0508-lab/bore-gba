// the ARM7 program for the no-libnds build: it plays sound for the ARM9 (commands arrive in a ring in main RAM) and reads the stylus and the X / Y buttons for it. No libnds, no BIOS calls.
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef volatile unsigned short vu16; typedef volatile unsigned int vu32;
typedef struct { u32 op, ch, a, b, c, d; } HwCmd;
typedef struct { volatile u32 head, tail, busy, keys, pen, px, py, alive, svType, svBusy, svOK, svSize; HwCmd q[64]; } HwIpc;
#define IPC ((HwIpc*)0x023F0000)
#define R16(a) (*(vu16*)(a))
#define R32(a) (*(vu32*)(a))
#define CH_CNT(c) R32(0x04000400+(c)*16)
#define CH_SAD(c) R32(0x04000404+(c)*16)
#define CH_TMR(c) R16(0x04000408+(c)*16)
#define CH_PNT(c) R16(0x0400040A+(c)*16)
#define CH_LEN(c) R32(0x0400040C+(c)*16)
#define SPICNT R16(0x040001C0)
#define SPIDAT R16(0x040001C2)
static u32 kind[16];
static void spiWait(void){ while(SPICNT&0x80){} }
#include "touchfilter.h"
static TouchCal tcal; static TouchState tst;
static u8 fwByte(int last){ SPICNT=(u16)(last?0x8100:0x8900); SPIDAT=0; spiWait(); return (u8)SPIDAT; }
static void fwRead(u32 addr,u8*buf,int n){ SPICNT=0x8900; SPIDAT=3; spiWait(); SPIDAT=(u8)(addr>>16); spiWait(); SPIDAT=(u8)(addr>>8); spiWait(); SPIDAT=(u8)addr; spiWait(); for(int i=0;i<n;i++) buf[i]=fwByte(i==n-1); }
static void tcInit(void){      // the calibration stored in the firmware's user settings: two copies, 0x100 bytes apart; the one with the higher update counter (and sane numbers) wins
    u8 hdr[2]; fwRead(0x20,hdr,2); u32 base=((u32)(hdr[0]|(hdr[1]<<8)))*8; if(base<0x3F000||base>0x3FF00) base=0x3FE00; u8 b0[0x100], b1[0x100]; fwRead(base,b0,0x100); fwRead(base+0x100,b1,0x100);
    TouchCal c0, c1; tcLoad(&c0,b0); tcLoad(&c1,b1); int n0=b0[0x70]|(b0[0x71]<<8), n1=b1[0x70]|(b1[0x71]<<8); tcal=(c0.ok&&(!c1.ok||((n0-n1)&0x7F)<0x40))?c0:c1; }
static u16 tsc(u32 cmd){   // one 12-bit conversion from the touch controller
    SPICNT=0x8A01; SPIDAT=cmd; spiWait(); SPIDAT=0; spiWait(); u16 r1=SPIDAT; SPICNT=0x8201; SPIDAT=0; spiWait(); u16 r2=SPIDAT; return (u16)(((r1&0x7F)<<5)|(r2>>3)); }
// ---- the cartridge's save memory (SPI): flash (3 byte addresses, 256 byte pages) or EEPROM (2 byte addresses, 128 byte pages)
#define AUXCNT R16(0x040001A0)
#define AUXDAT R16(0x040001A2)
static u8 aux(u8 b,int hold){ AUXCNT=(u16)(0xA000|(hold?0x40:0)); AUXDAT=b; u32 t=200000; while((AUXCNT&0x80)&&--t){} return (u8)AUXDAT; }
static int svWait(void){ for(u32 t=0;t<2000000u;t++){ aux(0x05,1); u8 st=aux(0,0); if(!(st&1)) return 1; } return 0; }
static void svAddr(u32 off,int fl){ if(fl) aux((u8)(off>>16),1); aux((u8)(off>>8),1); aux((u8)off,1); }
static void svRd(u32 off,u8*mem,u32 len,int fl){ aux(0x03,1); svAddr(off,fl); for(u32 i=0;i<len;i++) mem[i]=aux(0,i+1<len); }
static int svWr(u32 off,const u8*mem,u32 len,int fl){ u32 page=fl?256:128; for(u32 done=0;done<len;){ u32 n=len-done; u32 room=page-((off+done)%page); if(n>room) n=room; aux(0x06,0); aux(fl?0x0A:0x02,1); svAddr(off+done,fl); for(u32 i=0;i<n;i++) aux(mem[done+i],i+1<n); if(!svWait()) return 0; done+=n; } return 1; }
static int svUniform(const u8*a){ for(int i=1;i<16;i++) if(a[i]!=a[0]) return 0; return 1; }
static int svSame(const u8*a,const u8*b){ for(int i=0;i<16;i++) if(a[i]!=b[i]) return 0; return 1; }
static u32 svProbe(int fl){      // how big is the save memory? 0 = no idea
    if(fl){ aux(0x9F,1); aux(0,1); aux(0,1); u8 id2=aux(0,0); return (id2>=0x0F&&id2<=0x18)?(1u<<id2):0; }      // flash: the JEDEC capacity code is the power of two
    u8 a[16], b[16], pat[16]={0xC3,0x3C,0xA5,0x5A,0x96,0x69,0xE1,0x1E,0xD2,0x2D,0xB4,0x4B,0x87,0x78,0xF0,0x0F};       // EEPROM: 8 KB or 64 KB (16-bit addresses): does 0x2000 read back what is at 0?
    svRd(0,a,16,0); svRd(0x2000,b,16,0); if(!svUniform(a)) return svSame(a,b)?8192:65536;
    if(!svWr(0,pat,16,0)) return 0; svRd(0x2000,b,16,0); u32 cap=svSame(b,pat)?8192:65536; svWr(0,a,16,0); return cap; }      // (an empty chip: write a pattern, look, put it back)
static void svDetect(void){ aux(0x9F,1); u8 id0=aux(0,1); aux(0,1); aux(0,0); if(id0!=0x00&&id0!=0xFF){ IPC->svSize=svProbe(1); IPC->svType=1; return; }      // (the size first: the ARM9 reads it as soon as it sees the type)
    aux(0x05,1); u8 st=aux(0,0); if(st==0xFF){ IPC->svSize=0; IPC->svType=0; return; } IPC->svSize=svProbe(0); IPC->svType=2; }
static void svOp(HwCmd*c){ int fl=IPC->svType==1; u8*mem=(u8*)c->a; u32 len=c->b, off=c->c; IPC->svOK=1;
    if(!IPC->svType){ IPC->svOK=0; return; }
    if(c->op==9){ svRd(off,mem,len,fl); return; }
    if(!svWr(off,mem,len,fl)) IPC->svOK=0; }
static void doCmd(HwCmd*c){
    if(c->op==8||c->op==9){ svOp(c); IPC->svBusy=0; return; }
    u32 ch=c->ch&15, vol=c->d&0x7F, pan=(c->d>>8)&0x7F;
    switch(c->op){
    case 1: { u32 loop=(c->d>>16)&1; CH_CNT(ch)=0; kind[ch]=1; CH_SAD(ch)=c->a; CH_TMR(ch)=(u16)(-(0x1000000/(c->c?c->c:1))); CH_PNT(ch)=0; CH_LEN(ch)=c->b>>2; CH_CNT(ch)=0x80000000u|(loop?(1u<<27):(2u<<27))|vol|(pan<<16); } break;
    case 2: CH_CNT(ch)=0; kind[ch]=2; CH_TMR(ch)=(u16)(-(0x1000000/((c->c?c->c:1)*8))); CH_CNT(ch)=0x80000000u|(3u<<29)|(3u<<24)|vol|(pan<<16); break;
    case 3: CH_CNT(ch)=0; kind[ch]=3; CH_TMR(ch)=(u16)(-(0x1000000/(c->c?c->c:1))); CH_CNT(ch)=0x80000000u|(3u<<29)|vol|(pan<<16); break;
    case 4: CH_CNT(ch)=0; break;
    case 5: { u32 f=c->c?c->c:1; if(kind[ch]==2) f*=8; CH_TMR(ch)=(u16)(-(0x1000000/f)); } break;
    case 6: CH_CNT(ch)=(CH_CNT(ch)&~0x7Fu)|(c->c&0x7F); break;
    case 7: CH_CNT(ch)=(CH_CNT(ch)&~(0x7Fu<<16))|((c->c&0x7F)<<16); break;
    } }
void arm7main(void){
    R16(0x04000304)=1;                       // sound on (POWCNT2)
    R16(0x04000500)=0x8000|0x7F;             // sound master enable, full volume
    tcInit();
    int done=0, det=0;
    for(;;){
        if(IPC->alive==0x41524D39&&!det){ det=1; svDetect(); }
        if(IPC->alive==0x41524D39){ while(IPC->tail!=IPC->head){ doCmd(&IPC->q[IPC->tail]); IPC->tail=(IPC->tail+1)&63; } }
        u32 b=0; for(int i=0;i<16;i++) if(CH_CNT(i)&0x80000000u) b|=1u<<i; IPC->busy=b;
        u16 vc=R16(0x04000006);
        if(vc==192){ if(!done){ done=1; u16 ext=R16(0x04000136); IPC->keys=(u32)(((~ext)&3));
                int xs[5], ys[5], phys=!(ext&0x40); if(phys){ for(int k=0;k<5;k++){ xs[k]=tsc(0xD0); ys[k]=tsc(0x90); } } int px=0, py=0; int pen=tcFrame(&tst,&tcal,phys,xs,ys,5,&px,&py); if(pen){ IPC->px=(u32)px; IPC->py=(u32)py; } IPC->pen=(u32)pen; } }
        else done=0;
    } }

// the ARM7 program for the no-libnds build: it plays sound for the ARM9 (commands arrive in a ring in main RAM) and reads the stylus and the X / Y buttons for it. No libnds, no BIOS calls.
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef volatile unsigned short vu16; typedef volatile unsigned int vu32;
typedef struct { u32 op, ch, a, b, c, d; } HwCmd;
typedef struct { volatile u32 head, tail, busy, keys, pen, px, py, alive; HwCmd q[64]; } HwIpc;
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
static u16 tsc(u32 cmd){   // one 12-bit conversion from the touch controller
    SPICNT=0x8A01; SPIDAT=cmd; spiWait(); SPIDAT=0; spiWait(); u16 r1=SPIDAT; SPICNT=0x8201; SPIDAT=0; spiWait(); u16 r2=SPIDAT; return (u16)(((r1&0x7F)<<5)|(r2>>3)); }
static void doCmd(HwCmd*c){
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
    int done=0;
    for(;;){
        if(IPC->alive==0x41524D39){ while(IPC->tail!=IPC->head){ doCmd(&IPC->q[IPC->tail]); IPC->tail=(IPC->tail+1)&63; } }
        u32 b=0; for(int i=0;i<16;i++) if(CH_CNT(i)&0x80000000u) b|=1u<<i; IPC->busy=b;
        u16 vc=R16(0x04000006);
        if(vc==192){ if(!done){ done=1; u16 ext=R16(0x04000136); IPC->keys=(u32)(((~ext)&3));
                if(!(ext&0x40)){ u32 x=tsc(0xD0), y=tsc(0x90); if(x&&y&&x<4096){ IPC->px=(x>>4)>255?255:(x>>4); u32 py=y*192/4096; IPC->py=py>191?191:py; IPC->pen=1; } else IPC->pen=0; } else IPC->pen=0; } }
        else done=0;
    } }

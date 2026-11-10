// the ARM7 program for the no-libnds build: it plays sound for the ARM9 (commands arrive in a ring in main RAM) and reads the stylus and the X / Y buttons for it. No libnds, no BIOS calls.
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef volatile unsigned short vu16; typedef volatile unsigned int vu32;
typedef struct { u32 op, ch, a, b, c, d; } HwCmd;
typedef struct { volatile u32 head, tail, busy, keys, pen, px, py, alive, svType, svBusy, svOK; HwCmd q[64]; volatile u32 seq, tapX, tapY, rawX, rawY, tapRawX, tapRawY, svSize; } HwIpc;
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
// ---- the cartridge's save memory (SPI): flash (3 byte addresses, 256 byte pages) or EEPROM (2 byte addresses, 128 byte pages)
#define AUXCNT R16(0x040001A0)
#define AUXDAT R16(0x040001A2)
static u8 aux(u8 b,int hold){ AUXCNT=(u16)(0xA000|(hold?0x40:0)); AUXDAT=b; u32 t=200000; while((AUXCNT&0x80)&&--t){} return (u8)AUXDAT; }
static int svWait(void){ for(u32 t=0;t<2000000u;t++){ aux(0x05,1); u8 st=aux(0,0); if(!(st&1)) return 1; } return 0; }
static void svAddr(u32 off,int fl){ if(fl) aux((u8)(off>>16),1); aux((u8)(off>>8),1); aux((u8)off,1); }
static void svDetect(void){ aux(0x9F,1); u8 id0=aux(0,1); aux(0,1); u8 id2=aux(0,0); if(id0!=0x00&&id0!=0xFF){ IPC->svType=1; IPC->svSize=(id2>=0x10&&id2<=0x17)?(1u<<id2):0; return; }
    aux(0x05,1); u8 st=aux(0,0); IPC->svType=(st==0xFF)?0:2; }
static void svOp(HwCmd*c){ int fl=IPC->svType==1; u8*mem=(u8*)c->a; u32 len=c->b, off=c->c, page=fl?256:128; IPC->svOK=1;
    if(!IPC->svType){ IPC->svOK=0; return; }
    if(c->op==9){ aux(0x03,1); svAddr(off,fl); for(u32 i=0;i<len;i++) mem[i]=aux(0,i+1<len); return; }
    for(u32 done=0;done<len;){ u32 n=len-done; u32 room=page-((off+done)%page); if(n>room) n=room; aux(0x06,0); aux(fl?0x0A:0x02,1); svAddr(off+done,fl); for(u32 i=0;i<n;i++) aux(mem[done+i],i+1<n); if(!svWait()){ IPC->svOK=0; return; } done+=n; } }
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
// ---- the stylus. Sampled twice a frame (so a very short tap is not missed), five conversions each time with the middle three averaged, a noisy sample keeps the last good position
// instead of dropping the pen, a release must be seen twice, and the firmware's own calibration is used when it looks sane (otherwise the plain 12-bit scaling).
static int calOk, calTry; static int cAx1, cAy1, cSx1, cSy1, cAx2, cAy2, cSx2, cSy2;
static int ab(int v){ return v<0?-v:v; }
static void calInit(void){   // the user settings the firmware leaves in main RAM: ADC x1 y1, screen x1 y1, ADC x2 y2, screen x2 y2
    volatile u8*u=(volatile u8*)0x027FFC80; calTry++; cAx1=u[0x58]|(u[0x59]<<8); cAy1=u[0x5A]|(u[0x5B]<<8); cSx1=u[0x5C]; cSy1=u[0x5D]; cAx2=u[0x5E]|(u[0x5F]<<8); cAy2=u[0x60]|(u[0x61]<<8); cSx2=u[0x62]; cSy2=u[0x63];
    calOk=0; if(cAx2<cAx1+0x200||cAy2<cAy1+0x200||cSx2<cSx1+100||cSy2<cSy1+80||cAx2>4095||cAy2>4095) return;
    if(ab((cAx1>>4)-cSx1)>20||ab((cAx2>>4)-cSx2)>20||ab(cAy1*192/4096-cSy1)>20||ab(cAy2*192/4096-cSy2)>20) return;   // far from the plain mapping: do not trust it
    calOk=1; }
static void sort5(u32*a){ for(int i=1;i<5;i++){ u32 v=a[i]; int j=i-1; while(j>=0&&a[j]>v){ a[j+1]=a[j]; j--; } a[j+1]=v; } }
static u32 gRawX, gRawY;
static int penRead(u32*ox,u32*oy){   // 1 = a good position, 0 = the pen is up, -1 = touching but noisy
    if(R16(0x04000136)&0x40) return 0;
    u32 xs[5], ys[5]; tsc(0xD0); tsc(0x90);   // the first pair settles the converter
    for(int i=0;i<5;i++){ xs[i]=tsc(0xD0); ys[i]=tsc(0x90); }
    if(R16(0x04000136)&0x40) return 0;       // lifted while reading
    sort5(xs); sort5(ys);
    if(xs[3]-xs[1]>64||ys[3]-ys[1]>64) return -1;   // the samples disagree by more than about 4 pixels
    u32 rx=(xs[1]+xs[2]+xs[3])/3, ry=(ys[1]+ys[2]+ys[3])/3; if(rx==0||ry==0||rx>=4095||ry>=4095) return -1;
    gRawX=rx; gRawY=ry; int px, py;
    if(calOk){ px=cSx1+((int)rx-cAx1)*(cSx2-cSx1)/(cAx2-cAx1); py=cSy1+((int)ry-cAy1)*(cSy2-cSy1)/(cAy2-cAy1); }
    else { px=(int)(rx>>4); py=(int)(ry*192/4096); }
    if(px<0) px=0; if(px>255) px=255; if(py<0) py=0; if(py>191) py=191; *ox=(u32)px; *oy=(u32)py; return 1; }
static int penUp, penBad, penPend; static u32 pdX, pdY, pdRX, pdRY;
static void penTick(void){
    if(!calOk&&calTry<8) calInit();
    u32 x=0, y=0; int r=penRead(&x,&y);
    // STYLUS v2: the first sample of a contact bounces (the pressure is still rising), and the tap is decided on it. A new contact is now held back until two samples agree
    // (within 3 pixels), at most four samples (two frames); a tap that is over before that is delivered from the one sample there is, so no tap is lost.
    if(r>0){ penUp=0; penBad=0;
        if(!IPC->pen){ if(!((penPend&&ab((int)x-(int)pdX)<=3&&ab((int)y-(int)pdY)<=3)||penPend>=3)){ penPend++; pdX=x; pdY=y; pdRX=gRawX; pdRY=gRawY; return; }
            penPend=0; IPC->tapX=x; IPC->tapY=y; IPC->tapRawX=gRawX; IPC->tapRawY=gRawY; IPC->seq++; }
        IPC->px=x; IPC->py=y; IPC->rawX=gRawX; IPC->rawY=gRawY; IPC->pen=1; }
    else if(r<0){ if(IPC->pen&&++penBad>4) IPC->pen=0; }
    else { penBad=0;
        if(!IPC->pen&&penPend){ penPend=0; IPC->tapX=pdX; IPC->tapY=pdY; IPC->tapRawX=pdRX; IPC->tapRawY=pdRY; IPC->seq++; IPC->px=pdX; IPC->py=pdY; IPC->rawX=pdRX; IPC->rawY=pdRY; IPC->pen=1; penUp=0; return; }
        if(IPC->pen&&++penUp>=2) IPC->pen=0; if(!IPC->pen) penUp=0; } }
void arm7main(void){
    R16(0x04000304)=1;                       // sound on (POWCNT2)
    R16(0x04000500)=0x8000|0x7F;             // sound master enable, full volume
    int done=0, mid=0, det=0; calInit();
    for(;;){
        if(IPC->alive==0x41524D39&&!det){ det=1; svDetect(); }
        if(IPC->alive==0x41524D39){ while(IPC->tail!=IPC->head){ doCmd(&IPC->q[IPC->tail]); IPC->tail=(IPC->tail+1)&63; } }
        u32 b=0; for(int i=0;i<16;i++) if(CH_CNT(i)&0x80000000u) b|=1u<<i; IPC->busy=b;
        u16 vc=R16(0x04000006);
        if(vc>=192){ mid=0; if(!done){ done=1; u16 ext=R16(0x04000136); IPC->keys=(u32)(((~ext)&3)); penTick(); } }
        else { done=0; if(vc>=96&&!mid){ mid=1; penTick(); } }
    } }

// ui.h - Bore: GodGuy's interface, drawn by the game itself: the top screen is one 256 x 192 bitmap held in RAM (never written to VRAM a byte at a time: the DS ignores byte writes to VRAM)
// and copied over 16 bits at a time; the toolbar along the bottom of the touch screen is a second tile layer built the same way. Everything is made of a few primitives:
// rectangles, bevelled frames, a 5 x 7 font, bars, 10 x 10 power icons and 8 x 8 stat icons.
#ifndef UI_H
#define UI_H
#include "font5x7.h"
#include "icons.h"
enum { U_CLEAR, U_BG, U_PANEL, U_PANELHI, U_BHI, U_BLO, U_WHITE, U_DIM, U_GOLD, U_GREEN, U_RED, U_BLUE, U_PURPLE, U_ORANGE, U_SEL, U_SHADOW, U_G0, U_G1, U_G2, U_G3, U_G4, U_G5, U_G6, U_G7, U_YELLOW, U_CYAN, U_BROWN, U_LGREY, U_DGREY, U_PINK, U_TEAL, U_N };
static const u8 uiRgb[U_N][3]={ {0,0,0},{2,3,8},{4,6,13},{7,10,19},{16,19,28},{2,3,7},{31,31,31},{17,19,25},{31,26,6},{8,26,10},{29,6,6},{9,17,31},{20,9,28},{31,17,4},{9,14,27},{0,0,2},
    {31,30,12},{31,28,9},{31,25,7},{31,22,5},{30,18,4},{28,14,3},{25,10,3},{20,6,3},{31,31,8},{8,29,31},{14,8,3},{23,24,27},{10,11,14},{31,18,24},{6,22,20} };
#define UI_PAL0 160           // the toolbar's colours live at 160.. of the main palette (the day / night light only touches 0..143)
static u8 uiBuf[256*192] __attribute__((aligned(4))); static int uiPages, uiPageNow, uiPageOpen, uiDotX=96;   // page dots in the title bar
static const u8 uiKCol[8][3]={{31,8,8},{8,16,31},{10,28,10},{31,26,6},{24,10,30},{31,16,4},{6,28,28},{31,14,22}};   // kingdom colours (sub palette 40..47)
static void uiPalette(void){ for(int k=0;k<8;k++) BG_PALETTE_SUB[40+k]=RGB15(uiKCol[k][0],uiKCol[k][1],uiKCol[k][2]); for(int i=0;i<U_N;i++){ u16 c=RGB15(uiRgb[i][0],uiRgb[i][1],uiRgb[i][2]); BG_PALETTE_SUB[i]=c; BG_PALETTE[UI_PAL0+i]=c; } BG_PALETTE_SUB[0]=0; }
// the screen is copied to VRAM only when it differs from what is already there: a menu that is just sitting there costs nothing and cannot flicker or tear
static u8 uiShown[256*192]; static int uiShownOk;
static int uiSame(void){ static int n; if(++n>=90){ n=0; uiShownOk=0; }      // (and a refresh every 1.5 s anyway, in case something else wrote to that screen)
    if(!uiShownOk) return 0; return !memcmp(uiShown,uiBuf,sizeof uiShown); }
#ifdef NDSHW
static void uiFlushRaw(void){ if(uiSame()) return; hwSubFlush(uiBuf); memcpy(uiShown,uiBuf,sizeof uiShown); uiShownOk=1; }
#else
static void uiFlushRaw(void){ if(uiSame()) return; vramCopy(bgGetGfxPtr(3),uiBuf,256*192); memcpy(uiShown,uiBuf,sizeof uiShown); uiShownOk=1; }
#endif
#ifndef UI_NOTRANS
#define UI_NOTRANS 0
#endif
// ---- screen transitions: when the screen changes, the old picture slides out and the new one in (dir 1 forward, -1 back) or dissolves (dir 2)
static u8 uiPrev[256*192], uiNew[256*192]; static int uiTrans, uiHavePrev;
static int uiPending;
static void uiCommit(void){ if(uiPending){ uiPending=0; uiFlushRaw(); } }
static void uiFlush(void){
    if(uiTrans&&uiHavePrev&&!UI_NOTRANS){ int dir=uiTrans; uiTrans=0; memcpy(uiNew,uiBuf,sizeof uiNew);
        for(int f=1;f<=7;f++){ int p=f*256/7, sh=256-(256-p)*(256-p)/256;
            if(dir==2){ static const u8 bay[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5}; int th=f*16/7; for(int y=0;y<192;y++) for(int x=0;x<256;x++) uiBuf[y*256+x]=bay[(y&3)*4+(x&3)]<th?uiNew[y*256+x]:uiPrev[y*256+x]; }
            else for(int y=0;y<192;y++){ u8*o=&uiBuf[y*256], *a=&uiPrev[y*256], *n=&uiNew[y*256];
                if(dir>0){ for(int x=0;x<256;x++) o[x]=x<256-sh?a[x+sh]:n[x-(256-sh)]; } else { for(int x=0;x<256;x++) o[x]=x>=sh?a[x-sh]:n[x+(256-sh)]; } }
            swiWaitForVBlank(); uiFlushRaw(); }      // (wait first, then copy: the copy happens in the blank)
        memcpy(uiBuf,uiNew,sizeof uiNew); }
    uiPending=1; memcpy(uiPrev,uiBuf,sizeof uiPrev); uiHavePrev=1; }      // the screen is copied to VRAM at the next vertical blank (uiCommit), not now, in the middle of the picture
// ---- touch buttons: each drawn control registers a rectangle; a tap on it sets *sp=sv (if given) and presses key (key2 instead when it was already selected)
typedef struct { short x,y,w,h; unsigned short key,key2; int*sp; int sv; } UiBtn;
static UiBtn uiBtns[64]; static int uiNBtn;
static void uiBtn(int x,int y,int w,int h,int key,int key2,int*sp,int sv){ if(uiNBtn<64){ UiBtn*b=&uiBtns[uiNBtn++]; b->x=(short)x; b->y=(short)y; b->w=(short)w; b->h=(short)h; b->key=(unsigned short)key; b->key2=(unsigned short)key2; b->sp=sp; b->sv=sv; } }
// a tap inside a button hits it (the one drawn last wins); a tap that just misses (up to 4 pixels) takes the nearest button instead of being lost: fingers and styluses are bigger than the arrows
static int uiHit(int px,int py){ for(int i=uiNBtn-1;i>=0;i--){ const UiBtn*b=&uiBtns[i]; if(px>=b->x&&px<b->x+b->w&&py>=b->y&&py<b->y+b->h) return i; }
    int best=-1, bd=5*5+1; for(int i=uiNBtn-1;i>=0;i--){ const UiBtn*b=&uiBtns[i]; int dx=px<b->x?b->x-px:px>=b->x+b->w?px-(b->x+b->w-1):0, dy=py<b->y?b->y-py:py>=b->y+b->h?py-(b->y+b->h-1):0; int d=dx*dx+dy*dy; if(d<bd){ bd=d; best=i; } }
    return best; }
static inline void uiPx(int x,int y,int c){ if((unsigned)x<256&&(unsigned)y<192) uiBuf[y*256+x]=(u8)c; }
static void uiRect(int x,int y,int w,int h,int c){ for(int j=0;j<h;j++) for(int i=0;i<w;i++) uiPx(x+i,y+j,c); }
static void uiFrame(int x,int y,int w,int h,int fill,int hi,int lo){ uiRect(x,y,w,h,fill); for(int i=0;i<w;i++){ uiPx(x+i,y,hi); uiPx(x+i,y+h-1,lo); } for(int j=0;j<h;j++){ uiPx(x,y+j,hi); uiPx(x+w-1,y+j,lo); } }
static void uiRound(int x,int y,int w,int h,int fill,int edge){ uiRect(x+1,y,w-2,h,fill); uiRect(x,y+1,w,h-2,fill); for(int i=1;i<w-1;i++){ uiPx(x+i,y,edge); uiPx(x+i,y+h-1,edge); } for(int j=1;j<h-1;j++){ uiPx(x,y+j,edge); uiPx(x+w-1,y+j,edge); } }
static int uiChar(int x,int y,int ch,int col,int sc){ if(ch>='a'&&ch<='z') ch-=32; if(ch<32||ch>=96) ch=32; const unsigned char*g=font5x7[ch-32];
    for(int r=0;r<7;r++) for(int c=0;c<5;c++) if((g[r]>>(4-c))&1) for(int a=0;a<sc;a++) for(int b=0;b<sc;b++) uiPx(x+c*sc+a,y+r*sc+b,col); return 6*sc; }
static int uiTextW(const char*s,int sc){ int n=0; while(s[n]) n++; return n*6*sc; }
static int uiText(int x,int y,const char*s,int col,int sc){ int x0=x; while(*s){ x+=uiChar(x,y,(unsigned char)*s,col,sc); s++; } return x-x0; }
static void uiTextS(int x,int y,const char*s,int col,int sc){ uiText(x+sc,y+sc,s,U_SHADOW,sc); uiText(x,y,s,col,sc); }
static void uiTextR(int xr,int y,const char*s,int col){ uiText(xr-uiTextW(s,1),y,s,col,1); }
static void uiTextC(int xc,int y,const char*s,int col,int sc){ uiText(xc-uiTextW(s,sc)/2,y,s,col,sc); }
static int uiWrap(int x,int y,int cols,int maxl,const char*s,int col){ int line=0, pos=0, n=(int)strlen(s); while(pos<n&&line<maxl){ int len=n-pos<cols?n-pos:cols; if(pos+len<n){ int k=len; while(k>0&&s[pos+k]!=' ') k--; if(k>0) len=k; } char b[64]; if(len>63) len=63; memcpy(b,s+pos,len); b[len]=0; uiText(x,y+line*9,b,col,1); pos+=len; while(s[pos]==' ') pos++; line++; } return line; }
static void uiBar(int x,int y,int w,int h,int val,int max,int col){ uiFrame(x,y,w,h,U_SHADOW,U_BLO,U_BHI); int f=max>0?(w-2)*(val>max?max:val)/max:0; if(f>0) uiRect(x+1,y+1,f,h-2,col); if(f>2) uiRect(x+1,y+1,f,1,U_WHITE); }
static void uiIcon10(int x,int y,int idx){ for(int j=0;j<10;j++) for(int i=0;i<10;i++){ int c=icon10[idx][j*10+i]; if(c) uiPx(x+i,y+j,c); } }
static void uiIcon8(int x,int y,int idx){ for(int j=0;j<8;j++) for(int i=0;i<8;i++){ int c=icon8[idx][j*8+i]; if(c) uiPx(x+i,y+j,c); } }
// ---- a power's icon: paints show the ground they paint, everything else has its own picture
static const u8 uiKindIcon[20]={IC10_INSPECT,0,IC10_SPAWN,IC10_RAIN,IC10_FIRE,IC10_BOLT,IC10_METEOR,IC10_SMITE,IC10_HEAL,IC10_FEED,IC10_SETTLE,IC10_WAR,IC10_PEACE,IC10_BLESS,IC10_GIFT,IC10_MUTATE,IC10_QUAKE,IC10_FLOOD,IC10_BLOOM,IC10_VIBE};
// ground swatches use three shades per terrain: 100.. in the top palette, 200.. in the toolbar's
static int uiSwatch(int t,int i,int j,int base){ int p=tilePix(t,0,(i*8)/10,(j*8)/10)%8; return base+t*3+(p<=2?0:p<=4?1:2); }
static void uiShadePal(void){ for(int t=0;t<T_N;t++) for(int j=0;j<3;j++){ int r,g,b; shadeRGB(t*8+2+j*2,&r,&g,&b); BG_PALETTE_SUB[100+t*3+j]=RGB15(r,g,b); BG_PALETTE[200+t*3+j]=RGB15(r,g,b); } }
static void uiPowerIcon(int pwi,int x,int y){ const Pw*P=&pw[pwi];
    if(P->kind==PK_PAINT){ for(int j=0;j<10;j++) for(int i=0;i<10;i++) uiPx(x+i,y+j,uiSwatch(P->arg,i,j,100)); return; }
    uiIcon10(x,y,uiKindIcon[P->kind]); }
// ---- window furniture
static int uiBackHint;   // set while an Extras page is open: START goes back to the menu, so the page hint says so
static const char* uiTabPrev, *uiTabNext;   // names of the neighbouring Extras pages (L and R switch between them)
static void uiWin(const char*title,const char*right,const char*hint){ if(uiBackHint) hint=0;
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=2) for(int x=((y>>1)&1)*2;x<256;x+=4) uiPx(x,y,U_PANEL);
    for(int j=0;j<13;j++){ int c=j<2?U_PANELHI:j<11?U_PANEL:U_SHADOW; uiRect(0,j,256,1,c); } uiRect(0,12,256,1,U_BHI);
    uiText(6,3,title,U_GOLD,1); if(right) uiTextR(250,3,right,U_DIM);
    if(uiPages>0){ char pg[8]; int n=0; for(int i=0;i<=uiPageNow;i++) if(uiPageOpen&(1<<i)) n++; int t=0; for(int i=0;i<uiPages;i++) if(uiPageOpen&(1<<i)) t++; pg[0]='0'+n; pg[1]='/'; pg[2]='0'+t; pg[3]=0; uiText(uiDotX,3,pg,U_LGREY,1); }
    uiRect(0,179,256,13,U_PANEL); uiRect(0,179,256,1,U_BHI); if(hint) uiText(6,183,hint,U_DIM,1);
    if(uiBackHint){ if(uiTabPrev){ uiRound(3,181,12,10,U_SEL,U_BHI); uiText(7,183,"L",U_GOLD,1); uiText(19,183,uiTabPrev,U_DIM,1); uiBtn(2,180,84,12,KEY_L,0,0,0); } uiRound(98,180,60,12,U_SEL,U_GOLD); uiTextC(128,183,"BACK",U_WHITE,1); uiBtn(96,179,64,13,KEY_START,0,0,0); if(uiTabNext){ uiRound(241,181,12,10,U_SEL,U_BHI); uiText(245,183,"R",U_GOLD,1); uiTextR(237,183,uiTabNext,U_DIM); uiBtn(170,180,84,12,KEY_R,0,0,0); }
        uiRound(240,147,13,13,U_SEL,U_BHI); uiIcon8(243,150,IC8_UP); uiBtn(236,145,19,17,KEY_UP,0,0,0); uiRound(240,163,13,13,U_SEL,U_BHI); uiIcon8(243,166,IC8_DOWN); uiBtn(236,162,19,16,KEY_DOWN,0,0,0); } }
static void uiListRow(int y,int sel,int n,const char*left,const char*right,int col){ if(sel) uiRect(4,y-1,240,10,U_SEL); else if(n&1) uiRect(4,y-1,240,10,U_PANEL);
    if(sel) uiText(6,y,">",U_GOLD,1); uiText(14,y,left,col,1); if(right&&right[0]) uiTextR(242,y,right,sel?U_WHITE:U_DIM); }
static void uiScroll(int total,int top,int vis,int y0,int h){ uiRect(248,y0,5,h,U_SHADOW); if(total>vis){ int bh=h*vis/total; if(bh<6) bh=6; int by=y0+(h-bh)*top/(total-vis); uiRect(248,by,5,bh,U_BHI); } }
static int uiVx, uiVy, uiVw=32, uiVh=24;      // the view's top left tile and size in tiles, for the minimap's frame
// ---- the minimap (the whole planet squeezed to 128 x 128) with creatures, villages and the camera
static void uiMinimap(int ox,int oy){
    for(int y=0;y<128;y++) for(int x=0;x<128;x++) uiPx(ox+x,oy+y,64+world[y*WH/128][x*WW/128]);
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(c->alive){ int da=arch[sp[c->sp].a].diet; if(da==5||da==4||c->rk) uiPx(ox+c->x*128/WW,oy+c->y*128/WH,da==4?80:da==5?85:82); } }
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]){ int cx=ox+vil[v].x*128/WW, cy=oy+vil[v].y*128/WH, R=14*128/WW+1; for(int a=0;a<28;a++){ int rx=(int)(R*cosf(a*0.2244f)+0.5f), ry=(int)(R*sinf(a*0.2244f)+0.5f); if((a&1)==0) uiPx(cx+rx,cy+ry,40+vilK[v]-1); } }   // each kingdom's territory, a dotted ring round its cities
    for(int a=0;a<KN_N;a++) if(kd[a].on&&kd[a].cap) for(int b=a+1;b<KN_N;b++) if(kd[b].on&&kd[b].cap&&(kd[a].tradeW>>b&1)){ const Vil*A=&vil[kd[a].cap-1], *B=&vil[kd[b].cap-1]; int x0=ox+A->x*128/WW, y0=oy+A->y*128/WH, x1=ox+B->x*128/WW, y1=oy+B->y*128/WH; int st=abs(x1-x0)>abs(y1-y0)?abs(x1-x0):abs(y1-y0); for(int i=0;i<=st;i++) if(((i+(tick>>3))&3)==0) uiPx(x0+(x1-x0)*i/(st?st:1),y0+(y1-y0)*i/(st?st:1),U_YELLOW); }   // trade routes: gold dots running between trading capitals
    for(int v=0;v<VIL_N;v++) if(vil[v].on){ for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++) uiPx(ox+vil[v].x*128/WW+dx,oy+vil[v].y*128/WH+dy,vil[v].foe?81:84); }
    int cx0=uiVx*128/WW, cy0=uiVy*128/WH, cw=uiVw*128/WW, ch=uiVh*128/WH; if(cx0<0) cx0=0; if(cy0<0) cy0=0; if(cx0+cw>127) cx0=127-cw; if(cy0+ch>127) cy0=127-ch; for(int x=cx0;x<cx0+cw;x++){ uiPx(ox+x,oy+cy0,83); uiPx(ox+x,oy+cy0+ch,83); } for(int y=cy0;y<=cy0+ch;y++){ uiPx(ox+cx0,oy+y,83); uiPx(ox+cx0+cw,oy+y,83); } }
// ---- the toolbar (bottom of the touch screen): ten power slots with arrows, a second tile layer
#define TB_SLOTS 10
#define TB_TILE0 704
#define TB_BLANK 1023
static u8 tbPix[24][256]; static int tbTop=0; static u16*mapPtr1;
static void tbDraw(void){
    for(int y=0;y<24;y++) for(int x=0;x<256;x++) tbPix[y][x]=0;
    #define TBP(x,y,c) do{ if((unsigned)(x)<256&&(unsigned)(y)<24) tbPix[y][x]=(u8)(c); }while(0)
    for(int y=0;y<24;y++) for(int x=0;x<256;x++) tbPix[y][x]=(u8)(UI_PAL0+(y==0?U_BHI:y==1?U_PANELHI:U_PANEL));
    if(power<tbTop) tbTop=power; if(power>=tbTop+TB_SLOTS) tbTop=power-TB_SLOTS+1; if(tbTop<0) tbTop=0; if(tbTop>PW_N-TB_SLOTS) tbTop=PW_N-TB_SLOTS;
    for(int s=0;s<TB_SLOTS;s++){ int pi=tbTop+s; int x0=12+s*23, y0=3, sel=pi==power;
        for(int j=0;j<19;j++) for(int i=0;i<21;i++){ int edge=(i==0||j==0||i==20||j==18); int c=sel?(edge?U_GOLD:U_SEL):(edge?U_BLO:U_SHADOW); TBP(x0+i,y0+j,UI_PAL0+c); }
        // icon (10 x 10, doubled would be too coarse: drawn once, centred)
        { int kk=pw[pi].kind, cc=kk==PK_PAINT?U_BROWN:(kk==PK_SPAWN||kk==PK_HEAL||kk==PK_FEED||kk==PK_BLOOM||kk==PK_VIBE)?U_GREEN:(kk==PK_RAIN||kk==PK_FIRE||kk==PK_BOLT||kk==PK_METEOR||kk==PK_KILL||kk==PK_QUAKE||kk==PK_FLOOD||kk==PK_PLAGUE||kk==PK_ACID||kk==PK_TORNADO||kk==PK_NUKE)?U_RED:(kk==PK_SETTLE||kk==PK_WAR||kk==PK_PEACE||kk==PK_BLESS)?U_BLUE:U_GOLD; for(int i=1;i<20;i++){ TBP(x0+i,y0+1,UI_PAL0+cc); TBP(x0+i,y0+2,UI_PAL0+cc); } }   // a colour strip says what kind of power it is: land, life, disaster, people, tools
        if(favPow[pi]){ for(int j=0;j<3;j++) for(int i=0;i<3;i++) TBP(x0+16+i,y0+4+j,UI_PAL0+U_YELLOW); }   // a starred power
        const Pw*P=&pw[pi]; for(int j=0;j<10;j++) for(int i=0;i<10;i++){ int c;
            if(P->kind==PK_PAINT){ c=uiSwatch(P->arg,i,j,200); } else { c=icon10[uiKindIcon[P->kind]][j*10+i]; if(!c) continue; c+=UI_PAL0; }
            TBP(x0+5+i,y0+4+j,c); } }
    // arrows
    for(int j=0;j<7;j++){ int w=j<4?j+1:7-j; for(int i=0;i<w;i++){ TBP(6-i,8+j,UI_PAL0+(tbTop>0?U_WHITE:U_DGREY)); TBP(249+i,8+j,UI_PAL0+(tbTop<PW_N-TB_SLOTS?U_WHITE:U_DGREY)); } }
    for(int ty=0;ty<3;ty++) for(int tx=0;tx<32;tx++){ u8 tile[64] __attribute__((aligned(4))); for(int y=0;y<8;y++) for(int x=0;x<8;x++) tile[y*8+x]=tbPix[ty*8+y][tx*8+x]; vramCopy((u8*)BG_TILE_RAM(1)+(TB_TILE0+ty*32+tx)*64,tile,64); }
    #undef TBP
}
static void tbInit(void){   // the layer: all blank except the three bottom rows
    for(int i=0;i<1024;i++) mapPtr1[i]=TB_BLANK; for(int ty=0;ty<3;ty++) for(int tx=0;tx<32;tx++) mapPtr1[(21+ty)*32+tx]=(u16)(TB_TILE0+ty*32+tx);
    u8 z[64] __attribute__((aligned(4))); memset(z,0,64); vramCopy((u8*)BG_TILE_RAM(1)+TB_BLANK*64,z,64);
    uiShadePal();
    tbDraw(); }
static int tbTouch(int px,int py){   // a touch in the toolbar: returns 1 if it was used
    if(py<168) return 0;
    if(px<12){ power=power>0?power-1:0; return 1; } if(px>=244){ power=power<PW_N-1?power+1:PW_N-1; return 1; }
    int s=(px-12)/23; if(s>=0&&s<TB_SLOTS) power=tbTop+s; return 1; }
#endif

// ui.h - Bore: GodGuy's interface, drawn by the game itself: the top screen is one 256 x 192 bitmap held in RAM (never written to VRAM a byte at a time: the DS ignores byte writes to VRAM)
// and copied over 16 bits at a time; the toolbar along the bottom of the touch screen is a second tile layer built the same way. Everything is made of a few primitives:
// rectangles, bevelled frames, a 5 x 7 font, bars, 10 x 10 power icons and 8 x 8 stat icons.
#ifndef UI_H
#define UI_H
#include "font5x7.h"
#include "icons.h"
enum { U_CLEAR, U_BG, U_PANEL, U_PANELHI, U_BHI, U_BLO, U_WHITE, U_DIM, U_GOLD, U_GREEN, U_RED, U_BLUE, U_PURPLE, U_ORANGE, U_SEL, U_SHADOW, U_G0, U_G1, U_G2, U_G3, U_G4, U_G5, U_G6, U_G7, U_YELLOW, U_CYAN, U_BROWN, U_LGREY, U_DGREY, U_PINK, U_TEAL, U_N };
static const u8 uiRgb[U_N][3]={ {0,0,0},{2,3,8},{4,6,13},{7,10,19},{16,19,28},{2,3,7},{31,31,31},{21,24,30},{31,26,6},{8,26,10},{29,6,6},{9,17,31},{20,9,28},{31,17,4},{9,14,27},{0,0,2},
    {31,30,12},{31,28,9},{31,25,7},{31,22,5},{30,18,4},{28,14,3},{25,10,3},{20,6,3},{31,31,8},{8,29,31},{14,8,3},{23,24,27},{10,11,14},{31,18,24},{6,22,20} };
#define UI_PAL0_OLD 160           // the toolbar's colours live at 160.. of the main palette (the day / night light only touches 0..143)
static u8 uiBuf[256*192] __attribute__((aligned(4))); static int uiPages, uiPageNow, uiPageOpen, uiDotX=96;   // page dots in the title bar
static const u8 uiKCol[16][3]={{31,8,8},{8,16,31},{10,28,10},{31,26,6},{24,10,30},{31,16,4},{6,28,28},{31,14,22},{20,31,14},{14,20,31},{31,22,14},{18,10,22},{31,31,16},{10,24,20},{26,12,12},{22,22,28}};   // kingdom colours (sub palette 40..55)
// ---- v2 look: vertical gradients (8 palette steps each, 140.. of the sub palette, free of everything else) for window bars, buttons, selected rows and backdrops
enum { G_HEAD, G_SEL, G_BTN, G_BG, G_FOOT, G_N };
#define UI_GRAD0 148
static const u8 uiGradRgb[G_N][2][3]={ {{14,18,31},{4,6,17}}, {{17,25,31},{7,12,25}}, {{10,14,25},{4,7,15}}, {{5,7,16},{1,2,6}}, {{6,8,18},{2,3,9}} };
static void uiGradPal(void){ for(int g=0;g<G_N;g++) for(int k=0;k<8;k++){ int r=uiGradRgb[g][0][0]+(uiGradRgb[g][1][0]-uiGradRgb[g][0][0])*k/7, gg=uiGradRgb[g][0][1]+(uiGradRgb[g][1][1]-uiGradRgb[g][0][1])*k/7, b=uiGradRgb[g][0][2]+(uiGradRgb[g][1][2]-uiGradRgb[g][0][2])*k/7; BG_PALETTE_SUB[UI_GRAD0+g*8+k]=RGB15(r,gg,b); } }
static inline void uiPx(int x,int y,int c);
static inline void uiRow(int x,int y,int w,int c){ if((unsigned)y>=192) return; if(x<0){ w+=x; x=0; } if(x+w>256) w=256-x; if(w>0) memset(&uiBuf[y*256+x],c,(size_t)w); }
static void uiGrad(int x,int y,int w,int h,int g){ for(int j=0;j<h;j++){ int k=h>1?j*7/(h-1):0; uiRow(x,y+j,w,UI_GRAD0+g*8+k); } }
static void uiGradRound(int x,int y,int w,int h,int g,int edge){ for(int j=0;j<h;j++){ int k=h>1?j*7/(h-1):0; int c=UI_GRAD0+g*8+k; int a=(j==0||j==h-1)?1:0; for(int i=a;i<w-a;i++) uiPx(x+i,y+j,c); }
    for(int i=1;i<w-1;i++){ uiPx(x+i,y,edge); uiPx(x+i,y+h-1,edge); } for(int j=1;j<h-1;j++){ uiPx(x,y+j,edge); uiPx(x+w-1,y+j,edge); } }
// toolbar colours: 22 slots of the main palette that nothing else touches (applyLight skips UI_RESV; zoom's creature colours sit at 192..197 and 248..253)
#define UI_RESV(i) (((i)>=117&&(i)<=127)||((i)>=161&&(i)<=167))
static const u8 tcSlot[22]={117,118,119,120,121,122,123,124,125,126,127,161,162,163,164,165,166,167,198,199,254,255};
static const u8 tcRgb[22][3]={ {31,31,31},{31,26,6},{9,28,11},{31,10,10},{25,14,31},{31,17,4},{0,0,2},{31,31,8},{8,29,31},{24,14,6},{27,28,31},{13,14,18},   // the icon colours: white gold green red purple orange shadow yellow cyan brown light-grey grey
    {2,3,9},{12,16,28},{6,8,18},{12,15,27},{13,19,31},{4,6,15},{7,10,22},{10,13,25},{9,14,27},{10,19,31} };   // bar, bar top line, tab, tab edge, selected tab, slot, slot top, slot edge, selected slot, blue
enum { TC_BAR=12,TC_BARHI,TC_TAB,TC_TABEDGE,TC_TABSEL,TC_SLOT,TC_SLOTHI,TC_SLOTEDGE,TC_SELFILL,TC_BLUE };
// UIP: a UI colour (U_*) as a main palette slot
static const u8 uiPmap[31]={0,162,162,162,163,123,117,165,118,119,120,255,121,122,166,123,118,118,118,118,118,118,118,118,124,125,126,127,161,120,125};
#define UIP(c) ((int)(tcSlot[0]*0+uiPmap[(c)]))
static void uiPalette(void){ uiGradPal(); for(int k=0;k<16;k++) BG_PALETTE_SUB[40+k]=RGB15(uiKCol[k][0],uiKCol[k][1],uiKCol[k][2]); for(int i=0;i<U_N;i++){ u16 c=RGB15(uiRgb[i][0],uiRgb[i][1],uiRgb[i][2]); BG_PALETTE_SUB[i]=c; } for(int i=0;i<22;i++) BG_PALETTE[tcSlot[i]]=RGB15(tcRgb[i][0],tcRgb[i][1],tcRgb[i][2]); BG_PALETTE_SUB[0]=0; }
static unsigned uiFlushes;   // pictures sent to the screen so far (the tests count them: a menu must send one per redraw, never two)
static int uiHold;           // while set, uiFlush sends nothing: a screen that is drawn on top of another one (the pause menu over the status screen) must not show the one underneath for a frame
// ---- depth for the 3DS's 3D top screen (compat/stereo.h): each pixel of the picture has a level. Anything drawn with a level of its own (the planet's dome) puts it in uiZ; the rest take it from the colour
// they were drawn in (a table: backgrounds sink, panels sit at the screen, bevels, text and marks come out a little, the minimap's mountains stand up). Nothing else in the game knows about it.
#ifdef NDSSHIM
static signed char uiZ[256*192]; static signed char uiDepthLut[256]; static int uiDepthReady;
static void uiDepthInit(void){ memset(uiZ,-128,sizeof uiZ); for(int i=0;i<256;i++) uiDepthLut[i]=0;
    uiDepthLut[U_CLEAR]=-3; uiDepthLut[U_BG]=-3; uiDepthLut[U_PANEL]=-2; uiDepthLut[U_SHADOW]=-1; uiDepthLut[U_PANELHI]=0; uiDepthLut[U_BHI]=2; uiDepthLut[U_BLO]=-1; uiDepthLut[U_SEL]=1;
    for(int c=U_WHITE;c<=U_PURPLE;c++) uiDepthLut[c]=3; uiDepthLut[U_ORANGE]=3; for(int c=U_G0;c<=U_G7;c++) uiDepthLut[c]=2; for(int c=U_YELLOW;c<U_N;c++) uiDepthLut[c]=3; uiDepthLut[U_DIM]=2; uiDepthLut[U_DGREY]=1;
    for(int g=0;g<G_N;g++) for(int k=0;k<8;k++) uiDepthLut[UI_GRAD0+g*8+k]=(g==G_BG)?-3:2;            // the backgrounds sink, title bars, buttons and the footer come out
    for(int k=0;k<16;k++) uiDepthLut[40+k]=3;                                                           // kingdom colours
    static const signed char ter[13]={-2,-2,0,1,2,3,5,5,1,0,0,3,0}; for(int t=0;t<13;t++) uiDepthLut[64+t]=(signed char)(ter[t]+1);      // the minimap's land stands up: mountains and snow most
    for(int k=80;k<90;k++) uiDepthLut[k]=6;                                                           // creature dots and biome marks
    for(int k=100;k<140;k++) uiDepthLut[k]=4;                                                          // the planet (its own dome overrides this)
    uiDepthReady=1; }
static inline void uiZSet(int x,int y,int z){ if((unsigned)x<256&&(unsigned)y<192) uiZ[y*256+x]=(signed char)z; }
static void uiDepthBuild(signed char*out){ if(!uiDepthReady) uiDepthInit(); for(int i=0;i<256*192;i++){ int z=uiZ[i]; out[i]=(signed char)(z!=-128?z:uiDepthLut[uiBuf[i]]); } memset(uiZ,-128,sizeof uiZ); }
#else
#define uiZSet(x,y,z) ((void)0)
#endif
#ifdef NDSHW
static void uiFlushRaw(void){ uiFlushes++; hwSubFlush(uiBuf); }
#else
#ifdef NDSSHIM
static void uiFlushRaw(void){ uiFlushes++; vramCopy(bgGetGfxPtr(3),uiBuf,256*192); uiDepthBuild(shimDepth); }
#else
static int uiBitmapPage;
static void* uiBitmap(int page){ return (void*)(0x06200000+page*0x10000); }
static void uiFlushRaw(void){ uiFlushes++; vramCopy(uiBitmap(uiBitmapPage),uiBuf,256*192); }
#endif
#endif
#ifndef UI_NOTRANS
#define UI_NOTRANS 0
#endif
// ---- stage a complete view, then swap it onto the display at VBlank.
#ifndef NDSHW
static u8 uiPrev[256*192];
#endif
static u8 uiNew[256*192] __attribute__((aligned(4)));
static int uiTrans, uiHavePrev, uiCut, uiNextLcd=-1;
static void uiApplyLcd(void){
    if(uiNextLcd<0) return;
    if(uiNextLcd) lcdMainOnTop(); else lcdMainOnBottom();
    uiNextLcd=-1;
}
#ifdef NDSHW
static void uiFlipStaged(void){
    swiWaitForVBlank();
    hwSubFlip();
    uiApplyLcd();
    uiFlushes++;
}
#endif
static void uiPresent(void){
#ifdef NDSHW
    for(int y=0;y<192;y++) hwSubStageRow(y,uiBuf+y*256);
    uiFlipStaged();
#elif defined(NDSSHIM)
    uiFlushRaw();
    uiApplyLcd();
    swiWaitForVBlank();
#else
    int next=uiBitmapPage^1;
    vramCopy(uiBitmap(next),uiBuf,256*192);
    swiWaitForVBlank();
    // Extended affine bitmap bases advance by 16 KiB on the sub engine.
    vu16*cnt=(vu16*)0x0400100e;
    *cnt=(u16)((*cnt&~0x1F00)|(next<<10));
    uiBitmapPage=next;
    uiApplyLcd();
    uiFlushes++;
#endif
}

static void uiTransitionRow(u8*out,const u8*old,const u8*new,int dir,int shift){
    if(dir>0){
        memcpy(out,old+shift,256-shift);
        memcpy(out+256-shift,new,shift);
    } else {
        memcpy(out,new+256-shift,shift);
        memcpy(out+shift,old,256-shift);
    }
}
static void uiFlush(void){
    if(uiHold) return;
    if(uiCut){
        uiTrans=uiCut=0;
        uiPresent();
    } else if(uiTrans&&uiHavePrev&&!UI_NOTRANS){
        int dir=uiTrans;
        uiTrans=0;
#ifdef NDSHW
        // The live UI is the destination. Keep the previous frame in the
        // shared FMV scratch and stream rows straight to tile VRAM.
        hwSubGet(uiNew);
#else
        memcpy(uiNew,uiBuf,sizeof uiNew);
#endif
        for(int f=1;f<=7;f++){
            int p=f*256/7, shift=256-(256-p)*(256-p)/256;
            for(int y=0;y<192;y++){
                int off=y*256;
#ifdef NDSHW
                u8 row[256] __attribute__((aligned(4)));
                uiTransitionRow(row,uiNew+off,uiBuf+off,dir,shift);
                hwSubStageRow(y,row);
#else
                uiTransitionRow(uiBuf+off,uiPrev+off,uiNew+off,dir,shift);
#endif
            }
#ifdef NDSHW
            uiFlipStaged();
#else
            uiPresent();
#endif
        }
#ifndef NDSHW
        memcpy(uiBuf,uiNew,sizeof uiNew);
#endif
    } else uiFlushRaw();
#ifndef NDSHW
    memcpy(uiPrev,uiBuf,sizeof uiPrev);
#endif
    uiHavePrev=1;
}
// ---- touch buttons: each drawn control registers a rectangle; a tap on it sets *sp=sv (if given) and presses key (key2 instead when it was already selected)
typedef struct { short x,y,w,h; unsigned short key,key2; int*sp; int sv; } UiBtn;
static UiBtn uiBtns[64]; static int uiNBtn;
static void uiBtn(int x,int y,int w,int h,int key,int key2,int*sp,int sv){ if(uiNBtn<64){ UiBtn*b=&uiBtns[uiNBtn++]; b->x=(short)x; b->y=(short)y; b->w=(short)w; b->h=(short)h; b->key=(unsigned short)key; b->key2=(unsigned short)key2; b->sp=sp; b->sv=sv; } }
static int uiHit(int px,int py){ for(int i=uiNBtn-1;i>=0;i--){ const UiBtn*b=&uiBtns[i]; if(px>=b->x&&px<b->x+b->w&&py>=b->y&&py<b->y+b->h) return i; } return -1; }
static inline void uiPx(int x,int y,int c){ if((unsigned)x<256&&(unsigned)y<192) uiBuf[y*256+x]=(u8)c; }
static void uiRect(int x,int y,int w,int h,int c){ for(int j=0;j<h;j++) uiRow(x,y+j,w,(u8)c); }
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
static const u8 uiKindIcon[PK_KIND_N]={IC10_INSPECT,0,IC10_SPAWN,IC10_RAIN,IC10_FIRE,IC10_BOLT,IC10_METEOR,IC10_SMITE,IC10_HEAL,IC10_FEED,IC10_SETTLE,IC10_WAR,IC10_PEACE,IC10_BLESS,IC10_GIFT,IC10_MUTATE,IC10_QUAKE,IC10_FLOOD,IC10_BLOOM,IC10_VIBE,IC10_TORNADO,IC10_ACID,IC10_HOLE,IC10_NUKE,IC10_PLAGUE,IC10_CURSE,IC10_RAIN,
    [PK_INSPIRE]=IC10_SETTLE, [PK_WHISPER]=IC10_WAR, [PK_DISCORD]=IC10_WAR,
    [PK_UNITY]=IC10_PEACE, [PK_SPITE]=IC10_CURSE, [PK_FRIEND]=IC10_PEACE,
    [PK_DUSTW]=IC10_INSPECT, [PK_DUSTR]=IC10_MUTATE, [PK_DUSTG]=IC10_SETTLE,
    [PK_LOOT]=IC10_GIFT, [PK_CORRODE]=IC10_ACID, [PK_FIREEL]=IC10_FIRE,
    [PK_GOO]=IC10_FLOOD, [PK_CRABBOMB]=IC10_METEOR, [PK_CRABZILLA]=IC10_QUAKE,
    [PK_UPLIFT]=IC10_MUTATE, [PK_MONOLITH]=IC10_BLESS, [PK_STATUS]=IC10_VIBE,
    [PK_ALIEN]=IC10_METEOR, [PK_DRAGON]=IC10_FIRE, [PK_MOON]=IC10_CURSE,
    [PK_BIOMASS]=IC10_PLAGUE, [PK_ICE]=IC10_TORNADO, [PK_GARDEN]=IC10_BLOOM,
    [PK_BOLT_SMALL]=IC10_BOLT, [PK_BOLT_MEDIUM]=IC10_BOLT,
    [PK_PRINT_COPY]=IC10_INSPECT, [PK_PRINT_FOREST]=IC10_BLOOM,
    [PK_PRINT_WATER]=IC10_FLOOD, [PK_PRINT_MOUNTAIN]=IC10_QUAKE,
    [PK_PRINT_BIOME]=IC10_MUTATE, [PK_PRINT_CLEAR]=IC10_SMITE,
    [PK_POSSESS]=IC10_VIBE, [PK_HOME]=IC10_SETTLE, [PK_BORDER]=IC10_SETTLE,
    [PK_SPECIAL]=IC10_METEOR, [PK_FIELD]=IC10_FIELD };
static int uiIconForKind(int kind){
    static const u8 fieldIcons[FIELD_N]={IC10_SURVEY,IC10_RATIONS,IC10_CLINIC,IC10_RESTORE};
    return kind==PK_FIELD?fieldIcons[fieldMode]:uiKindIcon[kind];
}
#ifdef __3DS__
static const u8 tbIcoT[3]={10,20,24};
#else
static const u8 tbIcoT[3]={10,15,20};
#endif
// ground swatches use three shades per terrain: 100.. in the top palette, 200.. in the toolbar's
static int uiSwatch(int t,int i,int j,int base){ if(t>=100) t=T_N+t-101; int p=tilePix(t,0,(i*8)/10,(j*8)/10); p=(p>=104&&p<128)?4:p%8; return base+t*3+(p<=2?0:p<=4?1:2); }
static void uiShadePal(void){ for(int t=0;t<T_N+3;t++) for(int j=0;j<3;j++){ int r,g,b; shadeRGB(t*8+2+j*2,&r,&g,&b); BG_PALETTE_SUB[100+t*3+j]=RGB15(r,g,b); BG_PALETTE[200+t*3+j]=RGB15(r,g,b); } }
static void uiPowerIcon(int pwi,int x,int y){ const Pw*P=&pw[pwi];
    if(P->kind==PK_PAINT){ for(int j=0;j<10;j++) for(int i=0;i<10;i++) uiPx(x+i,y+j,uiSwatch(P->arg,i,j,100)); return; }
    uiIcon10(x,y,uiIconForKind(P->kind)); }
// ---- window furniture
static int uiBackHint;   // set while an Extras page is open: START goes back to the menu, so the page hint says so
static const char* uiTabPrev, *uiTabNext;   // names of the neighbouring Extras pages (L and R switch between them)
static void uiWin(const char*title,const char*right,const char*hint){ if(uiBackHint) hint=0;
    uiGrad(0,0,256,192,G_BG); for(int y=0;y<192;y+=2) for(int x=((y>>1)&1)*2;x<256;x+=4) uiPx(x,y,U_PANEL);
    uiGrad(0,0,256,13,G_HEAD); uiRect(0,0,256,1,U_BHI); uiRect(0,12,256,1,U_BHI); uiRect(0,12,44,1,U_GOLD); uiRect(0,13,256,1,U_SHADOW);
    uiTextS(6,3,title,U_GOLD,1); if(right) uiTextR(250,3,right,U_DIM);
    if(uiPages>0){ char pg[8]; int n=0; for(int i=0;i<=uiPageNow;i++) if(uiPageOpen&(1<<i)) n++; int t=0; for(int i=0;i<uiPages;i++) if(uiPageOpen&(1<<i)) t++; pg[0]='0'+n; pg[1]='/'; pg[2]='0'+t; pg[3]=0; uiText(uiDotX,3,pg,U_LGREY,1); }
    uiGrad(0,179,256,13,G_FOOT); uiRect(0,179,256,1,U_BHI); uiRect(0,178,256,1,U_SHADOW); if(hint) uiText(6,183,hint,U_DIM,1);
    if(uiBackHint){ if(uiTabPrev){ uiRound(3,181,12,10,U_SEL,U_BHI); uiText(7,183,"L",U_GOLD,1); uiText(19,183,uiTabPrev,U_DIM,1); uiBtn(2,180,84,12,KEY_L,0,0,0); } uiRound(98,180,60,12,U_SEL,U_GOLD); uiTextC(128,183,uiBackHint==2?"MAP":"BACK",U_WHITE,1); uiBtn(96,179,64,13,KEY_START,0,0,0); if(uiTabNext){ uiRound(241,181,12,10,U_SEL,U_BHI); uiText(245,183,"R",U_GOLD,1); uiTextR(237,183,uiTabNext,U_DIM); uiBtn(170,180,84,12,KEY_R,0,0,0); }
        uiRound(240,147,13,13,U_SEL,U_BHI); uiIcon8(243,150,IC8_UP); uiBtn(236,145,19,17,KEY_UP,0,0,0); uiRound(240,163,13,13,U_SEL,U_BHI); uiIcon8(243,166,IC8_DOWN); uiBtn(236,162,19,16,KEY_DOWN,0,0,0); } }
static void uiListRow(int y,int sel,int n,const char*left,const char*right,int col){ if(sel) uiRect(4,y-1,240,10,U_SEL); else if(n&1) uiRect(4,y-1,240,10,U_PANEL);
    if(sel) uiText(6,y,">",U_GOLD,1); uiText(14,y,left,col,1); if(right&&right[0]) uiTextR(242,y,right,sel?U_WHITE:U_DIM); }
static void uiScroll(int total,int top,int vis,int y0,int h){ uiRect(248,y0,5,h,U_SHADOW); if(total>vis){ int bh=h*vis/total; if(bh<6) bh=6; int by=y0+(h-bh)*top/(total-vis); uiRect(248,by,5,bh,U_BHI); } }
static int uiVx, uiVy, uiVw=32, uiVh=24;      // the view's top left tile and size in tiles, for the minimap's frame
static int relK;      // the kingdom the RELATIONS tint is drawn for (the last one looked at)
static int mmIcons;      // OPTIONS > MAP DOTS: 0 the folk and monsters only, 1 every creature by kind (grazers green, hunters orange, water folk cyan, magical white, terraformers yellow)
static int mmMode;      // what the territory rings show: 0 kingdom, 1 culture, 2 faith, 3 language (OPTIONS > MAP TINT)
static int mmTint(int k){ return mmMode==0?k:mmMode==1?kd[k].cul:mmMode==2?kd[k].rlg:kd[k].lang%5; }
static int mmRingCol(int k){ if(mmMode!=4) return 40+mmTint(k); if(!kd[relK].on||k==relK) return U_YELLOW; if(kd[relK].war>>k&1) return U_RED; if(kd[relK].allies>>k&1) return U_GREEN; if(kd[relK].lord==k+1||kd[k].lord==relK+1) return U_CYAN; return kd[relK].rel[k]<-20?U_ORANGE:kd[relK].rel[k]>20?U_TEAL:U_LGREY; }
// ---- the minimap (the whole planet squeezed to 128 x 128) with creatures, villages and the camera
static void uiMinimap(int ox,int oy){
    static int mmW=-1, mmH=-1; static short mmX[128], mmY[128]; if(mmW!=WW||mmH!=WH){ mmW=WW; mmH=WH; for(int i=0;i<128;i++){ mmX[i]=(short)(i*WW/128); mmY[i]=(short)(i*WH/128); } }
    // the land and sea of the planet is worked out only when it can have changed (about every half second of game time, or after a power was used), not on every frame: 16,384 samples of the world and the biomes each time was the biggest part of drawing the top screen
    static u8 mmBase[128*128]; static int mmKey=-1, mmKeyDone=-2; { int key=(tick>>5)^(int)(stUse*2654435761u)^(WW<<20); if(key!=mmKey){ mmKey=key; for(int y=0;y<128;y++){ int wy=mmY[y]; u8*row=&mmBase[y*128]; for(int x=0;x<128;x++){ int wx=mmX[x]; int bb=bioAt(wx,wy); row[x]=(u8)(bb?85+bb:64+world[wy][wx]); } } } }
    if(mmKey==mmKeyDone){} else { mmKeyDone=mmKey; for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(c->alive){ int da=arch[sp[c->sp].a].diet; int aa=sp[c->sp].a; if(mmIcons||da==5||da==4||c->rk){ int px=c->x*128/WW, py=c->y*128/WH; if(px<128&&py<128) mmBase[py*128+px]=(u8)(da==4?80:da==5?85:(mmIcons&&!c->rk)?(da==6?U_YELLOW:(aa>=55&&aa<=59)?U_WHITE:da==3?U_CYAN:da==1?U_ORANGE:U_GREEN):82); } } } }      // (the folk and monster dots are part of the cached picture too)
    for(int y=0;y<128;y++) memcpy(&uiBuf[(oy+y)*256+ox],&mmBase[y*128],128);
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]){ int cx=ox+vil[v].x*128/WW, cy=oy+vil[v].y*128/WH, R=14*128/WW+1; static int ringR=-1; static signed char ringX[14], ringY[14]; if(ringR!=R){ ringR=R; for(int a=0;a<28;a+=2){ ringX[a>>1]=(signed char)(int)(R*cosf(a*0.2244f)+0.5f); ringY[a>>1]=(signed char)(int)(R*sinf(a*0.2244f)+0.5f); } } for(int a=0;a<14;a++) uiPx(cx+ringX[a],cy+ringY[a],mmMode==5?40+(v&7):mmRingCol(vilK[v]-1)); }   // each kingdom's territory, a dotted ring round its cities
    for(int a=0;a<KN_N;a++) if(kd[a].on&&kd[a].cap) for(int b=a+1;b<KN_N;b++) if(kd[b].on&&kd[b].cap&&(kd[a].tradeW>>b&1)){ const Vil*A=&vil[kd[a].cap-1], *B=&vil[kd[b].cap-1]; int x0=ox+A->x*128/WW, y0=oy+A->y*128/WH, x1=ox+B->x*128/WW, y1=oy+B->y*128/WH; int st=abs(x1-x0)>abs(y1-y0)?abs(x1-x0):abs(y1-y0); for(int i=0;i<=st;i++) if(((i+(tick>>3))&3)==0) uiPx(x0+(x1-x0)*i/(st?st:1),y0+(y1-y0)*i/(st?st:1),U_YELLOW); }   // trade routes: gold dots running between trading capitals
    for(int v=0;v<VIL_N;v++) if(vil[v].on){ for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++) uiPx(ox+vil[v].x*128/WW+dx,oy+vil[v].y*128/WH+dy,vil[v].foe?81:84); }
    for(int i=0;i<MAP_PIN_N;i++) if(mapPinMask&(1u<<i)){
        int x=ox+(mapPin[i][0]+128)*16/WW, y=oy+(mapPin[i][1]+96)*16/WH;
        uiRect(x-1,y-1,3,3,U_WHITE);
        uiPx(x,y,U_CYAN);
    }
    int cx0=uiVx*128/WW, cy0=uiVy*128/WH, cw=uiVw*128/WW, ch=uiVh*128/WH; if(cx0<0) cx0=0; if(cy0<0) cy0=0; if(cx0+cw>127) cx0=127-cw; if(cy0+ch>127) cy0=127-ch; for(int x=cx0;x<cx0+cw;x++){ uiPx(ox+x,oy+cy0,83); uiPx(ox+x,oy+cy0+ch,83); } for(int y=cy0;y<=cy0+ch;y++){ uiPx(ox+cx0,oy+y,83); uiPx(ox+cx0+cw,oy+y,83); } }
// ---- the toolbar (bottom of the touch screen): power slots with arrows, a second tile layer. UI SCALE (hwUiScale 0..2) makes it taller with bigger slots and icons (24, 32 or 40 pixels; 10, 8 or 6 slots)
#ifndef HW_CAL
#ifdef __3DS__
static int hwUiScale=1;
#else
static int hwUiScale;
#endif
#endif
#define TB_TILE0 640
#define TB_BLANK 1023
// KIT shows starter and starred powers. Each other tab holds one category and Inspect.
static const u8 tbRowsT[3]={4,5,7}, tbTabT[3]={11,12,14}, tbSlotsT[3]={10,8,6}, tbPitchT[3]={23,29,38}, tbBoxW[3]={21,27,36}, tbBoxH[3]={18,25,34};
#define TB_ROWS (tbRowsT[hwUiScale])
#define TB_H (TB_ROWS*8)
#define TB_Y (192-TB_H)
#define TB_SLOTS (tbSlotsT[hwUiScale])
#define TB_NCAT 5
static const char* const tbCatNm[TB_NCAT]={"KIT","LAND","LIFE","EVENTS","FOLK"};
static const u8 tbCatCol[TB_NCAT]={118,126,119,120,255};   // main palette slots
static u8 tbPix[56][256]; static int tbTop=0, tbCat=0, tbN=0; static u8 tbList[PW_N]; static u16*mapPtr1;
static u32 tbStamp;
static int tbHome(int pi){
    int k=pw[pi].kind;
    if(k==PK_INSPECT) return 0;
    if(k==PK_PAINT||k==PK_AIR||(k>=PK_PRINT_COPY&&k<=PK_PRINT_CLEAR)) return 1;
    if(k==PK_SPAWN||k==PK_HEAL||k==PK_FEED||k==PK_BLOOM||k==PK_VIBE||k==PK_MUTATE
        ||k==PK_UPLIFT||k==PK_MONOLITH||k==PK_STATUS||k==PK_GARDEN||k==PK_POSSESS) return 2;
    if(k==PK_SETTLE||k==PK_WAR||k==PK_PEACE||k==PK_BLESS||k==PK_GIFT||(k>=PK_INSPIRE&&k<=PK_DUSTG)
        ||k==PK_LOOT||k==PK_HOME||k==PK_BORDER||k==PK_FIELD) return 4;
    return 3;
}
static int tbStarter(int i){
    const Pw*P=&pw[i];
    if(P->kind==PK_PAINT) return P->arg==T_WATER||P->arg==T_GRASS;
    return P->kind==PK_INSPECT||P->kind==PK_SPAWN
        ||P->kind==PK_SETTLE||P->kind==PK_FIELD;
}
static void tbBuild(void){
    tbN=0;
    for(int i=0;i<PW_N;i++){
        if(tbCat==0?(tbStarter(i)||favPow[i]):(pw[i].kind==PK_INSPECT||tbHome(i)==tbCat))
            tbList[tbN++]=(u8)i;
    }
}
static int tbPos(int p){ for(int i=0;i<tbN;i++) if(tbList[i]==p) return i; return -1; }
static void tbStep(int d){
    if(!tbN) tbBuild();
    int p=tbPos(power);
    if(p<0){ tbCat=tbHome(power); tbBuild(); p=tbPos(power); }
    power=tbList[(p+tbN+d)%tbN];
}
static void tbText(int x,int y,const char*s,int c){ for(;*s;s++){ int ch=(unsigned char)*s; if(ch<32||ch>=96) ch=32; const unsigned char*g=font5x7[ch-32];
    for(int r=0;r<7;r++) for(int q=0;q<5;q++) if((g[r]>>(4-q))&1){ int xx=x+q, yy=y+r; if((unsigned)xx<256&&(unsigned)yy<56) tbPix[yy][xx]=(u8)c; } x+=6; } }
static void tbInvalidate(void){ tbN=0; tbStamp=0; }
static u32 tbDrawKey(void){
    return 1u+(u32)power+((u32)tbCat<<8)+((u32)hwUiScale<<12)+((u32)fieldMode<<14);
}
static void tbDraw(void){
    if(tbStamp==tbDrawKey()) return;
    if(!tbN) tbBuild();
    if(tbPos(power)<0){ tbCat=tbHome(power); tbBuild(); }
    tbStamp=tbDrawKey();
#ifdef __3DS__
    const int spriteSize=20;
    const u8 (*sprites)[20*20]=icon20;
#else
    const int spriteSize=10;
    const u8 (*sprites)[10*10]=icon10;
#endif
    const int H=TB_H, TT=tbTabT[hwUiScale], SW=tbBoxW[hwUiScale], SH=tbBoxH[hwUiScale], PI=tbPitchT[hwUiScale], IS=tbIcoT[hwUiScale], K=hwUiScale?2:1, Y0=TT+(H-TT-SH)/2+1;
    #define TBP(x,y,c) do{ if((unsigned)(x)<256&&(unsigned)(y)<(unsigned)H) tbPix[y][x]=(u8)(c); }while(0)
    for(int y=0;y<H;y++) for(int x=0;x<256;x++) tbPix[y][x]=(u8)(y==0?tcSlot[TC_BARHI]:tcSlot[TC_BAR]);
    int pp=tbPos(power); if(pp<tbTop) tbTop=pp; if(pp>=tbTop+TB_SLOTS) tbTop=pp-TB_SLOTS+1; if(tbTop>tbN-TB_SLOTS) tbTop=tbN-TB_SLOTS; if(tbTop<0) tbTop=0;
    for(int c=0;c<TB_NCAT;c++){ int x0=c*256/TB_NCAT+1, x1=(c+1)*256/TB_NCAT-1, sel=c==tbCat;   // the tabs
        for(int y=1;y<TT;y++) for(int x=x0;x<x1;x++){ int edge=(x==x0||x==x1-1||y==1); int cc=sel?(edge?118:tcSlot[TC_TABSEL]):(edge?tcSlot[TC_TABEDGE]:tcSlot[TC_TAB]); TBP(x,y,cc); }
        for(int x=x0+1;x<x1-1;x++){ TBP(x,TT-1,tbCatCol[c]); if(!sel) TBP(x,TT-2,tbCatCol[c]); }
        int tw=(int)strlen(tbCatNm[c])*6-1; tbText((x0+x1)/2-tw/2,2+(TT-4-7)/2,tbCatNm[c],sel?117:127); }
    for(int s=0;s<TB_SLOTS&&tbTop+s<tbN;s++){ int pi=tbList[tbTop+s]; int x0=12+s*PI, y0=Y0, sel=pi==power;
        for(int j=0;j<SH;j++) for(int i=0;i<SW;i++){ int edge=(i==0||j==0||i==SW-1||j==SH-1); int c=sel?(edge?118:(j<=2?tcSlot[TC_SLOTEDGE]:tcSlot[TC_SELFILL])):(edge?tcSlot[TC_SLOTEDGE]:(j==1?tcSlot[TC_SLOTHI]:tcSlot[TC_SLOT])); TBP(x0+i,y0+j,c); }
        int cc=tbCatCol[tbHome(pi)];
        for(int i=1;i<SW-1;i++) for(int j=1;j<=2+(K>1);j++) TBP(x0+i,y0+j,cc);
        if(favPow[pi]){ for(int j=0;j<3*K;j++) for(int i=0;i<3*K;i++) TBP(x0+SW-5*K+i,y0+3+K+j,124); }   // a starred power
        const Pw*P=&pw[pi]; int ix=x0+(SW-IS)/2, iy=y0+(SH-IS)/2+(K>1);
        const u8*sprite=sprites[P->kind==PK_FIELD&&!sel?IC10_FIELD:uiIconForKind(P->kind)];
        for(int j=0;j<IS;j++) for(int i=0;i<IS;i++){ int c;
            if(P->kind==PK_PAINT){
                if((i==0||i==IS-1)&&(j==0||j==IS-1)) continue;
                c=uiSwatch(P->arg,i*10/IS,j*10/IS,200);
                if(j==0||i==0) c+=(c-200)%3<2;
                if(j==IS-1||i==IS-1) c-=(c-200)%3>0;
            } else {
                c=sprite[(j*spriteSize/IS)*spriteSize+i*spriteSize/IS];
                if(!c) continue;
                c=UIP(c);
            }
            TBP(ix+i,iy+j,c); } }
    // arrows (twice the size when the scale is up): they scroll the open category
    { int ay=TT+(H-TT-7*K)/2, lx=K>1?9:6, rx=K>1?246:249; for(int j=0;j<7*K;j++){ int jj=j/K, w=(jj<4?jj+1:7-jj)*K; for(int i=0;i<w;i++){ TBP(lx-i,ay+j,pp>0?117:161); TBP(rx+i,ay+j,pp<tbN-1?117:161); } } }
    for(int ty=0;ty<TB_ROWS;ty++) for(int tx=0;tx<32;tx++){ u8 tile[64] __attribute__((aligned(4))); for(int y=0;y<8;y++) for(int x=0;x<8;x++) tile[y*8+x]=tbPix[ty*8+y][tx*8+x]; vramCopy((u8*)BG_TILE_RAM(1)+(TB_TILE0+ty*32+tx)*64,tile,64); }
    #undef TBP
}
static void openStarterKit(void){
    tbCat=tbTop=0;
    tbInvalidate();
    tbDraw();
}
static void tbInit(void){   // the layer: all blank except the bottom rows
    for(int i=0;i<1024;i++) mapPtr1[i]=TB_BLANK; for(int ty=0;ty<TB_ROWS;ty++) for(int tx=0;tx<32;tx++) mapPtr1[(24-TB_ROWS+ty)*32+tx]=(u16)(TB_TILE0+ty*32+tx);
    u8 z[64] __attribute__((aligned(4))); memset(z,0,64); vramCopy((u8*)BG_TILE_RAM(1)+TB_BLANK*64,z,64);
    uiShadePal(); tbInvalidate();
    tbDraw(); }
static void tbSelect(int c){
    if(c==tbCat) return;
    tbCat=c; tbBuild(); tbTop=0;
    if(tbPos(power)<0) power=tbList[(c&&tbN>1)?1:0];
}
static int tbTouch(int px,int py){   // a touch in the toolbar: returns 1 if it was used
    if(py<TB_Y) return 0;
    if(!tbN) tbBuild();
    if(py-TB_Y<tbTabT[hwUiScale]){ int c=px*TB_NCAT/256; if(c<0) c=0; if(c>=TB_NCAT) c=TB_NCAT-1;   // a tab: open that category (the power stays if it is in it, otherwise the first real power of it)
        tbSelect(c); return 1; }
    int p=tbPos(power); if(p<0) p=0;
    if(px<12){ if(p>0) power=tbList[p-1]; return 1; } if(px>=244){ if(p<tbN-1) power=tbList[p+1]; return 1; }
    int s=(px-12)/tbPitchT[hwUiScale]; if(s>=0&&s<TB_SLOTS&&tbTop+s<tbN) power=tbList[tbTop+s]; return 1; }
#endif

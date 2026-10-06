// fx.h - GHOSTS and WEATHER.  Both are drawn with hardware sprites (OAM), so they cost no drawing time, no frame buffer and only a few
// hundred bytes of EWRAM.  Included late in main.c (after neighborhood.h, which holds the season); everything it needs earlier is forward declared.
//
// OBJ VRAM is 16 KB in the bitmap modes = 16 slots of 32 tiles.  The household (house.h) uses slots 0..7 (OBJ_SLOTS) and OBJ palettes 0..7, and
// OAM entries 0..15.  This file takes slot 8 (tiles 768..799), OBJ palettes 8..11 and OAM entries 16.. : ghosts first, then the weather particles.
//   tiles 768..775  ghost frame A (16 x 32)        784  rain streak
//   tiles 776..783  ghost frame B                  785  snow flake      786  splash
//   palettes 8, 9, 10  one per ghost (its colour says how it died)       11  weather
// Everything is semi-transparent (OAM mode 1, blended 10/16 over the picture like the household's "x-ray" sprites), and the window set by
// hhObjUpdate keeps it inside the room view, never over the HUD.
//
// GHOSTS    You die (die() in main.c)  ->  fxGhostBorn(why): a ghost rises where you fell and stays, up to GH_MAX (the oldest goes first).  It drifts
//           through the walls around the spot it died on, flickers by day, is solid by night, and haunts: close to you it says BOO (a SPOOK mood
//           hit and a wail), close to the household it puts a skull balloon over their head.  The ghosts are saved with the life (12 bytes in the
//           spare end of the 64 byte life block in SRAM: 'G', count, x y how x3, checksum) and cleared by a new life.
//           OPTIONS > SIM > BORES > GHOSTS: OFF, ON, HAUNTED (a ghost is always around, handy for testing).
// WEATHER   Nothing is stored.  The weather is worked out from the day, the six-hour block of it and the season (the town's, else the calendar),
//           so a day always has the same weather.  CLEAR, CLOUDY, FOG, RAIN, STORM, SNOW.  Rain and snow are sprite particles that land on
//           OUTDOOR tiles only (wInside, main.c) so it never rains in your house.  The room view is dimmed (or washed out for fog) with the
//           hardware blend (BLDCNT / BLDY), half as much indoors; lightning is a brighten flash plus thunder.  Standing outside in rain or
//           snow slowly soaks your mood (SOAKED).  The clock in the HUD shows the weather.  OPTIONS > TIME > DAY > WEATHER: AUTO or force one.
// Not drawn while the ZOOM is on, the action cam plays, or you are upstairs.  Untested on hardware: build it and look.

static void npcTick(void); static void npcPlayStart(void); static void npcObjUpdate(void);   // npc.h: AI skaters and police
#define FX_SLOT  8
#define FX_TILE  (512+FX_SLOT*32)     // first tile number of the slot
#define FX_OAM0  (2*OBJ_SLOTS)        // first OAM entry after the household's
#define GH_MAX   3
#define WX_N     40                   // most weather particles at once (OAM entries FX_OAM0+GH_MAX ..)
#define FX_PALG  8
#define FX_PALW  11
#define T_RAIN   (FX_TILE+16)
#define T_SNOW   (FX_TILE+17)
#define T_SPLASH (FX_TILE+18)

enum { WX_CLEAR, WX_CLOUDY, WX_FOG, WX_RAIN, WX_STORM, WX_SNOW };
static const char* const wxNm[6]={"CLEAR","CLOUDY","FOG","RAIN","STORM","SNOW"};

typedef struct { s32 fx,fy; s16 tx,ty; u8 hx,hy,why,age,cd,face; } FxGhost;
static FxGhost fxG[GH_MAX] EWRAM_BSS; static u8 fxGN EWRAM_BSS;
static u8 fxVramOk EWRAM_BSS;                 // the art and palettes are in OBJ VRAM (cleared whenever play starts or a ghost is born)
static u16 fxT EWRAM_BSS;                     // frames since play started
static int fxLastMin EWRAM_BSS, fxSoak EWRAM_BSS;   // (the small ones live in EWRAM too: IWRAM is nearly full)
static u8 wx EWRAM_BSS;                       // the weather now (WX_)
static u8 wxN EWRAM_BSS;                      // particles in use (eases up and down)
static s16 wxLvl EWRAM_BSS;                   // room dimming x16: > 0 darker, < 0 brighter (eases towards its target)
static u8 wxFlash EWRAM_BSS; static short wxThT EWRAM_BSS, wxBoom EWRAM_BSS;   // lightning: brightness left, steps to the next one, steps until the thunder
static u8 wxTx[WX_N] EWRAM_BSS, wxTy[WX_N] EWRAM_BSS, wxPh[WX_N] EWRAM_BSS;   // particle: its tile, its phase (255 = looking for a spot)

// ---- art (ROM) ----
// 16 x 32, two frames that differ in the wavy hem.  ' ' clear   # outline   o body   s shade   e eye   m mouth
static const char fxGhostArt[2][32][17]={
 {
    "                ",
    "                ",
    "                ",
    "     ######     ",
    "   #ooooooos#   ",
    "  #ooooooooos#  ",
    " #ooooooooooos# ",
    " #ooooooooooos# ",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#ooooooooooooos#",
    "#ooooommmmoooos#",
    "#oooooommooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#oooooooooooooo#",
    "#ooo#oo##oo#ooo#",
    "#oo# #o##o# #oo#",
    " ##   ##  ##   #",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "
 },
 {
    "                ",
    "                ",
    "                ",
    "     ######     ",
    "   #ooooooos#   ",
    "  #ooooooooos#  ",
    " #ooooooooooos# ",
    " #ooooooooooos# ",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#ooooooooooooos#",
    "#ooooommmmoooos#",
    "#oooooommooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#oooooooooooooo#",
    "#oo#ooo##ooo#oo#",
    "#o# #oo##oo# #o#",
    " #   ##  ##   ##",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "
 }
};
// body light, body shade, outline per cause of death (how die() says it: 0 plain, 1 shock, 2 gravity, 3 a wall, 4 hunger, 5 worn out)
static const u16 fxGhostCol[6][3]={
    {RGB(27,29,31),RGB(17,21,28),RGB(10,13,22)}, {RGB(31,31,22),RGB(26,24,10),RGB(18,14,2)}, {RGB(22,31,31),RGB(12,24,28),RGB(4,14,22)},
    {RGB(31,25,27),RGB(27,16,20),RGB(18,6,10)},  {RGB(24,31,24),RGB(14,25,14),RGB(4,14,6)},  {RGB(28,25,31),RGB(20,16,28),RGB(10,6,20)} };
static const char* const fxBoo[6]={"BOO","BOO  SHOCKED","BOO  FELL","BOO  WALLED","BOO  STARVED","BOO  WORN OUT"};

static const char* const wxRainArt[8]={"      1 ","      1 ","     1  ","     1  ","     1  ","    1   ","    1   ","        "};
static const char* const wxSnowArt[8]={"        ","        ","   2    ","  222   ","   2    ","        ","        ","        "};
static const char* const wxSplArt[8]={"        ","        ","        ","        ","        ","  1  1  "," 1 11 1 ","  1  1  "};

static void fxTileArt(volatile u16*d,const char*const*rows){   // one 8 x 8 tile, 4 bits a pixel (low nibble = left pixel)
    for(int y=0;y<8;y++) for(int h=0;h<2;h++){ u16 v=0; for(int k=0;k<4;k++){ char c=rows[y][h*4+k]; int ci=c==' '?0:c-'0'; v|=(u16)(ci<<(k*4)); } d[y*2+h]=v; }
}
static void fxUpload(void){   // in vblank: the ghost frames, the three weather tiles, the palettes
    for(int f=0;f<2;f++){
        u16 buf[128]; for(int i=0;i<128;i++) buf[i]=0;
        for(int y=0;y<32;y++) for(int x=0;x<16;x++){
            char c=fxGhostArt[f][y][x]; int ci=c==' '?0:c=='#'?1:c=='o'?2:c=='s'?3:c=='e'?4:5;
            int tile=(y>>3)*2+(x>>3), i=tile*16+(y&7)*2+((x&7)>>2);
            buf[i]|=(u16)(ci<<(((x&7)&3)*4)); }
        volatile u16*d=OBJ_VRAM+(FX_TILE-512+f*8)*16; for(int i=0;i<128;i++) d[i]=buf[i]; }
    fxTileArt(OBJ_VRAM+(T_RAIN-512)*16,wxRainArt); fxTileArt(OBJ_VRAM+(T_SNOW-512)*16,wxSnowArt); fxTileArt(OBJ_VRAM+(T_SPLASH-512)*16,wxSplArt);
    for(int g=0;g<GH_MAX;g++){ const u16*c=fxGhostCol[g<fxGN?fxG[g].why:0]; volatile u16*p=OBJ_PAL+(FX_PALG+g)*16;
        p[0]=0; p[1]=c[2]; p[2]=c[0]; p[3]=c[1]; p[4]=RGB(2,2,6); p[5]=RGB(2,2,6); }
    { volatile u16*p=OBJ_PAL+FX_PALW*16; p[0]=0; p[1]=RGB(22,27,31); p[2]=RGB(31,31,31); p[3]=RGB(18,24,30); }
}

// ---- small helpers ----
static int fxOut(void){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8); if(tx<0||ty<0||tx>=MW||ty>=MH) return 1; return !wInside[ty][tx]; }   // you are outdoors
static int fxSeason(void){ if(nbT.tag[0]=='T'&&nbT.tag[1]=='W'&&nbT.tag[2]=='N'&&nbT.tag[3]=='1') return nbT.season&3; return (simDay/28)&3; }
static int fxAbs(int v){ return v<0?-v:v; }
static void fxScreen(s32 fx,s32 fy,int*sx,int*sy){   // feet on screen (the same maths as hhCalc, house.h)
    s32 rx,ry; rotPos(fx,fy,&rx,&ry); *sx=LOX+(int)((rx-ry)>>5); *sy=LOY+(int)((rx+ry)>>6)-surfH(fx,fy);
}

// ---- ghosts ----
static void fxGhostBorn(int why){
    if(!xo[XO_GHOSTS]) return;
    if(fxGN>=GH_MAX){ for(int i=1;i<GH_MAX;i++) fxG[i-1]=fxG[i]; fxGN=GH_MAX-1; }
    FxGhost*g=&fxG[fxGN++]; g->fx=lfx; g->fy=lfy; g->tx=(s16)(lfx>>8)*256+128; g->ty=(s16)(lfy>>8)*256+128;
    g->hx=(u8)(lfx>>8); g->hy=(u8)(lfy>>8); g->why=(u8)(why>5||why<0?0:why); g->age=0; g->cd=120; g->face=0;
    fxVramOk=0; simsSave();
}
static void fxGhostClear(void){ fxGN=0; fxVramOk=0; }
static void fxGhostSave(volatile unsigned char*m){   // 12 bytes: 'G', count, (x, y, how) x 3, checksum
    unsigned sum=0x47; m[0]='G'; m[1]=fxGN; sum+=fxGN;
    for(int i=0;i<GH_MAX;i++){ int ok=i<fxGN; m[2+i*3]=ok?fxG[i].hx:0; m[3+i*3]=ok?fxG[i].hy:0; m[4+i*3]=ok?fxG[i].why:0; sum+=m[2+i*3]+m[3+i*3]+m[4+i*3]; }
    m[11]=(unsigned char)sum;
}
static void fxGhostLoad(volatile unsigned char*m){
    unsigned sum=0x47; fxGN=0; fxVramOk=0;
    if(m[0]!='G'||m[1]>GH_MAX) return;
    sum+=m[1]; for(int i=2;i<=10;i++) sum+=m[i];
    if(m[11]!=(unsigned char)sum) return;
    for(int i=0;i<m[1];i++){ int x=m[2+i*3], y=m[3+i*3], w=m[4+i*3]; if(x>=MW||y>=MH||w>5) continue;
        FxGhost*g=&fxG[fxGN++]; g->hx=(u8)x; g->hy=(u8)y; g->why=(u8)w; g->fx=x*256+128; g->fy=y*256+128; g->tx=(s16)g->fx; g->ty=(s16)g->fy; g->age=255; g->cd=240; g->face=0; }
}
static void fxGhostTick(void){
    for(int i=0;i<fxGN;i++){ FxGhost*g=&fxG[i];
        if(g->age<255) g->age++;
        int dx=g->tx-(int)g->fx, dy=g->ty-(int)g->fy, ax=fxAbs(dx), ay=fxAbs(dy);
        if(ax+ay<200||((fxT&511)==(unsigned)(i*170+60))){   // a new place to drift to, near the home tile (ghosts pass through walls)
            int nx=(int)g->hx-6+rnd8()%13, ny=(int)g->hy-6+rnd8()%13; if(nx<1) nx=1; if(ny<1) ny=1; if(nx>MW-2) nx=MW-2; if(ny>MH-2) ny=MH-2;
            g->tx=(s16)(nx*256+128); g->ty=(s16)(ny*256+128); }
        else { int mx, my; if(ax>=ay){ mx=dx<0?-3:3; my=ax?dy*3/ax:0; } else { my=dy<0?-3:3; mx=ay?dx*3/ay:0; }
               if(g->age>=60){ g->fx+=mx; g->fy+=my; } if(mx) g->face=(u8)(dx<0); }
        if(g->cd){ g->cd--; continue; }
        if(!xo[XO_GHOSTS]) continue;
        // haunting: someone close enough gets a scare, then the ghost leaves them alone for a while
        int scared=0;
        if(!ldead&&fxAbs((int)((lfx-g->fx)>>8))+fxAbs((int)((lfy-g->fy)>>8))<=4){
            moodEvent(M_SPOOK); lnote=fxBoo[g->why]; lnoteT=70; sfxPlay(SFX_GHOST); scared=1; }
        for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; if(s->act==HA_AWAY) continue;
            if(fxAbs((int)((s->fx-g->fx)>>8))+fxAbs((int)((s->fy-g->fy)>>8))<=3){ s->bub=IC_SKULL; s->bubT=80; scared=1; } }
        g->cd=(u8)(scared?240:90);   // (a u8: about 4 s after a scare at 60 steps a second, about 1.5 s when nobody was near)
    }
}

// ---- weather ----
static u8 wxRoll(unsigned key){   // the weather of a six-hour block: weights out of 16 per season (spring summer fall winter)
    static const u8 w[4][6]={ {6,3,1,4,2,0}, {10,2,0,2,2,0}, {4,4,3,4,1,0}, {5,4,2,0,0,5} };
    unsigned h=key*2654435761u; h^=h>>15; h*=2246822519u; h^=h>>13;
    int r=(int)((h>>7)&15), s=fxSeason();
    for(int k=0;k<6;k++){ if(r<w[s][k]) return (u8)k; r-=w[s][k]; }
    return WX_CLEAR;
}
static u8 wxNow(void){
    int o=xo[XO_WEATHER]; if(o>=1) return (u8)(o==1?WX_CLEAR:o==2?WX_CLOUDY:o==3?WX_FOG:o==4?WX_RAIN:o==5?WX_STORM:WX_SNOW);   // forced
    return wxRoll((unsigned)simDay*4u+(unsigned)(simMin/360));
}
static int wxTarget(void){ return wx==WX_RAIN?24:wx==WX_STORM?WX_N:wx==WX_SNOW?26:0; }   // particles wanted
static int fxSpot(int i){   // a spot on screen, outdoors, for particle i
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int a=0;a<4;a++){
        int tx=px-14+rnd8()%29, ty=py-14+rnd8()%29; if(tx<1||ty<1||tx>=MW-1||ty>=MH-1) continue;
        if(wInside[ty][tx]||lifeMap[ty][tx]=='W') continue;
        int sx,sy; fxScreen(tx*256+128,ty*256+128,&sx,&sy);
        if(sx<vpX0-6||sx>vpX1+6||sy<sbY0+8||sy>sbY1+4) continue;
        wxTx[i]=(u8)tx; wxTy[i]=(u8)ty; wxPh[i]=(u8)(rnd8()&7); return 1; }
    wxPh[i]=255; return 0;
}
#define WX_FALL  9      // rain: steps in the air (7 px a step from 63 px up), then 4 of splash
#define WX_SNOWL 50     // snow: steps in the air (4 px per 3 steps from 66 px up)
static void wxMinute(void){   // once a game minute
    wx=wxNow();
    if(wx==WX_STORM&&wxThT<=0) wxThT=(short)(300+rnd8()*4);
    if((wx==WX_RAIN||wx==WX_STORM||wx==WX_SNOW)&&!ldead&&fxOut()&&++fxSoak>=25){ fxSoak=0; moodEvent(M_SOAKED); lnote=wx==WX_SNOW?"FREEZING":"SOAKED"; lnoteT=60; }
}
static void wxTick(void){
    int tg=wxTarget(); if(wxN<tg&&(fxT&3)==0) wxN++; else if(wxN>tg&&(fxT&1)==0) wxN--;
    for(int i=0;i<wxN;i++){ int snow=wx==WX_SNOW, lim=snow?WX_SNOWL+2:WX_FALL+4;
        if(wxPh[i]==255){ fxSpot(i); continue; }
        if(++wxPh[i]>=lim) fxSpot(i); }
    // lightning and thunder
    if(wx==WX_STORM){ if(--wxThT<=0){ wxFlash=12; wxBoom=(short)(10+rnd8()%50); wxThT=(short)(400+rnd8()*5); } }
    if(wxFlash) wxFlash=(u8)(wxFlash>8?wxFlash-1:wxFlash>0?wxFlash-(fxT&1):0);
    if(wxBoom>0&&--wxBoom==0) sfxPlay(SFX_THUNDER);
    // the dimming eases to its target (half as strong indoors)
    static const signed char dim[6]={0,2,-4,3,5,-2};
    int t=dim[wx]*16; if(!fxOut()) t/=2; if(curFl) t=0;
    if(wxLvl<t) wxLvl++; else if(wxLvl>t) wxLvl--;
}
static void fxLight(void){   // in vblank, after hhObjUpdate set BLDCNT for the sprites: darken (3) or brighten (2) the picture and the opaque sprites
    volatile u16*bldcnt=(volatile u16*)0x04000050; volatile u16*bldy=(volatile u16*)0x04000054;
    if(wxFlash){ *bldcnt=0x0400|0x14|0x80; *bldy=wxFlash; }
    else if(wxLvl>15){ *bldcnt=0x0400|0x14|0xC0; *bldy=(u16)(wxLvl>>4); }
    else if(wxLvl<-15){ *bldcnt=0x0400|0x14|0x80; *bldy=(u16)((-wxLvl)>>4); }
    else *bldy=0;
}

// ---- per step (called from the life loop, even while you lie dead) ----
static void fxTick(void){
    fxT++;
    if(simMin!=fxLastMin){ fxLastMin=simMin; wxMinute(); }
    fxGhostTick(); wxTick(); npcTick();
}
static void fxPlayStart(void){   // play begins (or returns from a menu): reload the art, pick the weather, raise a ghost if the house is HAUNTED
    fxVramOk=0; fxT=0; fxLastMin=-1; wxN=0; wxFlash=0; wxBoom=0; wxThT=0; wxLvl=0; wx=wxNow(); fxSoak=0;
    for(int i=0;i<WX_N;i++) wxPh[i]=255;
    npcPlayStart();
    if(xo[XO_GHOSTS]==2&&fxGN==0){ int hx=(int)(lfx>>8)+3, hy=(int)(lfy>>8)+2; if(hx>MW-2) hx=MW-2; if(hy>MH-2) hy=MH-2;
        FxGhost*g=&fxG[fxGN++]; g->hx=(u8)hx; g->hy=(u8)hy; g->why=0; g->fx=hx*256+128; g->fy=hy*256+128; g->tx=(s16)g->fx; g->ty=(s16)g->fy; g->age=255; g->cd=120; g->face=0; }
}

// ---- in vblank, right after hhObjUpdate ----
static void fxObjUpdate(void){
    volatile u16*oam=OAM; int i;
    if(!fxVramOk){ fxUpload(); fxVramOk=1; }
    fxLight();
    int hide=(lcamF>0)||curFl||zoomDma;
    for(i=0;i<GH_MAX;i++){ volatile u16*e=oam+(FX_OAM0+i)*4; e[0]=0x200;
        if(hide||i>=fxGN||!xo[XO_GHOSTS]) continue;
        const FxGhost*g=&fxG[i];
        if(!(simIsNight()||((fxT>>5)&3)!=0||g->cd>200)) continue;   // by day it flickers out now and then
        int sx,sy; fxScreen(g->fx,g->fy,&sx,&sy);
        int rise=g->age<60?g->age*14/60:14, ph=(fxT>>2)&15, bob=ph<8?ph:15-ph;
        int x=sx-8, y=sy-(6+rise+bob)-27;
        if(x+16<=vpX0||x>=vpX1||y+32<=sbY0||y>=sbY1) continue;
        e[0]=(u16)((y&255)|0x400|0x8000); e[1]=(u16)((x&511)|0x8000|(g->face?0x1000:0)); e[2]=(u16)((FX_TILE+(((fxT>>4)&1)?8:0))|((FX_PALG+i)<<12)); }
    for(i=0;i<WX_N;i++){ volatile u16*e=oam+(FX_OAM0+GH_MAX+i)*4; e[0]=0x200;
        if(hide||i>=wxN||wxPh[i]==255) continue;
        int sx,sy; fxScreen(wxTx[i]*256+128,wxTy[i]*256+128,&sx,&sy);
        int ph=wxPh[i], x, y, tile;
        if(wx==WX_SNOW){ if(ph>=WX_SNOWL) continue; int h=66-ph*4/3; x=sx-4+((ph>>3)&1?1:-1)*((ph&7)>>1)+h/8; y=sy-h-8; tile=T_SNOW; }
        else { int h=63-ph*7; if(h<0){ if(ph>=WX_FALL+4) continue; x=sx-4; y=sy-8; tile=T_SPLASH; } else { x=sx-4+h/8; y=sy-h-8; tile=T_RAIN; } }
        if(x+8<=vpX0||x>=vpX1||y+8<=sbY0||y>=sbY1) continue;
        e[0]=(u16)((y&255)|0x400); e[1]=(u16)(x&511); e[2]=(u16)(tile|(FX_PALW<<12)); }
    npcObjUpdate();   // the AI skaters and the cop
}

// ---- the HUD (hud.h): a tiny weather sign over the sun / moon of the clock ----
static int fxWxHud(void){ return wx; }
static void fxWxIcon(int x,int y){
    if(wx==WX_CLEAR) return;
    rect(x-1,y-1,9,8,HC_BG0);
    u16 gray=wx==WX_STORM?RGB(14,14,18):RGB(20,22,26);
    if(wx==WX_FOG){ rect(x,y+1,7,1,gray); rect(x+1,y+3,7,1,gray); rect(x,y+5,7,1,gray); return; }
    rect(x+1,y+1,5,3,gray); rect(x+2,y,3,1,gray); rect(x,y+2,7,2,gray);
    if(wx==WX_RAIN){ px(x+1,y+5,RGB(10,20,31)); px(x+3,y+5,RGB(10,20,31)); px(x+5,y+5,RGB(10,20,31)); }
    else if(wx==WX_STORM){ rect(x+3,y+4,1,1,RGB(31,28,4)); rect(x+2,y+5,2,1,RGB(31,28,4)); px(x+3,y+6,RGB(31,28,4)); }
    else if(wx==WX_SNOW){ px(x+1,y+5,WHITE); px(x+3,y+6,WHITE); px(x+5,y+5,WHITE); }
}

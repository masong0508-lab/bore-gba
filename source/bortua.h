// bortua.h - BORTUA FIGHTER's MODE: ARCADE MODE. One on one on a ring at sunset over the sea, best of three, with PRE-RENDERED 3D fighters.
//   YOU      your own Bore (your name, skin, hair, top and bottom colours; a slim build if you are a woman, a heavy one if you are heavy) against a
//            ladder of eight, then the chrome boss BORAL.
//   BUTTONS  A punch, B kick, R (or L) guard, DOWN crouch (DOWN + A a low jab, DOWN + B a sweep, DOWN + R guards low), UP jump (A / B in the air),
//            toward + A the BORE PALM, away + B the SPIN KICK, A + B together up close a THROW. START pauses.
//   RULES    a round is 30 seconds; empty the bar for a K.O., knock them off the ring for a RING OUT, or have more left when TIME is up. Standing
//            guard stops high and mid blows, crouching dodges the high ones and its guard stops the low ones; throws go through guard (not
//            through a crouch or a jump). A blow that lands while they wind up is a COUNTER (half again as hard); blows that land before they
//            recover from the last one are a COMBO.
//   THE MOON (hidden; grown-up Bores only, never a teen or a child, and nobody moons a younger Bore) UP UP DOWN DOWN LEFT RIGHT LEFT RIGHT
//            B A in a fight arms it for the session (a tick says so): from then on about one round in 128 (rolled at each round's start, and
//            again the moment the code goes in) has a moon in it, yours or theirs, somewhere in the round. SUPER RARE on purpose: a player
//            sees a few of these in a lifetime, the burst maybe once. A fighter turns round, drops their pants and moons them (and you). Bare
//            or in underwear is a coin flip each time. If the pants come down before they reach you, they recoil (MOONED!: an opening); hit someone mid-moon and it counts double
//            (SPANKED). Moon a fighter and they will pay you back, and a fighter who wins a round now and then moons you anyway.
//   JIGGLE   a fighter with a seat (your BUTT switch and sliders: none, round or big) jiggles now and then: landing a jump, shaking off a
//            hit, standing about, and in the moon. A moon ends one of four ways, a quarter each: a wiggle; a slap and a bounce; the TOOT (they
//            freeze, one slips out, the cheeks shake like jelly, a green cloud drifts up, the screen shakes, and the pants come up in a hurry;
//            the smell holds whoever is near even longer than the moon); or the HELIUM (side on, a little tank out, a long pull on the hose,
//            the belly balloons, then it all comes out in one shaking TOOT and they go red; every voice, sound and note of music stays a
//            squeaky fifth higher for the rest of the match, and the red face stays as long: audHe in main.c). The hose goes to the mouth or, a coin flip, the back way
//            (seen from behind: a look over the shoulder, a jolt as it goes in, the fill, the hose pops loose, the release). One helium moon in
//            three goes too far: the belly keeps swelling (the seat with it, the head riding up on the ball) until they BURST: a flash, a bang,
//            bits everywhere, a puddle with the head and the shoes in it. A burst fighter loses the round and the match on the spot.
//            A burst gets a send-off: WHAT THE FUCK JUST HAPPENED??? in big wobbly comic letters (26 sprites, every colour of the rainbow,
//            jittering), then the screen fades to black and the match is over.
//            None of this happens until the code has been put in during this arcade session: before that nobody moons anybody.
//   RECORD   every match won counts (bfWins, the pack's missions); beating BORAL makes you BORTUA CHAMPION (bfChamp). Saved at BF_OFF and with
//            your player (chunk 'A').
// LOOKS    the Donkey Kong Country trick: tools/make_bortua.py ray casts shaded 3D models (key light, rim light, a shine, outlines) into every frame
//          of every move for three bodies (bortuaart.h), and paints the arena. Here the fighters are hardware sprites (two 64 x 64 each, tiles
//          512.. of the bitmap mode) over the arena picture, coloured by their own palette, mirrored when they face left; a frame's tiles are
//          copied in only when it changes. Shadows are see-through sprites, sparks fly where a blow lands (blue on a guard), a big hit stops the
//          fight for a beat and shakes the screen, a K.O. flashes white and plays in slow motion. The CPU draws only the HUD and the words, so it
//          runs at a true 60.
#include "bortuaart.h"
#include "bortuawtf.h"   // the BURST's send-off letters
#define BF_SLOT0 26               // the fight's sprites start at this OAM slot: the send-off's 26 letters sit in front of them (slots 0..25)
#define BF_TWTF (BF_TFX)          // the letters' tiles go where the sparks and the shadows were (loaded again by the next card)
#define BF_LEN 8
#define BF_GROUND BFA_GROUND
#define BF_T0 512                 // fighter i's tiles: BF_T0 + i*128 (two 64 x 64 sprites)
#define BF_TFX (BF_T0+256)        // the spark's four frames (16 tiles each), then the shadows (4 tiles, 2 tiles)
#define BF_TSH (BF_TFX+64)
#define BF_TGAS (BF_TSH+6)        // the TOOT's cloud: three 32 x 32 frames
#define BF_FXB 3776               // bytes of bortua_fx.bin
#define BF_BODY 8                 // how far in front of its root a fighter can be hit (px)
#define BF_SEP 34                 // the closest two fighters stand
#define BF_HUDH 30
#define R_BG2X (*(volatile u32*)0x04000028)
#define R_BG2Y (*(volatile u32*)0x0400002C)
#define R_BLDCNT (*(volatile u16*)0x04000050)
#define R_BLDAL (*(volatile u16*)0x04000052)
#define R_BLDY (*(volatile u16*)0x04000054)
static u8 bfSum(volatile const u8*m){ return (u8)(0x46+m[2]*3+m[3]*5+m[4]*7); }
static void bfSave(void){ volatile u8*m=SRAM_BASE+BF_OFF; m[0]='B'; m[1]='F'; m[2]=bfWins; m[3]=bfChamp; m[4]=0; m[5]=0; m[6]=0; m[7]=bfSum(m); }
static int bfLoadFrom(volatile const u8*m){ if(m[0]!='B'||m[1]!='F'||m[7]!=bfSum(m)) return 0; bfWins=m[2]; bfChamp=m[3]?1:0; return 1; }
static void bfLoad(void){ bfWins=0; bfChamp=0; bfLoadFrom(SRAM_BASE+BF_OFF); }
static void sgEncArcade(SlW*w){ bfSave(); volatile u8*m=SRAM_BASE+BF_OFF; for(int i=0;i<BF_LEN;i++) slwPut(w,m[i]); }
// ---- the moves: timing (frames of the fight), what they do, and which pre-rendered frames show them ----
enum { MV_PUNCH, MV_KICK, MV_LPUNCH, MV_LKICK, MV_PALM, MV_SPIN, MV_THROW, MV_N };
typedef struct { const char*nm; u8 start, act, rec, dmg, stun, kb, lvl, an, n, ai, aa; } BfMv;   // lvl 0 high 1 mid 2 low 3 throw; an: its first frame, n frames,
static const BfMv bfMv[MV_N]={                                                                   //  ai: the first that hits, aa: how many hit
    {"JAB",3,3,8,6,14,2,0,BFA_PUNCH,4,2,1}, {"KICK",6,4,12,9,18,4,1,BFA_KICK,5,2,2}, {"LOW JAB",4,3,9,5,12,1,2,BFA_LPUNCH,3,1,1}, {"SWEEP",6,4,14,8,20,3,2,BFA_SWEEP,4,2,1},
    {"BORE PALM",9,4,16,15,26,10,1,BFA_PALM,4,2,1}, {"SPIN KICK",11,5,18,14,26,12,0,BFA_SPIN,6,4,2}, {"THROW",3,2,22,18,40,14,3,BFA_THROW,4,1,1} };
static int bfReach(int mv){ const BfMv*m=&bfMv[mv]; int r=0; for(int i=0;i<m->aa;i++){ int q=bfaReach[m->an+m->ai+i]; if(q>r) r=q; } return r; }   // (measured off the renders)
static int bfAtkFr(const BfMv*m,int t){   // the frame a move shows t frames in: the wind-up frames spread over its start, then the hitting ones, then the way back
    if(t<m->start) return m->an+t*m->ai/m->start;
    t-=m->start; if(t<m->act) return m->an+m->ai+t*m->aa/m->act;
    t-=m->act; int nr=m->n-m->ai-m->aa; if(nr<=0) return m->an+m->n-1;
    int k=t*nr/m->rec; return m->an+m->ai+m->aa+(k<nr?k:nr-1);
}
typedef struct { const char*nm; u16 skin, top, bot, hair, acc; u8 body, butt, pw, sp, d; } BfWho;
static const BfWho bfLadder[9]={
    {"AKIRA YAWNKI",RGB(27,20,15),RGB(29,29,28),RGB(29,29,28),RGB(3,3,4),RGB(4,4,6),BFB_STANDARD,BFS_ROUND,100,16,0},
    {"PAI CHAN-MEH",RGB(29,23,18),RGB(6,12,28),RGB(6,12,28),RGB(3,3,4),RGB(30,30,30),BFB_SLIM,BFS_ROUND,90,18,1},
    {"LAU SNOOZE",RGB(27,21,16),RGB(28,26,20),RGB(9,9,11),RGB(28,28,28),RGB(26,20,6),BFB_STANDARD,BFS_ROUND,105,15,2},
    {"WOLF HAWKYAWN",RGB(26,18,13),RGB(22,4,4),RGB(4,6,14),RGB(20,12,4),RGB(30,24,4),BFB_HEAVY,BFS_ROUND,120,13,3},
    {"JEFFRY MCSNORE",RGB(18,11,7),RGB(28,22,6),RGB(10,8,4),RGB(4,3,2),RGB(28,8,4),BFB_HEAVY,BFS_BIG,125,12,4},
    {"KAGE-MARU-ZZZ",RGB(26,19,14),RGB(10,4,16),RGB(10,4,16),RGB(2,2,3),RGB(30,30,30),BFB_STANDARD,BFS_ROUND,100,19,5},
    {"SARAH BORYANT",RGB(30,23,18),RGB(28,6,6),RGB(4,4,6),RGB(31,26,10),RGB(30,30,30),BFB_SLIM,BFS_BIG,105,18,6},
    {"JACKY BORYANT",RGB(29,22,17),RGB(4,4,6),RGB(28,6,6),RGB(31,26,10),RGB(30,24,4),BFB_STANDARD,BFS_ROUND,110,17,7},
    {"BORAL",0,0,0,0,0,BFB_STANDARD,BFS_FLAT,135,18,9} };   // (chrome all over: bfPalMake)
enum { FS_IDLE, FS_WALKF, FS_WALKB, FS_GUARD, FS_CROUCH, FS_ATK, FS_HIT, FS_BLOCK, FS_JUMP, FS_KO, FS_WIN, FS_THROWN, FS_TAUNT };
typedef struct { s32 x, kbv; int y, vy, face, hp, trail, trailT, st, t, mv, cnt, wins, aiT, aiDo, fr, shown, body, butt, flash, jigT;
                 u8 pw, sp, d, jatk, lowG, combo, adult, ts, mooned, bare, blush, hoseB, popped; /* (ts: how the moon ends, 0 wiggle 1 slap 2 toot 3 helium 4 helium and burst) */ u16 botc; const char*nm; u16 pal[16], palM[16]; } Bf;   // (palM: the moon's colours)
static Bf bfF[2] EWRAM_BSS; static u32 bfPics EWRAM_BSS;   // (pictures shown: for measuring)
typedef struct { int x, y, t, blk; } BfSpark;
static BfSpark bfSp[2], bfGas;   // (bfGas: the TOOT's cloud; t counts down from 36)
static int bfStop, bfShake, bfFlashT, bfSlow;   // the hit stop (frames the fight stands still), the shake, the K.O. flash, the slow motion
static int bfWtfT, bfFadeY;   // the BURST's send-off: frames the letters have been up (0: not up), and the fade to black (0..16)
// ---- colours ----
static u16 bfMul(u16 c,int k){ int r=(c&31)*k>>4, g=((c>>5)&31)*k>>4, b=((c>>10)&31)*k>>4; return RGB(r>31?31:r,g>31?31:g,b>31?31:b); }
static u16 bfLift(u16 c){ int r=c&31, g=(c>>5)&31, b=(c>>10)&31; r=r*5/4+3; g=g*5/4+3; b=b*5/4+3; return RGB(r>31?31:r,g>31?31:g,b>31?31:b); }
static void bfRamp(u16*p,u16 c){ p[0]=bfMul(c,9); p[1]=c; p[2]=bfLift(c); }   // dark, mid, light (the renders' three shades of a material)
static void bfPalMake(Bf*f,u16 skin,u16 top,u16 bot,u16 hair,u16 acc,int chrome){
    u16*p=f->pal; p[0]=0; p[1]=RGB(2,1,3);
    if(chrome){ for(int i=0;i<3;i++){ p[2+i*3]=RGB(6,7,10); p[3+i*3]=RGB(19,21,25); p[4+i*3]=RGB(30,31,31); } p[11]=RGB(10,11,14); p[12]=RGB(26,28,31); p[13]=RGB(8,9,12); p[14]=RGB(31,26,8); p[15]=RGB(31,31,31); return; }
    bfRamp(p+2,skin); bfRamp(p+5,top); bfRamp(p+8,bot); p[11]=bfMul(hair,10); p[12]=hair;
    if((hair&31)+((hair>>5)&31)+((hair>>10)&31)<15) p[12]=RGB(7,7,9);   // (black hair still shows its shape)
    p[13]=RGB(5,4,5); p[14]=acc; p[15]=RGB(31,30,26);
}
static void bfPalSend(int i){   // a fighter's colours into its sprite palette (a white flash while it is hit; the moon's own with the pants down)
    const Bf*f=&bfF[i]; volatile u16*q=OBJ_PAL+i*16; const u16*src=(f->fr>=BFA_PANTS&&f->fr<BFA_MOON+BFA_MOON_N)?f->palM:f->pal;
    if(f->flash>0) for(int c=0;c<16;c++){ u16 v=src[c]; q[c]=c<2?v:(u16)(((v>>1)&0x3DEF)+RGB(15,15,15)); }
    else for(int c=0;c<16;c++) q[c]=src[c];
    if(f->blush) for(int c=2;c<5;c++){ u16 v=src[c]; int r=(v&31)+9, g=((v>>5)&31)*9/16, b=((v>>10)&31)*9/16; q[c]=RGB(r>31?31:r,g,b); }   // (red faced, and it stays that way till the match ends: the skin's three shades go red)
    if(f->fr>=BFA_MOON+31&&f->fr<BFA_MOON+34){   // the BURST frames: what was the top and the bottom is blood and meat
        static const u16 blood[3]={RGB(22,1,1),RGB(29,3,3),RGB(31,10,8)}, meat[3]={RGB(12,1,3),RGB(18,2,5),RGB(24,5,8)};
        for(int c=0;c<3;c++){ q[5+c]=blood[c]; q[8+c]=meat[c]; } }
}
static const u16 bfUndies[5]={RGB(29,29,30),RGB(31,16,22),RGB(28,4,6),RGB(10,20,31),RGB(30,24,6)};   // white, pink, red, sky blue, gold
static void bfMoonPal(Bf*f){   // the pants-down colours: the seat bare or in underwear, a coin flip (only grown-ups are ever bare), the pants round the ankles
    for(int c=0;c<16;c++) f->palM[c]=f->pal[c];
    f->bare=(u8)(f->adult&&(rnd8()&1));
    if(f->bare) for(int c=0;c<3;c++) f->palM[8+c]=f->pal[2+c];   // (the skin's three shades)
    else bfRamp(f->palM+8,bfUndies[rnd8()%5]);
    f->palM[13]=bfMul(f->botc,9); f->palM[14]=f->botc;
}
static void bfFxPal(void){
    static const u16 hit[8]={0,RGB(16,4,0),RGB(24,8,0),RGB(30,14,2),RGB(31,20,4),RGB(31,26,10),RGB(31,30,20),RGB(31,31,31)},
                     blk[8]={0,RGB(2,6,16),RGB(4,10,24),RGB(8,16,30),RGB(14,22,31),RGB(20,27,31),RGB(26,30,31),RGB(31,31,31)},
                     gas[8]={0,RGB(8,12,2),RGB(12,17,3),RGB(16,22,5),RGB(20,25,8),RGB(24,28,12),RGB(27,30,18),RGB(30,31,24)},
                     red[8]={0,RGB(10,0,0),RGB(16,1,1),RGB(22,2,2),RGB(27,4,3),RGB(31,8,6),RGB(31,14,10),RGB(31,22,18)};
    for(int c=0;c<8;c++){ OBJ_PAL[2*16+c]=hit[c]; OBJ_PAL[3*16+c]=blk[c]; OBJ_PAL[5*16+c]=gas[c]; OBJ_PAL[6*16+c]=red[c]; } OBJ_PAL[4*16]=0; OBJ_PAL[4*16+1]=0;
    static const u16 rb[9]={RGB(31,4,4),RGB(31,16,2),RGB(31,30,4),RGB(8,31,6),RGB(4,28,28),RGB(6,12,31),RGB(18,6,31),RGB(31,6,26),RGB(31,31,31)};   // the send-off's rainbow (banks 7..15)
    for(int b=0;b<9;b++){ volatile u16*q=OBJ_PAL+(7+b)*16; u16 c=rb[b]; q[0]=0; q[1]=RGB(2,1,4); q[2]=c; q[3]=(u16)(((c>>1)&0x3DEF)+RGB(15,15,15)); }
}
// ---- the sprites ----
static void bfObj(int i,int x,int y,u16 shape,u16 size,int tile,int pal,int hf,int semi){
    volatile u16*o=OAM+i*4;
    if(y>=SH||y<=-128||x>=SW||x<=-64){ o[0]=0x200; return; }
    o[0]=(u16)((y&255)|shape|(semi?0x400:0)); o[1]=(u16)((x&511)|size|(hf?0x1000:0)); o[2]=(u16)(tile|(pal<<12));
}
static void bfTiles(int i){   // fighter i's frame into its tiles, if it is not there already
    Bf*f=&bfF[i]; if(f->shown==f->fr) return;
    REG_DMA3SAD=(u32)(uintptr_t)(bfaFighters+(u32)((f->body*BFA_BUTTS+f->butt)*BFA_NFR+f->fr)*(BFA_FRB/4)); REG_DMA3DAD=(u32)(uintptr_t)(OBJ_VRAM+i*128*16); REG_DMA3CNT=(BFA_FRB/4)|0x84000000u;
    f->shown=f->fr;
}
static void bfWtfTiles(void){ REG_DMA3SAD=(u32)(uintptr_t)bfaWtf; REG_DMA3DAD=(u32)(uintptr_t)(OBJ_VRAM+(BF_TWTF-512)*16); REG_DMA3CNT=(BFA_WTF_N*8*32/4)|0x84000000u; }
static void bfFxTiles(void){ REG_DMA3SAD=(u32)(uintptr_t)bfaFx; REG_DMA3DAD=(u32)(uintptr_t)(OBJ_VRAM+(BF_TFX-512)*16); REG_DMA3CNT=(BF_FXB/4)|0x84000000u; }
static void bfPlace(int i,int slot,int sx,int sy){   // fighter i's two sprites at OAM slot, slot+1 (sx, sy: its root on the screen)
    const Bf*f=&bfF[i]; int ax=bfaAx[(f->body*BFA_BUTTS+f->butt)*BFA_NFR+f->fr], hf=f->face<0, x=hf?sx-(BFA_W-1-ax):sx-ax, y=sy-BFA_AY, t=BF_T0+i*128;
    bfObj(slot,x,y,0,0xC000,t,i,hf,0); bfObj(slot+1,x,y+64,0,0xC000,t+64,i,hf,0);
}
static void bfShow(int fighters){   // in the vblank: frames, palettes, sprites, the shake and the flash (fighters 0: the fight's sprites off)
    int dx=0, dy=0; if(bfShake>0){ dx=(bfShake&2)?2:-2; dy=(bfShake&1)?1:-1; }
    R_BG2X=(u32)(dx<<8); R_BG2Y=(u32)(dy<<8);   // (the picture moves one way, the sprites the other)
    if(bfFlashT>0){ R_BLDCNT=0x04BF; R_BLDY=(u16)(bfFlashT>8?16:bfFlashT*2); } else if(bfFadeY>0){ R_BLDCNT=0x00FF; R_BLDY=(u16)bfFadeY; } else { R_BLDCNT=0x0400; R_BLDY=0; }
    if(!fighters){ for(int s=0;s<BF_SLOT0+9;s++) OAM[s*4]=0x200; return; }
    for(int i=0;i<2;i++){ bfTiles(i); bfPalSend(i); }
    if(bfWtfT>0){   // the send-off: every letter its own sprite, its colour walking down the rainbow, a fresh jitter every frame
        for(int i=0;i<BFA_WTF_N;i++){ int l=bfaWtfL[i], jx=(rnd8()&3)-1, jy=(rnd8()&3)-1;
            bfObj(i,120-(l?BFA_WTF_W1:BFA_WTF_W0)/2+bfaWtfX[i]+jx-dx,(l?82:42)+jy-dy,0x4000,0x8000,BF_TWTF+i*8,7+(i+bfWtfT/3)%9,0,0); } }
    else for(int i=0;i<BFA_WTF_N;i++) OAM[i*4]=0x200;
    int s=BF_SLOT0;
    if(bfGas.t>0){ int e=36-bfGas.t; bfObj(s,bfGas.x-16-dx,bfGas.y-16-dy-e/3,0,0x8000,BF_TGAS+(e/12)*16,5,e&8,0); } else OAM[s*4]=0x200;   // (the cloud in front of everything)
    s++;
    for(int i=0;i<2;i++){ BfSpark*k=&bfSp[i]; if(k->t>0&&k->t<=8) bfObj(s,k->x-16-dx,k->y-16-dy,0,0x8000,BF_TFX+((8-k->t)>>1)*16,k->blk==2?6:k->blk?3:2,i,0); else OAM[s*4]=0x200; s++; }
    int top=bfF[1].st==FS_ATK&&bfF[0].st!=FS_ATK;   // (the one hitting in front)
    for(int n=0;n<2;n++){ int i=top?1-n:n; const Bf*f=&bfF[i]; bfPlace(i,s,(int)(f->x>>8)-dx,BF_GROUND+f->y-dy); s+=2; }
    for(int i=0;i<2;i++){ const Bf*f=&bfF[i]; int sx=(int)(f->x>>8)-dx, air=f->y<-24;
        if(f->y>4||bfWtfT) OAM[s*4]=0x200; else if(air) bfObj(s,sx-8,BF_GROUND-4-dy,0x4000,0,BF_TSH+4,4,0,1); else bfObj(s,sx-16,BF_GROUND-4-dy,0x4000,0x4000,BF_TSH,4,0,1); s++; }   // (no shadows under the send-off: its letters took their tiles)
}
// ---- the arena (straight from ROM) and the words over it ----
static void bfArRows(int y0,int y1){ REG_DMA3SAD=(u32)(uintptr_t)(bfaArena+y0*SW); REG_DMA3DAD=(u32)(uintptr_t)(fb+y0*SW); REG_DMA3CNT=(u32)((y1-y0)*SW/2)|0x84000000u; }
static void bfDim(int y0,int y1){ for(u32*p=(u32*)(fb+y0*SW),*e=(u32*)(fb+y1*SW);p<e;p++) *p=(*p>>1)&0x3DEF3DEFu; }   // (half as bright: behind the cards)
static void bfText(int x,int y,const char*s,u16 c,int sc){ u16 k=RGB(1,1,3); text(x-1,y,s,k,sc); text(x+1,y,s,k,sc); text(x,y-1,s,k,sc); text(x,y+sc,s,k,sc); text(x,y,s,c,sc); }   // outlined
static void bfTextC(int y,const char*s,u16 c,int sc){ bfText(120-tw(s,sc)/2,y,s,c,sc); }
static void bfBar(int x,int w){ if(w<=0) return; rect(x,11,w,2,RGB(31,30,16)); rect(x,13,w,2,RGB(31,25,4)); rect(x,15,w,2,RGB(25,16,2)); }
static int bfShownHp[2]={-1,-1}, bfShownTr[2]={-1,-1}, bfShownT=-1, bfShownW=-1;
static void bfHudBars(int secs){   // the bars, the round marks and the clock: cheap rectangles (the clock's digits only when they change: text is slow)
    for(int i=0;i<2;i++){ const Bf*f=&bfF[i]; int x=i?132:8, w=100, fill=f->hp*w/100, tr=f->trail*w/100;
        rect(x-2,9,w+4,10,RGB(1,1,2)); rect(x-1,10,w+2,8,RGB(22,17,5)); rect(x,11,w,6,RGB(5,2,4));
        if(tr>0){ if(i) rect(x+w-tr,11,tr,6,RGB(24,3,3)); else rect(x,11,tr,6,RGB(24,3,3)); }
        bfBar(i?x+w-fill:x,fill);
        for(int r=0;r<2;r++){ int cx=i?x+w-5-r*10:x+5+r*10; rect(cx-3,21,7,5,RGB(1,1,2)); rect(cx-2,22,5,3,r<f->wins?RGB(31,24,4):RGB(6,5,8)); } }
    if(secs!=bfShownT){ rect(110,6,20,18,RGB(1,1,2)); rect(111,7,18,16,RGB(6,3,10)); char b[4]; slNum(b,secs); text(120-tw(b,2)/2,8,b,secs<=5?RGB(31,8,6):WHITE,2); }
    for(int i=0;i<2;i++){ bfShownHp[i]=bfF[i].hp; bfShownTr[i]=bfF[i].trail; } bfShownT=secs; bfShownW=bfF[0].wins*4+bfF[1].wins;
}
static void bfHud(int secs){   // the whole HUD, names too (ten outlined text calls: once a round, never per frame)
    bfArRows(0,BF_HUDH); bfShownT=-1;
    for(int i=0;i<2;i++){ const Bf*f=&bfF[i]; int x=i?132:8, w=100; bfText(i?x+w-tw(f->nm,1):x,1,f->nm,WHITE,1); }
    bfHudBars(secs);
}
#define BF_BAN0 40
#define BF_BAN1 90
static void bfBanner(const char*s,const char*s2){ bfTextC(46,s,RGB(31,26,6),3); if(s2) bfTextC(76,s2,WHITE,1); }
// ---- one fighter's step ----
static int bfDist(void){ int d=(int)((bfF[1].x-bfF[0].x)>>8); return d<0?-d:d; }
static void bfStart(Bf*f,int mv){ f->st=FS_ATK; f->mv=mv; f->t=0; f->jatk=0; }
static int bfFree(const Bf*f){ return f->st==FS_IDLE||f->st==FS_WALKF||f->st==FS_WALKB||f->st==FS_GUARD||f->st==FS_CROUCH; }
static const char* bfCall; static u8 bfCallT; static char bfCallB[12];
static void bfSparkAt(int who,int x,int y,int blk){ BfSpark*k=&bfSp[who]; k->x=x; k->y=y; k->t=8; k->blk=blk; }
static void bfLand(Bf*a,Bf*o,int mv){   // a blow connects (or not)
    const BfMv*m=&bfMv[mv]; int d=bfDist(), who=a==&bfF[1];
    if(d>bfReach(mv)+BF_BODY) return;
    if(o->st==FS_KO||o->st==FS_THROWN||o->popped) return;
    int low=o->st==FS_CROUCH||(o->st==FS_GUARD&&o->lowG)||(o->st==FS_ATK&&(o->mv==MV_LPUNCH||o->mv==MV_LKICK));
    if(o->st==FS_JUMP&&m->lvl==2) return;
    if(m->lvl==0&&low) return;   // (high blows sail over a crouch)
    if(m->lvl==3&&(low||o->st==FS_JUMP)) return;
    static const s8 hy[4]={-80,-60,-22,-62};
    int sx=(int)(o->x>>8)-a->face*10, sy=BF_GROUND+o->y+hy[m->lvl]+(m->lvl==0&&o->st==FS_JUMP?10:0);
    int guard=o->st==FS_GUARD||o->st==FS_BLOCK;
    if(guard&&m->lvl!=3&&((m->lvl==2)==(o->lowG!=0))){ o->st=FS_BLOCK; o->t=8; o->kbv=(s32)(a->face*0x200); sfxPlay(SFX_TICK); bfSparkAt(who,sx,sy,1); bfStop=3; return; }
    int dmg=m->dmg*a->pw/100; if(o->st==FS_ATK&&o->t<bfMv[o->mv].start){ dmg=dmg*3/2; bfCall="COUNTER"; bfCallT=40; }
    if(o->st==FS_HIT){ o->combo++; slCat(slNum(bfCallB,o->combo+1)," HITS"); bfCall=bfCallB; bfCallT=40; } else o->combo=0;
    if(o->st==FS_TAUNT){ dmg*=2; bfCall="SPANKED"; bfCallT=45; sy=BF_GROUND+o->y-46; }   // (caught with their pants down)
    if(o->hp>dmg) o->hp-=dmg; else o->hp=0;
    o->trailT=24; o->flash=3;
    o->st=m->lvl==3?FS_THROWN:FS_HIT; o->t=m->stun; o->kbv=(s32)(a->face*(m->kb*60)); o->cnt=0; sfxPlay(m->dmg>=14?SFX_KO:SFX_HIT);
    bfSparkAt(who,sx,sy,0); bfStop=m->dmg>=14?8:4; if(m->dmg>=14) bfShake=10;
    if(m->lvl==3){ bfCall="THROW"; bfCallT=40; bfShake=12; }
    if(mv==MV_PALM||mv==MV_SPIN){ bfCall=m->nm; bfCallT=45; voxSay(hhPUid,V_yeha); }
    if(o->hp==0){ o->st=FS_KO; o->t=0; o->kbv=(s32)(a->face*0x380); bfStop=14; bfShake=16; bfFlashT=12; bfSlow=60; }
}
// ---- THE MOON: a script of its pre-rendered frames (0 turning, 1 back to you, 2 pants coming down, 3 bent over, 4 / 5 the wiggle, 6 the slap,
// 7 / 8 the bounce), and what it does ----
typedef struct { u8 f, n; } BfTs;   // a MOON frame and how many frames of the fight it shows
static const BfTs bfTsA[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{4,8},{5,8},{4,8},{5,8},{3,10},{2,8},{1,6},{0,6},{0,0}};                      // a wiggle
static const BfTs bfTsB[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{4,8},{5,8},{6,6},{7,5},{8,5},{7,4},{8,4},{3,10},{2,8},{1,6},{0,6},{0,0}};   // a wiggle, a slap, the jiggle
static const BfTs bfTsC[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{9,14},{10,3},{11,3},{10,3},{11,3},{10,3},{11,3},{10,3},{11,3},{10,4},{11,4},{9,16},{2,6},{1,5},{0,5},{0,0}};   // the TOOT
#define BF_TOOT_AT 7    // (bfTsC: the step it slips out on, and the one where the pants come back up)
#define BF_TOOT_UP 18
static const BfTs bfTsD[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{9,10},{12,16},{13,12},{14,22},{15,3},{16,3},{15,3},{16,3},{15,3},{16,3},{15,4},{16,4},{17,34},{2,6},{1,5},{0,5},{0,0}};   // the HELIUM, the hose to the mouth
static const BfTs bfTsE[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{9,8},{18,14},{19,10},{20,14},{21,14},{22,18},{23,8},{24,3},{25,3},{24,3},{25,3},{24,3},{25,3},{24,4},{25,4},{26,34},{2,6},{1,5},{0,5},{0,0}};   // ... or the back way
static const BfTs bfTsF[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{9,10},{12,16},{13,12},{14,16},{27,16},{28,20},{31,5},{32,8},{33,240},{0,0}};   // the HELIUM gone too far: the BURST (the mouth way)
static const BfTs bfTsG[]={{0,8},{1,10},{2,10},{3,14},{4,8},{5,8},{9,8},{18,14},{19,10},{20,14},{21,14},{22,16},{29,16},{30,20},{31,5},{32,8},{33,240},{0,0}};   // ... and the back way
static const BfTs* bfTsOf(const Bf*f){ return f->ts==4?(f->hoseB?bfTsG:bfTsF):f->ts==3?(f->hoseB?bfTsE:bfTsD):f->ts==2?bfTsC:f->ts?bfTsB:bfTsA; }
// the HELIUM's moments, by the MOON frame a step shows: the tank out, the hose in (the back way), the hiss, the hose popping loose, it all coming out, red faced
#define BF_HF_TANK(fr) ((fr)==12||(fr)==18)
#define BF_HF_PLUG(fr) ((fr)==19)
#define BF_HF_FILL(fr) ((fr)==13||(fr)==20)
#define BF_HF_LOOSE(fr) ((fr)==23)
#define BF_HF_OUT(fr) ((fr)==15||(fr)==24)
#define BF_HF_RED(fr) ((fr)==17||(fr)==26)
#define BF_HF_OVER1(fr) ((fr)==27||(fr)==29)   // too much
#define BF_HF_OVER2(fr) ((fr)==28||(fr)==30)
#define BF_HF_POP(fr) ((fr)==31)
static int bfTsAt(const Bf*f,int*first){   // the script step the moon is on (-1: over); first: it starts this frame
    const BfTs*q=bfTsOf(f); int a=0;
    for(int i=0;q[i].n;i++){ if(f->t<a+q[i].n){ if(first) *first=f->t==a; return i; } a+=q[i].n; }
    return -1;
}
static int bfLive;   // the round is on (a moon only stuns while it is)
static void bfTauntGo(Bf*f){ f->st=FS_TAUNT; f->t=0; f->kbv=0; f->mooned=0; f->ts=(u8)(rnd8()&3); if(f->ts==3&&bfLive&&rnd8()%3==0) f->ts=4; f->hoseB=(u8)(rnd8()&1); bfMoonPal(f); }   // (one helium moon in three bursts, while the round is on)   // (a wiggle, the slap and the jiggle, the TOOT or the HELIUM: a quarter each)
static void bfTauntStep(Bf*f,Bf*o){
    int first=0, i=bfTsAt(f,&first); if(i<0){ if(!f->popped) f->st=FS_IDLE; return; }   // (a red face stays red till the match ends; a burst one stays a puddle)
    if(first){ int fr=bfTsOf(f)[i].f;
        if(i==2) sfxPlay(SFX_POP);   // (the pants come down)
        if(i==3&&!f->mooned){ f->mooned=1; bfCall="MOONED!"; bfCallT=50; voxSay(hhPUid,V_laughing);
            if(bfLive&&bfFree(o)){ o->st=FS_HIT; o->t=34; o->kbv=0; o->combo=0; } }   // (they recoil: an opening)
        if(fr==6) sfxPlay(SFX_HIT);   // (the slap)
        if(f->ts==2&&i==BF_TOOT_AT){ sfxPlay(SFX_TOOT); bfShake=26; bfCall="TOOT!"; bfCallT=60;   // (oops)
            bfGas.x=(int)(f->x>>8); bfGas.y=BF_GROUND+f->y-50; bfGas.t=36;
            if(bfLive&&o->st!=FS_KO&&bfDist()<110&&(bfFree(o)||o->st==FS_HIT)){ o->st=FS_HIT; o->t=56; o->kbv=0; o->combo=0; } }   // (the smell holds them even longer)
        if(f->ts==2&&i==BF_TOOT_UP) voxSay(hhPUid,V_laughing_small_or_not);
        if(f->ts>=3){
            if(BF_HF_OVER1(fr)){ sfxPlay(SFX_SHOWER); bfCall="TOO MUCH!"; bfCallT=40; bfShake=4; }
            if(BF_HF_OVER2(fr)){ voxSay(hhPUid,V_shriek); bfShake=12; }
            if(BF_HF_POP(fr)){   // THE BURST
                sfxPlay(SFX_THUNDER); bfFlashT=14; bfShake=48; bfCall="BURST!"; bfCallT=80; f->popped=1; f->hp=0; f->trailT=0;
                int bx=(int)(f->x>>8), by=BF_GROUND+f->y-58; bfGas.x=bx; bfGas.y=by; bfGas.t=36;
                bfSparkAt(0,bx-10,by-6,2); bfSparkAt(1,bx+12,by+8,2);   // (blood)
                if(bfLive&&o->st!=FS_KO&&!o->popped&&(bfFree(o)||o->st==FS_HIT)){ o->st=FS_HIT; o->t=48; o->kbv=(s32)(-o->face*0x180); o->combo=0; } }
            if(BF_HF_TANK(fr)){ bfCall="HELIUM?"; bfCallT=40; sfxPlay(SFX_TICK); }   // (the valve)
            if(BF_HF_PLUG(fr)){ sfxPlay(SFX_POP); bfShake=6; voxSay(hhPUid,V_shriek); }   // (in it goes: a jolt)
            if(BF_HF_FILL(fr)) sfxPlay(SFX_SHOWER);   // (the hiss)
            if(BF_HF_LOOSE(fr)) sfxPlay(SFX_POP);   // (the hose pops loose)
            if(BF_HF_OUT(fr)){ sfxPlay(SFX_TOOT); bfShake=40; bfCall="TOOT!"; bfCallT=60;
                bfGas.x=(int)(f->x>>8)-f->face*12; bfGas.y=BF_GROUND+f->y-46; bfGas.t=36;
                if(bfLive&&o->st!=FS_KO&&bfDist()<110&&(bfFree(o)||o->st==FS_HIT)){ o->st=FS_HIT; o->t=56; o->kbv=0; o->combo=0; } }
            if(BF_HF_RED(fr)){ f->blush=1; audHe=384; bfCall="HELIUM VOICE"; bfCallT=70; voxSay(hhPUid,V_laughing_small_or_not); } }   // (from here every sound is a fifth higher, till the match ends)
    }
    f->t++;
}
static void bfPickFr(Bf*f){   // the pre-rendered frame for what it is doing
    int fr=BFA_IDLE;
    switch(f->st){
        case FS_IDLE: fr=f->jigT>0?BFA_JIGGLE+(12-f->jigT)/4:BFA_IDLE+((f->cnt>>3)&3); break;
        case FS_TAUNT: { int i=bfTsAt(f,0); fr=BFA_MOON+(i<0?(f->popped?33:0):bfTsOf(f)[i].f); } break;
        case FS_WALKF: fr=BFA_WALK+(f->cnt>>2)%6; break;
        case FS_WALKB: fr=BFA_WALK+5-(f->cnt>>2)%6; break;
        case FS_GUARD: case FS_BLOCK: fr=f->lowG?BFA_CGUARD:BFA_GUARD; break;
        case FS_CROUCH: fr=BFA_CROUCH; break;
        case FS_ATK: fr=bfAtkFr(&bfMv[f->mv],f->t); break;
        case FS_JUMP: fr=f->jatk&&f->t<14?(f->mv==MV_KICK?BFA_JKICK:BFA_PUNCH+2):BFA_JUMP+(f->vy>0); break;
        case FS_HIT: fr=BFA_HIT+(f->t<7); break;
        case FS_THROWN: { int e=bfMv[MV_THROW].stun-f->t; fr=f->t<8?BFA_CROUCH:BFA_KO+(e/5<3?e/5:3); } break;
        case FS_KO: fr=BFA_KO+(f->t/5<4?f->t/5:4); break;
        case FS_WIN: fr=BFA_WIN+(f->t<10?0:f->t<20?1:1+((f->t>>4)&1)); break;
    }
    f->fr=fr;
}
static void bfStep(Bf*f,Bf*o,u16 k,u16 pr){   // the moves and the physics of one fighter (k / pr: its pad, the player's or the AI's)
    int fwd=f->face>0?K_RIGHT:K_LEFT, back=f->face>0?K_LEFT:K_RIGHT;
    if(f->flash>0) f->flash--; if(f->jigT>0) f->jigT--;
    if(f->trailT>0) f->trailT--; else if(f->trail>f->hp) f->trail--; if(f->trail<f->hp) f->trail=f->hp;   // (the red part of the bar drains after a moment)
    if(f->st==FS_KO){ f->t++; f->kbv=f->kbv*7/8; }
    else if(f->st==FS_WIN){ f->t++; }
    else if(f->st==FS_HIT||f->st==FS_BLOCK||f->st==FS_THROWN){ if(--f->t<=0){ if(f->st==FS_HIT&&f->butt&&rnd8()<90) f->jigT=12; f->st=FS_IDLE; f->combo=0; } }   // (shaking it off: sometimes a jiggle)
    else if(f->st==FS_TAUNT) bfTauntStep(f,o);
    else if(f->st==FS_ATK){ const BfMv*m=&bfMv[f->mv]; f->t++;
        if(f->t==m->start+1) bfLand(f,o,f->mv);
        if(f->t>=m->start+m->act+m->rec) f->st=f->y<0?FS_JUMP:FS_IDLE; }
    else if(f->st==FS_JUMP){ if(!f->jatk&&(pr&(K_A|K_B))){ f->jatk=1; f->mv=(pr&K_A)?MV_PUNCH:MV_KICK; f->t=0; } if(f->jatk){ f->t++; if(f->t==bfMv[f->mv].start+1) bfLand(f,o,f->mv); } }
    else {   // free to act
        int crouch=(k&K_DOWN)!=0, guard=(k&(K_R|K_L))!=0, ab=((pr&K_A)&&(k&K_B))||((pr&K_B)&&(k&K_A));
        if(ab&&bfDist()<=BF_SEP+8) bfStart(f,MV_THROW);
        else if(pr&K_A) bfStart(f,crouch?MV_LPUNCH:(k&fwd)?MV_PALM:MV_PUNCH);
        else if(pr&K_B) bfStart(f,crouch?MV_LKICK:(k&back)?MV_SPIN:MV_KICK);
        else if((pr&K_UP)&&!crouch){ f->st=FS_JUMP; f->vy=-0x5A0; f->jatk=0; f->kbv=(k&fwd)?(s32)(f->face*0x1A0):(k&back)?(s32)(-f->face*0x150):0; }
        else if(guard) f->st=FS_GUARD;
        else if(crouch) f->st=FS_CROUCH;
        else if(k&fwd) f->st=FS_WALKF; else if(k&back) f->st=FS_WALKB; else f->st=FS_IDLE;
        f->lowG=f->st==FS_GUARD&&crouch;
        int sp=f->sp*0x15;
        if(f->st==FS_WALKF) f->x+=f->face*sp; else if(f->st==FS_WALKB) f->x-=f->face*sp*7/8;
    }
    if(f->st==FS_JUMP||f->y<0){ f->y+=f->vy>>8; f->vy+=0x5C; f->x+=f->kbv; if(f->y>=0){ f->y=0; f->vy=0; f->kbv=0; if(f->st==FS_JUMP){ f->st=FS_IDLE; sfxPlay(SFX_LAND); if(f->butt&&rnd8()<110) f->jigT=12; } } }   // (a landing: sometimes a jiggle)
    else if(f->kbv){ f->x+=f->kbv; f->kbv=f->kbv*13/16; if(f->kbv>-16&&f->kbv<16) f->kbv=0; }
    // off the ring: over the edge and down into the sea
    { int x=(int)(f->x>>8); if((x<BFA_RINGL-12||x>BFA_RINGR+12)&&f->y>=0){ if(f->y==0) f->vy=0x100; f->y+=f->vy>>8; f->vy+=0x5C; } }
    // keep the two apart, and inside the ring unless knocked out of it
    int d=(int)((o->x-f->x)>>8)*f->face; if(d<BF_SEP&&f->st!=FS_KO&&o->st!=FS_KO&&!f->popped&&!o->popped&&f->y==0&&o->y==0){ f->x-=(s32)(f->face*(BF_SEP-d)*128); o->x+=(s32)(f->face*(BF_SEP-d)*128); }
    if(bfFree(f)){ int x=(int)(f->x>>8); if(x<BFA_RINGL) f->x=(s32)BFA_RINGL<<8; if(x>BFA_RINGR) f->x=(s32)BFA_RINGR<<8; }
    if(f->st==FS_IDLE&&f->butt&&(f->cnt&127)==64&&rnd8()<40) f->jigT=12;   // (and now and then just standing there)
    f->cnt++; bfPickFr(f);
}
static u16 bfAi(Bf*f,Bf*o){   // the opponent's pad: approach, poke, block what it sees coming, sometimes a special
    int d=bfDist(), lv=f->d, fwd=f->face>0?K_RIGHT:K_LEFT, back=f->face>0?K_LEFT:K_RIGHT; u16 k=0;
    if(o->popped) return 0;
    if(o->st==FS_TAUNT&&(rnd8()&15)<4+lv) return d>40?fwd:(rnd8()&1)?K_B:K_A;   // (pants down: go and spank them)
    if(o->st==FS_ATK&&o->t<=bfMv[o->mv].start&&d<=bfReach(o->mv)+BF_BODY+8&&f->aiDo!=2){ if((rnd8()*100>>8)<14+lv*9){ f->aiDo=2; f->aiT=10+(rnd8()&7); } }
    if(f->aiT>0){ f->aiT--; }
    else { int r=rnd8()*100>>8; f->aiT=(18-lv)+(rnd8()&15);
        if(d>58) f->aiDo=r<85?1:(lv>=2?6:1);
        else if(d>40) f->aiDo=r<50?1:r<70?3:(r<80&&lv>=2)?4:0;
        else f->aiDo=r<35?7:r<55?3:r<65?8:(r<73&&lv>=1)?9:(r<78&&lv>=3)?5:r<88?0:10; }
    switch(f->aiDo){
        case 1: k|=fwd; break;                                       // walk in
        case 2: k|=K_R; if(o->st==FS_ATK&&(o->mv==MV_LPUNCH||o->mv==MV_LKICK)) k|=K_DOWN; break;   // guard (low when it sees a low one)
        case 3: k|=K_B; f->aiDo=0; break;                            // kick
        case 4: k|=fwd|K_A; f->aiDo=0; break;                        // the palm
        case 5: k|=back|K_B; f->aiDo=0; break;                       // the spin kick
        case 6: k|=K_UP|fwd; f->aiDo=0; break;                       // jump in
        case 7: k|=K_A; f->aiDo=0; break;                            // jab
        case 8: k|=K_DOWN|K_B; f->aiDo=0; break;                     // sweep
        case 9: k|=K_A|K_B; f->aiDo=0; break;                        // throw
        case 10: k|=back; break;                                     // back off
    }
    return k;
}
// ---- who fights ----
static u8 bfMayMoon;   // (grown-ups only: a teen or a child never moons, and is never mooned)
static u8 bfCodeOn;    // the code has been put in this arcade session: only then does anybody moon anybody
static void bfSetYou(Bf*p){   // your Bore: build, seat and clothes as the creator has them (bare tops and bottoms are for grown-ups only, as everywhere)
    setColors(); int adult=stage>=AG_ADULT; bfMayMoon=(u8)adult;
    p->body=look[LK_SEX]==SX_FEMALE?BFB_SLIM:slideEffS(look[LK_WEIGHT])>=2?BFB_HEAVY:BFB_STANDARD;
    p->butt=(look[LK_BUTT]==BUTT_OFF||(stage<AG_TEEN&&!sUnlock))?BFS_FLAT:slidePos(look[LK_BUTT])+slideEffS(look[LK_BUTTW])/2>=6?BFS_BIG:BFS_ROUND;   // (the seat: off, or how big)
    u16 sk=base[1], top=(look[LK_TOPSTY]==4&&adult)?sk:base[6], bot=(look[LK_BOTSTY]==3&&adult)?sk:base[7];
    bfPalMake(p,sk,top,bot,base[5],RGB(28,4,4),0); p->botc=bot; p->adult=(u8)adult; p->pw=100; p->sp=17; p->d=0; p->nm=hhPName[0]?hhPName:"YOU";
}
static void bfSetThem(Bf*o,int st){ const BfWho*w=&bfLadder[st]; o->body=w->body; o->butt=w->butt; bfPalMake(o,w->skin,w->top,w->bot,w->hair,w->acc,st==8);
    o->botc=st==8?RGB(19,21,25):w->bot; o->adult=1; o->pw=w->pw; o->sp=w->sp; o->d=w->d; o->nm=w->nm; }
static void bfReset(Bf*f,int x,int face){ f->x=(s32)x<<8; f->face=face; f->hp=f->trail=100; f->trailT=0; f->st=FS_IDLE; f->y=f->vy=0; f->kbv=0; f->t=0; f->cnt=face>0?0:13; f->flash=0; f->combo=0; f->lowG=0; f->jatk=0; f->aiT=0; f->aiDo=0; f->jigT=0; f->popped=0; bfPickFr(f); }
// ---- one match (best of three). 1 = you won, 0 = you lost, -1 = you quit ----
static const u16 bfKon[10]={K_UP,K_UP,K_DOWN,K_DOWN,K_LEFT,K_RIGHT,K_LEFT,K_RIGHT,K_B,K_A};   // THE MOON
static int bfMoonRoll(void){ return bfCodeOn&&rnd8()<2; }   // once a round, and when the code goes in: about one round in 128 gets a moon (super rare)
static int bfMatch(int stage){
    Bf*p=&bfF[0], *o=&bfF[1]; int konI=0, tauntQ=0, tauntAt=0, payback=0;   // (tauntQ: a moon this round, 1 yours 2 theirs, at tauntAt; payback: 1 they owe you one, 2 paid)
    for(int round=1;;round++){
        bfReset(p,120-40,1); bfReset(o,120+40,-1); bfSp[0].t=bfSp[1].t=0; bfGas.t=0; bfStop=bfShake=bfFlashT=bfSlow=0; bfWtfT=0; bfFadeY=0; bfFxTiles();
        bfArRows(0,SH); bfHud(30); vsync(); bfShow(1); REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR; REG_DMA3CNT=(SW*SH/2)|0x84000000u;
        int phase=0, pt=0, secs=30, winner=-1; const char*why=0; u16 prev=keyNow(); bfCall=0; bfCallT=0;
        tauntQ=bfMoonRoll()?1+(rnd8()&1):0; tauntAt=150+rnd8()*4;   // (the round's roll)
        const char*banTx=(const char*)1; char rname[12]; { char*e=slCat(rname,round==3?"FINAL ROUND":"ROUND "); if(round<3) slNum(e,round); }
        voxSay(hhPUid,V_lets_fight);
        tmStart(); u16 tl=R_TM2D; int acc=0, done=0, hud=0, ban=0;
        for(;!done;){
            vsync();
            bfShow(1); bfPics++;   // (everything the last pass worked out goes up in the vblank: no tearing)
            if(hud) vramCopy(0,0,SW,BF_HUDH);
            if(ban) vramCopy(0,BF_BAN0,SW,BF_BAN1);
            hud=ban=0;
            { u16 now=R_TM2D, dt=(u16)(now-tl); tl=now; acc+=dt; }
            int steps=acc/TICKS_FRAME; acc-=steps*TICKS_FRAME; if(steps>3){ steps=3; acc=0; }
            u16 k=keyNow(), pr=k&~prev; prev=k;
            { u16 q=pr&(K_UP|K_DOWN|K_LEFT|K_RIGHT|K_A|K_B);   // the code, one press at a time (the moves still happen as you put it in)
              if(q){ if(q==bfKon[konI]){ if(++konI==10){ konI=0; if(bfMayMoon&&!bfCodeOn){ bfCodeOn=1; sfxPlay(SFX_TICK); }   // (armed: a tick, nothing more)
                                                       if(!tauntQ&&bfMoonRoll()){ tauntQ=1+(rnd8()&1); tauntAt=pt+60+(rnd8()&63); } } } else konI=q==bfKon[0]?1:0; } }
            if((pr&K_START)&&phase==1){ static const char* const pm[2]={"KEEP FIGHTING","QUIT ARCADE MODE"}; bfShow(0); objHideAll();
                int q=menu("PAUSED",pm,2); if(q==1) return -1;
                prev=keyNow(); bfArRows(0,SH); bfHud(secs); vsync(); bfShow(1); REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR; REG_DMA3CNT=(SW*SH/2)|0x84000000u; banTx=(const char*)1; tmStart(); tl=R_TM2D; acc=0; continue; }
            for(int st=0;st<steps&&!done;st++){ if(st) pr=0;
                if(bfShake>0) bfShake--; if(bfFlashT>0) bfFlashT--;
                for(int i=0;i<2;i++) if(bfSp[i].t>0) bfSp[i].t--; if(bfGas.t>0) bfGas.t--;
                if(bfStop>0){ bfStop--; continue; }   // (the hit stop: everything holds still for a beat)
                if(bfSlow>0){ bfSlow--; if(bfSlow&1) continue; }   // (slow motion after a K.O.)
                pt++; bfLive=phase==1;
                if(phase==0&&pt>=100){ phase=1; pt=0; }
                if(tauntQ&&phase==1&&pt>=tauntAt){ Bf*w=tauntQ==1?p:o; if(bfFree(w)&&p->st!=FS_TAUNT&&o->st!=FS_TAUNT){ tauntQ=0; bfTauntGo(w); if(w==p){ if(!payback) payback=1; pr=0; } } }
                if(phase==1&&payback==1&&bfFree(o)&&p->st!=FS_TAUNT&&bfDist()>64&&(rnd8()&31)==0){ bfTauntGo(o); payback=2; }   // (they pay you back)
                if(phase==1){
                    if(pt%60==59&&secs>0) secs--;
                    u16 ak=bfAi(o,p);   // (its attack buttons last one frame: each is a fresh press)
                    bfStep(p,o,k,pr); bfStep(o,p,ak,ak);
                    if(bfFree(p)) p->face=o->x>p->x?1:-1; if(bfFree(o)||o->st==FS_ATK) o->face=p->x>o->x?1:-1;
                    int px_=(int)(p->x>>8), ox_=(int)(o->x>>8);
                    if(o->popped){ winner=0; why="BURST"; } else if(p->popped){ winner=1; why="BURST"; }
                    else if(o->st==FS_KO&&o->hp==0){ winner=0; why="K.O."; } else if(p->st==FS_KO&&p->hp==0){ winner=1; why="K.O."; }
                    else if(o->y>40){ winner=0; why="RING OUT"; o->st=FS_KO; } else if(p->y>40){ winner=1; why="RING OUT"; p->st=FS_KO; }
                    else if(secs==0){ winner=p->hp>=o->hp?0:1; why="TIME UP"; }
                    (void)px_; (void)ox_;
                    if(winner>=0){ phase=2; pt=0; if(winner==0){ p->wins++; if(o->hp<=0&&p->hp==100&&!o->popped) why="PERFECT"; } else o->wins++;
                        if(bfF[1-winner].popped) bfF[winner].wins=2;   // (a burst ends the match, not just the round)
                        if(!bfSlow) sfxPlay(SFX_BELL); }
                } else if(phase==2){ bfStep(p,o,0,0); bfStep(o,p,0,0);
                    if(pt==80){ Bf*wf=winner?o:p; if(wf->st!=FS_KO&&wf->st!=FS_TAUNT){ if(winner&&bfMayMoon&&bfCodeOn&&(payback||rnd8()<2)) bfTauntGo(o); else { wf->st=FS_WIN; wf->t=0; } }   // (a winner sometimes moons you)
                        voxSay(hhPUid,winner?V_lost_the_fight:V_win_the_fight); }
                    if(p->popped||o->popped){   // the send-off: the letters, then the fade to black, then the match is over
                        if(pt==30){ bfWtfT=1; bfWtfTiles(); voxSay(hhPUid,V_die_of_shock); } else if(bfWtfT>0) bfWtfT++;
                        if(pt>=190){ bfFadeY=(pt-190)*16/44; if(bfFadeY>16) bfFadeY=16; }
                        if(pt>=244) done=1; }
                    else if(pt>=200&&(p->st!=FS_TAUNT||p->popped)&&(o->st!=FS_TAUNT||o->popped)) done=1; }
                else { bfStep(p,o,0,0); bfStep(o,p,0,0); }
                if(bfCallT) bfCallT--; if(phase==0&&pt==60) sfxPlay(SFX_BELL); }
            if(done) break;
            // the words and the bars, into the picture (copied up at the next vblank)
            const char*bt=phase==0?(pt<60?rname:"FIGHT!"):(phase==2&&pt<170&&!bfWtfT)?why:(bfCallT&&!bfWtfT)?bfCall:0;   // (nothing over the send-off's letters)
            if(bt!=banTx||(bt==bfCallB&&bfCallT==39)){ bfArRows(BF_BAN0,BF_BAN1);
                if(!bt){} else if(phase==0) bfBanner(bt,0); else if(phase==2&&bt==why) bfBanner(bt,winner?"YOU LOSE THE ROUND":"YOU WIN THE ROUND"); else bfTextC(52,bt,RGB(31,20,6),2);
                banTx=bt; ban=1; }
            if(p->hp!=bfShownHp[0]||o->hp!=bfShownHp[1]||p->trail!=bfShownTr[0]||o->trail!=bfShownTr[1]||secs!=bfShownT||p->wins*4+o->wins!=bfShownW){ bfHudBars(secs); hud=1; }
        }
        if(p->wins>=2) return 1;
        if(o->wins>=2) return 0;
    }
}
// ---- the cards between fights: the arena dimmed, the fighters standing on it ----
static void bfCard(void){ bfArRows(0,SH); bfDim(0,SH); }
static void bfCardShow(int both){   // in the vblank: the picture, and the fighters' sprites
    REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR; REG_DMA3CNT=(SW*SH/2)|0x84000000u;
    bfSp[0].t=bfSp[1].t=0; bfShake=0; bfFlashT=0; bfWtfT=0; bfFadeY=0; R_BLDCNT=0x0400; R_BLDY=0; bfFxTiles();   // (after a send-off: the picture back, the sparks' tiles back)
    for(int i=0;i<2;i++){ bfTiles(i); bfPalSend(i); }
    for(int s=0;s<BF_SLOT0+9;s++) OAM[s*4]=0x200;
    bfPlace(0,BF_SLOT0+2,bfF[0].x>>8,BF_GROUND+bfF[0].y); if(both) bfPlace(1,BF_SLOT0+4,bfF[1].x>>8,BF_GROUND+bfF[1].y);
}
// ---- ARCADE MODE (expand.h exModeRun) ----
XPF static int bfRun(void){
    objHideAll(); zoomOff(); u16 al=R_BLDAL; REG_DISPCNT=0x1443; R_BLDCNT=0x0400; R_BLDAL=(u16)(16|(9<<8)); R_BLDY=0;   // (shadows: the arena at 9/16 under them)
    bfLoad(); bfFxTiles(); bfFxPal(); for(int i=0;i<2;i++) bfF[i].shown=-1;
    int stage=0, won=0, out=0;
    bfSetYou(&bfF[0]); bfCodeOn=(u8)(dbgOn&&bfMayMoon);   // (the debug code already in: the moon's chance is armed from the start)
    while(!out){
        // the VS card: the two of them on the dimmed ring, names, the stage
        { const BfWho*w=&bfLadder[stage]; bfSetThem(&bfF[1],stage); bfReset(&bfF[0],60,1); bfReset(&bfF[1],180,-1); u16 prev=keyNow();
          bfCard(); bfTextC(5,"BORTUA FIGHTER",RGB(31,26,6),2);
          { char b[24]; char*e=slCat(b,stage==8?"FINAL STAGE":"STAGE "); if(stage<8) slNum(e,stage+1); bfTextC(25,b,RGB(17,29,31),1); }
          bfTextC(66,"VS",RGB(31,8,6),3);
          bfText(60-tw(bfF[0].nm,1)/2,141,bfF[0].nm,WHITE,1); bfText(180-tw(w->nm,1)/2,141,w->nm,WHITE,1);
          bfTextC(152,"A FIGHT   B QUIT",DIMC,1);
          for(u32 c=0;;c++){ u16 k=keyNow(), pr=k&~prev; prev=k;
            bfF[0].cnt++; bfF[1].cnt++; bfPickFr(&bfF[0]); bfPickFr(&bfF[1]); vsync(); if(!c) bfCardShow(1); else { for(int i=0;i<2;i++){ bfTiles(i); } bfPlace(0,BF_SLOT0+2,60,BF_GROUND); bfPlace(1,BF_SLOT0+4,180,BF_GROUND); }
            if(pr&K_A) break; if(pr&K_B){ out=1; break; } } }
        if(out) break;
        int r=bfMatch(stage); audHe=0; bfF[0].blush=bfF[1].blush=0; if(r<0) break;   // (the helium and the red face wear off with the match)
        if(r==1){ won++; if(bfWins<250) bfWins++; bfSave(); stQEvent(QE_KO,1);
            if(stage==8){ bfChamp=1; bfSave(); u16 prev=keyNow(); Bf*p=&bfF[0]; bfReset(p,120,1); p->st=FS_WIN; p->t=0;
                bfCard(); bfTextC(20,"BORTUA",RGB(31,26,6),3); bfTextC(46,"CHAMPION",RGB(31,26,6),3); bfTextC(150,"NOBODY IS BORED OF YOU NOW",WHITE,1);
                for(u32 c=0;;c++){ u16 k=keyNow(), pr=k&~prev; prev=k; if(c>60&&(pr&(K_A|K_B|K_START))) break;
                    p->t++; bfPickFr(p); vsync(); if(!c) bfCardShow(0); else { bfTiles(0); bfPlace(0,BF_SLOT0+2,120,BF_GROUND); } }
                voxPlay(V_yahoo); break; }
            stage++; }
        else {   // CONTINUE? (a countdown, like the arcade) with you flat on the ring
            u16 prev=keyNow(); int cont=0; Bf*p=&bfF[0]; bfReset(p,120,1); p->st=FS_KO; p->t=40; bfPickFr(p);
            for(int n=9;n>=0&&!cont;n--){ bfCard(); bfTextC(36,"CONTINUE?",RGB(31,26,6),3); char b[4]; slNum(b,n); bfTextC(70,b,WHITE,3); bfTextC(150,"A CONTINUE   B GIVE UP",DIMC,1);
                vsync(); bfCardShow(0); sfxPlay(SFX_TICK);
                for(int i=0;i<60&&!cont;i++){ u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&K_A) cont=1; if(pr&K_B){ n=-1; break; } vsync(); } }
            if(!cont) break; }
    }
    bfShake=0; bfFlashT=0; audHe=0; R_BG2X=0; R_BG2Y=0; R_BLDAL=al; objHideAll(); REG_DISPCNT=0x0403;
    hhSlotsFree(); hhExtraDone();   // (the house's, the ghosts' and the cops' sprites and colours go back into OBJ memory: the fighters used it)
    return won;
}

// npc.h - AI SKATERS and POLICE: two small NPC systems on hardware sprites (no frame buffer, no baked characters).
//
//  * ART: 4 frames of 16 x 32 in ROM (npcArt: cop standing, cop stepping, skater rolling, skater in the air), uploaded to OBJ tiles 800.. (8 tiles a frame),
//    OBJ palettes 12 (cop) and 13..15 (the three skaters). OAM entries 75..78 (and 79.. for cops 2 and up, see COP_MAX). fx.h owns tiles 768..799, palettes 8..11 and OAM 32..74; the household
//    owns tiles 512..767, palettes 0..7 and OAM 0..31. In the prison the crowd of inmates borrows palettes 8..10 and 12..15 while nothing here uses them (hhBanksBusy, at the end of this file). Nothing here is saved.
//  * AI SKATERS: on a map with at least NPC_PARK skate objects (kickers, quarter pipes, launch ramps, funboxes, rails, ledges, jersey barriers, manual
//    pads) up to 3 skaters roll from object to object and hop beside each one. They cost about 90 bytes of EWRAM and no drawing time.
//  * POLICE: punching Sims is a crime (copCrime, called from fightHit in house.h). Heat 3 or more (a knock-out is worth 2 at once) and about 5 seconds later
//    a cop walks in from the edge of the lot (the visitor path: hhPlan / hhStepAlong, twFar), runs at you, and BUSTS you: a fine of up to 50 and a hold of
//    10 seconds where you cannot move (lstun), then he walks off. After that the cops leave you alone for a minute. Heat fades by 1 every 10 seconds.
//    Slice 1 on purpose: jail is a timed hold on the spot, not a room yet.
#define NPC_TILE 800
#define NPC_OAM0 75   // (after the Sims' 0..31 and fx.h's 32..74)
#define NPC_PALC 12
#define NPC_PALS 13
#define NPC_SKN  3   // (was SK_N: that name is the skill count in sims.h)
#define NPC_PARK 6
static const char npcArt[4][32][17]={
 {
    "                ",
    "                ",
    "    ########    ",
    "   #hhhhhhhh#   ",
    "   #hhyyyyhh#   ",
    "   #hhhhhhhh#   ",
    "  #hhhhhhhhhh#  ",
    "   ##ssssss##   ",
    "    #s#ss#s#    ",
    "    #ssssss#    ",
    "    #ssssss#    ",
    "    #ssssss#    ",
    "  ##bbbbbbbb##  ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbyybbb# ",
    " #bbbbbbbyybbb# ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbbbbbb# ",
    " #ssppppppppss# ",
    " #ssbbbbbbbbss# ",
    "  ###pppppp###  ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "   #wwwwwwww#   ",
    "   #wwwwwwww#   ",
    "    ########    ",
 },
 {
    "                ",
    "                ",
    "    ########    ",
    "   #hhhhhhhh#   ",
    "   #hhyyyyhh#   ",
    "   #hhhhhhhh#   ",
    "  #hhhhhhhhhh#  ",
    "   ##ssssss##   ",
    "    #s#ss#s#    ",
    "    #ssssss#    ",
    "    #ssssss#    ",
    "    #ssssss#    ",
    "  ##bbbbbbbb##  ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbyybbb# ",
    " #bbbbbbbyybbb# ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbbbbbb# ",
    " #bbbbbbbbbbbb# ",
    " #ssppppppppss# ",
    " #ssbbbbbbbbss# ",
    "  ###pppppp###  ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "    #pppppp#    ",
    "   #wwwwppp#    ",
    "   #wwwwppp#    ",
    "    ####wwww#   ",
    "       #wwww#   ",
    "        ####    ",
 },
 {
    "                ",
    "                ",
    "     ######     ",
    "    #hhhhhh#    ",
    "   #hgghhhhh#   ",
    "   #hghhhhhh#   ",
    "   #hssssssh#   ",
    "   #hssssssk#   ",
    "   #ssessesk#   ",
    "   #sssssssk#   ",
    "   #sssssssk#   ",
    "  ####kkkk####  ",
    " #lblbbbbbbdbd# ",
    " #bb#lbbbbd#bb# ",
    " #sk#lbblbd#sk# ",
    " #sk#lbblbd#sk# ",
    " #sk#lbbbbd#sk# ",
    " #ss#lbbbbd#sk# ",
    "  ###dddddd###  ",
    "    #qqqqqq#    ",
    "    #ppqppq#    ",
    "    #ppqppq#    ",
    "    #ppqppq#    ",
    "    #ppqppq#    ",
    "    #ppqppq#    ",
    "    #ppqppq#    ",
    " ###oooooooo### ",
    "#aaaaaaaaaaaaaa#",
    "#wwwwwwwwwwwwww#",
    " ##yy######yy## ",
    "  #yy#    #yy#  ",
    "   ##      ##   ",
 },
 {
    "                ",
    "                ",
    "                ",
    "                ",
    "     ######     ",
    "    #hhhhhh#    ",
    "   #hgghhhhh#   ",
    "   #hghhhhhh#   ",
    "## #hssssssh#   ",
    "ks##hssssssk# ##",
    "ss##ssessesk##sk",
    "#lb#sssssssk##ss",
    "#bb#sssssssk#bd#",
    " #bb##kkkk##bbb#",
    " #bblbbbbbbdbb# ",
    "  ###lbbbbd###  ",
    "    #lbblbd#    ",
    "    #lbblbd#    ",
    "    #lbbbbd#    ",
    "    #lbbbbd#    ",
    "    #dddddd#    ",
    "    #qqqqqq#    ",
    "   #ppq##ppq#   ",
    "   #ppq##ppq#   ",
    "   #ppq##ppq#   ",
    "  #oooo##oooo## ",
    "  #oooo#aaaaaaa#",
    " #######wwwwwww#",
    "#aaaaaaa###yy## ",
    "#wwwwwww# #yy#  ",
    " ##yy###   ##   ",
    "   ##           ",
 },
};

static const u16 npcPal[4][16]={   // 0 clear, 1 outline, 2 skin, 3 hair / hat, 4 shirt, 5 pants, 6 board, 7 wheels / badge, 8 skin shade, 9 shirt light, 10 shirt dark, 11 pants dark, 12 hair light, 13 shoes, 14 board graphic, 15 eyes (the skaters, frames 2 and 3, use all 16)
    {0,RGB(2,2,6),RGB(30,23,17),RGB(3,5,13),RGB(7,11,25),RGB(3,5,13),RGB(2,2,3),RGB(31,27,5),RGB(24,17,12),RGB(10,15,31),RGB(5,8,17),RGB(2,3,8),RGB(6,9,20),RGB(2,2,3),RGB(31,27,5),RGB(3,2,4)},
    {0,RGB(2,2,5),RGB(30,23,17),RGB(9,5,2),RGB(28,6,5),RGB(8,10,18),RGB(24,17,8),RGB(28,28,28),RGB(23,17,13),RGB(31,7,6),RGB(19,4,3),RGB(5,6,11),RGB(17,9,3),RGB(28,28,28),RGB(31,24,6),RGB(3,2,4)},
    {0,RGB(2,2,5),RGB(23,16,10),RGB(3,3,4),RGB(6,22,10),RGB(14,14,18),RGB(26,22,6),RGB(28,28,28),RGB(17,12,7),RGB(7,28,13),RGB(4,15,7),RGB(9,9,11),RGB(9,9,11),RGB(26,6,6),RGB(6,6,8),RGB(3,2,4)},
    {0,RGB(2,2,5),RGB(17,11,7),RGB(26,22,4),RGB(8,10,28),RGB(6,8,14),RGB(20,8,22),RGB(28,28,28),RGB(13,8,5),RGB(10,13,31),RGB(5,7,19),RGB(3,5,9),RGB(31,31,7),RGB(28,28,10),RGB(31,31,31),RGB(3,2,4)} };
typedef struct { s32 fx, fy; u8 tx, ty, air, face, wait, tries; } NpcSk;
static NpcSk npcSk[NPC_SKN] EWRAM_BSS; static u8 npcSkN EWRAM_BSS, npcVramOk EWRAM_BSS;

static void npcUpload(void){   // in vblank
    for(int f=0;f<4;f++){
        u16 buf[128]; for(int i=0;i<128;i++) buf[i]=0;
        for(int y=0;y<32;y++) for(int x=0;x<16;x++){
            char c=npcArt[f][y][x]; int ci=0; for(int q=1;q<16;q++) if(" #shbpwykldqgoae"[q]==c){ ci=q; break; }   // (the cop's art only uses the first eight; the skaters shade with the rest)
            int tile=(y>>3)*2+(x>>3), i=tile*16+(y&7)*2+((x&7)>>2);
            buf[i]|=(u16)(ci<<(((x&7)&3)*4)); }
        volatile u16*d=OBJ_VRAM+(NPC_TILE-512+f*8)*16; for(int i=0;i<128;i++) d[i]=buf[i]; }
    for(int p=0;p<4;p++){ volatile u16*q=OBJ_PAL+(NPC_PALC+p)*16; for(int i=0;i<16;i++) q[i]=npcPal[p][i]; }
}
static int npcObjCh(char c){ return (c>='1'&&c<='<')||c=='X'||c=='='||c=='L'||c=='J'||c=='M'; }   // kicker 1-4, quarter pipe 5-8, launch 9-<, funbox, rail, ledge, jersey, manual pad
static void npcAim(NpcSk*k){   // pick the next skate object to ride to
    int n=0; for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(npcObjCh(lifeMap[y][x])) n++;
    if(!n) return;
    int r=(int)(((u32)rnd8()<<8|rnd8())%(unsigned)n);
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(npcObjCh(lifeMap[y][x])&&r--==0){ k->tx=(u8)x; k->ty=(u8)y; return; }
}
static void npcSkTick(NpcSk*k){
    if(k->air){ k->air--; if(!k->air) npcAim(k); return; }
    if(k->wait){ k->wait--; return; }
    int dx=(int)k->tx*256+128-(int)k->fx, dy=(int)k->ty*256+128-(int)k->fy, ax=fxAbs(dx), ay=fxAbs(dy);
    if(ax+ay<=320){ k->air=26; k->tries=0; return; }      // beside the object: a hop
    const int sp=9; int tryX=ax>=ay;
    for(int pass=0;pass<2;pass++,tryX=!tryX){
        s32 nx=k->fx, ny=k->fy;
        if(tryX){ if(ax<sp) continue; nx+=dx<0?-sp:sp; } else { if(ay<sp) continue; ny+=dy<0?-sp:sp; }
        if(!hhWalk((int)(nx>>8),(int)(ny>>8))) continue;
        k->fx=nx; k->fy=ny; if(tryX) k->face=(u8)(dx<0); else k->face=(u8)(dy>0); return; }
    k->wait=40; if(++k->tries>3){ k->tries=0; npcAim(k); }      // boxed in: wait, then try somewhere else
}
static void npcSkSpawn(void){
    npcSkN=0; int n=0; for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(npcObjCh(lifeMap[y][x])) n++;
    if(FLG(1)&&n){   // SKATE FLAGS (main.c): each one spawns a skater (up to NPC_SKN), even on a lot with only a few things to skate
        for(int f=0;f<FLG(1)&&npcSkN<NPC_SKN;f++){ NpcSk*k=&npcSk[npcSkN++]; k->fx=flgX[1][f]*256+128; k->fy=flgY[1][f]*256+128; k->air=0; k->face=0; k->wait=(u8)(20*npcSkN); k->tries=0; npcAim(k); }
        return; }
    if(n<NPC_PARK) return;
    for(int t=0;t<200&&npcSkN<NPC_SKN;t++){
        int x=(int)(((u32)rnd8()<<8|rnd8())%MW), y=(int)(((u32)rnd8()<<8|rnd8())%MH);
        if(!hhWalk(x,y)||fxAbs(x-(int)(lfx>>8))+fxAbs(y-(int)(lfy>>8))<4) continue;
        NpcSk*k=&npcSk[npcSkN++]; k->fx=x*256+128; k->fy=y*256+128; k->air=0; k->face=0; k->wait=(u8)(20*npcSkN); k->tries=0; npcAim(k); }
}

// ---- police ----
// A crime (punching Sims) may send the cops (copCrime: about 6 times in 10 once the heat is 3, a little likelier with every WANTED star). The cops walk in
// from the way off the lot (a community flag or an exit) and RUN at you at the pace you run at (2 steps of a walk each logic step = F_RUN): they get NO speed
// boost, so your BODY skill (skTop) makes you faster than them. They catch you because the chase tires you out (every ~1.5 s of chase costs a point of energy,
// BODY slows that down) and because BACKUP keeps coming (one more cop every 15 s, up to copMax). To get away, reach the lot's way out (the flags / exits where visitors
// come in): "YOU GOT AWAY". But they remember you: each escape adds a WANTED star (copWant, up to 6), and every midnight there is a small chance (about 5% a star)
// that the cops RAID your home lot, with more cops the more stars you have. Being BUSTED clears the stars. Not saved (a new session starts clean).
#define COP_CAP 5    // the most cops a chase or a raid ever sends (what the game asks for is clamped to this)
#define COP_MAX 20   // HARD LIMIT: the size of every cop array and the OAM entries set aside (cop 0 = NPC_OAM0, cop n = NPC_OAM0+3+n). Never exceeded, whatever COP_CAP says.
_Static_assert(COP_CAP<=COP_MAX,"COP_CAP can not be above the hard limit COP_MAX");
_Static_assert(NPC_OAM0+3+COP_MAX-1<128,"the cops' OAM entries run past the 128 the GBA has");
static HhSim copS[COP_MAX] EWRAM_BSS;                          // the cops walk like visitors: path, position and speed are an HhSim's. copS[0] leads (about 190 bytes of EWRAM each: COP_MAX of them are always reserved)
static u8 copN EWRAM_BSS, copMax EWRAM_BSS, copFast EWRAM_BSS, copRp[COP_MAX] EWRAM_BSS;   // cops on the lot, most this chase, raid (backup comes quickly), replan counters
static u8 copSt EWRAM_BSS, copHeat EWRAM_BSS, copTry EWRAM_BSS, copWant EWRAM_BSS, copRaid EWRAM_BSS; static u16 copHT EWRAM_BSS, copCool EWRAM_BSS, copRT EWRAM_BSS, copTired EWRAM_BSS; static short copT EWRAM_BSS;
static char copMsg[20] EWRAM_BSS;                              // copSt: 0 none, 1 called (waiting), 2 running at you, 3 holding you, 4 walking away
static int copHome(void){ return !nbOk||nbT.cur==nbT.home; }    // you are on your own lot
static int copAtExit(int px,int py){   // you are on or next to a way out of the lot (where visitors come in)
    if(FLG(0)){ for(int f=0;f<flgN[0];f++){ int dx=px-flgX[0][f], dy=py-flgY[0][f]; if(dx>=-1&&dx<=1&&dy>=-1&&dy<=1) return 1; } return 0; }
    for(int i=0;i<hhExN;i++){ int dx=px-hhEx[i]%MW, dy=py-hhEx[i]/MW; if(dx>=-1&&dx<=1&&dy>=-1&&dy<=1) return 1; }
    return 0;
}
static int copSpawn(void){   // one more cop walks in from a way off the lot
    if(copN>=COP_MAX||copN>=COP_CAP||copN>=copMax) return 0;
    int a=twFar(); if(a<0) return 0;
    HhSim*c=&copS[copN]; c->fx=(a%MW)*256+128; c->fy=(a/MW)*256+128; c->pn=c->pi=0; c->gok=0; c->stage=AG_ADULT; c->act=HA_WALK; c->hd=0; copRp[copN]=45; copN++; return 1;
}
static void copPlaceAgain(int i){ int a=twFar(); if(a<0) return; HhSim*c=&copS[i]; c->fx=(a%MW)*256+128; c->fy=(a/MW)*256+128; c->pn=c->pi=0; c->gok=0; }
static void copStart(int max,int fast,int wait){ copSt=1; copN=0; copMax=(u8)(max>COP_CAP?COP_CAP:max<1?1:max); if(copMax>COP_MAX) copMax=COP_MAX; copFast=(u8)fast; copT=(short)wait; copTry=0; copRT=0; copTired=0; }
static int prNote(int n); static int prBook(void); static int prGuardTick(int*planned); static const HhSim* prVisSprite(void);   // prison.h
static void copCrime(int n){   // called when you hurt someone
    if(prNote(n)) return;   // prison.h: the record grows (in the prison itself: more days, and no cops)
    if(copCool>0||copSt) return;
    copHeat=(u8)(copHeat+n>9?9:copHeat+n);
    if(copHeat>=3&&(int)rnd8()<150+copWant*15){ copStart(1+(copWant>=2)+(copWant>=4)+(copWant>=6),0,300+rnd8()); lnote="SOMEONE CALLED THE COPS"; lnoteT=90; }   // "there is a chance": not every crime is seen
}
static void copDay(void){   // once a night (sims.h, midnight): the cops that remember you may raid your home
    if(copHeat) copHeat--;
    if(!copWant||copRaid) return;
    if((int)rnd8()<4+copWant*8) copRaid=1;      // a remote chance: about 5% with one star, 20% with six
    else if((rnd8()&3)==0) copWant--;            // and they forget a little
}
static void copArrest(void){
    int jail=prBook();   // prison.h: the rap sheet decides the sentence (before the stars are cleared). 1 = off to the prison
    int fine=simMoney>=50?50:simMoney; simMoney-=fine; copHeat=0; copWant=0; copRaid=0; copSt=3; copT=600; for(int i=0;i<copN;i++) copS[i].act=HA_IDLE;
    moodEvent(M_HURT_BIG); sfxPlay(SFX_HIT); lsp=0; lgrind=0; lscore-=lscore/4;
    if(jail){ copSt=0; copN=0; lnote="BUSTED  OFF TO PRISON"; lnoteT=90; return; }   // (no 10 second hold: the cell it is)
    simCatN(simCat(copMsg,"BUSTED  JAIL "),10); lnote=copMsg; lnoteT=90;
}
static void copEscape(void){   // you reached the way out: the cops lose you, but they remember
    copSt=0; copN=0; copHeat=0; copCool=1800; if(copWant<6) copWant++;
    lnote="YOU GOT AWAY  THEY REMEMBER"; lnoteT=100;
}
static void copTick(int*planned){   // once per logic step (hhTick)
    if(prGuardTick(planned)) return;   // prison.h: in your cell the guards walk the yard and no cops come
    if(copHeat&&++copHT>=600){ copHT=0; copHeat--; }
    if(copCool) copCool--;
    if(ldead){ if(copSt) copSt=0; copN=0; return; }
    if(copRaid&&!copSt&&!copCool&&copHome()){ copRaid=0; copStart(1+(copWant+1)/2+(copWant>=6),1,150); lnote="POLICE AT YOUR DOOR"; lnoteT=100; }   // more cops the more they remember (1 star: 2 cops ... 6 stars: COP_CAP)
    int px=(int)(lfx>>8), py=(int)(lfy>>8), i;
    switch(copSt){
    case 1:
        if(--copT>0) break;
        if(copSpawn()){ copSt=2; copRT=copFast?700:0; lnote="POLICE  FREEZE"; lnoteT=80; } else copSt=0;
        break;
    case 2: {
        int caught=0;
        for(i=0;i<copN;i++){ int ddx=px-(int)(copS[i].fx>>8), ddy=py-(int)(copS[i].fy>>8); if(ddx*ddx+ddy*ddy<=2){ caught=1; break; } }
        if(caught){ copArrest(); break; }
        if(copAtExit(px,py)){ copEscape(); break; }
        if(copN<copMax&&++copRT>=900){ copRT=0; if(copSpawn()){ lnote="MORE COPS ARE COMING"; lnoteT=60; } }   // backup: more of them, never faster
        if(++copTired>=(unsigned)(90+30*skLvl(SK_BODY))){ copTired=0; if(sNrg>0) sNrg--; }   // the chase wears you out (an athlete lasts longer): a tired Sim is slower
        // PLANNING: the game allows one search per logic step (the shared *planned flag), so the cop that is MOST OVERDUE goes first (a cop with no path left
        // before one that is merely due), not simply the lowest number: with many cops the last ones are never starved. A cop close to you re-plans sooner
        // (every 12 + 2 per tile away, up to 45 steps): that is where it matters, and a search that ends next to you is cheap.
        for(i=0;i<copN;i++) if(copRp[i]<255) copRp[i]++;
        if(!*planned){ int best=-1, bw=-1;
            for(i=0;i<copN;i++){ const HhSim*c=&copS[i]; int w;
                if(c->pi>=c->pn) w=1000+copRp[i];   // out of path: needs one now
                else { int ddx=px-(int)(c->fx>>8), ddy=py-(int)(c->fy>>8), iv=12+2*((ddx<0?-ddx:ddx)+(ddy<0?-ddy:ddy)); if(iv>45) iv=45; w=(int)copRp[i]-iv; if(w<0) continue; }
                if(w>bw){ bw=w; best=i; } }
            if(best>=0){ HhSim*c=&copS[best]; *planned=1; copRp[best]=0; hhGX=px; hhGY=py; int r=hhPlan(c,1);
                if(r==1){ copArrest(); break; }
                if(r==0){ if(++copTry>=4){ copSt=0; copN=0; copHeat=0; break; } else copPlaceAgain(best); } } }
        for(i=0;i<copN;i++){ HhSim*c=&copS[i];
            if(c->pi<c->pn){ hhStepAlong(c); if(c->pi<c->pn) hhStepAlong(c); } }   // he runs, at your running pace
        break; }
    case 3:
        if(lstun<2) lstun=2;
        lsp=0; lgrind=0;
        copT--;
        if(copT%60==0&&copT>0){ simCatN(simCat(copMsg,"BUSTED  JAIL "),copT/60); lnote=copMsg; lnoteT=90; }
        if(copT<=0){ copSt=4; copT=1200; copCool=3600; copHeat=0; lnote="RELEASED  BEHAVE";  lnoteT=90;
            for(i=0;i<copN;i++){ copS[i].pn=copS[i].pi=0; copS[i].gok=0; copS[i].act=HA_IDLE; } }
        break;
    case 4: {
        int left=0;
        if(--copT<=0){ copSt=0; copN=0; break; }
        for(i=0;i<copN;i++){ HhSim*c=&copS[i]; if(c->act==HA_AWAY) continue;
            if(c->pn==0){ if(!*planned){ int a=twFar(); *planned=1; if(a>=0){ hhGX=a%MW; hhGY=a/MW; if(hhPlan(c,1)>1){ c->act=HA_WALK; left=1; continue; } } c->act=HA_AWAY; } else left=1; continue; }
            if(c->pi>=c->pn){ c->act=HA_AWAY; continue; }
            hhStepAlong(c); left=1; }
        if(!left) { copSt=0; copN=0; }
        break; }
    default: break; }
}

// ---- per step / play start / vblank (fx.h calls these) ----
static void npcTick(void){ for(int i=0;i<npcSkN;i++) npcSkTick(&npcSk[i]); }
static void npcPlayStart(void){
    npcVramOk=0; if(copSt&&copSt!=3){ copSt=0; copN=0; }
    npcSkSpawn();
}
static void npcObjUpdate(void){
    volatile u16*oam=OAM; if(!npcVramOk){ npcUpload(); npcVramOk=1; }
    int hide=(lcamF>0)||curFl||zoomDma, i;
    for(int ci=0;ci<COP_MAX;ci++){ volatile u16*e=oam+(ci?NPC_OAM0+3+ci:NPC_OAM0)*4; e[0]=0x200;   // the cops: NPC_OAM0, then NPC_OAM0+4.. (NPC_OAM0+1..3 are the skaters)
      if(hide||copSt<2||ci>=copN||copS[ci].act==HA_AWAY) continue;
      const HhSim*c=&copS[ci]; int sx,sy; fxScreen(c->fx,c->fy,&sx,&sy); int x=sx-8, y=sy-30;
      int fr=(copSt==2||copSt==4||(copSt==5&&c->pi<c->pn))&&(((fxT>>3)+ci)&1)?1:0;
      if(x+16<=vpX0||x>=vpX1||y+32<=sbY0||y>=sbY1) continue;
      e[0]=(u16)((y&255)|0x8000); e[1]=(u16)((x&511)|0x8000|((c->hd==4||c->hd==8)?0x1000:0)); e[2]=(u16)((NPC_TILE+fr*8)|(NPC_PALC<<12)); }
    for(i=0;i<NPC_SKN;i++){ volatile u16*e=oam+(NPC_OAM0+1+i)*4; e[0]=0x200;
        if(hide||i>=npcSkN) continue;
        const NpcSk*k=&npcSk[i]; int sx,sy; fxScreen(k->fx,k->fy,&sx,&sy);
        int a=k->air, h=a?a*(26-a)/10:0, x=sx-8, y=sy-30-h;
        if(x+16<=vpX0||x>=vpX1||y+32<=sbY0||y>=sbY1) continue;
        e[0]=(u16)((y&255)|0x8000); e[1]=(u16)((x&511)|0x8000|(k->face?0x1000:0)); e[2]=(u16)((NPC_TILE+(a?3:2)*8)|((NPC_PALS+i)<<12)); }
    if(!hide&&!npcSkN){   // the prison visitor (prison.h) borrows the first skater's sprite slot: the cop art in a skater's colours
        const HhSim*v=prVisSprite();
        if(v){ volatile u16*e=oam+(NPC_OAM0+1)*4; int sx,sy; fxScreen(v->fx,v->fy,&sx,&sy); int x=sx-8, y=sy-30, fr=(v->pi<v->pn)&&((fxT>>3)&1)?1:0;
            if(!(x+16<=vpX0||x>=vpX1||y+32<=sbY0||y>=sbY1)){ e[0]=(u16)((y&255)|0x8000); e[1]=(u16)((x&511)|0x8000|((v->hd==4||v->hd==8)?0x1000:0)); e[2]=(u16)((NPC_TILE+fr*8)|((NPC_PALS+(v->uid%3))<<12)); } } }
}

// ---- lent palette banks (house.h hhObjUpdate0: the prison's crowd) ----
static int hhBanksBusy(void){   // which OBJ palette banks fx.h and npc.h draw with right now (bit = bank): the crowd must not touch them
    int m=0;
    if(xo[XO_GHOSTS]) for(int g=0;g<GH_MAX&&g<fxGN;g++) m|=1<<(FX_PALG+g);
    if(copSt||copN) m|=1<<NPC_PALC;
    if(npcSkN||prVisSprite()) m|=(1<<NPC_PALS)|(1<<(NPC_PALS+1))|(1<<(NPC_PALS+2));
    return m;
}
static void hhExtraDone(void){ fxVramOk=0; npcVramOk=0; }   // the crowd gave the banks back: the ghosts' and the cops' art and colours are written again this vblank (fxObjUpdate, npcObjUpdate)

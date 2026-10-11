// pets.h - PETS: dogs and cats that live with the household (The Sims 2 Pets, on a GBA). The screens are in petui.h.
//   WHO     up to PET_MAX pets a household. A pet has a LOOK (petart.h: DOG, SPOTTED DOG, SPITZ, SPOTTED SPITZ, CAT, TABBY, POINTED CAT), a SIZE,
//           a COAT and an ACCENT colour (any of PET_NC), a name and three traits 0..10: SMART (learns tricks and house rules), HYPER (zoomies, play;
//           low = lazy, sleeps a lot) and FRIENDLY (follows you and greets people; low = independent). A puppy or kitten grows to its size in 3 days.
//   NEEDS   FOOD, BLADDER, ENERGY, FUN (0..100, high is good, per game minute: petMinute) and LOVE (how it feels about the house).
//   LIFE    free will (petDecide): wanders, follows you, begs at the fridge or at you when hungry (a household Sim with nothing on feeds it), sleeps on
//           rugs, next to beds and sofas (anywhere when it is wrecked, and longer at night), the zoomies, barks at the cops, greets visitors.
//           A full bladder: a pet that can get outside goes and does it there. One that cannot (or is already indoors and bursting) MAY have an
//           accident: a puddle on the floor or, next to someone, a lifted leg on them. It is a roll, never certain: house training and
//           LET IT OUT lower the odds, and a pet that holds on whines at you first.
//   SPRITES 32 x 32 hardware sprites (no drawing time): OBJ VRAM slots 14 and 15 (the prison's extra crowd blocks: pets never go to the prison), palette
//           bank 7 shared by every pet (1 outline, 2 eyes and nose, 3 red, then 4 coat colours each), OAM entries in the household's depth order
//           (petOvList, called by hhObjUpdate0). The shadow, balloon and puddles go into the picture like a Sim's (petDrawBand, petRc, petSig).
//   SAVE    in the household block, version 'HC': a count, then PET_B bytes each (petPack / petUnpack), so pets move with their household.
#include "petart.h"
enum { PF_STAND, PF_WALK1, PF_WALK2, PF_SIT, PF_LIE, PF_SLEEP, PF_EAT, PF_HAPPY, PF_PEE };
enum { PN_FOOD, PN_BLAD, PN_NRG, PN_FUN, PN_N };
enum { PTR_SMART, PTR_HYPER, PTR_FRIEND, PTR_N };
enum { PA_IDLE, PA_WANDER, PA_FOLLOW, PA_BEG, PA_SLEEP, PA_ZOOM, PA_OUT, PA_BARK, PA_GREET, PA_SIT, PA_LIE, PA_PEE, PA_EAT, PA_TRICK, PA_HOLD };
#define PET_NC 18
#define PET_OAMT 960     // first OBJ tile of the pets: slot 14 (two pets) and slot 15 (the third)
#define PET_TRK 5        // tricks a pet can learn
typedef struct {
    HhSim w;                       // where it is, its heading and its path (the household's walking: hhPlan / hhStepAlong)
    char name[HH_NM];
    u8 look, size, adult, coat, acc, tr[PTR_N], need[PN_N], love, tricks, potty, age;
    u8 act, go, bub, bubT, pose, view, flip, tgt;   // act and go (0 there, 1 on the way), balloon, the frame shown, whom it walks to (uid+1, 0 you)
    short t, think;
} Pet;
static Pet petM[PET_MAX] EWRAM_BSS; static u8 petN;
static const char* const petLookNm[PET_LOOKS]={"DOG","SPOTTED DOG","SPITZ","SPOTTED SPITZ","CAT","TABBY CAT","POINTED CAT"};
static const char* const petSizeNm[3]={"SMALL","MEDIUM","LARGE"};
static const u16 petCol[PET_NC]={RGB(30,27,20),RGB(28,20,9),RGB(24,17,10),RGB(26,13,6),RGB(30,17,6),RGB(14,9,5),RGB(6,5,6),RGB(11,11,12),RGB(18,18,19),RGB(25,25,26),RGB(31,31,30),RGB(15,17,22),RGB(27,22,15),RGB(19,14,9),RGB(31,19,24),RGB(19,11,26),RGB(12,23,9),RGB(10,21,27)};
static const char* const petColNm[PET_NC]={"CREAM","GOLDEN","TAN","RED","GINGER","CHOCOLATE","BLACK","CHARCOAL","GRAY","SILVER","WHITE","BLUE","FAWN","SABLE","PINK","PURPLE HAZE","KUSH GREEN","SKY"};
enum { PC_CREAM, PC_GOLD, PC_TAN, PC_RED, PC_GINGER, PC_CHOC, PC_BLACK, PC_CHAR, PC_GRAY, PC_SILVER, PC_WHITE, PC_BLUE, PC_FAWN, PC_SABLE, PC_PINK, PC_PURPLE, PC_GREEN, PC_SKY };
typedef struct { const char*nm; u8 look, size, coat, acc; } PetBreed;
#define PET_NBR 18
static const PetBreed petBreed[PET_NBR]={
    {"GOLDEN RETRIEVER",0,2,PC_GOLD,PC_CREAM},{"BLACK LAB",0,2,PC_BLACK,PC_CHAR},{"CHOCOLATE LAB",0,2,PC_CHOC,PC_TAN},{"BEAGLE",0,1,PC_TAN,PC_WHITE},
    {"DALMATIAN",1,2,PC_WHITE,PC_BLACK},{"MUTT",1,1,PC_TAN,PC_CHOC},{"HUSKY",2,2,PC_GRAY,PC_WHITE},{"SHIBA",2,1,PC_GINGER,PC_CREAM},
    {"CORGI",2,0,PC_GINGER,PC_WHITE},{"POMERANIAN",2,0,PC_GOLD,PC_CREAM},{"WOLF",3,2,PC_CHAR,PC_GRAY},
    {"TUXEDO CAT",4,1,PC_BLACK,PC_WHITE},{"BLACK CAT",4,1,PC_BLACK,PC_BLACK},{"ORANGE TABBY",5,1,PC_GINGER,PC_RED},{"GRAY TABBY",5,1,PC_SILVER,PC_CHAR},
    {"MAINE COON",5,2,PC_TAN,PC_CHOC},{"SIAMESE",6,1,PC_CREAM,PC_CHOC},{"SNOWSHOE",6,0,PC_WHITE,PC_GRAY} };
static const char* const petTrickNm[2][PET_TRK]={{"SIT","SHAKE","ROLL OVER","PLAY DEAD","SPEAK"},{"HIGH FIVE","SIT","COME","SPIN","BEG"}};
static const char* const petNameDog[16]={"BISCUIT","NUGGET","MUNCHIES","WAFFLES","BUSTER","SNOOP","MEATBALL","PICKLES","CHEWIE","SIR BARKS","DIESEL","BANDIT","NOODLE","TOAST","ZIGGY","MOOSE"};
static const char* const petNameCat[16]={"MITTENS","SMOKEY","NACHO","OREO","CHEETO","LUNA","SIR PURRS","WHISKERS","BEANS","SHADOW","GIZMO","TOFU","PEACHES","MOCHI","SALEM","DOOBIE"};
static int petCat(const Pet*p){ return p->look>=4; }
static u16 petTk[PET_MAX];       // what each pet's tiles hold now: look, size, view, pose (0xFFFF: nothing)
static short petX[PET_MAX], petY[PET_MAX], petB[PET_MAX], petH[PET_MAX];   // feet on screen, depth band, surface height
static char petMsg[44] EWRAM_BSS;
// PUDDLES: up to 4 on the live floor; R next to one mops it up. Not saved (a new day, a clean floor).
#define PUD_N 4
static u8 pudX[PUD_N], pudY[PUD_N], pudN;
static int petAtHome(void);      // petui.h: on your own lot, not upstairs, not in the prison
static int petCopAt(int*x,int*y);// petui.h: a cop on the lot to bark at (npc.h)
static int petOutside(int x,int y){ return x>=0&&y>=0&&x<MW&&y<MH&&!wInside[y][x]; }   // the outdoors: not inside a walled room

// ---- saving: PET_B bytes ----
static void petPack(volatile u8*m,const Pet*p){
    for(int j=0;j<HH_NM;j++) m[j]=(u8)p->name[j];
    int k=HH_NM; m[k++]=p->look; m[k++]=(u8)(p->size|(p->adult<<2)); m[k++]=p->coat; m[k++]=p->acc;
    for(int j=0;j<PTR_N;j++) m[k++]=p->tr[j];
    for(int j=0;j<PN_N;j++) m[k++]=p->need[j];
    m[k++]=p->love; m[k++]=p->tricks; m[k++]=p->potty; m[k++]=p->age; m[k++]=0; m[k++]=0;   // (two spare bytes)
}
_Static_assert(HH_NM+4+PTR_N+PN_N+6==PET_B,"PET_B is the size of a saved pet");
static int petUnpack(volatile const u8*m,Pet*p){   // 0 = not a pet (the household then has no pets)
    for(int j=0;j<HH_NM;j++) p->name[j]=(char)m[j]; p->name[HH_NM-1]=0;
    int k=HH_NM; p->look=m[k++]; { int s=m[k++]; p->size=(u8)(s&3); p->adult=(u8)((s>>2)&3); } p->coat=m[k++]; p->acc=m[k++];
    for(int j=0;j<PTR_N;j++){ p->tr[j]=m[k++]; if(p->tr[j]>10) p->tr[j]=5; }
    for(int j=0;j<PN_N;j++){ p->need[j]=m[k++]; if(p->need[j]>100) p->need[j]=80; }
    p->love=m[k++]; p->tricks=m[k++]; p->potty=m[k++]; p->age=m[k++];
    if(p->look>=PET_LOOKS||p->size>2||p->adult>2||p->coat>=PET_NC||p->acc>=PET_NC||!p->name[0]) return 0;
    if(p->love>100) p->love=50; if(p->potty>100) p->potty=0;
    return 1;
}
static int petSaveTo(volatile u8*m){ m[0]=petN; for(int i=0;i<petN;i++) petPack(m+1+i*PET_B,&petM[i]); return 1+petN*PET_B; }   // (hhSave)
static int petLoadFrom(volatile u8*m,int n,int avail){   // (hhLoad: n pets at m+1). Returns the bytes the tail takes
    petN=0; if(!m) return 0; (void)avail;
    for(int i=0;i<n&&i<PET_MAX;i++) if(petUnpack(m+1+i*PET_B,&petM[petN])) petN++;
    return 1+n*PET_B;
}
// ---- colours: each pet's four coat colours (dark, mid, light from its coat, then its accent) ----
static u16 petShade(u16 c,int num,int add){ int r=(c&31)*num/16+add, g=((c>>5)&31)*num/16+add, b=((c>>10)&31)*num/16+add; if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; return (u16)(r|(g<<5)|(b<<10)); }
static void petRamp(const Pet*p,u16*o){ u16 c=petCol[p->coat]; o[0]=petShade(c,9,0); o[1]=petShade(c,14,0); o[2]=petShade(c,16,1); o[3]=petShade(petCol[p->acc],15,1); }
static void petPal(void){   // palette bank 7 (in vblank)
    volatile u16*q=OBJ_PAL+PET_PALB*16; q[0]=0; q[1]=RGB(2,2,5); q[2]=RGB(3,2,4); q[3]=RGB(28,7,9);
    for(int i=0;i<PET_MAX;i++){ u16 r[4]={0,0,0,0}; if(i<petN) petRamp(&petM[i],r); for(int c=0;c<4;c++) q[4+i*4+c]=r[c]; }
    hhPkey[PET_PALB]=-1;   // (no Sim's colours are in the bank any more)
}
static void petUp(int i){   // the frame pet i shows into its tiles, its coat colours moved to its own four (in vblank)
    const Pet*p=&petM[i]; u16 key=(u16)(p->look|(p->size<<3)|(p->view<<5)|(p->pose<<6)); if(petTk[i]==key) return; petTk[i]=key;
    const u32*s=petArt[p->look][p->size][p->view][p->pose]; volatile u32*d=(volatile u32*)(OBJ_VRAM+(PET_OAMT-512+i*16)*16); u32 add=(u32)(4*i);
    for(int k=0;k<128;k++){ u32 w=s[k]; if(add){ u32 m=((w>>2)|(w>>3))&0x11111111u; w+=m*add; } d[k]=w; }   // (coat nibbles are 4..7: +4 or +8 cannot carry)
}

// ---- where they are (every logic step, like hhCalc) ----
static int petHgt(const Pet*p){ static const u8 h[3]={13,16,19}; return petCat(p)?h[p->size]-3:h[p->size]; }
static void petCalc(void){
    for(int i=0;i<petN;i++){ Pet*p=&petM[i]; s32 rx,ry; rotPos(p->w.fx,p->w.fy,&rx,&ry);
        petX[i]=(short)(LOX+(int)((rx-ry)>>5)); petH[i]=(short)surfH(p->w.fx,p->w.fy); petY[i]=(short)(LOY+(int)((rx+ry)>>6)); petB[i]=(short)((rx>>8)+(ry>>8));
        int d=(p->w.hd>>2)&3; s32 ax,ay; rotPos(p->w.fx+hhDx[d]*256,p->w.fy+hhDy[d]*256,&ax,&ay);   // which way it faces on screen
        int sdx=(int)((ax-rx)-(ay-ry)), sdy=(int)((ax-rx)+(ay-ry));
        p->view=(u8)(sdy<0); p->flip=(u8)(sdx>0);   // front: towards the camera (left-down; flipped right-down), back: away (left-up; flipped right-up)
    }
}
static int petMoving(const Pet*p){ return p->w.pi<p->w.pn; }
static void petPose(void){   // the frame each pet shows this step
    for(int i=0;i<petN;i++){ Pet*p=&petM[i]; int f=PF_STAND, fast=p->act==PA_ZOOM||p->act==PA_BARK||p->act==PA_GREET;
        if(petMoving(p)) f=((lfr+i*3)>>(fast?2:3))&1?PF_WALK2:PF_WALK1;
        else switch(p->act){
            case PA_SLEEP: f=p->go?PF_STAND:PF_SLEEP; break;
            case PA_EAT: f=PF_EAT; break;
            case PA_BEG: f=((lfr>>5)&1)&&!petCat(p)?PF_HAPPY:PF_SIT; break;
            case PA_SIT: case PA_HOLD: f=PF_SIT; break;
            case PA_LIE: f=PF_LIE; break;
            case PA_PEE: f=PF_PEE; break;
            case PA_BARK: case PA_GREET: f=((lfr>>3)&1)?PF_HAPPY:PF_STAND; break;
            case PA_TRICK: f=p->tgt; break;
            case PA_FOLLOW: f=PF_SIT; break;
            default: f=PF_STAND; }
        p->pose=(u8)f; }
}
// ---- into the picture: shadow, balloon, puddles (band by band, like hhDrawBand) ----
static void pudScreen(int k,int*sx,int*sy){ s32 rx,ry; rotPos(pudX[k]*256+128,pudY[k]*256+128,&rx,&ry); *sx=LOX+(int)((rx-ry)>>5); *sy=LOY+(int)((rx+ry)>>6); }
static int pudBand(int k){ s32 rx,ry; rotPos(pudX[k]*256+128,pudY[k]*256+128,&rx,&ry); return (int)((rx>>8)+(ry>>8)); }
static void petDrawBand(int s0,int s1){
    for(int k=0;k<pudN;k++){ int b=pudBand(k); if(b<s0||b>s1) continue; int x,y; pudScreen(k,&x,&y);
        rect(x-4,y-2,8,1,RGB(24,22,6)); rect(x-6,y-1,12,1,RGB(26,24,8)); rect(x-5,y,10,1,RGB(22,20,5)); rect(x-3,y+1,6,1,RGB(19,17,4)); px(x-2,y-1,RGB(31,31,22)); }
    for(int i=0;i<petN;i++){ const Pet*p=&petM[i]; if(petB[i]<s0||petB[i]>s1) continue;
        int x=petX[i], y=petY[i]-petH[i];
        if(sShad) rect(x-4,y-1,9,2,RGB(10,8,5));
        if(p->bubT){ int bx=x-5, by=y-petHgt(p)-13; rect(bx,by,11,10,RGB(14,16,22)); rect(bx+1,by+1,9,8,WHITE);
            simIcon(bx+2,by+1,p->bub,p->bub==IC_HEART?RGB(28,6,12):p->bub==IC_ANGRY?RGB(26,4,4):RGB(4,4,10)); px(x,by+10,RGB(14,16,22)); } }
}
static void petRc(int i,HhR*r){ const Pet*p=&petM[i]; int x=petX[i], y=petY[i]-petH[i];
    r->x0=(short)(x-5); r->x1=(short)(x+6); r->y0=(short)(y-1); r->y1=(short)(y+1);
    if(p->bubT){ r->y0=(short)(y-petHgt(p)-13); r->x0=(short)(x-5); r->x1=(short)(x+6); } }
static unsigned petSig(int i){ const Pet*p=&petM[i]; return (unsigned)(petX[i]&0x3FF)|((unsigned)(petY[i]&0x3FF)<<10)|((unsigned)(petH[i]&15)<<20)|((unsigned)(p->bubT?1+(p->bub&31):0)<<24); }
static HhR petOld[PET_MAX], pudOld[PUD_N]; static unsigned petOldSig[PET_MAX], pudOldSig; static u8 pudOldN;   // what the last picture had (main.c livePatch)
static unsigned pudSig(void){ unsigned h=pudN; for(int k=0;k<pudN;k++) h=h*131u+pudX[k]*7u+pudY[k]*977u; return h; }
static void pudRc(int k,HhR*r){ int x,y; pudScreen(k,&x,&y); r->x0=(short)(x-7); r->x1=(short)(x+7); r->y0=(short)(y-3); r->y1=(short)(y+3); }
// ---- the sprites (hhObjUpdate0 calls this in vblank): load what changed, say where each one goes ----
static int petBehind(int i){ return hhBehindAt(petM[i].w.fx,petM[i].w.fy); }
static int petPlayerFront(int sx,int sy){ s32 rx,ry; rotPos(lfx,lfy,&rx,&ry); int px=LOX+(int)((rx-ry)>>5), py=LOY+(int)((rx+ry)>>6), dx=px-sx; return py>sy&&py-34<sy&&dx>-16&&dx<16; }
static int petOvList(PetOv*o){
    if(!petN||!petAtHome()||prHere()){ petVramBad=1; return 0; }
    if(petVramBad){ for(int i=0;i<PET_MAX;i++) petTk[i]=0xFFFF; petVramBad=0; }
    petPal(); int n=0;
    for(int i=0;i<petN;i++){ int x=petX[i]-16, y=petY[i]-petH[i]-27;
        if(x+32<=vpX0||x>=vpX1||y+32<=vpY0||y>=vpY1) continue;
        petUp(i);
        PetOv*q=&o[n++]; q->x=(short)x; q->y=(short)y; q->dep=petB[i]; q->tile=(u16)(PET_OAMT+i*16); q->flip=petM[i].flip; q->blend=(u8)(petBehind(i)||petPlayerFront(petX[i],petY[i]-petH[i])); }
    for(int a=1;a<n;a++){ PetOv t=o[a]; int b=a; while(b>0&&o[b-1].dep<t.dep){ o[b]=o[b-1]; b--; } o[b]=t; }   // nearest the camera first
    return n;
}

// ---- free will ----
static void petSay(Pet*p,int ic,int t){ p->bub=(u8)ic; p->bubT=(u8)t; }
static void petNote(const Pet*p,const char*w){ if(lnoteT>40) return; char*e=simCat(petMsg,p->name); simCat(e,w); lnote=petMsg; lnoteT=150; }   // (a long note slides along the top bar: hud.h MARQUEE)
static int petNear(const Pet*p,s32 fx,s32 fy,int d){ s32 dx=p->w.fx-fx, dy=p->w.fy-fy; return dx<d&&dx>-d&&dy<d&&dy>-d; }
// A search from the pet (the household's BFS scratch): the nearest tile that fits mode, a path there in p->w. Modes: 0 somewhere random, 1 next to (gx, gy),
// 2 outdoors, 3 a place to sleep (a rug, next to a bed or a sofa), 4 next to the fridge. 1 = a way was found (or it is there already).
static int petPlan(Pet*p,int mode,int gx,int gy){
    HhSim*s=&p->w; int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8);
    if(!hhWalk(sx,sy)){ if(!hhOut(s)) return 0; sx=(int)(s->fx>>8); sy=(int)(s->fy>>8); }
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    int qh=0, qt=0, goal=-1, pick=mode==0?(rnd8()&63)+12:0;
    hhDist[sy*MW+sx]=0; hhQ[qt++]=(u16)(sy*MW+sx);
    while(qh<qt){ int q=hhQ[qh++], x=q%MW, y=q/MW, ok=0;
        switch(mode){ case 0: ok=qh>=pick; break; case 1: ok=(x-gx)*(x-gx)+(y-gy)*(y-gy)<=2&&!(x==gx&&y==gy); break; case 2: ok=petOutside(x,y)&&hhDist[q]>0; break;
            case 3: ok=lifeMap[y][x]=='r'||hhNextTo(x,y,'S')||hhNextTo(x,y,'C'); break; case 4: ok=hhNextTo(x,y,'F'); break; }
        if(ok){ goal=q; break; }
        if(hhDist[q]>=HH_PATH) continue;
        for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d], np=ny*MW+nx; if(hhWalk(nx,ny)&&hhDist[np]==0xFFFF){ hhDist[np]=(u16)(hhDist[q]+1); hhQ[qt++]=(u16)np; } } }
    if(goal<0) return 0;
    int n=hhDist[goal], q=goal; s->pn=(u8)n; s->pi=0; s->gok=0;
    for(int k=n-1;k>=0;k--){ int x=q%MW, y=q/MW;
        for(int d=0;d<4;d++){ int px=x-hhDx[d], py=y-hhDy[d], pp=py*MW+px; if(px>=0&&py>=0&&px<MW&&py<MH&&hhDist[pp]==hhDist[q]-1){ s->path[k]=(u8)d; q=pp; break; } } }
    return 1;
}
static void petStep(Pet*p){   // along the path; a lazy pet ambles, the zoomies are a sprint
    HhSim*s=&p->w; int d=s->path[s->pi];
    if(!s->gok&&!hhWalk((int)(s->fx>>8)+hhDx[d],(int)(s->fy>>8)+hhDy[d])){ s->pi=s->pn; return; }
    if(!s->gok){ s->gx=((int)(s->fx>>8)+hhDx[d])*256+128; s->gy=((int)(s->fy>>8)+hhDy[d])*256+128; s->fx=(s->fx&~255)|128; s->fy=(s->fy&~255)|128; s->gok=1; }
    int sp=petCat(p)?6:7; if(p->act==PA_ZOOM||p->act==PA_BARK||p->act==PA_GREET) sp=15+p->tr[PTR_HYPER]/2; else if(p->act==PA_OUT&&p->need[PN_BLAD]<15) sp=11;
    if(p->size==0) sp=sp*4/5; if(sp<3) sp=3;
    if(hhDx[d]){ s->fx+=hhDx[d]*sp; if((hhDx[d]>0&&s->fx>=s->gx)||(hhDx[d]<0&&s->fx<=s->gx)){ s->fx=s->gx; s->pi++; s->gok=0; } }
    else { s->fy+=hhDy[d]*sp; if((hhDy[d]>0&&s->fy>=s->gy)||(hhDy[d]<0&&s->fy<=s->gy)){ s->fy=s->gy; s->pi++; s->gok=0; } }
    s->hd=(u8)(d*4);
}
static void petFace(Pet*p,s32 fx,s32 fy){ s32 dx=fx-p->w.fx, dy=fy-p->w.fy; p->w.hd=(u8)((dx<0?-dx:dx)>(dy<0?-dy:dy)?(dx>0?0:8):(dy>0?4:12)); }
static int pudAt(int x,int y){ for(int k=0;k<pudN;k++) if(pudX[k]==x&&pudY[k]==y) return k; return -1; }
static void pudAdd(int x,int y){ if(pudAt(x,y)>=0) return; if(pudN>=PUD_N){ for(int k=1;k<PUD_N;k++){ pudX[k-1]=pudX[k]; pudY[k-1]=pudY[k]; } pudN--; } pudX[pudN]=(u8)x; pudY[pudN]=(u8)y; pudN++; }
// THE ACCIDENT ROLL: a pet that has to go and is not outdoors. Never certain: a trained pet mostly holds on and whines instead.
// Someone right next to a dog (you or a household Sim) may get it on the leg.
static void petAccident(Pet*p){
    int pct=40-p->potty*3/10+(p->need[PN_BLAD]<5?20:0)-p->tr[PTR_SMART];   // 40% untrained .. about 10% fully house trained (more when it is bursting)
    if(pct<6) pct=6;
    if((int)(((int)rnd8()*100>>8))>=pct){ p->act=PA_HOLD; p->go=0; p->t=240; petSay(p,IC_WC,240); petNote(p,petCat(p)?" HAS TO GO  LET IT OUT":" WHINES AT THE DOOR"); return; }
    int x=(int)(p->w.fx>>8), y=(int)(p->w.fy>>8);
    if(!petCat(p)&&(rnd8()&1)){   // a leg nearby?
        if(petNear(p,lfx,lfy,400)&&!lskate){ petFace(p,lfx,lfy); p->act=PA_PEE; p->go=0; p->t=70; p->need[PN_BLAD]=100;
            sHyg=sHyg>30?sHyg-30:0; moodEvent(M_SOAKED); voxPlay(V_shriek); petNote(p," PEED ON YOUR LEG"); return; }
        for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; if(s->act==HA_AWAY||!petNear(p,s->fx,s->fy,400)) continue;
            petFace(p,s->fx,s->fy); p->act=PA_PEE; p->go=0; p->t=70; p->need[PN_BLAD]=100;
            s->need[HN_CLEAN]=(u8)(s->need[HN_CLEAN]>30?s->need[HN_CLEAN]-30:0); s->bub=IC_ANGRY; s->bubT=120;
            char*e=simCat(petMsg,p->name); e=simCat(e," PEED ON "); simCat(e,s->name); lnote=petMsg; lnoteT=150; voxSay(s->uid,V_shriek); return; } }
    p->act=PA_PEE; p->go=0; p->t=70; p->need[PN_BLAD]=100; pudAdd(x,y); petNote(p,petCat(p)?" MISSED THE LITTER BOX":" HAD AN ACCIDENT");
}
static void petStartAct(Pet*p,int act,int t){ p->act=(u8)act; p->go=(u8)(p->w.pn>0); p->t=(short)t; }
static void petDecide(Pet*p){
    int night=simIsNight(), cat=petCat(p), hy=p->tr[PTR_HYPER], fr=p->tr[PTR_FRIEND];
    if(p->need[PN_BLAD]<22){   // has to go: outdoors if it can get there, else the accident roll
        if(!petOutside((int)(p->w.fx>>8),(int)(p->w.fy>>8))&&petPlan(p,2,0,0)&&p->w.pn<=30){ petStartAct(p,PA_OUT,0); return; }
        if(petOutside((int)(p->w.fx>>8),(int)(p->w.fy>>8))){ p->act=PA_PEE; p->go=0; p->t=60; p->need[PN_BLAD]=100; if(p->potty<100) p->potty++; return; }
        petAccident(p); return; }
    if(p->need[PN_NRG]<20||(night&&p->need[PN_NRG]<75)||(!hy&&p->need[PN_NRG]<50)){   // tired: a bed of its choosing, or right here
        if(petPlan(p,3,0,0)&&p->w.pn<=24) petStartAct(p,PA_SLEEP,night?1200:500); else { p->w.pn=p->w.pi=0; petStartAct(p,PA_SLEEP,night?1200:500); } return; }
    if(p->need[PN_FOOD]<30){   // hungry: begs at you (if you are close) or sits by the fridge
        if(petNear(p,lfx,lfy,8*256)&&petPlan(p,1,(int)(lfx>>8),(int)(lfy>>8))){ petStartAct(p,PA_BEG,420); petSay(p,IC_FOOD,200); return; }
        if(petPlan(p,4,0,0)){ petStartAct(p,PA_BEG,420); petSay(p,IC_FOOD,200); return; } }
    { int cx,cy; if(!cat&&petCopAt(&cx,&cy)&&petPlan(p,1,cx,cy)){ petStartAct(p,PA_BARK,150); petSay(p,IC_ANGRY,150); petNote(p," BARKS AT THE COPS"); return; } }
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); if(v>=hhN&&twHas[k]&&twOn[k]==1&&!cat&&fr>=4&&(rnd8()&3)==0){ HhSim*s=&hhM[v]; if(petPlan(p,1,(int)(s->fx>>8),(int)(s->fy>>8))){ petStartAct(p,PA_GREET,120); petSay(p,IC_HEART,120); return; } } }   // a visitor walking in: the dog runs to say hello
    int r=((int)rnd8()*100>>8);
    if(p->need[PN_FUN]<35||r<hy*3){   // the zoomies (a cat: a mad dash and a pounce)
        if(petPlan(p,0,0,0)){ petStartAct(p,PA_ZOOM,0); if((rnd8()&3)==0) petNote(p,cat?" HAS THE MIDNIGHT CRAZIES":" HAS THE ZOOMIES"); return; } }
    if(r<35+fr*4&&petNear(p,lfx,lfy,10*256)&&!petNear(p,lfx,lfy,2*256)&&petPlan(p,1,(int)(lfx>>8),(int)(lfy>>8))){ petStartAct(p,PA_FOLLOW,200+fr*20); return; }   // friendly: comes to you
    if(r<70&&petPlan(p,0,0,0)){ petStartAct(p,PA_WANDER,0); return; }
    if(cat&&r<80){ p->act=PA_SIT; p->go=0; p->t=300; if((rnd8()&7)==0) petNote(p,(rnd8()&1)?" STARES AT NOTHING":" JUDGES YOU SILENTLY"); return; }
    p->act=(u8)((r&1)?PA_LIE:PA_SIT); p->go=0; p->t=(short)(240+(10-hy)*30);
}
static void petTick(int*planned){
    if(!petN||!petAtHome()) return;
    for(int i=0;i<petN;i++){ Pet*p=&petM[i];
        if(p->bubT) p->bubT--;
        if(petMoving(p)){ petStep(p);
            if(p->act==PA_BARK){ int cx,cy; if(petCopAt(&cx,&cy)&&(lfr&31)==0) petSay(p,IC_ANGRY,40); }
            continue; }
        if(p->go){ p->go=0;   // arrived
            switch(p->act){
                case PA_OUT: p->act=PA_PEE; p->t=60; p->need[PN_BLAD]=100; if(p->potty<100) p->potty+=2; break;
                case PA_BEG: case PA_FOLLOW: petFace(p,lfx,lfy); break;
                case PA_WANDER: case PA_ZOOM: p->think=(short)(p->act==PA_ZOOM&&p->need[PN_FUN]<90?8:60+(rnd8()&63)); p->act=PA_IDLE; break;   // (the zoomies go on to the next spot)
                case PA_BARK: case PA_GREET: p->t=90; break;
                default: break; }
            continue; }
        switch(p->act){
            case PA_IDLE: break;
            case PA_SLEEP: if(p->need[PN_NRG]<100&&(lfr&7)==0) p->need[PN_NRG]++; if((lfr&127)==0) petSay(p,IC_ZZZ,60);
                if(--p->t<=0||(p->need[PN_NRG]>=100&&!simIsNight())){ p->act=PA_IDLE; p->think=30; } continue;
            case PA_BEG: if(--p->t<=0||p->need[PN_FOOD]>=60){ p->act=PA_IDLE; p->think=40; break; }
                if((p->t&127)==0) petSay(p,IC_FOOD,80);
                if(p->t==200&&hhN&&xo[XO_FREEWILL]){ for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; if(s->act!=HA_IDLE&&s->act!=HA_WANDER) continue; if(s->stage<AG_CHILD) continue;   // someone with nothing on feeds it
                        p->need[PN_FOOD]=100; p->act=PA_EAT; p->t=120; s->bub=IC_HEART; s->bubT=60; char*e=simCat(petMsg,s->name); e=simCat(e," FED "); simCat(e,p->name); lnote=petMsg; lnoteT=150; break; } }
                continue;
            case PA_HOLD: if(--p->t<=0){ p->act=PA_IDLE; p->think=10; } continue;
            default: if(--p->t<=0){ p->act=PA_IDLE; p->think=(short)(30+(rnd8()&63)); } continue; }
        if(--p->think<=0&&!*planned){ *planned=1; petDecide(p); p->think=(short)(90+(rnd8()&63)); }
    }
    petPose();
}
static void petMinute(void){   // once per game minute: needs
    for(int i=0;i<petN;i++){ Pet*p=&petM[i]; int m=simMin;
        if(m%9==i&&p->need[PN_FOOD]>0) p->need[PN_FOOD]--;
        if(m%5==i&&p->need[PN_BLAD]>0&&p->act!=PA_SLEEP) p->need[PN_BLAD]--;
        if(m%(7-p->tr[PTR_HYPER]/4)==0&&p->act!=PA_SLEEP&&p->need[PN_NRG]>0) p->need[PN_NRG]--;
        if(m%6==i&&p->need[PN_FUN]>0) p->need[PN_FUN]--;
        if(m%90==0&&p->love>0&&p->need[PN_FOOD]<20) p->love--;   // a hungry pet sulks
        if(p->act==PA_ZOOM&&p->need[PN_FUN]<100) p->need[PN_FUN]+=2; }
}
static void petDay(void){   // midnight: a day older; a puppy or kitten grows; the floor is mopped by morning
    for(int i=0;i<petN;i++){ Pet*p=&petM[i]; if(p->age<255) p->age++; if(p->size<p->adult&&p->age>=3*(p->size+1)){ p->size++; petTk[i]=0xFFFF; } }
}
static void petPlace(Pet*p,int k){   // somewhere free near you
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int r=1;r<=6;r++)for(int t=0;t<16;t++){ int x=px+((t+k*3)%(2*r+1))-r, y=py+r-((t*7+k)%(2*r+1)); if(hhWalk(x,y)&&!(x==px&&y==py)){ p->w.fx=x*256+128; p->w.fy=y*256+128; p->w.pn=p->w.pi=0; p->w.gok=0; return; } }
    p->w.fx=lfx; p->w.fy=lfy; p->w.pn=p->w.pi=0; p->w.gok=0;
}
static void petStart(void){   // entering the life game (after hhLoad): stand the pets near you
    pudN=0; petVramBad=1;
    for(int i=0;i<petN;i++){ Pet*p=&petM[i]; petPlace(p,i); p->act=PA_IDLE; p->go=0; p->think=(short)(30+i*40); p->bubT=0; p->w.hd=(u8)((rnd8()&3)*4); p->w.stage=AG_ADULT; }
    petCalc(); petPose();
}

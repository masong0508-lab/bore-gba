// house.h - BORE households: up to 8 Sims living together. You control one (the player, main.c's life globals); the others look after
// themselves with FREE WILL. SELECT (tap) in the life game switches who you control.
// Include AFTER faceView / rotPos / tileH / lifeMap / bakeSprites / blit / menu / toast and BEFORE drawRoomRect.
//
//  MEMBERS   hhM[0..hhN-1] are the Sims you are NOT controlling. Each has a look and persona (like the creator's), a stage, a position, its
//            own needs (FOOD WC REST CLEAN COMFY FUN) and an action. Their sprites are baked into hhSpr[] (11 KB each, EWRAM).
//  FREE WILL every so often a member scores what it could do: each kind of furniture ADVERTISES a need (fridge FOOD, toilet WC, bed REST,
//            shower CLEAN, sofa COMFY) and skating about advertises FUN. score = how low the need is (squared, so urgent wins) x the trait
//            tilt, minus the walk. It picks one of the two best at random, finds a path (BFS on the 40x40 tiles, one member plans per
//            step), walks next to the furniture and uses it. With nothing pressing it wanders. OPTIONS > PLAY > FREE WILL: OFF / LOW / HIGH.
//  SWITCH    SELECT swaps the player with the next member: position, needs, look, persona and sprites change places. (The aspiration
//            meter, wants, job and cash belong to the household for now: they stay with whoever you control.)
//  FAMILIES  premade households (original characters) move in from the pause menu (HOUSEHOLD). Saved in SRAM at HH_OFF.
//
// TUNING
#define HH_MAX     7       // members besides the player (8 Sims in all)
#define HH_THINK   90      // steps between a member's decisions (FREE WILL HIGH; LOW thinks half as often and lets needs sink lower)
#define HH_PATH    96      // longest path a member remembers (steps between tiles)
#define HH_USE     240     // steps a member spends using a piece of furniture
#define HH_OFF     18448   // SRAM: the household (after SAVE MEMORY TEST's 16 bytes; slots start at 20480)
enum { HA_IDLE, HA_WALK, HA_USE, HA_WANDER };
enum { HN_FOOD, HN_WC, HN_REST, HN_CLEAN, HN_COMFY, HN_FUN, HN_N };
static const char hnFurn[HN_N]={'F','T','S','H','C',0};   // what each need's furniture is (FUN has none: skate about)
typedef struct {
    u8 look[LK_N], stage, asp, ltw, tr[TR_N];
    char name[10];
    s32 fx, fy;              // position in 1/256 tiles (like lfx, lfy)
    u8 hd, need[HN_N];       // heading (16 steps), needs 0..100 (WC here is 100 = empty bladder, like every other need: high is good)
    u8 act, use, pn, pi;     // action, need being refilled, path length, step along it
    u8 gok; s32 gx, gy;      // the tile centre this step walks to (fixed when the step starts)
    short think, t;          // steps to the next decision, steps left in the action
    u8 path[HH_PATH];        // directions: 0 +x, 1 +y, 2 -x, 3 -y
} HhSim;
static HhSim hhM[HH_MAX] EWRAM_BSS; static int hhN;
static u16 hhSpr[HH_MAX][4][SPW*SPH] EWRAM_BSS;   // baked sprites, one set per member
static u16 hhDist[MH*MW] EWRAM_BSS; static u16 hhQ[MH*MW] EWRAM_BSS;   // BFS scratch, shared (one member plans per step)
static int hhPlanNext;   // round robin: whose turn it is to plan
static const signed char hhDx[4]={1,0,-1,0}, hhDy[4]={0,1,0,-1};

// ---- premade families (original characters) ----
typedef struct { const char* name; u8 look[LK_TAIL]; u8 stage, asp, sign; } HhPre;   // looks without Spore parts; traits come from a sign
typedef struct { const char* fam; u8 n; HhPre m[4]; } HhFam;
//                     look: SHAPE SKIN EYES MOUTH EARS HSTYLE HCOL TOP BOT  TONE EARSZ EARLF
static const HhFam hhFams[]={
    {"THE GRINDERS",3,{ {"REX", {5,2,2,1,1,0,0,1,1, 0,0,0},AG_ADULT,AS_POP,  0},
                        {"DEE", {4,1,1,1,1,2,3,3,0, 0,0,0},AG_ADULT,AS_FORTUNE,9},
                        {"PIP", {0,1,2,2,2,1,3,2,5, 0,0,0},AG_CHILD,AS_GROW,  2} }},
    {"THE MIDNIGHTS",3,{ {"MORTIMER",{6,0,0,0,0,2,0,7,1, 4,0,0},AG_ADULT,AS_KNOW, 5},
                        {"VESPER", {4,0,0,1,1,2,7,5,1, 4,0,0},AG_ADULT,AS_HOME, 7},
                        {"WREN",   {0,0,0,0,1,1,0,7,7, 4,0,0},AG_TEEN, AS_PLEAS,10} }},
    {"THE FRESHLYS",2,{ {"BEN",   {0,3,1,1,1,0,1,4,3, 0,0,0},AG_ADULT,AS_FORTUNE,1},
                        {"BEA",   {4,3,2,1,1,2,2,6,0, 0,0,0},AG_ADULT,AS_PLEAS, 6} }},
};
#define HH_NFAM ((int)(sizeof(hhFams)/sizeof(hhFams[0])))

// ---- baking: render a member's look with the creator's own code, then put the player's creature back ----
static void hhBakeAll(void){
    static u8 sv[H][D][W] EWRAM_BSS; static u16 sd[H][D][W] EWRAM_BSS; u8 sl[LK_N]; u8 sst=stage; int sc=custom;
    for(int i=0;i<LK_N;i++) sl[i]=look[i];
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ sv[y][z][x]=vox[y][z][x]; sd[y][z][x]=dec[y][z][x]; }
    for(int m=0;m<hhN;m++){
        for(int i=0;i<LK_N;i++) look[i]=hhM[m].look[i]; stage=hhM[m].stage;
        buildLook(); setColors(); bakeInto(hhSpr[m]);
    }
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=sst;
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ vox[y][z][x]=sv[y][z][x]; dec[y][z][x]=sd[y][z][x]; }
    custom=sc; setColors();
    bakeInto(spr4);   // the player last: it also sets the blit box (spBx0..), widened below to hold every member
    for(int m=0;m<hhN;m++) for(int v=0;v<4;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) if(hhSpr[m][v][y*SPW+x]!=SKY){
        if(x<spBx0) spBx0=x; if(x+1>spBx1) spBx1=x+1; if(y<spBy0) spBy0=y; if(y+1>spBy1) spBy1=y+1; }
}

// ---- where members can stand ----
static int hhWalk(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 0; char c=lifeMap[y][x]; return c!='w'&&c!='W'&&tileH(x,y)<=3; }
static void hhPlace(HhSim*s,int k){   // somewhere free near the spawn point, spread out a little
    for(int r=1;r<12;r++)for(int t=0;t<40;t++){ int x=spx+((rnd8()%(2*r+1))-r), y=spy+((rnd8()%(2*r+1))-r);
        if(hhWalk(x,y)&&(x!=spx||y!=spy)){ s->fx=x*256+128; s->fy=y*256+128; return; } }
    s->fx=spx*256+128; s->fy=spy*256+128; (void)k;
}
static void hhNew(HhSim*s,const HhPre*p){
    for(int i=0;i<LK_N;i++) s->look[i]=i<LK_TAIL?p->look[i]:0;
    s->stage=p->stage; s->asp=p->asp; s->ltw=0; for(int i=0;i<TR_N;i++) s->tr[i]=signTr[p->sign][i];
    int i=0; for(;p->name[i]&&i<9;i++) s->name[i]=p->name[i]; s->name[i]=0;
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15)); s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
    hhPlace(s,0);
}

// ---- path finding: BFS from the member's tile; the goal is any free tile next to furniture c (or a random free tile for c=0) ----
static int hhNextTo(int x,int y,char c){ for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d]; if(nx>=0&&ny>=0&&nx<MW&&ny<MH&&lifeMap[ny][nx]==c) return 1; } return 0; }
static int hhPlan(HhSim*s,char c){   // fills s->path; returns its length+1 (1 = already there), 0 = no way
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if(!hhWalk(sx,sy)) return 0;
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    int qh=0, qt=0, goal=-1, pick=c?0:(rnd8()*4+rnd8())%400+40;   // wander: the pick-th tile the search reaches
    hhDist[sy*MW+sx]=0; hhQ[qt++]=(u16)(sy*MW+sx);
    while(qh<qt){ int p=hhQ[qh++], x=p%MW, y=p/MW;
        if(c?hhNextTo(x,y,c):(qh>=pick)){ goal=p; break; }
        if(hhDist[p]>=HH_PATH) continue;
        for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d], np=ny*MW+nx; if(hhWalk(nx,ny)&&hhDist[np]==0xFFFF){ hhDist[np]=(u16)(hhDist[p]+1); hhQ[qt++]=(u16)np; } } }
    if(goal<0) return 0;
    int n=hhDist[goal], p=goal; s->pn=(u8)n; s->pi=0; s->gok=0;
    for(int k=n-1;k>=0;k--){ int x=p%MW, y=p/MW;   // walk back down the distances
        for(int d=0;d<4;d++){ int px=x-hhDx[d], py=y-hhDy[d], pp=py*MW+px; if(px>=0&&py>=0&&px<MW&&py<MH&&hhDist[pp]==hhDist[p]-1){ s->path[k]=(u8)d; p=pp; break; } } }
    return n+1;
}

// ---- free will ----
static int hhTilt(const HhSim*s,int n){   // traits: neat Sims shower sooner, lazy ones sit, playful ones skate (percent)
    switch(n){ case HN_CLEAN: return 70+s->tr[TR_NEAT]*6; case HN_COMFY: return 130-s->tr[TR_ACT]*6; case HN_FUN: return 70+s->tr[TR_PLAY]*6; default: return 100; }
}
static void hhDecide(HhSim*s){
    int best[2]={-1,-1}, bs[2]={0,0}, low=xo[XO_FREEWILL]==1?35:55;   // LOW free will waits until needs are lower
    for(int n=0;n<HN_N;n++){
        if(hnFurn[n]&&!(simHave&(n==HN_FOOD?SR_FRIDGE:n==HN_WC?SR_TOILET:n==HN_REST?SR_BED:n==HN_CLEAN?SR_SHOWER:SR_SOFA))) continue;   // no such furniture
        int v=s->need[n]; if(v>=low+30) continue;
        int u=(100-v)*(100-v)/100*hhTilt(s,n)/100;
        if(u>bs[0]){ bs[1]=bs[0]; best[1]=best[0]; bs[0]=u; best[0]=n; } else if(u>bs[1]){ bs[1]=u; best[1]=n; }
    }
    int n=best[0]; if(best[1]>=0&&bs[1]*4>=bs[0]*3&&(rnd8()&1)) n=best[1];   // close call: either of the two
    if(n<0){ if(hhPlan(s,0)>1){ s->act=HA_WANDER; s->use=HN_FUN; } else s->act=HA_IDLE; return; }
    int r=hhPlan(s,hnFurn[n]?hnFurn[n]:0);
    if(r==1&&hnFurn[n]){ s->act=HA_USE; s->use=(u8)n; s->t=HH_USE; }
    else if(r>1){ s->act=hnFurn[n]?HA_WALK:HA_WANDER; s->use=(u8)n; }
    else s->act=HA_IDLE;
}
static void hhTick(void){   // once per logic step in the life game
    if(!hhN) return;
    int fe=oFoodEvery(), we=oWcEvery(), planned=0;
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m];
        // needs drain (gently: the Sims you do not watch should not be in constant crisis)
        if(fe&&lfr%(fe*2)==0&&s->need[HN_FOOD]>0) s->need[HN_FOOD]--;
        if(we&&lfr%(we*2)==0&&s->need[HN_WC]>0) s->need[HN_WC]--;
        if(lfr%300==m*7){ for(int n=HN_REST;n<HN_N;n++) if(s->need[n]>0) s->need[n]--; }
        if(!xo[XO_FREEWILL]){ s->act=HA_IDLE; continue; }
        if(s->act==HA_USE){   // using furniture: refill, then free again
            if(s->need[s->use]<100&&(lfr&1)) s->need[s->use]++;
            if(--s->t<=0||s->need[s->use]>=100){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); }
            continue;
        }
        if(s->act==HA_WALK||s->act==HA_WANDER){   // follow the path, tile centre to tile centre
            if(s->pi>=s->pn){ if(s->act==HA_WALK){ s->act=HA_USE; s->t=HH_USE; } else { s->act=HA_IDLE; if(s->need[HN_FUN]<90) s->need[HN_FUN]+=10; } continue; }
            int d=s->path[s->pi];
            if(!s->gok){ s->gx=((int)(s->fx>>8)+hhDx[d])*256+128; s->gy=((int)(s->fy>>8)+hhDy[d])*256+128; s->fx=(s->fx&~255)|128; s->fy=(s->fy&~255)|128; s->gok=1; }
            int sp=F_WALK*stSpd[s->stage]/100; if(sp<2) sp=2;
            if(hhDx[d]){ s->fx+=hhDx[d]*sp; if((hhDx[d]>0&&s->fx>=s->gx)||(hhDx[d]<0&&s->fx<=s->gx)){ s->fx=s->gx; s->pi++; s->gok=0; } }
            else { s->fy+=hhDy[d]*sp; if((hhDy[d]>0&&s->fy>=s->gy)||(hhDy[d]<0&&s->fy<=s->gy)){ s->fy=s->gy; s->pi++; s->gok=0; } }
            s->hd=(u8)(d*4);
            continue;
        }
        if(--s->think<=0&&!planned&&m==hhPlanNext%hhN){   // one plan per step (BFS is the costly part)
            hhDecide(s); planned=1; s->think=(short)(xo[XO_FREEWILL]==1?HH_THINK*2:HH_THINK)+(rnd8()&31);
        }
    }
    hhPlanNext++;
}

// ---- drawing: like the player, inside drawRoomRect's back-to-front walk. hhCalc (once a picture) works out where everyone is ----
static int hhX[HH_MAX], hhY[HH_MAX], hhB[HH_MAX], hhV[HH_MAX], hhH[HH_MAX];   // feet on screen, band (tile x+y), view, floor height
static void hhCalc(void){
    for(int m=0;m<hhN;m++){ const HhSim*s=&hhM[m]; s32 rx,ry; rotPos(s->fx,s->fy,&rx,&ry);
        hhX[m]=LOX+(int)((rx-ry)>>5); hhY[m]=LOY+(int)((rx+ry)>>6); hhB[m]=(int)((rx>>8)+(ry>>8)); hhV[m]=faceView[(s->hd+4*cview)&15]; hhH[m]=surfH(s->fx,s->fy); }
}
static void hhDrawBand(int s0,int s1){   // the members whose band is in s0..s1
    for(int m=0;m<hhN;m++){ if(hhB[m]<s0||hhB[m]>s1) continue;
        if(sShad) rect(hhX[m]-3,hhY[m]-hhH[m]-1,7,2,RGB(10,8,5));
        blit(hhSpr[m][hhV[m]],hhX[m]-16,hhY[m]-40-hhH[m]); }
}
typedef struct { short x0,y0,x1,y1; } HhR;   // (hud.h's Rc comes later in main.c)
static void hhRc(int m,HhR*r){ int sx=hhX[m]-16, sy=hhY[m]-40-hhH[m];
    r->x0=(short)(sx+spBx0); r->x1=(short)(sx+spBx1); r->y0=(short)(sy+spBy0); r->y1=(short)(sy+spBy1); if(r->y1<hhY[m]-hhH[m]+2) r->y1=(short)(hhY[m]-hhH[m]+2);   // the sprite's feet or the shadow, whichever is lower
    if(r->x0>hhX[m]-3) r->x0=(short)(hhX[m]-3); if(r->x1<hhX[m]+4) r->x1=(short)(hhX[m]+4); }
static unsigned hhSig(int m){ return (unsigned)(hhX[m]&0x3FF)|((unsigned)(hhY[m]&0x3FF)<<10)|((unsigned)hhV[m]<<20)|((unsigned)(hhH[m]&63)<<22); }
static HhR hhOld[HH_MAX]; static unsigned hhOldSig[HH_MAX];

static void hhLoad(void);
static void hhStart(void){   // entering the life game: load the household and stand everyone somewhere free
    hhLoad(); for(int m=0;m<hhN;m++){ hhPlace(&hhM[m],m); hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
}
// ---- switching who you control ----
static void hhSwap(HhSim*s);   // main.c: trades the player's position, needs, look and persona with s
static void hhSwitch(void){
    if(!hhN) return;
    HhSim t=hhM[0]; for(int m=0;m<hhN-1;m++) hhM[m]=hhM[m+1];   // the player goes to the back of the line, the first member steps in
    hhSwap(&t); hhM[hhN-1]=t;
    static u16 tmp[4][SPW*SPH] EWRAM_BSS; for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) tmp[v][i]=spr4[v][i];
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) spr4[v][i]=hhSpr[0][v][i];
    for(int m=0;m<hhN-1;m++) for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) hhSpr[m][v][i]=hhSpr[m+1][v][i];
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) hhSpr[hhN-1][v][i]=tmp[v][i];
}

// ---- saving (SRAM at HH_OFF): 'H' 'H' count, then per member its look, stage, persona, name and needs; checksum last ----
#define HH_REC (LK_N+3+TR_N+10+HN_N)
static void hhSave(void){
    volatile u8*m=(volatile u8*)0x0E000000+HH_OFF; int k=3; u8 sum=0x48;
    m[0]='H'; m[1]='H'; m[2]=(u8)hhN;
    for(int i=0;i<hhN;i++){ const HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) m[k++]=s->look[j]; m[k++]=s->stage; m[k++]=s->asp; m[k++]=s->ltw;
        for(int j=0;j<TR_N;j++) m[k++]=s->tr[j]; for(int j=0;j<10;j++) m[k++]=(u8)s->name[j]; for(int j=0;j<HN_N;j++) m[k++]=s->need[j]; }
    for(int i=2;i<k;i++) sum+=m[i];
    m[k]=sum;
}
static void hhLoad(void){
    volatile u8*m=(volatile u8*)0x0E000000+HH_OFF; u8 sum=0x48; hhN=0;
    if(m[0]!='H'||m[1]!='H'||m[2]>HH_MAX) return;
    int n=m[2], k=3+n*HH_REC; for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum) return;
    k=3;
    for(int i=0;i<n;i++){ HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) s->look[j]=m[k++]; s->stage=m[k++]; s->asp=m[k++]; s->ltw=m[k++];
        for(int j=0;j<TR_N;j++) s->tr[j]=m[k++]; for(int j=0;j<10;j++) s->name[j]=(char)m[k++]; s->name[9]=0; for(int j=0;j<HN_N;j++) s->need[j]=m[k++];
        if(s->stage>=AG_N||s->asp>=AS_PICK) return;
        s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0; }
    hhN=n;
}
_Static_assert(HH_OFF+3+HH_MAX*HH_REC+1<=20480,"the household must fit before the room slots");

// ---- the pause menu's HOUSEHOLD screen ----
static void hhMenu(void){
    static char lb[HH_NFAM+2][24]; const char* it[HH_NFAM+2]; int n=0;
    for(int f=0;f<HH_NFAM;f++){ char*e=lb[n]; const char*p="MOVE IN "; while(*p) *e++=*p++; p=hhFams[f].fam; while(*p) *e++=*p++; *e=0; it[n]=lb[n]; n++; }
    it[n++]="MOVE EVERYONE OUT";
    char t[24]; { char*e=t; const char*p="HOUSEHOLD  "; while(*p) *e++=*p++; e+=numStr(e,hhN+1); p=" OF 8"; while(*p) *e++=*p++; *e=0; }
    int c=menu(t,it,n); if(c<0) return;
    if(c==HH_NFAM){ hhN=0; hhSave(); toast("ONLY YOU LIVE HERE NOW"); return; }
    const HhFam*F=&hhFams[c]; int add=0;
    for(int i=0;i<F->n&&hhN<HH_MAX;i++){ hhNew(&hhM[hhN],&F->m[i]); hhN++; add++; }
    if(!add){ toast("THE HOUSE IS FULL"); return; }
    for(int m=0;m<hhN;m++){ hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave(); toast(add<F->n?"SOME DID NOT FIT":"WELCOME HOME");
}

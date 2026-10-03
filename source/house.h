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
enum { HA_IDLE, HA_WALK, HA_USE, HA_WANDER, HA_SEEK, HA_SOC };   // SEEK: walking to someone to talk to; SOC: in a conversation
enum { HN_FOOD, HN_WC, HN_REST, HN_CLEAN, HN_COMFY, HN_FUN, HN_SOC, HN_N };
static const char hnFurn[HN_N]={'F','T','S','H','C',0,0};   // what each need's furniture is (FUN: skate about; SOCIAL: find someone)
typedef struct {
    u8 look[LK_N], stage, asp, ltw, tr[TR_N];
    char name[10];
    s32 fx, fy;              // position in 1/256 tiles (like lfx, lfy)
    u8 hd, need[HN_N];       // heading (16 steps), needs 0..100 (WC here is 100 = empty bladder, like every other need: high is good)
    u8 act, use, pn, pi;     // action, need being refilled, path length, step along it
    u8 gok; s32 gx, gy;      // the tile centre this step walks to (fixed when the step starts)
    u8 uid, tgt, bub, bubT;  // who this is (relationships are kept by uid), who it is going to talk to (uid), balloon icon and time
    short think, t;          // steps to the next decision, steps left in the action
    u8 path[HH_PATH];        // directions: 0 +x, 1 +y, 2 -x, 3 -y
} HhSim;
static HhSim hhM[HH_MAX] EWRAM_BSS; static int hhN;
// ---- relationships (Sims 2 style): for every pair a DAILY and a LIFETIME score, -100..100, kept by uid and one-way (how a feels about b) ----
#define HU_N (HH_MAX+1)
static signed char relD[HU_N][HU_N], relL[HU_N][HU_N]; static u8 relF[HU_N][HU_N];
enum { RF_CRUSH=1, RF_LOVE=2, RF_STEADY=4, RF_KISSED=8, RF_FRIEND=16, RF_BFF=32, RF_ENEMY=64 };   // FRIEND/BFF/ENEMY: remembered so they fire once
static int hhPUid;                         // the uid of the Sim you control
static char hhPName[10]="YOU";             // the name of the Sim you control (premade Sims bring theirs)
static u8 hhBubT; static const char* hhBubTxt;   // the word over your head during a social (shown by hud.h's bubble)
// HARDWARE SPRITES: the members are GBA OBJ sprites (32x64, 16 colours each), so moving them costs no drawing: the CPU only draws their
// shadow and talk balloons into the room. Their 4 views are baked like the player's, cut down to 15 colours + clear (hhQuant), and kept
// as 4bpp tiles; each frame the view being shown goes into OBJ VRAM (1 KB a member) and OAM says where (hhObjUpdate, in vblank).
// A window keeps them inside the room view (never over the HUD); menus hide them (box() -> objHideAll). Sprites always sit on top of the
// picture, so a member standing behind a full-height wall is drawn see-through instead (the "x-ray" blend).
#define OBJ_VRAM ((volatile u16*)0x06014000)   // OBJ tiles 512.. in the bitmap modes
#define OBJ_PAL  ((volatile u16*)0x05000200)
#define OAM      ((volatile u16*)0x07000000)
static u8 hhObj[HH_MAX][4][1024] EWRAM_BSS;    // 4 views x 32x64 x 4bpp, tiles in 1D order
static u16 hhPal[HH_MAX][16];                  // a palette per member (index 0 = clear)
static u16 hhTmp[4][SPW*SPH] EWRAM_BSS;        // a 16-bit bake (one Sim) on its way to 4bpp, or back
static signed char hhObjV[HH_MAX];             // the view in OBJ VRAM for each member (-1 = must copy)
static u16 hhDist[MH*MW] EWRAM_BSS;
#define hhQ bfsQ   // (main.c's shared search queue)   // BFS scratch, shared (one member plans per step)
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

// ---- 16-bit sprite <-> 15 colours + clear, 4bpp tiles ----
static void hhQuant(u16 (*src)[SPW*SPH],u8 (*dst)[1024],u16*pal){
    static u16 col[256] EWRAM_BSS; static u32 cnt[256] EWRAM_BSS; int n=0;
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++){ u16 c=src[v][i]; if(c==SKY) continue; int k=0; while(k<n&&col[k]!=c) k++;
        if(k==n){ if(n==256) continue; col[n]=c; cnt[n]=0; n++; } cnt[k]++; }
    while(n>15){   // merge the two closest colours (weighted by how often they appear) until 15 are left
        int ba=0, bb=1, bd=1<<30;
        for(int a=0;a<n;a++)for(int b=a+1;b<n;b++){ int dr=(col[a]&31)-(col[b]&31), dg=((col[a]>>5)&31)-((col[b]>>5)&31), db=((col[a]>>10)&31)-((col[b]>>10)&31);
            int d=(dr*dr*3+dg*dg*4+db*db*2)*(int)(cnt[a]<cnt[b]?cnt[a]:cnt[b]); if(d<bd){ bd=d; ba=a; bb=b; } }
        u32 w=cnt[ba]+cnt[bb]; if(!w) w=1;
        int r=(int)(((col[ba]&31)*cnt[ba]+(col[bb]&31)*cnt[bb])/w), g=(int)((((col[ba]>>5)&31)*cnt[ba]+((col[bb]>>5)&31)*cnt[bb])/w), bl=(int)((((col[ba]>>10)&31)*cnt[ba]+((col[bb]>>10)&31)*cnt[bb])/w);
        if(cnt[bb]>cnt[ba]) col[ba]=col[bb]; else if(cnt[ba]==cnt[bb]) col[ba]=(u16)(r|(g<<5)|(bl<<10));   // keep the commoner one exact (faces stay crisp)
        cnt[ba]=w; col[bb]=col[n-1]; cnt[bb]=cnt[n-1]; n--; }
    pal[0]=0; for(int k=0;k<15;k++) pal[k+1]=k<n?col[k]:0;
    for(int v=0;v<4;v++){
        for(int i=0;i<1024;i++) dst[v][i]=0;
        for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ u16 c=src[v][y*SPW+x]; if(c==SKY) continue;
            int best=1, bd=1<<30; for(int k=0;k<n;k++){ int dr=(c&31)-(col[k]&31), dg=((c>>5)&31)-((col[k]>>5)&31), db=((c>>10)&31)-((col[k]>>10)&31), d=dr*dr*3+dg*dg*4+db*db*2; if(d<bd){ bd=d; best=k+1; if(!d) break; } }
            int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1); dst[v][o]|=(u8)(best<<((x&1)*4)); }
    }
}
static void hhUnquant(u8 (*src)[1024],const u16*pal,u16 (*dst)[SPW*SPH]){   // back to 16-bit (when a member becomes the one you control)
    for(int v=0;v<4;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1), k=(src[v][o]>>((x&1)*4))&15; dst[v][y*SPW+x]=k?pal[k]:SKY; }
}
static void spBounds(void){   // the box that holds every opaque pixel of the player's four views (blits and redraw rectangles stay inside it)
    spBx0=SPW; spBx1=0; spBy0=SPH; spBy1=0;
    for(int v=0;v<4;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) if(spr4[v][y*SPW+x]!=SKY){ if(x<spBx0) spBx0=x; if(x+1>spBx1) spBx1=x+1; if(y<spBy0) spBy0=y; if(y+1>spBy1) spBy1=y+1; }
    if(spBx0>=spBx1){ spBx0=0; spBx1=SPW; spBy0=0; spBy1=SPH; }
}
// ---- baking: render a member's look with the creator's own code, then put the player's creature back ----
static void hhBakeAll(void){
    static u8 sv[H][D][W] EWRAM_BSS; static u16 sd[H][D][W] EWRAM_BSS; u8 sl[LK_N]; u8 sst=stage; int sc=custom;
    for(int i=0;i<LK_N;i++) sl[i]=look[i];
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ sv[y][z][x]=vox[y][z][x]; sd[y][z][x]=dec[y][z][x]; }
    for(int m=0;m<hhN;m++){
        for(int i=0;i<LK_N;i++) look[i]=hhM[m].look[i]; stage=hhM[m].stage;
        buildLook(); setColors(); bakeInto(hhTmp); hhQuant(hhTmp,hhObj[m],hhPal[m]); hhObjV[m]=-1;
    }
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=sst;
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ vox[y][z][x]=sv[y][z][x]; dec[y][z][x]=sd[y][z][x]; }
    custom=sc; setColors();
    bakeInto(spr4);   // the player (still drawn by the CPU, so walls and furniture in front cover it and the action cam can zoom it)
    for(int m=0;m<hhN;m++) for(int i=0;i<16;i++) OBJ_PAL[m*16+i]=hhPal[m][i];
}
// ---- where members can stand ----
static int hhWalk(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 0; char c=lifeMap[y][x]; return c!='w'&&c!='W'&&tileH(x,y)<=3; }
static void hhPlace(HhSim*s,int k){   // somewhere free near the spawn point, spread out a little
    for(int r=1;r<12;r++)for(int t=0;t<40;t++){ int x=spx+((rnd8()%(2*r+1))-r), y=spy+((rnd8()%(2*r+1))-r);
        if(hhWalk(x,y)&&(x!=spx||y!=spy)){ s->fx=x*256+128; s->fy=y*256+128; return; } }
    s->fx=spx*256+128; s->fy=spy*256+128; (void)k;
}
static int hhFreeUid(void);
static void hhNew(HhSim*s,const HhPre*p){
    s->uid=(u8)hhFreeUid(); s->bubT=0;
    for(int i=0;i<LK_N;i++) s->look[i]=i<LK_TAIL?p->look[i]:0;
    s->stage=p->stage; s->asp=p->asp; s->ltw=0; for(int i=0;i<TR_N;i++) s->tr[i]=signTr[p->sign][i];
    int i=0; for(;p->name[i]&&i<9;i++) s->name[i]=p->name[i]; s->name[i]=0;
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15)); s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
    hhPlace(s,0);
}

// ---- path finding: BFS from the member's tile; the goal is any free tile next to furniture c (or a random free tile for c=0) ----
static int hhGX, hhGY;   // hhPlan(s,1): walk next to this tile (a person)
static int hhNextTo(int x,int y,char c){
    if(c==1){ int dx=x-hhGX, dy=y-hhGY; return (dx==0&&(dy==1||dy==-1))||(dy==0&&(dx==1||dx==-1)); } for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d]; if(nx>=0&&ny>=0&&nx<MW&&ny<MH&&lifeMap[ny][nx]==c) return 1; } return 0; }
static int hhPlan(HhSim*s,char c){   // fills s->path; returns its length+1 (1 = already there), 0 = no way
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if(!hhWalk(sx,sy)) return 0;
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    if(c==1&&((sx-hhGX)*(sx-hhGX)+(sy-hhGY)*(sy-hhGY))<=2) return 1;   // already next to them
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
    switch(n){ case HN_CLEAN: return 70+s->tr[TR_NEAT]*6; case HN_COMFY: return 130-s->tr[TR_ACT]*6; case HN_FUN: return 70+s->tr[TR_PLAY]*6; case HN_SOC: return 60+s->tr[TR_OUT]*8; default: return 100; }
}
static void hhSeek(HhSim*s);   // social: pick someone and walk over (below)
static void hhDecide(HhSim*s){
    int best[2]={-1,-1}, bs[2]={0,0}, low=xo[XO_FREEWILL]==1?35:55;   // LOW free will waits until needs are lower
    for(int n=0;n<HN_N;n++){
        if(hnFurn[n]&&!(simHave&(n==HN_FOOD?SR_FRIDGE:n==HN_WC?SR_TOILET:n==HN_REST?SR_BED:n==HN_CLEAN?SR_SHOWER:SR_SOFA))) continue;   // no such furniture
        if(n==HN_SOC&&hhN<1) continue;
        int v=s->need[n]; if(v>=low+30) continue;
        int u=(100-v)*(100-v)/100*hhTilt(s,n)/100;
        if(u>bs[0]){ bs[1]=bs[0]; best[1]=best[0]; bs[0]=u; best[0]=n; } else if(u>bs[1]){ bs[1]=u; best[1]=n; }
    }
    int n=best[0]; if(best[1]>=0&&bs[1]*4>=bs[0]*3&&(rnd8()&1)) n=best[1];   // close call: either of the two
    if(n==HN_SOC){ hhSeek(s); return; }
    if(n<0&&hhN>0&&(rnd8()*100>>8)<25+s->tr[TR_OUT]*5){ hhSeek(s); return; }   // nothing pressing: go and see someone (outgoing Sims more often)
    if(n<0){ if(hhPlan(s,0)>1){ s->act=HA_WANDER; s->use=HN_FUN; } else s->act=HA_IDLE; return; }
    int r=hhPlan(s,hnFurn[n]?hnFurn[n]:0);
    if(r==1&&hnFurn[n]){ s->act=HA_USE; s->use=(u8)n; s->t=HH_USE; }
    else if(r>1){ s->act=hnFurn[n]?HA_WALK:HA_WANDER; s->use=(u8)n; }
    else s->act=HA_IDLE;
}
static void hhArrive(int m);   // social: reached the person (below)
static void relTick(void);
static int hhStill;   // steps you have been standing still (hud.h only shows the thought bubble when you stop)
static void hhTick(void){   // once per logic step in the life game
    if(hhBubT) hhBubT--;
    if(lvx||lvy||lsp||lairF||lgrind) hhStill=0; else if(hhStill<1000) hhStill++;
    if(!hhN) return;
    relTick();
    int fe=oFoodEvery(), we=oWcEvery(), planned=0;
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m];
        // needs drain (gently: the Sims you do not watch should not be in constant crisis)
        if(fe&&lfr%(fe*2)==0&&s->need[HN_FOOD]>0) s->need[HN_FOOD]--;
        if(we&&lfr%(we*2)==0&&s->need[HN_WC]>0) s->need[HN_WC]--;
        if(lfr%300==m*7){ for(int n=HN_REST;n<HN_N;n++) if(s->need[n]>0) s->need[n]--; }
        if(s->bubT) s->bubT--;
        if(lfr%(150-s->tr[TR_OUT]*8)==0&&s->need[HN_SOC]>0) s->need[HN_SOC]--;   // lonely sooner when outgoing
        if(s->act==HA_SOC){ if(--s->t<=0){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); } continue; }   // standing in a conversation
        if(!xo[XO_FREEWILL]){ s->act=HA_IDLE; continue; }
        if(s->act==HA_USE){   // using furniture: refill, then free again
            if(s->need[s->use]<100&&(lfr&1)) s->need[s->use]++;
            if(--s->t<=0||s->need[s->use]>=100){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); }
            continue;
        }
        if(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK){   // follow the path, tile centre to tile centre
            if(s->pi>=s->pn&&s->act==HA_SEEK){ hhArrive(m); continue; }
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

// ---- SOCIAL: interactions, acceptance, relationships ----
// The player presses R next to a household Sim (a menu like the Sims' pie menu); free will makes Sims start them too, with each other and
// with you. Whether it is ACCEPTED depends on how the target feels about the one asking (daily score), its traits, its mood and age.
// Accepted: both feel better about each other and their SOCIAL (and sometimes FUN) fills. Rejected: the asker is embarrassed, and likes the
// other a bit less. Mean ones (ARGUE, INSULT, SLAP) always land: the target likes the asker less.
// Statuses follow the scores: FRIEND (daily 50+), BEST FRIEND (daily and lifetime 70+), ENEMY (daily -50 or less), and the romance
// steps CRUSH (a flirt was accepted), IN LOVE (kissed, and lifetime 60+ both ways), STEADY (asked and said yes). Daily drifts back to
// lifetime over the hours, so friendships need keeping up.
enum { SA_ROM=1, SA_MEAN=2, SA_CRUSH=4, SA_LOVE=8, SA_KID=16 };
typedef struct { const char* name; signed char dA,lA,dR,lR; u8 soc,fun; signed char minD,maxD; u8 base,tr,fl,icA,icR; const char*say,*yes,*no; } SocAct;
enum { SC_TALK, SC_JOKE, SC_COMPL, SC_HIGH5, SC_HUG, SC_TRICK, SC_FLIRT, SC_KISS, SC_STEADY, SC_SORRY, SC_ARGUE, SC_INSULT, SC_SLAP, SC_N };
static const SocAct socT[SC_N]={
  //  name           dA  lA  dR  lR soc fun minD maxD base trait   flags                icon yes  icon no     you say  they did        they did not
    {"TALK",          3,  1, -2,  0, 22,  0,-100, 100, 85,TR_OUT, SA_KID,              IC_TALK, IC_BAIL, "BLAH BLAH","CHATTED",     "IGNORED YOU"},
    {"JOKE",          6,  2, -4, -1, 14, 10, -20, 100, 55,TR_PLAY,SA_KID,              IC_STAR, IC_BAIL, "HA HA",   "LAUGHED",       "DID NOT LAUGH"},
    {"COMPLIMENT",    5,  2, -3, -1, 14,  0,   0, 100, 65,TR_NICE,SA_KID,              IC_STAR, IC_BAIL, "NICE",    "BLUSHED",       "SHRUGGED"},
    {"HIGH FIVE",     5,  1, -3,  0, 12,  6,  15, 100, 70,TR_ACT, SA_KID,              IC_HAND, IC_BAIL, "UP TOP",  "HIGH FIVED",    "LEFT YOU HANGING"},
    {"HUG",           9,  3, -8, -2, 20,  0,  35, 100, 55,TR_NICE,SA_KID,              IC_HEART,IC_BAIL, "HUG",     "HUGGED YOU",    "PUSHED AWAY"},
    {"SHOW A TRICK",  4,  1, -2,  0, 12, 12, -10, 100, 65,TR_PLAY,SA_KID,              IC_STAR, IC_GLASS,"WATCH",   "WAS IMPRESSED", "WAS BORED"},
    {"FLIRT",         7,  2, -7, -2, 16,  6,  20, 100, 45,TR_OUT, SA_ROM,              IC_HEART,IC_BAIL, "HEY YOU", "FLIRTED BACK",  "REJECTED YOU"},
    {"KISS",         12,  5,-12, -4, 22,  8,  55, 100, 55,TR_OUT, SA_ROM|SA_CRUSH,     IC_HEART,IC_BAIL, "MWAH",    "KISSED YOU",    "TURNED AWAY"},
    {"GO STEADY",    15, 10,-15, -6, 22,  0,  70, 100, 65,TR_NICE,SA_ROM|SA_LOVE,      IC_HEART,IC_BAIL, "BE MINE",  "SAID YES",      "SAID NO"},
    {"APOLOGIZE",    12,  4, -3,  0, 10,  0,-100,  -5, 55,TR_NICE,SA_KID,              IC_TALK, IC_ANGRY,"SORRY",   "FORGAVE YOU",   "IS STILL MAD"},
    {"ARGUE",        -8, -3,  0,  0,  6,  0,-100, 100,100,TR_NICE,SA_MEAN|SA_KID,      IC_ANGRY,IC_ANGRY,"GRR",     "ARGUED BACK",   ""},
    {"INSULT",      -10, -4,  0,  0,  4,  0,-100,  30,100,TR_NICE,SA_MEAN|SA_KID,      IC_SAD,  IC_SAD,  "LOSER",   "LOOKS HURT",    ""},
    {"SLAP",        -16, -6,  0,  0,  4,  0,-100, -20,100,TR_NICE,SA_MEAN,             IC_HURT, IC_HURT, "SMACK",   "GOT SLAPPED",   ""},
};
static int hhFreeUid(void){ for(int u=0;u<HU_N;u++){ if(u==hhPUid) continue; int k=0; for(int m=0;m<hhN;m++) if(hhM[m].uid==u) k=1; if(!k) return u; } return 0; }
static int hhOthers(void){ return hhN>0; }
static int hhMemOf(int uid){ for(int m=0;m<hhN;m++) if(hhM[m].uid==uid) return m; return -1; }   // -1: the player (or nobody)
static int uStage(int u){ int m=hhMemOf(u); return m<0?stage:hhM[m].stage; }
static int uTr(int u,int t){ int m=hhMemOf(u); return m<0?pTr[t]:hhM[m].tr[t]; }
static int uMood(int u){ int m=hhMemOf(u); if(m<0) return moodHapPct(); const HhSim*s=&hhM[m]; int v=0; for(int n=0;n<HN_N;n++) v+=s->need[n]; return v/HN_N; }
static const char* uName(int u){ int m=hhMemOf(u); return m<0?hhPName:hhM[m].name; }
static int ageBand(int st){ return st<AG_TEEN?0:st==AG_TEEN?1:2; }   // romance only within a band: teens with teens, adults with adults and elders
static int romOk(int a,int b){ int sa=uStage(a), sb=uStage(b); return sa>=AG_TEEN&&sb>=AG_TEEN&&ageBand(sa)==ageBand(sb); }
static int hhRomanceOk(void){ for(int m=0;m<hhN;m++) if(romOk(hhPUid,hhM[m].uid)) return 1; return 0; }
static int clampR(int v){ return v<-100?-100:v>100?100:v; }
static const char* relWord(int a,int b){   // how a sees b
    u8 f=relF[a][b]; int d=relD[a][b], l=relL[a][b];
    if(f&RF_STEADY) return "STEADY"; if(f&RF_LOVE) return "IN LOVE"; if(f&RF_CRUSH) return "CRUSH";
    if(d>=70&&l>=70) return "BEST FRIEND"; if(d>=50) return "FRIEND"; if(d<=-50) return "ENEMY"; if(d<=-20) return "DISLIKE";
    if(d==0&&l==0) return "STRANGER"; return "ACQUAINTANCE";
}
static int socAllowed(int a,int b,int i){   // may a do interaction i to b now?
    const SocAct*S=&socT[i]; int d=relD[a][b];
    if(d<S->minD||d>S->maxD) return 0;
    if(!(S->fl&SA_KID)&&(uStage(a)<AG_TEEN||uStage(b)<AG_TEEN)) return 0;
    if((S->fl&SA_ROM)&&!romOk(a,b)) return 0;
    if((S->fl&SA_CRUSH)&&!(relF[a][b]&RF_CRUSH)) return 0;
    if((S->fl&SA_LOVE)&&(!(relF[a][b]&RF_LOVE)||(relF[a][b]&RF_STEADY))) return 0;
    if(i==SC_TRICK&&uStage(a)<AG_CHILD) return 0;
    return 1;
}
static void needAdd(int u,int n,int v){   // a need of anyone (n: HN_SOC or HN_FUN)
    int m=hhMemOf(u);
    if(m<0){ if(n==HN_SOC){ sSoc+=v; if(sSoc>100) sSoc=100; if(sSoc<0) sSoc=0; } else moodFun=moodClamp(moodFun+v*MOOD_ONE); return; }
    int x=hhM[m].need[n]+v; hhM[m].need[n]=(u8)(x<0?0:x>100?100:x);
}
static void relMilestones(int a,int b){   // statuses that just started: notes and (for you) wants and fears
    u8*f=&relF[a][b]; int d=relD[a][b], l=relL[a][b], you=(a==hhPUid);
    if(d>=50&&!(*f&RF_FRIEND)){ *f|=RF_FRIEND; if(you){ simEvent(SE_FRIEND); simCat(simCat(simMsg2,"NEW FRIEND: "),uName(b)); simQueue(simMsg2); } }
    if(d<40) *f&=~RF_FRIEND;
    if(d>=70&&l>=70&&!(*f&RF_BFF)){ *f|=RF_BFF; if(you){ simEvent(SE_BFF); simCat(simCat(simMsg2,"BEST FRIENDS: "),uName(b)); simQueue(simMsg2); } }
    if(l<60) *f&=~RF_BFF;
    if(d<=-50&&!(*f&RF_ENEMY)){ *f|=RF_ENEMY; if(you){ simEvent(SE_ENEMY); simCat(simCat(simMsg2,"NEW ENEMY: "),uName(b)); simQueue(simMsg2); } }
    if(d>-40) *f&=~RF_ENEMY;
    if((*f&RF_KISSED)&&!(*f&RF_LOVE)&&l>=60&&relL[b][a]>=60){ *f|=RF_LOVE; relF[b][a]|=RF_LOVE; if(you||b==hhPUid){ simEvent(SE_LOVE); simQueue("IN LOVE"); } }
}
static void hhSay(int u,int icon,const char* word){   // a balloon over someone's head: an icon for household Sims, a word for you
    int m=hhMemOf(u); if(m<0){ hhBubTxt=word; hhBubT=90; } else { hhM[m].bub=(u8)icon; hhM[m].bubT=90; }
}
static void hhFreeze(int u,int steps){ int m=hhMemOf(u); if(m<0){ if(lstun<steps) lstun=steps; lsp=0; lgrind=0; } else { hhM[m].act=HA_SOC; hhM[m].t=(short)steps; } }
static void socNote(int a,int b,int i,int ok){   // what you read when you are part of it: "REX LAUGHED +6"
    if(a!=hhPUid&&b!=hhPUid) return;
    const SocAct*S=&socT[i]; char*e=simMsg2;
    if(a==hhPUid){ e=simCat(e,uName(b)); *e++=' '; e=simCat(e,ok?S->yes:S->no); }
    else { e=simCat(e,uName(a)); *e++=' '; const char*w=S->name; char lw[16]; int k=0; for(;w[k]&&k<15;k++) lw[k]=w[k]; lw[k]=0;
        e=simCat(e,(S->fl&SA_MEAN)?(i==SC_SLAP?"SLAPPED YOU":i==SC_ARGUE?"PICKED A FIGHT":"INSULTED YOU"):i==SC_TALK?"CAME TO CHAT":i==SC_FLIRT?"FLIRTS WITH YOU":i==SC_KISS?"KISSED YOU":i==SC_HUG?"HUGS YOU":i==SC_STEADY?"ASKS YOU OUT":lw); }
    lnote=simMsg2; lnoteT=110;
}
// a does interaction i to b. Returns 1 if it was accepted (mean ones: 1 = it landed)
static int socDo(int a,int b,int i){
    const SocAct*S=&socT[i]; int ok;
    if(S->fl&SA_MEAN) ok=1;
    else {
        int c=S->base+relD[b][a]/2+(uTr(b,S->tr)-5)*4+(uMood(b)-50)/5;
        if(S->fl&SA_ROM){ if(relF[b][a]&RF_CRUSH) c+=20; for(int u=0;u<HU_N;u++) if(u!=a&&(relF[b][u]&RF_STEADY)) c-=40; }   // taken: jealousy
        if(uTr(b,TR_OUT)<=2&&relD[b][a]<20) c-=10;   // shy with people it hardly knows
        c=c<5?5:c>95?95:c; ok=(rnd8()*100>>8)<c;
    }
    hhFreeze(a,80); hhFreeze(b,80);
    hhSay(a,S->icA,S->say);
    if(S->fl&SA_MEAN){
        relD[b][a]=(signed char)clampR(relD[b][a]+S->dA); relL[b][a]=(signed char)clampR(relL[b][a]+S->lA);
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dA/2);
        needAdd(b,HN_SOC,-S->soc); needAdd(a,HN_SOC,S->soc); if(uTr(a,TR_NICE)<=3) needAdd(a,HN_FUN,8);   // grouchy Sims enjoy it a little
        hhSay(b,S->icR,i==SC_SLAP?"OW":i==SC_ARGUE?"GRR":"HEY");
        if(b==hhPUid){ simEvent(i==SC_SLAP?SE_SLAPPED:SE_FIGHT); moodEventN(M_FEAR,i==SC_SLAP?2:1); }
        if(a==hhPUid&&i==SC_ARGUE) simEvent(SE_FIGHT);
    } else if(ok){
        relD[b][a]=(signed char)clampR(relD[b][a]+S->dA); relL[b][a]=(signed char)clampR(relL[b][a]+S->lA);
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dA*2/3); relL[a][b]=(signed char)clampR(relL[a][b]+S->lA*2/3);
        needAdd(a,HN_SOC,S->soc); needAdd(b,HN_SOC,S->soc); if(S->fun){ needAdd(a,HN_FUN,S->fun); needAdd(b,HN_FUN,S->fun); }
        if(i==SC_FLIRT){ relF[a][b]|=RF_CRUSH; relF[b][a]|=RF_CRUSH; }
        if(i==SC_KISS){ int first=!(relF[a][b]&RF_KISSED); relF[a][b]|=RF_KISSED; relF[b][a]|=RF_KISSED; if(first&&(a==hhPUid||b==hhPUid)) simEvent(SE_KISS); }
        if(i==SC_STEADY){ relF[a][b]|=RF_STEADY; relF[b][a]|=RF_STEADY; if(a==hhPUid||b==hhPUid){ simEvent(SE_STEADY); simQueue("GOING STEADY"); } }
        if(a==hhPUid||b==hhPUid){ simEvent(SE_TALK); if(i==SC_JOKE) simEvent(SE_LAUGH); if(i==SC_HUG) simEvent(SE_HUGGED); moodEvent(M_WANT); }
        hhSay(b,S->icA,i==SC_JOKE?"HA HA":i==SC_HUG||i==SC_KISS?"AWW":i==SC_STEADY?"YES":"YEAH");
    } else {
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dR); relL[a][b]=(signed char)clampR(relL[a][b]+S->lR); relD[b][a]=(signed char)clampR(relD[b][a]+S->dR/2);
        needAdd(a,HN_SOC,-6);
        if(S->fl&SA_ROM){ relF[a][b]&=~RF_CRUSH; }
        if(a==hhPUid){ if(S->fl&SA_ROM) simEvent(SE_REJECT); moodEvent(M_BUMP); }
        hhSay(b,S->icR,"NO");
    }
    relMilestones(a,b); relMilestones(b,a);
    socNote(a,b,i,ok);
    return ok;
}
static void relTick(void){   // every game hour (900 steps): daily scores drift one step back towards lifetime
    static int t; if(++t<900) return; t=0;
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ int d=relD[a][b], l=relL[a][b]; if(d>l) relD[a][b]--; else if(d<l) relD[a][b]++; }
}
// ---- free will socials ----
static int uTile(int u,int*x,int*y){ int m=hhMemOf(u); if(m<0){ *x=(int)(lfx>>8); *y=(int)(lfy>>8); return 1; } *x=(int)(hhM[m].fx>>8); *y=(int)(hhM[m].fy>>8); return hhM[m].act!=HA_USE; }
static void hhSeek(HhSim*s){   // pick someone to go and see: friends most, enemies when grouchy, anyone when lonely enough
    int best=-1, bs=-999, me=s->uid;
    for(int u=0;u<HU_N;u++){ if(u==me) continue; if(u!=hhPUid&&hhMemOf(u)<0) continue;
        int x,y; if(!uTile(u,&x,&y)) continue;
        int d=relD[me][u], sc=d+(rnd8()&31); if(s->tr[TR_NICE]<=3&&d<-20) sc=-d+(rnd8()&31);   // grouchy Sims go looking for trouble
        if(relF[me][u]&(RF_CRUSH|RF_LOVE|RF_STEADY)) sc+=30;
        if(relD[me][u]==0&&relL[me][u]==0) sc+=20+s->tr[TR_OUT]*4;   // someone new: go and say hello (outgoing Sims more)
        if(u==hhPUid) sc+=15;                                          // and they like to come and see you
        if(sc>bs){ bs=sc; best=u; } }
    if(best<0){ s->act=HA_IDLE; return; }
    uTile(best,&hhGX,&hhGY); s->tgt=(u8)best;
    int r=hhPlan(s,1); if(r==1){ s->act=HA_SEEK; s->pn=s->pi=0; } else if(r>1) s->act=HA_SEEK; else s->act=HA_IDLE;
}
static int socPick(int a,int b){   // what a free-will Sim says to b
    int d=relD[a][b], nice=uTr(a,TR_NICE), r=rnd8();
    if(d<-30||(nice<=2&&r<40)){ if(socAllowed(a,b,SC_SLAP)&&r<70) return SC_SLAP; return (r&1)?SC_ARGUE:SC_INSULT; }
    if(d<-5&&nice>=6&&socAllowed(a,b,SC_SORRY)) return SC_SORRY;
    if(socAllowed(a,b,SC_STEADY)&&r<90) return SC_STEADY;
    if(socAllowed(a,b,SC_KISS)&&r<120) return SC_KISS;
    if(socAllowed(a,b,SC_FLIRT)&&uTr(a,TR_OUT)>=5&&r<70) return SC_FLIRT;
    int pool[8], n=0;
    pool[n++]=SC_TALK; if(socAllowed(a,b,SC_JOKE)&&uTr(a,TR_PLAY)>=4) pool[n++]=SC_JOKE; if(socAllowed(a,b,SC_COMPL)&&nice>=5) pool[n++]=SC_COMPL;
    if(socAllowed(a,b,SC_HIGH5)) pool[n++]=SC_HIGH5; if(socAllowed(a,b,SC_HUG)&&nice>=4) pool[n++]=SC_HUG; if(socAllowed(a,b,SC_TRICK)&&uTr(a,TR_ACT)>=5) pool[n++]=SC_TRICK;
    return pool[(r*n)>>8];
}
static void hhArrive(int m){   // a free-will Sim reached the one it wanted to see
    HhSim*s=&hhM[m]; int b=s->tgt, x, y; s->act=HA_IDLE; s->think=(short)HH_THINK;
    if(!uTile(b,&x,&y)) return;
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if((sx-x)*(sx-x)+(sy-y)*(sy-y)>4) return;   // they walked off
    int bm=hhMemOf(b); if(bm>=0&&(hhM[bm].act==HA_SOC||hhM[bm].act==HA_USE)) return;
    if(bm<0&&(lstun>0||simAct||lz>(s32)(surfH(lfx,lfy)<<8))) return;   // you are busy, or in the air
    s->hd=(u8)(x>sx?0:x<sx?8:y>sy?4:12); if(bm>=0) hhM[bm].hd=(u8)((s->hd+8)&15);
    socDo(s->uid,b,socPick(s->uid,b));
}
// ---- you: R next to a household Sim opens the social menu (furniture you stand at is offered first) ----
static int hhNearest(void){ int best=-1, bd=1<<30; for(int m=0;m<hhN;m++){ s32 dx=hhM[m].fx-lfx, dy=hhM[m].fy-lfy; int d=(int)((dx*dx+dy*dy)>>8); if(d<bd){ bd=d; best=m; } } return bd<=(380*380>>8)?best:-1; }   // within 1.5 tiles
static void liveInvalidate(void);
static int hhSocR(int useLabel){   // 1 = handled (a social, or the menu was closed), 0 = go on and use the furniture
    int m=hhNearest(); if(m<0) return 0;
    HhSim*s=&hhM[m]; int b=s->uid, a=hhPUid;
    if(s->act==HA_USE){ lnote="THEY ARE BUSY"; lnoteT=50; return 0; }
    static const char* it[SC_N+1]; static char tl[28]; int id[SC_N+1], n=0;
    static const char* const useNm[6]={0,"USE THE FRIDGE","USE THE TOILET","SLEEP IN BED","TAKE A SHOWER","SIT ON SOFA"};
    if(useLabel>0&&useLabel<6){ it[n]=useNm[useLabel]; id[n++]=-1; }
    for(int i=0;i<SC_N;i++) if(socAllowed(a,b,i)){ it[n]=socT[i].name; id[n++]=i; }
    { char*e=simCat(tl,s->name); *e++=' '; *e++=' '; simCat(e,relWord(a,b)); }
    int c=menu(tl,it,n); liveInvalidate();
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]<0) return 0;
    int px=(int)(lfx>>8), py=(int)(lfy>>8), sx=(int)(s->fx>>8), sy=(int)(s->fy>>8);
    s->hd=(u8)(px>sx?0:px<sx?8:py>sy?4:12); lhd=(s->hd+8)&15;
    socDo(a,b,id[c]);
    return 1;
}
// ---- the RELATIONSHIPS screen (pause menu > HOUSEHOLD): how you feel about everyone, and how they feel about you ----
static void relBar(int x,int y,int v){   // -100..100 around a centre line, green above 0, red below
    rect(x,y,61,4,RGB(3,4,8)); rect(x+30,y-1,1,6,RGB(14,16,20));
    int w=v*30/100; if(w>0) rect(x+31,y,w,4,RGB(8,26,8)); else if(w<0) rect(x+30+w,y,-w,4,RGB(28,8,6));
}
static void relScreen(void){
    u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) return;
        box(3,1,234,157); char t[28]; simCat(simCat(t,"RELATIONSHIPS OF "),hhPName); text(10,6,t,GOLD,1);
        text(84,16,"YOU TO THEM",DIMC,1); text(162,16,"THEM TO YOU",DIMC,1);
        if(!hhN) text(10,40,"NO ONE ELSE LIVES HERE",DIMC,1);
        for(int m=0;m<hhN;m++){ int y=26+m*18, b=hhM[m].uid, a=hhPUid;
            text(10,y,hhM[m].name,WHITE,1); text(10,y+8,relWord(a,b),(relF[a][b]&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):relD[a][b]<=-20?RGB(30,10,8):RGB(16,26,16),1);
            relBar(84,y+1,relD[a][b]); relBar(84,y+8,relL[a][b]); relBar(162,y+1,relD[b][a]); relBar(162,y+8,relL[b][a]);
            if(relF[a][b]&RF_STEADY) simIcon(226,y+2,IC_HEART,RGB(31,14,20)); }
        text(10,150,"TOP BAR DAILY  LOWER BAR LIFETIME",RGB(12,14,16),1);
        present();
    }
}
// ---- drawing: like the player, inside drawRoomRect's back-to-front walk. hhCalc (once a picture) works out where everyone is ----
static int hhX[HH_MAX], hhY[HH_MAX], hhB[HH_MAX], hhV[HH_MAX], hhH[HH_MAX];   // feet on screen, band (tile x+y), view, floor height
static void hhCalc(void){
    for(int m=0;m<hhN;m++){ const HhSim*s=&hhM[m]; s32 rx,ry; rotPos(s->fx,s->fy,&rx,&ry);
        hhX[m]=LOX+(int)((rx-ry)>>5); hhY[m]=LOY+(int)((rx+ry)>>6); hhB[m]=(int)((rx>>8)+(ry>>8)); hhV[m]=faceView[(s->hd+4*cview)&15]; hhH[m]=surfH(s->fx,s->fy); }
}
static void hhDrawBand(int s0,int s1){   // the members whose band is in s0..s1
    for(int m=0;m<hhN;m++){ if(hhB[m]<s0||hhB[m]>s1) continue;
        if(sShad) rect(hhX[m]-3,hhY[m]-hhH[m]-1,7,2,RGB(10,8,5));   // (the Sim itself is a hardware sprite: hhObjUpdate)
        if(hhM[m].bubT){ int bx=hhX[m]-5, by=hhY[m]-hhH[m]-56; rect(bx,by,11,10,RGB(14,16,22)); rect(bx+1,by+1,9,8,WHITE);   // a balloon with an icon (Sims style)
            simIcon(bx+2,by+1,hhM[m].bub,hhM[m].bub==IC_HEART?RGB(28,6,12):hhM[m].bub==IC_ANGRY||hhM[m].bub==IC_HURT?RGB(26,4,4):RGB(4,4,10)); px(hhX[m],by+10,RGB(14,16,22)); } }
}
typedef struct { short x0,y0,x1,y1; } HhR;   // (hud.h's Rc comes later in main.c)
static void hhRc(int m,HhR*r){   // what a member puts INTO the picture: only its shadow (and a balloon); the body is a hardware sprite
    r->x0=(short)(hhX[m]-3); r->x1=(short)(hhX[m]+4); r->y0=(short)(hhY[m]-hhH[m]-1); r->y1=(short)(hhY[m]-hhH[m]+1);
    if(hhM[m].bubT){ int by=hhY[m]-hhH[m]-56; if(r->y0>by) r->y0=(short)by; if(r->x0>hhX[m]-5) r->x0=(short)(hhX[m]-5); if(r->x1<hhX[m]+6) r->x1=(short)(hhX[m]+6); } }
static unsigned hhSig(int m){ return (unsigned)(hhX[m]&0x3FF)|((unsigned)(hhY[m]&0x3FF)<<10)|((unsigned)(hhH[m]&15)<<22)|((unsigned)(hhM[m].bubT?1+(hhM[m].bub&31):0)<<26); }
static int hhBehindWall(int m){   // is a full-height wall in front of this member (towards the camera)? then it is drawn see-through
    s32 rx,ry; rotPos(hhM[m].fx,hhM[m].fy,&rx,&ry); int x=(int)(rx>>8), y=(int)(ry>>8);
    static const signed char d[5][2]={{1,0},{0,1},{1,1},{2,1},{1,2}};
    for(int k=0;k<5;k++){ int wx=x+d[k][0], wy=y+d[k][1]; if(!wallAtR(wx,wy)||cellAt(wx,wy)!='W'||sWall==2) continue;
        if(sWall==1&&(wInAt(wx,wy-1)||wInAt(wx-1,wy))) continue;   // that wall is cut away
        return 1; }
    return 0;
}
static void hhObjUpdate(void){   // in vblank: the members' sprites (OAM 0..6), their current view in OBJ VRAM, the window that clips them
    *(volatile u16*)0x04000040=240; *(volatile u16*)0x04000044=(u16)((vpY0<<8)|vpY1);   // WIN0: the room view
    *(volatile u16*)0x04000048=0x34; *(volatile u16*)0x0400004A=0x04;                   // inside: BG2 + sprites + blend; outside: BG2 only
    *(volatile u16*)0x04000050=0x0400; *(volatile u16*)0x04000052=(6<<8)|10;            // see-through sprites blend 10/16 over the picture
    for(int m=0;m<HH_MAX;m++){
        volatile u16*o=OAM+m*4;
        if(m>=hhN||lcamF>0){ o[0]=0x200; continue; }
        int x=hhX[m]-16, y=hhY[m]-40-hhH[m];
        if(x+32<=0||x>=SW||y+SPH<=vpY0||y>=vpY1){ o[0]=0x200; continue; }
        if(hhObjV[m]!=hhV[m]){ hhObjV[m]=(signed char)hhV[m]; const u16*s=(const u16*)hhObj[m][hhV[m]]; volatile u16*d=OBJ_VRAM+m*512; for(int i=0;i<512;i++) d[i]=s[i]; }
        o[0]=(u16)((y&255)|(hhBehindWall(m)?0x400:0)|0x8000); o[1]=(u16)((x&511)|0xC000); o[2]=(u16)((512+m*32)|(m<<12));
    }
}
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
    static u8 ob[4][1024] EWRAM_BSS; u16 pl[16];
    hhUnquant(hhObj[0],hhPal[0],hhTmp);   // the member you take over: back to a full 16-bit sprite
    hhQuant(spr4,ob,pl);                   // the one you leave: down to a hardware sprite
    for(int m=0;m<hhN-1;m++){ for(int v=0;v<4;v++)for(int i=0;i<1024;i++) hhObj[m][v][i]=hhObj[m+1][v][i]; for(int i=0;i<16;i++) hhPal[m][i]=hhPal[m+1][i]; }
    for(int v=0;v<4;v++)for(int i=0;i<1024;i++) hhObj[hhN-1][v][i]=ob[v][i]; for(int i=0;i<16;i++) hhPal[hhN-1][i]=pl[i];
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) spr4[v][i]=hhTmp[v][i];
    spBounds();
    for(int m=0;m<hhN;m++){ hhObjV[m]=-1; for(int i=0;i<16;i++) OBJ_PAL[m*16+i]=hhPal[m][i]; }
}

// ---- saving (SRAM at HH_OFF): 'H' '2' count, your uid, then per member its look, stage, persona, name, needs and uid, then the
// relationships (daily, lifetime, flags for every pair of uids); checksum last ----
#define HH_REC (LK_N+3+TR_N+10+HN_N+1)
#define HH_RELB (3*HU_N*HU_N)
static void hhSave(void){
    volatile u8*m=(volatile u8*)0x0E000000+HH_OFF; int k=3; u8 sum=0x48;
    m[0]='H'; m[1]='2'; m[2]=(u8)hhN; m[k++]=(u8)hhPUid;
    for(int i=0;i<hhN;i++){ const HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) m[k++]=s->look[j]; m[k++]=s->stage; m[k++]=s->asp; m[k++]=s->ltw;
        for(int j=0;j<TR_N;j++) m[k++]=s->tr[j]; for(int j=0;j<10;j++) m[k++]=(u8)s->name[j]; for(int j=0;j<HN_N;j++) m[k++]=s->need[j]; m[k++]=s->uid; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ m[k++]=(u8)relD[a][b]; m[k++]=(u8)relL[a][b]; m[k++]=relF[a][b]; }
    for(int i=2;i<k;i++) sum+=m[i];
    m[k]=sum;
}
static void hhLoad(void){
    volatile u8*m=(volatile u8*)0x0E000000+HH_OFF; u8 sum=0x48; hhN=0;
    if(m[0]!='H'||m[1]!='2'||m[2]>HH_MAX) return;
    int n=m[2], k=4+n*HH_REC+HH_RELB; for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum) return;
    if(m[3]>=HU_N) return;
    k=4; hhPUid=m[3];
    for(int i=0;i<n;i++){ HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) s->look[j]=m[k++]; s->stage=m[k++]; s->asp=m[k++]; s->ltw=m[k++];
        for(int j=0;j<TR_N;j++) s->tr[j]=m[k++]; for(int j=0;j<10;j++) s->name[j]=(char)m[k++]; s->name[9]=0; for(int j=0;j<HN_N;j++) s->need[j]=m[k++]; s->uid=m[k++];
        if(s->stage>=AG_N||s->asp>=AS_PICK||s->uid>=HU_N) return;
        s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0; s->bubT=0; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=(signed char)m[k++]; relL[a][b]=(signed char)m[k++]; relF[a][b]=m[k++]; }
    hhN=n;
}
_Static_assert(HH_OFF+4+HH_MAX*HH_REC+HH_RELB+1<=20480,"the household must fit before the room slots");

// ---- the pause menu's HOUSEHOLD screen ----
static void hhMenu(void){
    static char lb[HH_NFAM+3][24]; const char* it[HH_NFAM+3]; int n=0;
    it[n++]="RELATIONSHIPS";
    for(int f=0;f<HH_NFAM;f++){ char*e=lb[n]; const char*p="MOVE IN "; while(*p) *e++=*p++; p=hhFams[f].fam; while(*p) *e++=*p++; *e=0; it[n]=lb[n]; n++; }
    it[n++]="MOVE EVERYONE OUT";
    char t[24]; { char*e=t; const char*p="HOUSEHOLD  "; while(*p) *e++=*p++; e+=numStr(e,hhN+1); p=" OF 8"; while(*p) *e++=*p++; *e=0; }
    int c=menu(t,it,n); if(c<0) return;
    if(c==0){ relScreen(); return; }
    c--;
    if(c==HH_NFAM){ hhN=0; for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; } hhSave(); toast("ONLY YOU LIVE HERE NOW"); return; }
    const HhFam*F=&hhFams[c]; int add=0, first=hhN;
    for(int i=0;i<F->n&&hhN<HH_MAX;i++){ hhNew(&hhM[hhN],&F->m[i]); hhN++; add++; }
    for(int i=first;i<hhN;i++)for(int j=first;j<hhN;j++) if(i!=j){   // a family already knows and likes each other; couples (the first two adults) are in love
        int a=hhM[i].uid, b=hhM[j].uid; relD[a][b]=40; relL[a][b]=50; relF[a][b]=0;
        if(i<first+2&&j<first+2&&hhM[i].stage>=AG_ADULT&&hhM[j].stage>=AG_ADULT){ relD[a][b]=70; relL[a][b]=80; relF[a][b]=RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF; } }
    if(!add){ toast("THE HOUSE IS FULL"); return; }
    for(int m=0;m<hhN;m++){ hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave(); toast(add<F->n?"SOME DID NOT FIT":"WELCOME HOME");
}

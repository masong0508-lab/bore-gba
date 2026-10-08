// savegame.h - PLAYERS: a save file for every player, so other people can play their own life on the same cart (PLAY on the main menu).
//
// A PLAYER SAVE FILE is a slot run of kind SLK_PLAYER (slots.h: header byte 28 = the player's number). Its payload is a list of chunks, like every slot:
//   'P' place (version, home lot, lot you stand on, town slot, town key)   'C' the person   'L' the life   'D' DNA and unlocked parts
//   'F' the household (the house.h block, byte for byte)                   'Y' the story block
// WRITING   a new copy goes into free slots first and is checked; only then is the old copy of the same player retired. A power cut in the middle
//           can only lose the new copy: the old one stays and wins (the highest save counter of a player is the one that loads). With no free
//           slots at all (32 KB SRAM) the file is written over itself.
// LOADING   the file is checked completely (checksum and every chunk) before anything is replaced. Then the town of the player is found by its
//           name key, their home lot becomes yours and the lot you stood on opens.
// SHARING   players share the neighborhood: every player lives on a house lot of their own (a new player gets a lot nobody lives on; if there is
//           none, a new lot is made). Each player keeps their own life, creature, household and story.
// SAVING    leaving play saves the player; PAUSE > SAVE GAME saves at once. A life started outside a player (the old PLAY tiles) belongs to nobody.
// Needs before it: slots.h, neighborhood.h, households.h, story.h, newGame, playScreen, lifeMode, neighborhoodScreen, the UI kit.
#define SLC_PLACE  'P'
#define SLC_DNA    'D'
#define SLC_FAMILY 'F'
#define SLC_STORY  'Y'
#define SLC_SKILLS 'K'   // skills.h: the skill points of this player (SK_N bytes)
#define SLC_GHOST  'G'   // fx.h: the ghosts of this life (12 bytes, the same block as in the life in SRAM)
static void fxGhostSave(volatile unsigned char*m); static void fxGhostLoad(volatile unsigned char*m); static void fxGhostClear(void);
static void sgEncGhost(SlW*w){ unsigned char t[12]; fxGhostSave(t); for(int i=0;i<12;i++) slwPut(w,t[i]); }
static int sgPlHome EWRAM_BSS, sgPlCur EWRAM_BSS, sgPlSlot EWRAM_BSS; static u16 sgPlKey EWRAM_BSS;   // what the last parse read from a 'P' chunk

static void sgEncPlace(SlW*w){
    slwPut(w,1); slwPut(w,nbOk?nbT.home:255); slwPut(w,nbOk&&nbT.cur<NB_LOTS?nbT.cur:255); slwPut(w,nbOk&&nbTS>=0?nbTS:255); slwPut16(w,nbOk?nbKey(&nbT):0);
}
static void sgEncDna(SlW*w){ slwPut16(w,pDna); slwPut16(w,pUnl); }
static void sgEncFamily(SlW*w){ volatile u8*b=SL_HHBLK; int n=hhBlockLen(b,SL_HH_LEN); for(int i=0;i<n;i++) slwPut(w,b[i]); }
static void sgEncSkills(SlW*w){ for(int i=0;i<SK_N;i++) slwPut(w,skPts[i]); }
static void sgEncStory(SlW*w){ volatile u8*m=SRAM_BASE+STORY_OFF; for(int i=0;i<8;i++) slwPut(w,m[i]); }
static void sgEncStats(SlW*w){ lsEnsure(); for(int i=0;i<LS_N*LS_CH;i++) for(int b=0;b<4;b++) slwPut(w,(u8)(lsFlat[i]>>(8*b))); }
static int sgBuild(SlW*w){
    slChunk(w,SLC_PLACE,sgEncPlace); slChunk(w,SLC_PERSON,slEncPerson);
    if(simsCheck(SIM_SRAM)) slChunk(w,SLC_LIFE,slEncLife);
    slChunk(w,SLC_DNA,sgEncDna);
    if(hhBlockLen(SL_HHBLK,SL_HH_LEN)) slChunk(w,SLC_FAMILY,sgEncFamily);
    slChunk(w,SLC_STORY,sgEncStory);
    slChunk(w,SLC_GHOST,sgEncGhost);
    slChunk(w,SLC_SKILLS,sgEncSkills);
    slChunk(w,SLC_STATS,sgEncStats);
    slwPut(w,0); return w->pos;
}
// all the players (the newest copy of each) into l: their slots. Scans the slots.
static int sgList(int*l){
    slScan(); int n=0;
    for(int i=0;i<SLOT_N&&n<SLOT_MAX;i++){
        if(slOwner[i]!=i||slI[i].kind!=SLK_PLAYER||!slGood[i]) continue;
        int dup=0; for(int k=0;k<n;k++) if(slI[l[k]].pid==slI[i].pid){ dup=1; if((short)(slI[i].seq-slI[l[k]].seq)>0) l[k]=i; }
        if(!dup) l[n++]=i;
    }
    return n;
}
static int sgFind(int pid){   // the newest good copy of a player's file (after a scan), or -1
    int best=-1;
    for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_PLAYER&&slI[i].pid==pid&&slGood[i]&&(best<0||(short)(slI[i].seq-slI[best].seq)>0)) best=i;
    return best;
}
static int sgNewPid(void){   // a player number nobody has (after a scan), 0 = none left
    for(int p=1;p<250;p++){ int used=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_PLAYER&&slI[i].pid==p) used=1; if(!used) return p; }
    return 0;
}
static int sgHomeOf(int slot){   // the home lot kept in a player's file (the 'P' chunk comes first), 255 = unknown
    volatile u8*b=SLB(slot)+SLOT_HDR;
    return (b[0]==SLC_PLACE&&b[3]==1)?b[4]:255;
}
static int sgSaveI(void){   // the player in play into a fresh copy of their file. SLE_OK or an error (the old copy is untouched then)
    if(!sgPid) return SLE_EMPTY;
    simsSaveNow(); hhSave(); ageSave(); persSave(); stSave(); slScan();
    int old=sgFind(sgPid);
    SlW d; slwInit(&d,0,1<<20); sgBuild(&d);
    int span=(SLOT_HDR+d.pos+SLOT_SZ-1)/SLOT_SZ; if(span>4) return SLE_BIG;
    int dst=-1, inplace=0;
    for(int s=0;s+span<=SLOT_N&&dst<0;s++){ if(!slFits(s,span)) continue; int f=1; for(int k=0;k<span;k++) if(slOwner[s+k]>=0) f=0; if(f) dst=s; }
    if(dst<0&&old>=0&&slI[old].span>=span){ dst=old; inplace=1; }   // no spare room: written over itself
    if(dst<0) return SLE_NOROOM;
    char nm[SLOT_NAME+1]; { int i=0; for(;i<SLOT_NAME&&hhPName[i];i++) nm[i]=hhPName[i]; if(!i) nm[i++]='P'; nm[i]=0; }
    int seq=old>=0?slI[old].seq+1:1;
    slOpen(dst,span);
    SlW w; slwInit(&w,SLO(dst)+SLOT_HDR,span*SLOT_SZ-SLOT_HDR); sgBuild(&w);
    if(w.over) return SLE_BIG;
    slHPid=(u8)sgPid; slHeader(dst,SLK_PLAYER,span,0,w.pos,slwSum(&w),seq,nm);
    int e=slVerify(dst); if(e) return e;
    if(old>=0&&!inplace){ svWr(SLO(old),0); svWr(SLO(old)+1,0); }   // the old copy goes only now
    svCommit(); return SLE_OK;
}
static int sgParse(volatile u8*body,int len,int apply){   // apply 0: check every chunk. apply 1: put it all in place
    SlR r={body,0,len,0}; int gotC=0, gotP=0;
    while(r.pos<len){
        int tag=slrGet(&r); if(tag==0) break;
        int cl=slrGet16(&r); if(r.bad||r.pos+cl>len) return SLE_FMT;
        SlR c={body+r.pos,0,cl,0}; r.pos+=cl;
        if(tag==SLC_PLACE){ if(cl<6||c.p[0]!=1) return SLE_FMT; if(apply){ sgPlHome=c.p[1]; sgPlCur=c.p[2]; sgPlSlot=c.p[3]; sgPlKey=(u16)(c.p[4]|(c.p[5]<<8)); } gotP=1; }
        else if(tag==SLC_PERSON){ if(!slDecPerson(&c,apply)) return SLE_FMT; gotC=1; }
        else if(tag==SLC_LIFE){ if(!slDecLife(&c,apply)) return SLE_FMT; }
        else if(tag==SLC_DNA){ if(cl!=4) return SLE_FMT; if(apply){ pDna=(u16)(c.p[0]|(c.p[1]<<8)); if(pDna>9999) pDna=9999; pUnl=(u16)(c.p[2]|(c.p[3]<<8)); persSave(); } }
        else if(tag==SLC_FAMILY){ int n=hhBlockLen(c.p,cl); if(!n||n!=cl) return SLE_FMT; if(apply){ volatile u8*d=SL_HHBLK; for(int i=0;i<n;i++) d[i]=c.p[i]; } }
        else if(tag==SLC_STORY){ if(cl!=8||c.p[0]!='S'||c.p[1]!='Y') return SLE_FMT; if(apply){ volatile u8*d=SRAM_BASE+STORY_OFF; for(int i=0;i<8;i++) d[i]=c.p[i]; } }
        else if(tag==SLC_GHOST){ if(cl!=12) return SLE_FMT; if(apply){ fxGhostLoad(c.p); fxGhostSave(SIM_SRAM+SIM_BLOCK); } }
        else if(tag==SLC_SKILLS){ if(cl!=SK_N) return SLE_FMT; if(apply){ for(int i=0;i<SK_N;i++) skPts[i]=c.p[i]; skSave(); } }
        else if(tag==SLC_STATS){ if(cl<4||cl%4||cl>4*LS_N*LS_CH) return SLE_FMT; if(apply){ for(int i=0;i<cl/4;i++) lsFlat[i]=(u32)c.p[4*i]|((u32)c.p[4*i+1]<<8)|((u32)c.p[4*i+2]<<16)|((u32)c.p[4*i+3]<<24); lsInit=1; lsDirty=1; lsSave(); } }
        // anything else: a later version's chunk, skipped on purpose
    }
    return (gotP&&gotC)?SLE_OK:SLE_FMT;
}
static int sgSave(void){   // sgSaveI under a loading screen (flash writes are slow); clears the unsaved flag
    ldShow("SAVING GAME",0,2); int e=sgSaveI(); ldShow("SAVING GAME",2,2); ldEnd(); if(!e) sgDirty=0; return e;
}
static void sgEnterTown(void){   // the player's town and lot become the live ones
    int l[SLOT_MAX], n=nbTownList(l,SLOT_MAX), pick=-1;
    for(int i=0;i<n;i++) if(l[i]==sgPlSlot&&nbRead(l[i],&nbTmp)&&nbKey(&nbTmp)==sgPlKey) pick=l[i];
    for(int i=0;i<n&&pick<0;i++) if(nbRead(l[i],&nbTmp)&&nbKey(&nbTmp)==sgPlKey) pick=l[i];
    if(pick>=0&&nbSwitch(pick)){
        nbOk=1;
        if(sgPlHome<NB_LOTS&&nbT.lot[sgPlHome].on&&nbT.lot[sgPlHome].kind==LKIND_RES) nbT.home=(u8)sgPlHome;
        int want=(sgPlCur<NB_LOTS&&nbT.lot[sgPlCur].on)?sgPlCur:nbT.home;
        if(want<NB_LOTS&&nbT.lot[want].on&&nbT.cur!=want) nbGo(want);
    } else nbOk=nbLoad();   // the town is gone: the one that is there
    nbBounds(); liveInvalidate(); camSnap=1;
}
static int sgLoadPlayer(int slot){   // 0 = the player is in play now, else a SLE_ error and nothing was replaced
    SlInfo I; if(!slInfo(slot,&I)||I.kind!=SLK_PLAYER) return SLE_EMPTY;
    volatile u8*b=SLB(slot)+SLOT_HDR;
    if(slSumOf(b,I.len)!=I.sum) return SLE_BAD;
    int e=sgParse(b,I.len,0); if(e) return e;
    skReset(); lsReset(); sgParse(b,I.len,1);   // (a file from before the skills has no 'K' chunk: they start at zero)
    svCommit(); hhLoad(); stLoad(); ageLoad();
    twKeep=0; sprKey=0; for(int m=0;m<HH_MAX;m++) hhKey[m]=0; hhSlotsFree(); moodReset(); lscore=0; simLastScore=0;
    sgPid=I.pid; sgDirty=0;
    sgEnterTown();
    return SLE_OK;
}
static int sgDeletePid(int pid){
    slScan(); for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_PLAYER&&slI[i].pid==pid) slDelete(i);
    if(sgPid==pid) sgPid=0;
    return 0;
}
static void sgPickHome(void){   // a NEW player: a house lot nobody lives on (their own home), else a new lot on free land
    int l[SLOT_MAX], n=sgList(l); u8 used[NB_LOTS]; for(int i=0;i<NB_LOTS;i++) used[i]=0;
    for(int i=0;i<n;i++){ int h=sgHomeOf(l[i]); if(h<NB_LOTS) used[h]=1; }
    int pick=-1;
    for(int li=0;li<NB_LOTS&&pick<0;li++){ const NbLot*L=&nbT.lot[li]; if(L->on&&L->kind==LKIND_RES&&!used[li]&&!nbWho(li,0)) pick=li; }
    if(pick<0){
        int i=0; while(i<NB_LOTS&&nbT.lot[i].on) i++;
        if(i<NB_LOTS) for(int sz=6;sz>=4&&pick<0;sz--) for(int y=0;y+sz<=NB_H&&pick<0;y++) for(int x=0;x+sz<=NB_W&&pick<0;x++) if(nbFree(x,y,sz,sz,-1)){
            char nm[SLOT_NAME+1]; slNameN(nm,"HOME ",i+1); nbLotAdd(i,nm,x,y,sz,sz,LKIND_RES,0);
            for(int q=y;q<y+sz;q++)for(int p=x;p<x+sz;p++) nbT.cell[q][p]&=7;   // what stood there is cleared
            pick=i; }
    }
    if(pick<0){ toast("NO FREE LOT  SHARING A HOME"); return; }
    nbT.home=(u8)pick; nbGo(pick);
}
static void sgAdopt(void){   // the first time: the life already on the cart becomes player 1 (nothing is lost)
    if(!simsCheck(SIM_SRAM)) return;
    int l[SLOT_MAX]; if(sgList(l)) return;
    int pid=sgNewPid(); if(!pid) return;
    sgPid=(u8)pid; if(sgSave()) sgPid=0;
}

// ---------- the PLAYERS screen ----------
#define SG_ROWS(n) ((n)+2+(dbgOn?1:0))   // the TEST MAP row only exists with the debug code on
static void sgDraw(const int*l,int n,int sel){
    fillCols(0,ROW_W,RGB(3,4,8));
    box(3,1,234,157); text(12,6,"PLAYERS",GOLD,1);
    int top=sel-2; if(top>SG_ROWS(n)-6) top=SG_ROWS(n)-6; if(top<0) top=0;
    for(int i=top;i<top+6&&i<SG_ROWS(n);i++){
        int y=18+(i-top)*15;
        if(i==sel){ rect(8,y-2,222,14,RGB(6,16,8)); rect(8,y-2,2,14,GOLD); }
        u16 nc=i==sel?WHITE:DIMC;
        if(i<n){ const SlInfo*I=&slI[l[i]]; char b[40]; char*e=slCat(b,"SAVED "); e=slNum(e,I->seq); slCat(e,I->seq==1?" TIME":" TIMES");
            text(16,y,I->name[0]?I->name:"NO NAME",nc,1); text(16,y+7,b,i==sel?RGB(22,25,28):RGB(11,13,18),1);
            { u32 hs=lsPlayerSecs(l[i]); if(hs){ char hb[16]; char*he=simCatN(hb,(int)(hs/3600)); he=simCat(he," H "); simCatN(he,(int)(hs/60%60)); simCat(hb," M"); text(230-tw(hb,1),y+7,hb,i==sel?RGB(22,25,28):RGB(11,13,18),1); } }
            if(sgPid==I->pid) text(190,y,"PLAYING",GOLD,1); }
        else text(16,y+2,i==n?"NEW PLAYER":i==n+1?"NEIGHBORHOODS":"TEST MAP",nc,1);
    }
    text(12,112,"UP DOWN PICK  A PLAY  START MORE",WHITE,1);
    text(12,125,"EVERY PLAYER HAS A SAVE FILE OF THEIR OWN",DIMC,1);
    text(12,134,"AND LIVES ON A LOT OF THE SHARED TOWN",DIMC,1);
    text(12,143,"PAUSE  SAVE GAME SAVES AT ONCE",DIMC,1);
}
// SAVING (OPTIONS > DATA): AUTO saves the player when play is left, as it always did. MANUAL (the default) saves only on PAUSE > SAVE GAME: leaving play with
// unsaved progress asks (SAVE, or QUIT WITHOUT SAVING, which loads the last save file again), and a power cut or reset loses everything since the last
// save (PLAY loads the player from the file, never from the live copy). sgDirty is set by simsTick (a game minute or a score) and cleared by a save or a load.
static int sgReload(void){ slScan(); int s=sgFind(sgPid); if(s<0) return SLE_EMPTY; return sgLoadPlayer(s); }   // the last save file replaces the live player
static int sgAsk(int cancel){
    if(!sgPid||!sgDirty) return 1;
    static const char* const it3[3]={"SAVE AND QUIT","QUIT WITHOUT SAVING","KEEP PLAYING"}, *const it2[2]={"SAVE","QUIT WITHOUT SAVING"};
    for(;;){
        int c=cancel?menu("YOU HAVE UNSAVED PROGRESS",it3,3):menu("YOU HAVE UNSAVED PROGRESS",it2,2);
        if(c<0||(cancel&&c==2)){ if(cancel) return 0; continue; }   // (B or KEEP PLAYING; after the fact there is no staying: ask again)
        if(c==0){ int e=sgSave(); if(!e){ toast("GAME SAVED"); return 1; } toast(slErrMsg(e)); return cancel?0:1; }
        if(menu("LOSE YOUR PROGRESS?",slYesNo,2)==1) return 2;
    }
}
static void sgBeforeLeave(void){   // another player (or a new one) is about to load: the one in play is saved (AUTO) or asked about (MANUAL)
    if(!sgPid) return;
    if(!sgManual()){ int e=sgSave(); if(e) toast(slErrMsg(e)); } else sgAsk(0);
}
static void sgLeaveSave(void){   // play was left
    if(!sgPid) return;
    if(!sgManual()&&sgDiscard!=2){ int e=sgSave(); toast(e?slErrMsg(e):"GAME SAVED"); return; }   // (sgDiscard 2 = chapter 4 was lost: the last save file comes back even with AUTO saving)
    int r=sgDiscard?2:sgAsk(0); sgDiscard=0;
    if(r==2){ int e=sgReload(); toast(e?slErrMsg(e):"BACK TO YOUR LAST SAVE"); }
}
static void sgPlayerMenu(int slot,int c0){   // c0: -1 asks (CONTINUE / NEIGHBORHOOD / DELETE), else that choice at once (A on a player = CONTINUE)
    static const char* const it[3]={"CONTINUE","NEIGHBORHOOD","DELETE PLAYER"};
    int pid=slI[slot].pid, c=c0>=0?c0:menu(slI[slot].name[0]?slI[slot].name:"PLAYER",it,3); if(c<0) return;
    if(c==2){ if(menu("DELETE THIS PLAYER",slYesNo,2)==1){ sgDeletePid(pid); toast("PLAYER DELETED"); } return; }
    if(sgPid!=pid){
        sgBeforeLeave();   // the player who was in play is saved before the next one loads
        slScan(); int s=sgFind(pid); if(s<0){ toast("SAVE FILE IS DAMAGED"); return; }
        box(60,64,120,24); text(76,72,"LOADING...",WHITE,1); present();
        int e=sgLoadPlayer(s); if(e){ toast(slErrMsg(e)); return; }
    }
    nbBounds();
    if(c==0) lifeMode(0); else neighborhoodScreen();
    if(!gToMenu||sgPid) sgLeaveSave();
}
static void sgNewPlayer(void){
    int l[SLOT_MAX], n=nbTownList(l,SLOT_MAX); if(n>16) n=16;
    if(!n){ toast("MAKE A NEIGHBORHOOD FIRST"); return; }
    static char tn[16][NB_NAME+1] EWRAM_BSS; const char* nm[16];
    for(int i=0;i<n;i++){ nbRead(l[i],&nbTmp); int k=0; for(;nbTmp.name[k]&&k<NB_NAME;k++) tn[i][k]=nbTmp.name[k]; tn[i][k]=0; nm[i]=tn[i]; }
    int c=n==1?0:menu("WHICH NEIGHBORHOOD",nm,n); if(c<0) return;   // (one town: nothing to ask)
    sgBeforeLeave();
    slScan(); int pid=sgNewPid(); if(!pid){ toast("TOO MANY PLAYERS"); return; }
    sgWant=(u8)pid;
    int started=newGame(l[c]);   // (it gives the new player a home lot and the number above, then a fresh life)
    sgWant=0;
    if(started) sgLeaveSave();
}
static void playerScreen(void){
    int l[SLOT_MAX]; nbFirstTowns(l); nbOk=nbLoad(); nbBounds();
    sgAdopt();
    int n=sgList(l), sel=0, dirty=1; u16 prev=keyNow();
    for(int i=0;i<n;i++) if(sgPid&&slI[l[i]].pid==sgPid) sel=i;   // the cursor starts on the player already in play
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_DOWN){ sel=(sel+1)%SG_ROWS(n); dirty=1; }
        if(pr&K_UP){ sel=(sel+SG_ROWS(n)-1)%SG_ROWS(n); dirty=1; }
        if(pr&K_B) break;
        if(pr&(K_A|K_START)){   // A on a player plays at once; START opens the player menu (neighborhood, delete)
            if(sel==n+1) playScreen();   // the neighborhoods: make, rename, delete, visit
            else if(sel==n) sgNewPlayer();
            else if(sel==n+2){ sgBeforeLeave(); newGame(nbOk?nbTS:-1); }   // SECRET (debug code): the old NEW GAME on the old assigned lot, the TEST MAP. The life belongs to no player
            else sgPlayerMenu(l[sel],(pr&K_START)?-1:0);
            if(gToMenu) break;
            n=sgList(l); if(sel>=SG_ROWS(n)) sel=SG_ROWS(n)-1;
            prev=keyNow(); dirty=1; mmPick();
        }
        if(dirty){ sgDraw(l,n,sel); uiPresent(); dirty=0; } else vsync();
        uiTicks++; menuMusTick();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();
}

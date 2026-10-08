// goals.h - the GOALS of a lot (roadmap #2 and #3): four goals per lot, saved per town, each paid once.
//   TAPE   take the hidden tape (+50)         SKATE  spell S K A T E (+100)       CLEAR  LOT CLEARED, every letter and the tape of a run (+100)
//   SCORE  reach TG_SCORE points in one run (+150)       all four on a lot = LOT MASTERED (+300 more)
// The tape also stays found: it does not come back on a lot where it was taken. Only lots with something to skate have goals (clLive).
// VIEW GOALS (PAUSE > WANTS) lists them for the lot you stand on, the TIMED RUN best, and the totals of the town.
// SRAM: TG_OFF, 39 bytes in the free gap after the timed run block: 'G' 'V', then TG_N entries (the newest town first, the oldest falls off):
//       town key (nbKey) and five 16 bit lot masks: tape found, lots played that have goals, SKATE done, CLEAR done, SCORE done; then a checksum.
// Not built yet: goals that unlock parts or maps (the pay is cash for now), a wallride goal (needs wallride); the GAP bonus goal is in (gpLoad). The intro flyover (introFly) is at the end.
// Needs before it: nbOk, nbT, nbKey, NB_LOTS (neighborhood.h, households.h), stBack (story.h), clLive / clTook / clReal / clGot / CL_ON (main.c),
// trnBestGet (timedrun.h), simMoney, simCat / simCatN, the UI kit.
#define TG_OFF 4825
#define TG_N   3
#define TG_LEN 39
#define TGF_TAPE  1
#define TGF_SKATE 2
#define TGF_CLEAR 4
#define TGF_SCORE 8
#define TG_MASTER 6000  // the bonus for all four
_Static_assert(TRN_OFF+TRN_LEN<=TG_OFF&&TG_OFF+TG_LEN<=SET_OFF&&TG_LEN==3+TG_N*12,"the goals block must sit between the timed run block and the settings");
static const u16 tgPay[4]={1000,2000,2000,3000};
static const u8 tgIx[4]={0,2,3,4};   // goal -> its mask (1 is 'lots played')
static u16 tgKey[TG_N] EWRAM_BSS, tgM[TG_N][5] EWRAM_BSS;   // a copy of the block (tgLoad fills it)
static u8 tgHave EWRAM_BSS;   // the goals done on the lot in play (bits TGF_*), read when the lot opens
static const char* tgPend EWRAM_BSS; static char tgMsg[28] EWRAM_BSS;   // a goal note waits until the note line is free
static void tgLoad(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x50; int ok=(m[0]=='G'&&m[1]=='V');
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    if(ok&&m[TG_LEN-1]!=(u8)sum) ok=0;
    for(int i=0;i<TG_N;i++){ int o=2+i*12; tgKey[i]=ok?(u16)(m[o]|(m[o+1]<<8)):0; for(int k=0;k<5;k++) tgM[i][k]=ok?(u16)(m[o+2+k*2]|(m[o+3+k*2]<<8)):0; }
}
static void tgSave(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x50;
    for(int i=0;i<TG_N;i++){ int o=2+i*12; m[o]=(u8)tgKey[i]; m[o+1]=(u8)(tgKey[i]>>8); for(int k=0;k<5;k++){ m[o+2+k*2]=(u8)tgM[i][k]; m[o+3+k*2]=(u8)(tgM[i][k]>>8); } }
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    m[TG_LEN-1]=(u8)sum; m[0]='G'; m[1]='V';
}
static u16 tgKeyNow(void){ return nbOk?nbKey(&nbT):0; }
static int tgLotNow(void){ return (nbOk&&nbT.cur<NB_LOTS)?nbT.cur:0; }
static int tgAt(u16 key){ for(int i=0;i<TG_N;i++){ if(tgKey[i]!=key) continue; for(int k=0;k<5;k++) if(tgM[i][k]) return i; } return -1; }
static int tgBitsOf(int i,int lot){ int h=0; for(int g=0;g<4;g++) if(tgM[i][tgIx[g]]>>lot&1) h|=1<<g; return h; }   // the goals done on a lot of entry i
static int tgFound(void){ return tgHave&TGF_TAPE; }   // (tgSeen read it when the lot opened)
static void tgTouch(int j,u16 b){   // set a bit of mask j (and 'played') for the town in play; its entry moves to the front
    tgLoad(); u16 key=tgKeyNow(); int at=tgAt(key); u16 m[5]={0,0,0,0,0};
    if(at>=0){ for(int k=0;k<5;k++) m[k]=tgM[at][k]; } else at=TG_N-1;
    m[j]|=b; m[1]|=b;
    for(int i=at;i>0;i--){ tgKey[i]=tgKey[i-1]; for(int k=0;k<5;k++) tgM[i][k]=tgM[i-1][k]; }
    tgKey[0]=key; for(int k=0;k<5;k++) tgM[0][k]=m[k]; tgSave();
}
static void tgSeen(void){   // the lot in play has things to skate, so it has goals: count it, and read which are done
    u16 b=(u16)(1u<<tgLotNow()); tgLoad(); int i=tgAt(tgKeyNow());
    tgHave=(u8)(i>=0?tgBitsOf(i,tgLotNow()):0);
    if(i>=0&&(tgM[i][1]&b)) return;
    tgTouch(1,b);
}
static void tgDone(int bit){   // a goal was reached on the lot in play (nothing happens when it was done before)
    if(!CL_ON||(tgHave&bit)) return;
    int g=bit==TGF_TAPE?0:bit==TGF_SKATE?1:bit==TGF_CLEAR?2:3, pay=tgPay[g];
    tgHave|=(u8)bit; tgTouch(tgIx[g],(u16)(1u<<tgLotNow()));
    char*e=simCat(tgMsg,"GOAL DONE  +");
    if(tgHave==15){ pay+=TG_MASTER; simCatN(simCat(tgMsg,"LOT MASTERED  +"),pay); } else simCatN(e,pay);
    simMoneyAdd(pay);
    tgPend=tgMsg;
}
// ---- the GAP goal (phase 1): a bonus goal outside the four that make LOT MASTERED (so old saves and the +300 stay as they were) ----
// One jump on the board that carries you GP_NEED tiles from where you took off (main.c keeps the takeoff spot: lgx0 / lgy0) and lands without a bail.
// SRAM: GP_OFF, 15 bytes in the free gap between the story block and the slot directory: 'G' '2', then GP_N entries of (town key, 16 bit lot mask), the newest town first, then a checksum.
#define GP_OFF  4976
#define GP_N    3
#define GP_LEN  15
#define GP_NEED 5     // tiles (the two kickers that face each other in the rail park are 5 apart)
#define GP_PAY  2400
_Static_assert(STORY_OFF+8<=GP_OFF&&GP_OFF+GP_LEN<=SLOT_DIR&&GP_LEN==3+GP_N*4,"the gap goal block must sit between the story block and the slot directory");
static u16 gpKey[GP_N] EWRAM_BSS, gpM[GP_N] EWRAM_BSS;
static void gpLoad(void){
    volatile u8*m=SRAM_BASE+GP_OFF; unsigned sum=0x47; int ok=(m[0]=='G'&&m[1]=='2');
    for(int i=2;i<GP_LEN-1;i++) sum+=m[i];
    if(ok&&m[GP_LEN-1]!=(u8)sum) ok=0;
    for(int i=0;i<GP_N;i++){ int o=2+i*4; gpKey[i]=ok?(u16)(m[o]|(m[o+1]<<8)):0; gpM[i]=ok?(u16)(m[o+2]|(m[o+3]<<8)):0; }
}
static void gpSave(void){
    volatile u8*m=SRAM_BASE+GP_OFF; unsigned sum=0x47;
    for(int i=0;i<GP_N;i++){ int o=2+i*4; m[o]=(u8)gpKey[i]; m[o+1]=(u8)(gpKey[i]>>8); m[o+2]=(u8)gpM[i]; m[o+3]=(u8)(gpM[i]>>8); }
    for(int i=2;i<GP_LEN-1;i++) sum+=m[i];
    m[GP_LEN-1]=(u8)sum; m[0]='G'; m[1]='2';
}
static int gpAt(u16 key){ for(int i=0;i<GP_N;i++) if(gpM[i]&&gpKey[i]==key) return i; return -1; }
static int gpDone(int lot){ gpLoad(); int i=gpAt(tgKeyNow()); return i>=0&&(gpM[i]>>lot&1); }   // the gap goal of a lot of the town in play
static void tgGap(int tiles){   // a clean landing `tiles` from the takeoff spot (main.c, lifeStep)
    if(tiles<GP_NEED||!CL_ON||gpDone(tgLotNow())) return;
    u16 key=tgKeyNow(), b=(u16)(1u<<tgLotNow()); int at=gpAt(key); u16 m=b;
    if(at>=0) m|=gpM[at]; else at=GP_N-1;
    for(int i=at;i>0;i--){ gpKey[i]=gpKey[i-1]; gpM[i]=gpM[i-1]; }
    gpKey[0]=key; gpM[0]=m; gpSave();
    simCatN(simCat(tgMsg,"GAP GOAL  +"),GP_PAY);
    simMoneyAdd(GP_PAY);
    tgPend=tgMsg;
}
static void tgMark(void){ tgDone(TGF_TAPE); }   // the tape of the lot in play was just taken
static void tgPump(void){ if(tgPend&&lnoteT<=0){ lnote=tgPend; lnoteT=100; tgPend=0; sfxPlay(SFX_STICK); } }   // (from clTick, every step)
static void goalsScreen(void){
    static char b[12] EWRAM_BSS;
    static const char* const gn[4]={"FIND THE HIDDEN TAPE  +50","SPELL S K A T E  +100","CLEAR THE LOT  +100","SCORE 2000 IN A RUN  +150"};
    tgLoad(); int ti=tgAt(tgKeyNow()), lots=0, tapes=0, mast=0, lot=tgLotNow();
    if(nbOk&&ti>=0){ for(int i=0;i<NB_LOTS;i++) if(nbT.lot[i].on&&(tgM[ti][1]>>i&1)){ lots++; int h=tgBitsOf(ti,i); if(h&TGF_TAPE) tapes++; if(h==15) mast++; } }
    int live=clLive!=0, have=ti>=0?tgBitsOf(ti,lot):0, best=trnBestGet(); u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(cnt&7){ vsync(); continue; }
        stBack("GOALS",(int)cnt);
        const u16 lab=RGB(22,25,28), ok=RGB(14,30,14), no=RGB(12,14,16);
        text(10,21,nbOk&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on?nbT.lot[nbT.cur].name:"THIS LOT",GOLD,1);
        if(!live){ text(10,36,"NOTHING TO SKATE HERE",lab,1); text(10,48,"BUILD A RAMP OR RAIL FOR GOALS",no,1); }
        else {
            for(int g=0;g<4;g++){ int d=have>>g&1; text(10,34+g*12,gn[g],lab,1); text(230-tw(d?"DONE":"NOT YET",1),34+g*12,d?"DONE":"NOT YET",d?ok:no,1); }
            int gd=gpDone(lot); text(10,82,"JUMP A 5 TILE GAP  +120",lab,1); text(230-tw(gd?"DONE":"NOT YET",1),82,gd?"DONE":"NOT YET",gd?ok:no,1);   // the bonus goal (not one of the four)
            int all=have==15; text(10,94,"ALL FOUR  LOT MASTERED  +300",lab,1); text(230-tw(all?"DONE":"NOT YET",1),94,all?"DONE":"NOT YET",all?GOLD:no,1);
        }
        text(10,104,"TIMED RUN  HIGH SCORE",lab,1); if(best) numText(190,104,best,GOLD); else text(230-tw("NONE",1),104,"NONE",no,1);
        { char*e=simCatN(b,tapes); e=simCat(e," OF "); simCatN(e,lots); text(10,114,"TAPES FOUND IN TOWN",lab,1); text(230-tw(b,1),114,b,tapes&&tapes==lots?ok:WHITE,1); }
        { char*e=simCatN(b,mast); e=simCat(e," OF "); simCatN(e,lots); text(10,124,"LOTS MASTERED",lab,1); text(230-tw(b,1),124,b,mast&&mast==lots?GOLD:WHITE,1); }
        text(10,134,"ONLY LOTS YOU HAVE SKATED COUNT",no,1);
        text(10,144,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}
// ---- the INTRO FLYOVER (roadmap #4): when a lot opens, the camera pans to the goal spots, each with a banner in the top bar ----
// It only visits the goals that are still open on this lot (the saved goal bits, tgHave): the tape spot if the tape is not found, the first and last
// letter if SKATE is not done, then back to you (with the score goal when that is open). Any of A, B, START skips it. The world holds still meanwhile.
// It plays once per lot per session (flySeen: gone at power off); a lot visited again, or the same lot after a retry, goes straight to play.
// camFollow looks at flyX / flyY instead of the skater while flyOn is set (main.c).
static u16 flySeen EWRAM_BSS, flyKey EWRAM_BSS;   // the lots whose flyover already played since power on (a bit per lot) and the town they belong to
static void introFly(void){
    if(!CL_ON||tutOn||tutModal!=TM_NONE||stModal||tgHave==15) return;
    { u16 key=tgKeyNow(), b=(u16)(1u<<tgLotNow()); if(key!=flyKey){ flyKey=key; flySeen=0; } if(flySeen&b) return; flySeen|=b; }   // once per lot per session
    s32 sx[4], sy[4]; const char* tx[4]; int n=0;
    if(!(tgHave&TGF_TAPE)&&!(clGot&32)){ sx[n]=clx[5]*256+128; sy[n]=cly[5]*256+128; tx[n++]="FIND THE HIDDEN TAPE"; }
    if(!(tgHave&TGF_SKATE)){
        if(!(clGot&1)){ sx[n]=clx[0]*256+128; sy[n]=cly[0]*256+128; tx[n++]="SKATE  FIRST LETTER"; }
        if(!(clGot&16)){ sx[n]=clx[4]*256+128; sy[n]=cly[4]*256+128; tx[n++]="SKATE  LAST LETTER"; } }
    if(!n) return;
    sx[n]=lfx; sy[n]=lfy; tx[n++]=(tgHave&TGF_SCORE)?"GOOD LUCK":"SCORE 2000 FOR A GOAL";
    u16 prev=keyNow(); int skip=0; flyOn=1; flyX=lfx; flyY=lfy; camSnap=1; lifeDraw();   // (first picture: at the skater)
    for(int i=0;i<n&&!skip;i++){
        flyX=sx[i]; flyY=sy[i]; lnote=tx[i]; lnoteT=200;
        for(int f=0;f<80&&!skip;f++){
            u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) skip=1;
            vsync(); gmTick(); lifeDraw();
        }
    }
    flyOn=0; camSnap=1; lnote=""; lnoteT=0;
    if(skip) while((~REG_KEYINPUT)&0x3FF) vsync();
}

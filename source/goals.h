// goals.h - VIEW GOALS (roadmap #2 and #3, first slice) and the tape count per lot.
// The hidden tape now stays found: the lots you took it on are remembered (a 16 bit mask of the town's lots), and it does not come back there.
// VIEW GOALS (PAUSE > WANTS) lists the goals of the lot you stand on: the tape, the S K A T E letters and LOT CLEARED of this run, the score,
// the TIMED RUN best, and how many lots of the town have had their tape found.
// SRAM: TG_OFF, 33 bytes in the free gap after the timed run block: 'G' 'U', then TG_N entries of (town key, lots with the tape found,
//       lots played that have a tape; 16 bits each), newest first (the oldest town falls off the end), then a checksum. A town is found by its name key (households.h nbKey).
// Needs before it: nbOk, nbT, nbKey, NB_LOTS (neighborhood.h, households.h), stBack (story.h), clLive / clTook / clReal / clGot (main.c),
// trnBestGet (timedrun.h), the UI kit.
#define TG_OFF 4826
#define TG_N   5
#define TG_LEN 33
_Static_assert(TRN_OFF+TRN_LEN<=TG_OFF&&TG_OFF+TG_LEN<=SET_OFF,"the goals block must sit between the timed run block and the settings");
static u16 tgKey[TG_N] EWRAM_BSS, tgFnd[TG_N] EWRAM_BSS, tgQual[TG_N] EWRAM_BSS;   // (a copy of the block; tgLoad fills it) town key / lots with the tape found / lots played that have a tape
static void tgLoad(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x4F; int ok=(m[0]=='G'&&m[1]=='U');
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    if(ok&&m[TG_LEN-1]!=(u8)sum) ok=0;
    for(int i=0;i<TG_N;i++){ int o=2+i*6; tgKey[i]=ok?(u16)(m[o]|(m[o+1]<<8)):0; tgFnd[i]=ok?(u16)(m[o+2]|(m[o+3]<<8)):0; tgQual[i]=ok?(u16)(m[o+4]|(m[o+5]<<8)):0; }
}
static void tgSave(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x4F;
    for(int i=0;i<TG_N;i++){ int o=2+i*6; m[o]=(u8)tgKey[i]; m[o+1]=(u8)(tgKey[i]>>8); m[o+2]=(u8)tgFnd[i]; m[o+3]=(u8)(tgFnd[i]>>8); m[o+4]=(u8)tgQual[i]; m[o+5]=(u8)(tgQual[i]>>8); }
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    m[TG_LEN-1]=(u8)sum; m[0]='G'; m[1]='U';
}
static u16 tgKeyNow(void){ return nbOk?nbKey(&nbT):0; }
static int tgLotNow(void){ return (nbOk&&nbT.cur<NB_LOTS)?nbT.cur:0; }
static int tgAt(u16 key){ for(int i=0;i<TG_N;i++) if((tgFnd[i]||tgQual[i])&&tgKey[i]==key) return i; return -1; }
static int tgFound(void){ tgLoad(); int i=tgAt(tgKeyNow()); return i>=0&&(tgFnd[i]>>tgLotNow()&1); }   // the tape of the lot in play was taken before
static void tgTouch(u16 addFound,u16 addQual){   // remember bits for the town in play (its entry moves to the front; the oldest town falls off)
    tgLoad(); u16 key=tgKeyNow(); int at=tgAt(key); u16 f=addFound, q=addQual;
    if(at>=0){ f|=tgFnd[at]; q|=tgQual[at]; } else at=TG_N-1;
    for(int i=at;i>0;i--){ tgKey[i]=tgKey[i-1]; tgFnd[i]=tgFnd[i-1]; tgQual[i]=tgQual[i-1]; }
    tgKey[0]=key; tgFnd[0]=f; tgQual[0]=q; tgSave();
}
static void tgMark(void){ u16 b=(u16)(1u<<tgLotNow()); tgTouch(b,b); }   // the tape of the lot in play was just taken
static void tgSeen(void){   // the lot in play has things to skate (so it has a tape): it counts towards the town's total
    u16 b=(u16)(1u<<tgLotNow()); tgLoad(); int i=tgAt(tgKeyNow());
    if(i>=0&&(tgQual[i]&b)) return;
    tgTouch(0,b);
}
static int tgBits(unsigned v){ int n=0; while(v){ n+=(int)(v&1); v>>=1; } return n; }
static void goalsScreen(void){
    static char b[12] EWRAM_BSS;
    tgLoad();
    int lots=0, tapes=0, ti=tgAt(tgKeyNow());
    if(nbOk&&ti>=0){ for(int i=0;i<NB_LOTS;i++) if(nbT.lot[i].on&&(tgQual[ti]>>i&1)){ lots++; if(tgFnd[ti]>>i&1) tapes++; } }   // only lots you have played count: others may have nothing to skate
    int live=clLive!=0, tape=tgFound()||(clTook&32)!=0, nl=tgBits(clTook&31), cleared=live&&clReal&&clTook==clReal;
    int best=trnBestGet(); u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(cnt&7){ vsync(); continue; }
        stBack("GOALS",(int)cnt);
        text(10,21,nbOk&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on?nbT.lot[nbT.cur].name:"THIS LOT",GOLD,1);
        const u16 lab=RGB(22,25,28), ok=RGB(14,30,14), no=RGB(12,14,16);
        if(!live){ text(10,36,"NOTHING TO SKATE HERE",lab,1); text(10,48,"BUILD A RAMP OR RAIL FOR GOALS",no,1); }
        else {
            text(10,36,"FIND THE HIDDEN TAPE",lab,1); text(230-tw(tape?"DONE":"NOT YET",1),36,tape?"DONE":"NOT YET",tape?ok:no,1);
            char*e=b; *e++=(char)('0'+nl); *e++=' '; *e++='O'; *e++='F'; *e++=' '; *e++='5'; *e=0;
            text(10,48,"COLLECT S K A T E",lab,1); text(230-tw(b,1),48,b,nl==5?ok:no,1);
            text(10,60,"CLEAR THE LOT  +500",lab,1); text(230-tw(cleared?"DONE":"NOT YET",1),60,cleared?"DONE":"NOT YET",cleared?ok:no,1);
            text(10,72,"SCORE THIS RUN",lab,1); numText(230-12*3,72,lscore,WHITE);
        }
        text(10,92,"TIMED RUN  HIGH SCORE",lab,1); if(best) numText(230-12*3,92,best,GOLD); else text(230-tw("NONE",1),92,"NONE",no,1);
        text(10,108,"TAPES FOUND IN TOWN",lab,1);
        { char*e=simCatN(b,tapes); *e++=' '; *e++='O'; *e++='F'; *e++=' '; e=simCatN(e,lots); text(230-tw(b,1),108,b,tapes&&tapes==lots?ok:WHITE,1); }
        text(10,124,"ONLY LOTS YOU HAVE SKATED COUNT",no,1); text(10,134,"A FOUND TAPE DOES NOT COME BACK",no,1);
        text(10,146,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}

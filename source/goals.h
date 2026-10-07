// goals.h - VIEW GOALS (roadmap #2 and #3, first slice) and the tape count per lot.
// The hidden tape now stays found: the lots you took it on are remembered (a 16 bit mask of the town's lots), and it does not come back there.
// VIEW GOALS (PAUSE > WANTS) lists the goals of the lot you stand on: the tape, the S K A T E letters and LOT CLEARED of this run, the score,
// the TIMED RUN best, and how many lots of the town have had their tape found.
// SRAM: TG_OFF, 27 bytes in the free gap after the timed run block: 'G' 'T', then TG_N entries of (town key 16 bit, lot mask 16 bit),
//       newest first (the oldest town falls off the end), then a checksum. A town is found by its name key (households.h nbKey).
// Needs before it: nbOk, nbT, nbKey, NB_LOTS (neighborhood.h, households.h), stBack (story.h), clLive / clTook / clReal / clGot (main.c),
// trnBestGet (timedrun.h), the UI kit.
#define TG_OFF 4826
#define TG_N   6
#define TG_LEN 27
_Static_assert(TRN_OFF+TRN_LEN<=TG_OFF&&TG_OFF+TG_LEN<=SET_OFF,"the goals block must sit between the timed run block and the settings");
static u16 tgKey[TG_N] EWRAM_BSS, tgMask[TG_N] EWRAM_BSS;   // (a copy of the block; tgLoad fills it)
static void tgLoad(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x4E; int ok=(m[0]=='G'&&m[1]=='T');
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    if(ok&&m[TG_LEN-1]!=(u8)sum) ok=0;
    for(int i=0;i<TG_N;i++){ tgKey[i]=ok?(u16)(m[2+i*4]|(m[3+i*4]<<8)):0; tgMask[i]=ok?(u16)(m[4+i*4]|(m[5+i*4]<<8)):0; }
}
static void tgSave(void){
    volatile u8*m=SRAM_BASE+TG_OFF; unsigned sum=0x4E;
    for(int i=0;i<TG_N;i++){ m[2+i*4]=(u8)tgKey[i]; m[3+i*4]=(u8)(tgKey[i]>>8); m[4+i*4]=(u8)tgMask[i]; m[5+i*4]=(u8)(tgMask[i]>>8); }
    for(int i=2;i<TG_LEN-1;i++) sum+=m[i];
    m[TG_LEN-1]=(u8)sum; m[0]='G'; m[1]='T';
}
static u16 tgKeyNow(void){ return nbOk?nbKey(&nbT):0; }
static int tgLotNow(void){ return (nbOk&&nbT.cur<NB_LOTS)?nbT.cur:0; }
static u16 tgMaskOf(u16 key){ for(int i=0;i<TG_N;i++) if(tgMask[i]&&tgKey[i]==key) return tgMask[i]; return 0; }
static int tgFound(void){ tgLoad(); return (tgMaskOf(tgKeyNow())>>tgLotNow())&1; }   // the tape of the lot in play was taken before
static void tgMark(void){   // the tape of the lot in play was just taken
    tgLoad(); u16 key=tgKeyNow(), msk=(u16)(tgMaskOf(key)|(1u<<tgLotNow()));
    int at=TG_N-1; for(int i=0;i<TG_N;i++) if(tgMask[i]&&tgKey[i]==key){ at=i; break; }
    for(int i=at;i>0;i--){ tgKey[i]=tgKey[i-1]; tgMask[i]=tgMask[i-1]; }   // newest first
    tgKey[0]=key; tgMask[0]=msk; tgSave();
}
static int tgBits(unsigned v){ int n=0; while(v){ n+=(int)(v&1); v>>=1; } return n; }
static void goalsScreen(void){
    static char b[12] EWRAM_BSS;
    tgLoad();
    int lots=0, tapes=0; u16 msk=tgMaskOf(tgKeyNow());
    if(nbOk){ for(int i=0;i<NB_LOTS;i++) if(nbT.lot[i].on){ lots++; if(msk>>i&1) tapes++; } }
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
        text(10,124,"THE TAPE STAYS FOUND AND DOES NOT",no,1); text(10,134,"COME BACK ON THAT LOT",no,1);
        text(10,146,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}

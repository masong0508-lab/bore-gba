// timedrun.h - TIMED RUN: a 2:00 trick score attack. PAUSE > WANTS > TIMED RUN starts it: the score goes back to 0 and the clock in the top
// bar becomes a countdown (it turns red and blinks in the last 10 seconds). At 0:00 the run ends, the score is checked against the HIGH SCORE
// and a results card shows both. The countdown counts game steps (60 a second) from lifeStep, so it holds still while a menu, the pause
// menu or a pop-up is open (and during the action cam, which runs no steps).
// SRAM: TRN_OFF, 9 bytes in the free gap between the layout marker (4808..4810) and the settings (SET_OFF):
//       'T' 'R', best score (3 bytes), runs finished (2 bytes), spare, checksum.
// Needs before it: the UI kit (box, menu, toast, text, tw, numText, present, keyNow), simCat / simCatN (sims.h), lscore, simLastScore, lhave / lskate.
// (trnOn, trnLeft, trnDone are declared early in main.c: hud.h and lifeStep use them.)
#define TRN_OFF   4816
#define TRN_LEN   9
#define TRN_STEPS 7200     // 2:00 at 60 game steps a second
_Static_assert(SL_MIG_TAG+3<=TRN_OFF&&TRN_OFF+TRN_LEN<=SET_OFF,"the timed run block must sit between the layout marker and the settings");
static const char* const trnYesNo[2]={"NO","YES  GO"};
static char trnLb[28] EWRAM_BSS;

static int trnBestGet(void){   // the high score (0 = none yet)
    volatile u8*m=SRAM_BASE+TRN_OFF; unsigned sum=0x7B;
    if(m[0]!='T'||m[1]!='R') return 0;
    for(int i=2;i<TRN_LEN-1;i++) sum+=m[i];
    if(m[TRN_LEN-1]!=(u8)sum) return 0;
    return m[2]|(m[3]<<8)|(m[4]<<16);
}
static int trnRunsGet(void){ volatile u8*m=SRAM_BASE+TRN_OFF; return (trnBestGet()||m[0]=='T')?(m[5]|(m[6]<<8)):0; }
static void trnBestPut(int best,int runs){
    volatile u8*m=SRAM_BASE+TRN_OFF; unsigned sum=0x7B;
    if(best<0) best=0; if(best>0xFFFFFF) best=0xFFFFFF; if(runs>0xFFFF) runs=0xFFFF;
    m[2]=(u8)best; m[3]=(u8)(best>>8); m[4]=(u8)(best>>16); m[5]=(u8)runs; m[6]=(u8)(runs>>8); m[7]=0;
    for(int i=2;i<TRN_LEN-1;i++) sum+=m[i];
    m[TRN_LEN-1]=(u8)sum; m[0]='T'; m[1]='R';
}
static void trnStart(void){
    trnOn=1; trnDone=0; trnLeft=TRN_STEPS; trnCombo=0;
    lscore=0; simLastScore=0; lcN=0; lcPts=0; lcT=0; lcBank=0; lcBankT=0;   // a clean score and no old combo
    if(lhave&&!lskate&&stage>=AG_CHILD&&lz<=(surfH(lfx,lfy)<<8)){ lskate=1; lsp=0; lgrind=0; lspin=0; lflip=0; feelReset(lhd); }   // on the board from the first second
    lnote="GO  2:00 ON THE CLOCK"; lnoteT=70;
}
static void trnTick(void){   // from lifeStep: one game step
    if(!trnOn) return;
    if(--trnLeft<=0){ trnLeft=0; trnOn=0; trnDone=1; }
}
static const char* trnLabel(void){   // the WANTS menu entry
    if(trnOn) return "END TIMED RUN";
    char*e=simCat(trnLb,"TIMED RUN  BEST "); simCatN(e,trnBestGet()); return trnLb;
}
static void trnPick(void){   // the entry was chosen
    if(trnOn){ if(menu("END THE RUN?",trnYesNo,2)==1){ trnOn=0; toast("RUN ENDED"); } return; }
    if(stage<AG_CHILD){ toast("TOO YOUNG FOR A RUN"); return; }
    if(menu("START A 2:00 RUN?",trnYesNo,2)==1) trnStart();
}
#define TRN_BRONZE 1000   // medal scores (tune to taste)
#define TRN_SILVER 3000
#define TRN_GOLD   6000
static void trnResult(void){   // the results card (the game holds still behind it)
    int sc=lscore<0?0:lscore, old=trnBestGet(), runs=trnRunsGet()+1, nw=sc>old;
    int md=sc>=TRN_GOLD?3:sc>=TRN_SILVER?2:sc>=TRN_BRONZE?1:0, pay=sc/20;   // medal and prize money (1 per 20 points)
    static const char* const mdNm[4]={"NONE","BRONZE","SILVER","GOLD"};
    static const u16 mdCol[4]={RGB(12,14,16),RGB(24,14,6),RGB(24,26,28),RGB(31,26,6)};
    trnBestPut(nw?sc:old,runs);
    simMoney+=pay; if(simMoney>9999) simMoney=9999;
    u16 prev=keyNow(); u32 t=0; const u16 lab=RGB(22,25,28);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; t++;
        if(t>45&&(pr&(K_A|K_B|K_START))) break;
        box(36,18,168,124);
        rect(37,19,166,17,RGB(5,12,24)); rect(37,35,166,1,GOLD);
        text(120-tw("TIME UP",1)/2,23,"TIME UP",GOLD,1);
        text(48,44,"SCORE",lab,1); numText(130,44,sc,WHITE);
        text(48,57,"BEST COMBO",lab,1); numText(130,57,trnCombo,WHITE);
        text(48,70,"MEDAL",lab,1); text(130,70,mdNm[md],mdCol[md],1);
        text(48,83,"PRIZE",lab,1); numText(130,83,pay,RGB(14,30,14));
        text(48,96,"HIGH SCORE",lab,1); numText(130,96,nw?sc:old,nw?GOLD:WHITE);
        if(nw){ if((t>>3)&1) text(120-tw("NEW HIGH SCORE",1)/2,110,"NEW HIGH SCORE",RGB(14,30,14),1); }
        else if(old>0){ text(48,110,"TO BEAT",lab,1); numText(130,110,old-sc,RGB(30,20,8)); }
        text(48,123,"RUNS",RGB(12,14,16),1); numText(130,123,runs,RGB(12,14,16));
        text(190-tw("A OK",1),123,"A OK",RGB(12,14,16),1);
        present();
    }
}

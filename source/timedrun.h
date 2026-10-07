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
    trnOn=1; trnDone=0; trnLeft=TRN_STEPS;
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
static void trnResult(void){   // the results card (the game holds still behind it)
    int sc=lscore<0?0:lscore, old=trnBestGet(), runs=trnRunsGet()+1, nw=sc>old;
    trnBestPut(nw?sc:old,runs);
    u16 prev=keyNow(); u32 t=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; t++;
        if(t>45&&(pr&(K_A|K_B|K_START))) break;
        box(36,30,168,100);
        rect(37,31,166,17,RGB(5,12,24)); rect(37,47,166,1,GOLD);
        text(120-tw("TIME UP",1)/2,35,"TIME UP",GOLD,1);
        text(48,56,"SCORE",RGB(22,25,28),1); numText(130,56,sc,WHITE);
        text(48,70,"HIGH SCORE",RGB(22,25,28),1); numText(130,70,nw?sc:old,nw?GOLD:WHITE);
        text(48,84,"RUNS",RGB(22,25,28),1); numText(130,84,runs,RGB(22,25,28));
        if(nw){ if((t>>3)&1) text(120-tw("NEW HIGH SCORE",1)/2,100,"NEW HIGH SCORE",RGB(14,30,14),1); }
        else if(old>0){ text(48,100,"TO BEAT",RGB(22,25,28),1); numText(130,100,old-sc,RGB(30,20,8)); }
        text(120-tw("A OK",1)/2,116,"A OK",RGB(12,14,16),1);
        present();
    }
}

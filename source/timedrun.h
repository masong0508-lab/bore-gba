// timedrun.h - A 2:00 skate session: free practice (no salary) or PARK JAM at a skate park (one contest a day).
// The optional contest awards a capped medal prize and a qualification toward the professional team tryout.
// The score goes back to 0 and the clock in the top
// bar becomes a countdown (it turns red and blinks in the last 10 seconds). At 0:00 the run ends, the score is checked against the HIGH SCORE
// and a results card shows both. The countdown counts game steps (60 a second) from lifeStep, so it holds still while a menu, the pause
// menu or a pop-up is open (and during the action cam, which runs no steps).
// SRAM: TRN_OFF, 9 bytes in the free gap between the layout marker (4808..4810) and the settings (SET_OFF):
//       'T' 'R', best score (3 bytes), runs finished (2 bytes), spare (jam medal+day), checksum.
// Needs before it: the UI kit (box, menu, toast, text, tw, numText, present, keyNow), simCat / simCatN (sims.h), lscore, simLastScore, lhave / lskate.
// (trnOn, trnLeft, trnDone are declared early in main.c: hud.h and lifeStep use them.)
#define TRN_OFF   4816
#define TRN_LEN   9
#define TRN_STEPS 7200     // 2:00 at 60 game steps a second
#include "skatejamrules.h"
_Static_assert(SL_MIG_TAG+3<=TRN_OFF&&TRN_OFF+TRN_LEN<=SET_OFF,"the timed run block must sit between the layout marker and the settings");
static const char* const trnYesNo[2]={"NO","YES  GO"};
static const char* const trnChoices[3]={"FREE PRACTICE","PARK JAM  PRIZES","CANCEL"};
static int trnJamVenue(void); // defined in board.h, after neighborhood and venues are available
static char trnLb[28] EWRAM_BSS;

static int trnValid(void){
    volatile u8*m=SRAM_BASE+TRN_OFF; unsigned sum=0x7B;
    if(m[0]!='T'||m[1]!='R') return 0;
    for(int i=2;i<TRN_LEN-1;i++) sum+=m[i];
    return m[TRN_LEN-1]==(u8)sum;
}
static int trnBestGet(void){
    volatile u8*m=SRAM_BASE+TRN_OFF;
    return trnValid()?(m[2]|(m[3]<<8)|(m[4]<<16)):0;
}
static int trnRunsGet(void){
    volatile u8*m=SRAM_BASE+TRN_OFF;
    return trnValid()?(m[5]|(m[6]<<8)):0;
}
static u8 trnMetaGet(void){ return trnValid()?SRAM_BASE[TRN_OFF+7]:0; }
static void trnBestPut(int best,int runs,u8 meta){
    volatile u8*m=SRAM_BASE+TRN_OFF; unsigned sum=0x7B;
    if(best<0) best=0;
    if(best>0xFFFFFF) best=0xFFFFFF;
    if(runs>0xFFFF) runs=0xFFFF;
    m[2]=(u8)best; m[3]=(u8)(best>>8); m[4]=(u8)(best>>16); m[5]=(u8)runs; m[6]=(u8)(runs>>8); m[7]=meta;
    for(int i=2;i<TRN_LEN-1;i++) sum+=m[i];
    m[TRN_LEN-1]=(u8)sum; m[0]='T'; m[1]='R';
}
static void trnStart(int mode){
    trnMode=mode; trnOn=1; trnDone=0; trnLeft=TRN_STEPS; trnCombo=0;
    lscore=0; simLastScore=0; lcN=0; lcPts=0; lcT=0; lcBank=0; lcBankT=0;   // a clean score and no old combo
    if(lhave&&!lskate&&stage>=AG_CHILD&&lz<=(surfH(lfx,lfy)<<8)){ lskate=1; lsp=0; lgrind=0; lspin=0; lflip=0; feelReset(lhd); }   // on the board from the first second
    lnote=trnMode?"PARK JAM  LAND YOUR BEST TRICKS":"FREE PRACTICE  NO JOB REQUIRED"; lnoteT=70;
}
static void trnTick(void){   // from lifeStep: one game step
    if(!trnOn) return;
    if(--trnLeft<=0){ trnLeft=0; trnOn=0; trnDone=1; }
}
static const char* trnLabel(void){
    if(trnOn) return trnMode?"END PARK JAM":"END PRACTICE";
    char*e=simCat(trnLb,"SKATE SESSION  BEST "); simCatN(e,trnBestGet()); return trnLb;
}
static void trnPick(void){
    if(trnOn){ if(menu("END THIS SESSION?",trnYesNo,2)==1){ trnOn=0; toast("SESSION ENDED"); } return; }
    if(stage<AG_CHILD){ toast("TOO YOUNG FOR A RUN"); return; }
    if(ldead||lstun>0||simAct){ toast("FINISH YOUR ACTION FIRST"); return; }
    if(lz>surf8(lfx,lfy)){ toast("LAND BEFORE STARTING"); return; }
    if(!lhave){ toast(bdOwn?"NO SKATING ON THIS LOT":"FIND OR BUY A BOARD FIRST"); return; }
    int choice=menu("SKATE SESSION  2:00",trnChoices,3);
    if(choice<0||choice==2) return;
    if(choice==1){
        if(!trnJamVenue()){ toast("PARK JAM  GO TO A SKATE PARK"); return; }
        if(sjAlreadyToday(trnMetaGet(),(unsigned)simDay)){ toast("PARK JAM DONE FOR TODAY"); return; }
    }
    if(menu(choice?"ENTER PARK JAM?":"START FREE PRACTICE?",trnYesNo,2)==1) trnStart(choice);
}
static const char* trnGoalHint(void){ // ROM literals; no per-frame buffer or UI allocation
    switch(sjMedal(lscore)){
    case 0: return "JAM  BRONZE 1000";
    case 1: return "JAM  SILVER 3000";
    case 2: return "JAM  GOLD 6000";
    default: return "JAM  GOLD  KEEP GOING";
    }
}
static void trnResult(void){   // the results card (the game holds still behind it)
    int sc=lscore<0?0:lscore, old=trnBestGet(), runs=trnRunsGet()+1, nw=sc>old;
    int md=sjMedal(sc), pay=trnMode?sjPrize(md):0;   // free practice pays no salary; park jam has a capped once-daily medal purse
    static const char* const mdNm[4]={"NONE","BRONZE","SILVER","GOLD"};
    static const u16 mdCol[4]={RGB(12,14,16),RGB(24,14,6),RGB(24,26,28),RGB(31,26,6)};
    u8 meta=trnMetaGet(), before=meta;
    if(trnMode){ meta=(u8)sjRecord(meta,(unsigned)simDay,(unsigned)md); }
    trnBestPut(nw?sc:old,runs,meta);
    if(pay) simMoneyAdd(pay);
    if(trnMode&&md>0){
        simEvent(SE_SHOWOFF);
        if(!(before&128u)&&sjQualified(meta,skillLvl,simTricks,simBestCombo))
            simQueue("SKATE TRYOUT OPEN  CHECK THE JOB BOARD");
    }
    u16 prev=keyNow(); u32 t=0; const u16 lab=RGB(22,25,28);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; t++;
        if(t>45&&(pr&(K_A|K_B|K_START))) break;
        box(36,18,168,124);
        rect(37,19,166,17,RGB(5,12,24)); rect(37,35,166,1,GOLD);
        text(120-tw(trnMode?"PARK JAM":"PRACTICE",1)/2,23,trnMode?"PARK JAM":"PRACTICE",GOLD,1);
        text(48,44,"SCORE",lab,1); numText(130,44,sc,WHITE);
        text(48,57,"BEST COMBO",lab,1); numText(130,57,trnCombo,WHITE);
        text(48,70,"MEDAL",lab,1); text(130,70,mdNm[md],mdCol[md],1);
        text(48,83,trnMode?"PRIZE":"HOBBY  FREE",lab,1); numText(130,83,pay,RGB(14,30,14));
        text(48,96,"HIGH SCORE",lab,1); numText(130,96,nw?sc:old,nw?GOLD:WHITE);
        if(nw){ if((t>>3)&1) text(120-tw("NEW HIGH SCORE",1)/2,110,"NEW HIGH SCORE",RGB(14,30,14),1); }
        else if(old>0){ text(48,110,"TO BEAT",lab,1); numText(130,110,old-sc,RGB(30,20,8)); }
        text(48,123,"RUNS",RGB(12,14,16),1); numText(130,123,runs,RGB(12,14,16));
        text(190-tw("A OK",1),123,"A OK",RGB(12,14,16),1);
        present();
    }
}

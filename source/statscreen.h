// statscreen.h - PAUSE > MY SIM > MORE > LIFETIME STATS: hours played, lifetime score and the other totals of lifestats.h.
// Needs: lifestats.h, stBack, text, tw, numText, present, keyNow, simCat / simCatN, house.h (HU_N, HH_REC ...) for the SRAM check below.
_Static_assert(LS_OFF+3+4*LS_N<=SLOT_BASE&&LS_OFF>=SL_HH_OFF+4+2*HH_NM+HH_MAX*HH_REC+2*HU_N*HU_N*4+8,"the lifetime stats block overlaps the household block");
static void statsScreen(void){
    lsEnsure(); static char b[32] EWRAM_BSS; u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(cnt&7){ vsync(); continue; }
        stBack("LIFETIME STATS",(int)cnt);
        const u16 lab=RGB(22,25,28); u32 s=ls[LS_SECS]; char*e;
        text(10,21,"TIME PLAYED",GOLD,1);
        e=simCatN(b,(int)(s/3600)); e=simCat(e," H "); e=simCatN(e,(int)(s/60%60)); e=simCat(e," MIN "); simCatN(e,(int)(s%60)); simCat(b," S");
        text(230-tw(b,1),21,b,WHITE,1);
        text(10,34,"LIFETIME SCORE",GOLD,1); numText(150,34,(int)(ls[LS_SCORE]>0x7FFFFFFF?0x7FFFFFFF:ls[LS_SCORE]),GOLD);
        static const char* const nm[8]={"TRICKS LANDED","COMBOS BANKED","BEST COMBO","BEST TRICK","BAILS","DEATHS","DAYS LIVED","LIVES LIVED"};
        for(int i=0;i<8;i++){
            u32 v=i==0?ls[LS_TRICKS]:i==1?ls[LS_COMBOS]:i==2?ls[LS_BESTCOMBO]:i==3?ls[LS_BESTTRICK]:i==4?ls[LS_BAILS]:i==5?ls[LS_DEATHS]:i==6?ls[LS_DAYS]:(ls[LS_LIVES]?ls[LS_LIVES]:1);
            text(10,48+i*11,nm[i],lab,1); numText(150,48+i*11,(int)(v>0x7FFFFFFF?0x7FFFFFFF:v),WHITE);
        }
        text(10,138,"TOTALS SURVIVE NEW LIVES  ONE SET PER PLAYER",RGB(12,14,16),1);
        text(10,148,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}

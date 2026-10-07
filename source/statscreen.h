// statscreen.h - PAUSE > MY SIM > MORE > LIFETIME STATS: hours played, lifetime score and the other totals of lifestats.h. L / R change the page.
// Needs: lifestats.h, stBack, text, tw, numText, present, keyNow, simCat / simCatN, house.h (HU_N, HH_REC ...) for the SRAM check below.
_Static_assert(LS_OFF+3+4*LS_N<=SLOT_BASE&&LS_OFF>=SL_HH_OFF+4+2*HH_NM+HH_MAX*HH_REC+2*HU_N*HU_N*4+8,"the lifetime stats block overlaps the household block");
static char* lsTime(char*b,u32 s){ char*e=simCatN(b,(int)(s/3600)); e=simCat(e," H "); e=simCatN(e,(int)(s/60%60)); e=simCat(e," MIN "); e=simCatN(e,(int)(s%60)); return simCat(e," S"); }
static int lsI(u32 v){ return (int)(v>0x7FFFFFFF?0x7FFFFFFF:v); }
static void statsScreen(void){
    lsEnsure(); static char b[32] EWRAM_BSS; u16 prev=keyNow(); u32 cnt=0; int pg=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(pr&(K_L|K_R|K_LEFT|K_RIGHT)) pg^=1;
        if(cnt&7){ vsync(); continue; }
        stBack("LIFETIME STATS",(int)cnt);
        const u16 lab=RGB(22,25,28);
        text(10,21,"TIME PLAYED",GOLD,1); lsTime(b,ls[LS_SECS]); text(230-tw(b,1),21,b,WHITE,1);
        if(pg==0){
            text(10,34,"LIFETIME SCORE",GOLD,1); numText(150,34,lsI(ls[LS_SCORE]),GOLD);
            static const char* const nm[8]={"TRICKS LANDED","COMBOS BANKED","BEST COMBO","BEST TRICK","BAILS","DEATHS","DAYS LIVED","LIVES LIVED"};
            for(int i=0;i<8;i++){
                u32 v=i==0?ls[LS_TRICKS]:i==1?ls[LS_COMBOS]:i==2?ls[LS_BESTCOMBO]:i==3?ls[LS_BESTTRICK]:i==4?ls[LS_BAILS]:i==5?ls[LS_DEATHS]:i==6?ls[LS_DAYS]:(ls[LS_LIVES]?ls[LS_LIVES]:1);
                text(10,48+i*11,nm[i],lab,1); numText(150,48+i*11,lsI(v),WHITE);
            }
        } else {
            static const char* const nm[7]={"WANTS MET","PROMOTIONS","JUMPS AND LAUNCHES","RAILS GRINDED","MEALS EATEN","TIME ON THE BOARD","TIME GRINDING"};
            for(int i=0;i<7;i++){
                u32 v=ls[i==0?LS_WANTS:i==1?LS_PROMOS:i==2?LS_AIRS:i==3?LS_GRINDS:i==4?LS_MEALS:i==5?LS_BOARDSECS:LS_GRINDSECS];
                text(10,36+i*12,nm[i],lab,1);
                if(i>=5){ lsTime(b,v); text(230-tw(b,1),36+i*12,b,WHITE,1); } else numText(170,36+i*12,lsI(v),WHITE);
            }
        }
        text(10,128,pg?"PAGE 2 OF 2":"PAGE 1 OF 2",GOLD,1); text(230-tw("L R PAGE",1),128,"L R PAGE",RGB(17,29,31),1);
        text(10,138,"TOTALS SURVIVE NEW LIVES  ONE SET PER PLAYER",RGB(12,14,16),1);
        text(10,148,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}

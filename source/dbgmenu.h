// dbgmenu.h - WEAPONS (module 5: DEBUG MENU). Only with the DEBUG CODE on (title screen: UP UP DOWN DOWN LEFT LEFT RIGHT B A START, dbgOn).
// Open it: PAUSE (START), then SELECT. Every row is one instant cheat; the menu stays open so you can take several. B leaves.
//   GIVE ALL WEAPONS      every weapon, ammo topped up        TAKE ALL WEAPONS    back to nothing (like a prison search)
//   MAX AMMO              bullets and missiles to the limit   RESET STASHES       the secret stashes can be found again
//   MONEY +1000           (up to 9999)                        FULL HEALTH + NEEDS you and every Sim in the house
//   CLEAR WANTED          cops forget you, the record is wiped    FREE FROM PRISON   ends the sentence on the spot
static void dbgMenu(void){
    static const char* const it[9]={"GIVE ALL WEAPONS","MAX AMMO","TAKE ALL WEAPONS","RESET STASHES","MONEY +1000","FULL HEALTH + NEEDS","CLEAR WANTED","FREE FROM PRISON","BACK"};
    for(;;){
        int c=menu("DEBUG",it,9); if(c<0||c==8) return;
        wpEnsure();
        if(c==0){ for(int w=0;w<WP_N;w++) wpOwn|=(u8)(1<<w); wpBul=WP_BMAX; wpMis=WP_MMAX; wpSave(); toast("ALL WEAPONS"); }
        else if(c==1){ wpBul=WP_BMAX; wpMis=WP_MMAX; wpSave(); toast("AMMO FULL"); }
        else if(c==2){ wpConfiscate(); toast("WEAPONS GONE"); }
        else if(c==3){ wpSecr=0; wpSave(); toast("STASHES ARE BACK"); }
        else if(c==4){ simMoney+=1000; if(simMoney>9999) simMoney=9999; simsSaveNow(); toast("+1000"); }
        else if(c==5){ lhp=HP_MAX; lfood=100; lbl=0; sNrg=100; sHyg=100; sCom=100; for(int m=0;m<hhN;m++){ hhM[m].hp=100; for(int n=0;n<HN_N;n++) hhM[m].need[n]=100; } toast("ALL FIT AND WELL"); }
        else if(c==6){ copHeat=0; copWant=0; copCool=3600; prRec=0; toast("CLEAN RECORD"); }
        else if(c==7){ if(prShown()){ prRelease(); toast("SENTENCE ENDED"); } else toast("NOT IN PRISON"); }
    }
}

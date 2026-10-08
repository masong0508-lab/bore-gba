// dbgmenu.h - WEAPONS (module 5: DEBUG MENU). Only with the DEBUG CODE on (title screen: UP UP DOWN DOWN LEFT LEFT RIGHT B A START, dbgOn).
// Open it: PAUSE (START), then SELECT. Every row is one instant cheat; the menu stays open so you can take several. B leaves.
//   GIVE ALL WEAPONS      every weapon, ammo topped up        TAKE ALL WEAPONS    back to nothing (like a prison search)
//   MAX AMMO              bullets and missiles to the limit   RESET STASHES       the secret stashes can be found again
//   MONEY +100000         (up to 999,999,999,999)                     FULL HEALTH + NEEDS you and every Sim in the house
//   CLEAR WANTED          cops forget you, the record is wiped    FREE FROM PRISON   ends the sentence on the spot
static void dbgMenu(void){
    static const char* const it[11]={"GIVE ALL WEAPONS","MAX AMMO","TAKE ALL WEAPONS","RESET STASHES","MONEY +100000","FULL HEALTH + NEEDS","CLEAR WANTED","FREE FROM PRISON","STORY NEXT CHAPTER","PLAY CUTSCENE","BACK"};
    for(;;){
        int c=menu("DEBUG",it,11); if(c<0||c==10) return;
        wpEnsure();
        if(c==0){ for(int w=0;w<WP_N;w++) wpOwn|=(u8)(1<<w); wpBul=WP_BMAX; wpMis=WP_MMAX; wpSave(); toast("ALL WEAPONS"); }
        else if(c==1){ wpBul=WP_BMAX; wpMis=WP_MMAX; wpSave(); toast("AMMO FULL"); }
        else if(c==2){ wpConfiscate(); toast("WEAPONS GONE"); }
        else if(c==3){ wpSecr=0; wpSave(); toast("STASHES ARE BACK"); }
        else if(c==4){ simMoneyAdd(100000); simsSaveNow(); toast("+100000"); }
        else if(c==5){ lhp=HP_MAX; lfood=100; lbl=0; sNrg=100; sHyg=100; sCom=100; for(int m=0;m<hhN;m++){ hhM[m].hp=100; for(int n=0;n<HN_N;n++) hhM[m].need[n]=100; } toast("ALL FIT AND WELL"); }
        else if(c==6){ copHeat=0; copWant=0; copCool=3600; prRec=0; toast("CLEAN RECORD"); }
        else if(c==7){ if(prShown()){ prRelease(); toast("SENTENCE ENDED"); } else toast("NOT IN PRISON"); }
        else if(c==8){ if(stId&&stCh<stLen[stId]&&stChs[stId][stCh].goal!=SG_END){ stComplete(); toast("CHAPTER DONE"); } else toast("NO CHAPTER TO FINISH"); }   // (story.h: pays it and starts the next chapter, to test a story)
        else if(c==9){ int s=menu("CUTSCENE",csNames,6); if(s>=0){ csPlay(s); rect(0,0,SW,SH,RGB(2,4,8)); } }   // (cutscene.h: preview any scene)
    }
}

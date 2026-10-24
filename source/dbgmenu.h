// dbgmenu.h - WEAPONS (module 5: DEBUG MENU). Only with the DEBUG CODE on (title screen: UP UP DOWN DOWN LEFT LEFT RIGHT B A START, dbgOn).
// Open it: PAUSE (START), then SELECT. Every row is one instant cheat; the menu stays open so you can take several. B leaves.
//   GIVE ALL WEAPONS      every weapon, ammo topped up        TAKE ALL WEAPONS    back to nothing (like a prison search)
//   MAX AMMO              bullets and missiles to the limit   RESET STASHES       the secret stashes can be found again
//   MONEY +100000         (up to 999,999,999,999)                     FULL HEALTH + NEEDS you and every Sim in the house
//   SOUND TEST            every song, sound effect and voice clip, played by picking it (below)
//   CLEAR WANTED          cops forget you, the record is wiped    FREE FROM PRISON   ends the sentence on the spot
static void csMenu(void){   // OPTIONS > SIM > CHEAT > PLAY CUTSCENES: pick one scene, or play all eight in story order
    static const u8 ord[8]={7,0,1,2,3,4,5,6};
    const char* it[9]; it[0]="PLAY ALL IN STORY ORDER"; for(int i=0;i<8;i++) it[i+1]=csNames[i];
    for(;;){ int s=menu("CUTSCENES",it,9); if(s<0) return;
        if(s==0){ for(int i=0;i<8;i++) csPlay(ord[i]); } else csPlay(s-1);
        rect(0,0,SW,SH,RGB(2,4,8)); }
}
#include "voicenames.h"   // GENERATED (tools/encode_voices.py): the voice clips' names
// ---- SOUND TEST (debug menu): every song, every sound effect and every voice clip, played by picking it. A plays, B goes back; the cursor stays where it was. ----
//   MUSIC: all the songs in songs.h, the secret and locked ones too (what the jukebox hides). The song replaces whatever plays; STOP SOUND ends it.
//   SOUND EFFECTS: SFX_BONK .. SFX_KO and the TOOT.   VOICES: the 54 clips; the top row picks the pitch they are said at (100 is the recording, 133 a female Sim, ...).
static const char* const sdSfxN[]={"BONK","HIT","GASP","SCREAM","CRY","GROAN","NEARLY","DEATH","INSTANT","TICK","POP","LAND","STICK","GRIND","THUNDER","GHOST","BELL","DIAL","FLUSH","SHOWER","MUNCH","CASH","PAGE","BREW","TV","SPLASH","BOOT","YELP","KO","TOOT"};
_Static_assert(sizeof(sdSfxN)/sizeof(sdSfxN[0])==SFX_VOICE0+1,"dbgmenu.h: sdSfxN must name every effect in front of SFX_VOICE0, then the TOOT");
static const char* const sdVoiceN[]={ VOICE_NAME_TAB };   // (unsized: the assert below counts the names)
_Static_assert(sizeof(sdVoiceN)/sizeof(sdVoiceN[0])==VOICE_N,"voicenames.h: run tools/encode_voices.py");
static void sdSong(int i){ if(!sSnd){ toast("SOUND IS OFF"); return; } const Song*sg=&songs[i]; menuOn=0; creOn=0; musBegin(sg->adp?1:0,sg->adp,sg->xm); }   // (as jbStart does)
static void sdMusic(void){ const char* it[NSONGS]; for(int i=0;i<NSONGS;i++) it[i]=songs[i].name; int at=0;
    menuQuiet=1; for(;;){ menuInit=at; int s=menu("MUSIC TEST",it,NSONGS); if(s<0) break; at=s; sdSong(s); } menuQuiet=0; }
static void sdEffects(void){ const int n=SFX_VOICE0+1; int at=0;
    menuQuiet=1; for(;;){ menuInit=at; int s=menu("EFFECTS TEST",sdSfxN,n); if(s<0) break; at=s; if(sSnd) sfxPlay(s<SFX_VOICE0?s:SFX_TOOT); else toast("SOUND IS OFF"); } menuQuiet=0; }
static void sdVoices(void){
    static const struct { const char*n; int p; } pit[5]={{"AS RECORDED  100",100},{"FEMALE  133",133},{"NON BINARY  116",116},{"BABY  130",130},{"ELDER  94",94}};
    const char* it[VOICE_N+1]; char row0[28]; int pi=0, at=0;
    for(int i=0;i<VOICE_N;i++) it[i+1]=sdVoiceN[i];
    menuQuiet=1;
    for(;;){ char*e=row0; for(const char*s="PITCH  ";*s;) *e++=*s++; for(const char*s=pit[pi].n;*s;) *e++=*s++; *e=0; it[0]=row0;
        menuInit=at; int s=menu("VOICE TEST",it,VOICE_N+1); if(s<0) break; at=s;
        if(s==0){ pi=(pi+1)%5; continue; }
        if(sSnd) voxSayP(hhPUid,s-1,pit[pi].p); else toast("SOUND IS OFF"); }
    menuQuiet=0; }
static void sdMenu(void){
    static const char* const it[5]={"MUSIC","SOUND EFFECTS","VOICES","STOP SOUND","BACK"}; int at=0;
    for(;;){ menuInit=at; int c=menu("SOUND TEST",it,5); if(c<0||c==4) return; at=c;
        if(c==0) sdMusic(); else if(c==1) sdEffects(); else if(c==2) sdVoices(); else { sfxStop(); musStop(); toast("SOUND STOPPED"); } }
}
static void dbgMenu(void){
    static const char* const it[12]={"GIVE ALL WEAPONS","MAX AMMO","TAKE ALL WEAPONS","RESET STASHES","MONEY +100000","FULL HEALTH + NEEDS","CLEAR WANTED","FREE FROM PRISON","STORY NEXT CHAPTER","PLAY CUTSCENE","SOUND TEST","BACK"};
    for(;;){
        int c=menu("DEBUG",it,12); if(c<0||c==11) return;
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
        else if(c==9){ int s=menu("CUTSCENE",csNames,8); if(s>=0){ csPlay(s); rect(0,0,SW,SH,RGB(2,4,8)); } }   // (cutscene.h: preview any scene)
        else if(c==10) sdMenu();   // (the sound test, above)
    }
}

// skills.h - SKILLS, the Sims way, and the HOME PACK items (TV, bookshelf, coffee maker, aquarium, treadmill).
//
//  Two groups of skills, each level 0..5:
//   LIFE SKILLS     what you do at home: COOKING, LOGIC, BODY, CHARISMA, CREATIVITY. Trained with the furniture and the household.
//   SKATER SKILLS   what you do on the board: SKATING (the old skill: skillPts in sims.h, it still drives jobs, wants and the trick bonus),
//                   GRINDING, AIR TRICKS and BALANCE. Trained by landing tricks.
//  Every skill has an effect (see skFx). A level needs skAt[] points; points stop at 250.
//  SAVING: SK_LEN bytes at SK_OFF (5014..5023, the spare end of the old life-stage block's 16 reserved bytes): 'K', one byte per skill, checksum.
//          A player save file carries them in a 'K' chunk (savegame.h), so every player keeps their own. An old save has no block: all zero.
//  RAM:    skPts is 8 bytes and the home pack's counters a few more, all EWRAM_BSS. No IWRAM code, no IWRAM statics.
//
//  HOME PACK (the item chars are placed with the ITEM tool; sprites in homeart.h)
//   TV 'v'          R: next channel. SKATE VIDEO (+1 skating skill point), COMEDY NIGHT, THE NEWS (logic), HORROR FLICK (a scare: the bladder fills).
//                   Comfort rises, twice as much with a sofa or beanbag within three tiles (COUCH BONUS). Watching at night costs energy.
//   BOOKSHELF 'b'   R: read. Three reads a day train LOGIC (3, 2, then 1 point); a fresh coffee adds one (STUDY BUZZ).
//   COFFEE MAKER 'c' R: energy +25, the bladder fills a little, then you are WIRED (10 % faster) for 40 s and CRASH after it. Three cups a day.
//   AQUARIUM 'q'    R: feed the fish (once a day; the feeding is not saved). Fed fish calm you: comfort creeps up within two tiles.
//   TREADMILL 'm'   R: a run. Costs food, energy and hygiene; trains BODY and heals a little. Three runs a day.
//  Needs before it: sims.h, house.h (hhM, hhPUid), story.h (stBack, s2rr, s2grad, s2pill), career.h, the UI kit.
// (the SK_ numbers and SK_LIFE are in sims.h: the life loop and house.h use them before this file)
#define SK_OFF 5014
#define SK_LEN (SK_N+2)
_Static_assert(AGE_OFF+5<=SK_OFF&&SK_OFF+SK_LEN<=PERS_OFF,"the skills block overlaps the life stage or the persona block");
static u8 skPts[SK_N] EWRAM_BSS;
static char skMsg[24] EWRAM_BSS;
static const short skAt[5]={5,15,30,55,90};
static const char* const skNm[SK_N]={"COOKING","LOGIC","BODY","CHARISMA","CREATIVITY","GRINDING","AIR TRICKS","BALANCE"};
static const char* const skFx[SK_N+1]={
    "FRIDGE MEALS FILL 4 MORE A LEVEL", "BILLS COST 5 PERCENT LESS A LEVEL", "FASTER ON FOOT AND ON BOARD  2 PERCENT",
    "TALKS FILL SOCIAL 10 PERCENT MORE", "TRICKS SCORE 2 PERCENT MORE A LEVEL", "A GRIND SCORES MORE EVERY 2 LEVELS",
    "SPINS AND FLIPS SCORE 4 PERCENT MORE", "BAILS HURT 6 PERCENT LESS A LEVEL", "TRICKS SCORE 8 PERCENT MORE A LEVEL" };
static const char* const skHow[SK_N+1]={
    "TRAIN  EAT AT THE FRIDGE", "TRAIN  READ  WATCH THE NEWS", "TRAIN  RUN ON THE TREADMILL", "TRAIN  TALK TO THE HOUSEHOLD",
    "TRAIN  MUSIC AND TV", "TRAIN  LAND ON RAILS", "TRAIN  LAND SPINS AND FLIPS", "TRAIN  LAND CLEAN AND PERFECT", "TRAIN  LAND TRICKS AND COMBOS" };

static int skLvl(int k){ int l=0; for(int i=0;i<5;i++) if(skPts[k]>=skAt[i]) l=i+1; return l; }
static void skGain(int k,int n){
    if(n<=0) return;
    int o=skLvl(k), v=skPts[k]+n; if(v>250) v=250; skPts[k]=(u8)v;
    if(skLvl(k)>o){ simCat(simCat(skMsg,skNm[k])," UP"); simQueue(skMsg); moodEvent(M_SKILL); simEvent(SE_SKILL); }
}
static void skReset(void){ for(int i=0;i<SK_N;i++) skPts[i]=0; }
static void skSave(void){
    volatile u8*m=SRAM_BASE+SK_OFF; u8 sum=0x4B; m[0]='K';
    for(int i=0;i<SK_N;i++){ m[1+i]=skPts[i]; sum=(u8)(sum+skPts[i]); }
    m[SK_LEN-1]=sum;
}
static void skLoad(void){
    volatile u8*m=SRAM_BASE+SK_OFF; u8 sum=0x4B;
    if(m[0]!='K'){ skReset(); return; }
    for(int i=0;i<SK_N;i++) sum=(u8)(sum+m[1+i]);
    if(m[SK_LEN-1]!=sum){ skReset(); return; }
    for(int i=0;i<SK_N;i++) skPts[i]=m[1+i];
}
// ---- the home pack's small state: a few bytes, not saved (a new day or a power cycle starts them fresh) ----
static u16 hmCoffee EWRAM_BSS;    // frames of coffee buzz left
static u16 hmFishDay EWRAM_BSS;   // simDay+1 of the last feeding (0 = never)
static u16 hmDay EWRAM_BSS;       // simDay+1 the daily counters belong to
static u8 hmBook EWRAM_BSS, hmRuns EWRAM_BSS, hmCups EWRAM_BSS, hmMus EWRAM_BSS, hmTv EWRAM_BSS;   // reads, runs, cups, radio tunings today; the next TV channel
static char hmMsg[32] EWRAM_BSS;
static int skTop(int top){   // sims.h simsTop: BODY and the coffee buzz
    top+=top*skLvl(SK_BODY)*2/100; if(hmCoffee>0) top+=top/10; return top;
}
static int hmNear(int r,char c){   // is there an item c within r tiles of you
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int y=py-r;y<=py+r;y++)for(int x=px-r;x<=px+r;x++) if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]==c) return 1;
    return 0;
}
static void hmNeed(int*v,int d){ *v+=d; if(*v>100) *v=100; if(*v<0) *v=0; }
static int hmFishHungry(void){ return hmFishDay!=(u16)(simDay+1); }   // item module 13: house.h asks, and a Sim at the aquarium feeds them
static void hmFishFed(void){ hmFishDay=(u16)(simDay+1); }
static void homeTick(void){   // every step of the life
    if(hmDay!=(u16)(simDay+1)){ hmDay=(u16)(simDay+1); hmBook=0; hmRuns=0; hmCups=0; hmMus=0; }
    if(hmCoffee>0&&--hmCoffee==0){ hmNeed(&sNrg,-12); lnote="CAFFEINE CRASH"; lnoteT=60; }
    if((lfr&31)==0&&hmFishDay&&(int)(simDay+1)-(int)hmFishDay>=0&&(int)(simDay+1)-(int)hmFishDay<=1&&sCom<100&&hmNear(2,'q')) sCom++;   // fed fish calm you
}
static void homeMusic(void){ if(hmMus<4){ hmMus++; skGain(SK_CREAT,1); } }   // tuning the radio or the sound system: four a day count
static void homeUse(int k){   // R at a home pack item: lnear 11 TV, 12 bookshelf, 13 coffee maker, 14 aquarium, 15 treadmill
    if(stage==AG_BABY){ lnote="TOO YOUNG"; lnoteT=40; return; }
    lsp=0; lgrind=0;
    htUse(k);   /* hardtime.h: agility in the prison gym */
    if(k==16){ prClimb(); return; }   // prison.h: the sealed fence
    if(k==11){
        static const char* const tvN[5]={"MACHINI-TV","SKATE VIDEO","COMEDY NIGHT","THE NEWS","HORROR FLICK"};   // channel 0 plays a 3 second clip (tvclip.h)
        int ch=hmTv%5, sofa=hmNear(3,'C')||hmNear(3,'U'); hmTv=(u8)((hmTv+1)%5);
        lstun=90; hmNeed(&sCom,sofa?30:12);
        if(ch==0){ tvClip=1; simEvent(SE_LAUGH); moodEvent(M_SOFA); }
        else if(ch==1){ simSkillAdd(1); skGain(SK_CREAT,1); }
        else if(ch==2){ simEvent(SE_LAUGH); voxPlay(V_laughing); moodEvent(M_SOFA); }
        else if(ch==3){ skGain(SK_LOGIC,1); }
        else { sfxPlay(SFX_GASP); hmNeed(&lbl,15); moodEvent(M_SPOOK); }
        if(simMin>=SIM_NIGHT_FROM||simMin<SIM_NIGHT_TO) hmNeed(&sNrg,-8);
        simEvent(SE_TV);
        simCat(simCat(hmMsg,tvN[ch]),sofa?"  COUCH BONUS":""); lnote=hmMsg; lnoteT=70;
    } else if(k==12){
        if(hmBook>=3){ lnote="NO MORE BOOKS TODAY"; lnoteT=50; return; }
        int g=3-hmBook; if(hmCoffee>0) g++; if(prIn()) g++;   // the prison library counts extra
        lstun=100; hmNeed(&sNrg,-3); hmNeed(&sCom,6); skGain(SK_LOGIC,g); hmBook++;
        simEvent(SE_READ); lnote=hmCoffee>0?"READ A BOOK  STUDY BUZZ":"READ A BOOK"; lnoteT=60;
    } else if(k==13){
        if(hmCups>=3){ lnote="TOO JITTERY FOR MORE"; lnoteT=50; return; }
        hmCups++; hmCoffee=2400; lstun=40; hmNeed(&sNrg,25); hmNeed(&lbl,12); if(lbl>99) lbl=99;
        moodEvent(M_SOFA); lnote="COFFEE  WIRED"; lnoteT=60;
    } else if(k==14){
        if(hmFishDay==(u16)(simDay+1)){ lnote="THE FISH ARE FULL"; lnoteT=50; return; }
        hmFishDay=(u16)(simDay+1); lstun=40; hmNeed(&sCom,8); moodEvent(M_SOFA); simEvent(SE_FISH); lnote="FED THE FISH  CALM"; lnoteT=60;
    } else {
        if(lfood<20){ lnote="TOO HUNGRY TO RUN"; lnoteT=50; return; }
        if(sNrg<25){ lnote="TOO TIRED TO RUN"; lnoteT=50; return; }
        if(hmRuns>=3){ lnote="JELLY LEGS  TRY TOMORROW"; lnoteT=60; return; }
        hmRuns++; lstun=150; lfood-=12; hmNeed(&sHyg,-20); hmNeed(&sNrg,-15); lbl+=5; if(lbl>99) lbl=99;
        skGain(SK_BODY,prIn()?3:2); hpHeal(8); moodEvent(M_SOFA); simEvent(SE_RUN); lnote="WORKED OUT"; lnoteT=60;
    }
}
// ---- the SKILLS screen (pause menu > WANTS > SKILLS) ----
static void skRow(int y,int sel,const char*nm,int pts,int lvl,const short*at){
    if(sel) rect(6,y-1,228,10,RGB(6,16,8));
    text(10,y,nm,sel?WHITE:RGB(20,26,30),1);
    for(int i=0;i<5;i++) rect(88+i*9,y+1,7,5,i<lvl?GOLD:RGB(7,14,22));
    if(lvl>=5) text(142,y,"MAX",RGB(10,28,12),1);
    else { int lo=lvl?at[lvl-1]:0, hi=at[lvl], w=(pts-lo)*40/(hi-lo); rect(142,y+2,42,4,RGB(3,5,9)); if(w>0) rect(143,y+3,w,2,RGB(14,26,31));
           numText(numText(190,y,pts,RGB(24,27,30))+2,y,hi,RGB(12,18,24)); }
}
static void skillsScreen(void){
    static const u8 ord[SK_N+1]={SK_COOK,SK_LOGIC,SK_BODY,SK_CHARM,SK_CREAT,SK_N,SK_GRIND,SK_AIR,SK_BAL};   // top to bottom; SK_N stands for SKATING
    int sel=0; u16 prev=keyNow(); u32 cnt=0, lt=~0u; int lastSel=-1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(pr&K_DOWN) sel=(sel+1)%(SK_N+1);
        if(pr&K_UP) sel=(sel+SK_N)%(SK_N+1);
        if(sel==lastSel&&(cnt>>3)==lt){ vsync(); continue; }
        lastSel=sel; lt=cnt>>3;
        stBack("SKILLS",(int)cnt);
        text(10,21,"LIFE SKILLS",GOLD,1);
        text(10,83,"SKATER SKILLS",GOLD,1);
        for(int r=0;r<=SK_N;r++){ int id=ord[r], y=r<SK_LIFE?31+r*10:93+(r-SK_LIFE)*10;
            if(id==SK_N) skRow(y,r==sel,"SKATING",skillPts,skillLvl,simSkillAt);
            else skRow(y,r==sel,skNm[id],skPts[id],skLvl(id),skAt); }
        rect(8,133,224,1,RGB(14,26,31));
        text(10,137,skFx[ord[sel]],WHITE,1); text(10,146,skHow[ord[sel]],RGB(17,29,31),1);
        present();
    }
}

// pause menu > WANTS > VIEW TRICKS: the trick controls on one page (text only; the same moves the tutorial teaches)
static void tricksScreen(void){
    static const char* const ln[]={"L  STEP ON OR OFF THE BOARD","A  PUSH  (ON FOOT: HOP)","DPAD  STEER  (IN THE AIR: SPIN)","B  OLLIE  (IN THE AIR: KICKFLIP)","RAILS  LAND ON ONE TO GRIND","LAND ON GREEN  RED IS A BAIL","R ON A MANUAL PAD  MANUAL","R INTO A WALL  WALL TAP","TOUCH S K A T E  FIVE = BONUS","FIND THE TAPE  +1000"};
    u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START|K_A)) return;
        if(cnt&7){ vsync(); continue; }
        stBack("TRICKS",(int)cnt);
        text(10,21,"SKATE CONTROLS",GOLD,1);
        for(int i=0;i<10;i++) text(10,34+i*10,ln[i],i<5?WHITE:i<8?RGB(17,29,31):GOLD,1);
        text(10,142,"A OR B BACK",RGB(12,14,16),1);
        present();
    }
}

// dojo.h - THE DOJO CLASS: the dojo is a school now. The sensei's counter (R) has three more things beside KATA, MEDITATE, CALL OUT A CHALLENGER and TEACH A CLASS:
//   ATTEND CLASS      three classes a day (9 AM, 1 PM, 6 PM; each runs two hours). Each weekday teaches one thing out loud: the guard and the parry, the combo, kicks, the
//                     FIREBALL, the UPPERCUT, the SPIN KICK (the lesson card spells the pad motion out), and Sunday is meditation. A class costs 10, tires you, and trains BODY;
//                     coming day after day makes you a regular (a streak, more BODY points each class). Nothing here is saved: the belt is your BODY skill, which is.
//   DRILL THE BAG     any time you have the energy: a little BODY and a charge of the special meter.
//   BELT AND SCHEDULE your belt (WHITE, YELLOW, GREEN, BLUE, BROWN, BLACK = BODY level 0..5), points to the next one, today's lesson and the next class.
//   A higher belt makes the special moves cheaper (fight.h fgSpecial: 6 percent less meter a belt, 30 percent at BLACK).
static const char* const djBelt[6]={"WHITE","YELLOW","GREEN","BLUE","BROWN","BLACK"};
static short djKey=-1, djLast=-1; static u8 djStreak;   // the class you were in (day x 3 + which), the day of your last class, the days in a row (not saved)
static const u8 djStart[3]={9,13,18};
static const char* const djAt[3]={"9 AM","1 PM","6 PM"};
static int djSess(void){ for(int i=0;i<3;i++) if(simMin>=djStart[i]*60&&simMin<(djStart[i]+2)*60) return i; return -1; }
static const char* djNext(void){ for(int i=0;i<3;i++) if(simMin<djStart[i]*60) return djAt[i]; return "9 AM TOMORROW"; }
static const char* const djDay[7]={"STANCE AND GUARD","THE COMBO","KICKS","THE FIREBALL","THE UPPERCUT","THE SPIN KICK","MEDITATION"};
static int djBeltNo(void){ int l=skLvl(SK_BODY); return l>5?5:l; }
static int djCost(int cost){ int b=djBeltNo(); return cost*(100-(b>=5?30:b*6))/100; }   // fight.h fgSpecial
static void djLesson(int d){   // the lesson card: the text is the real controls
    static const char* const L[7][6]={
        {"HOLD L TO GUARD","A BLOW FROM THE FRONT","HITS AT A QUARTER","GUARD AS IT LANDS: A PARRY","NO DAMAGE  THEY REEL","YOUR NEXT HIT HITS HARDER"},
        {"L+R TAKES THE STANCE","THEN R  JAB CROSS HOOK","KEEP THE BEAT AND THE","COMBO GROWS  X2  X3","A MISS DROPS THE CHAIN",0},
        {"IN THE STANCE","PRESS A DIRECTION  THEN R","FOR A KICK THAT REACHES","FURTHER THAN A PUNCH","BEST AGAINST A GUARD",0},
        {"IN THE STANCE","DOWN  DOWN-RIGHT  RIGHT","THEN R","THE FIREBALL FLIES THE WAY","YOU AIM  COSTS METER",0},
        {"IN THE STANCE","RIGHT  DOWN  DOWN-RIGHT","THEN R","THE UPPERCUT LIFTS THE FOE","COSTS THE MOST METER",0},
        {"IN THE STANCE","RIGHT  LEFT  RIGHT","THEN R","THE SPIN KICK HITS ALL","AROUND YOU",0},
        {"SIT STILL","BREATHE IN  BREATHE OUT","THE BODY RESTS","AND THE MIND CLEARS",0,0}};
    const char* it[6]; int n=0; for(int i=0;i<6;i++) if(L[d][i]) it[n++]=L[d][i];
    menu(djDay[d],it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
}
static int djCanDo(int sp){   // 1 = go ahead (venueDo asks before it takes the money)
    if(sp==VS_CLASS){
        int s=djSess(); if(s<0){ static char m[40] EWRAM_BSS; simCat(simCat(m,"NEXT CLASS  "),djNext()); lnote=m; lnoteT=90; voxPlay(V_nah); return 0; }
        if(djKey==(short)(simDay*3+s)){ lnote="YOU ARE ALREADY IN THIS CLASS"; lnoteT=70; return 0; }
        if(sNrg<15){ lnote="TOO TIRED FOR CLASS"; lnoteT=60; voxPlay(V_nah); return 0; } }
    if(sp==VS_BAG&&sNrg<10){ lnote="TOO TIRED TO DRILL"; lnoteT=60; voxPlay(V_nah); return 0; }
    return 1;
}
static void djDo(int sp){
    if(sp==VS_CLASS){
        int s=djSess(), d=simDay%7; djKey=(short)(simDay*3+s);
        if(djLast==simDay-1) djStreak=(u8)(djStreak<250?djStreak+1:djStreak); else if(djLast!=simDay) djStreak=1;
        djLast=(short)simDay;
        vAdd(&sNrg,-12); vAdd(&sHyg,-8);
        if(d==6){ vAdd(&sCom,10); skGain(SK_LOGIC,1); moodEvent(M_CHILL); }
        skGain(SK_BODY,2+(djStreak>8?4:djStreak/2));
        lstun=240; lsp=0; lgrind=0; djLesson(d);
        static char m[40] EWRAM_BSS; char*e=simCat(m,djStreak>1?"A REGULAR  DAY ":"CLASS OVER  DAY "); e=simCatN(e,djStreak); lnote=m; lnoteT=110; }
    else if(sp==VS_BAG){
        vAdd(&sNrg,-8); vAdd(&sHyg,-5); skGain(SK_BODY,1); lspec=lspec+60>SPEC_MAX?SPEC_MAX:lspec+60; moodEvent(M_WANT);
        lstun=120; lsp=0; lgrind=0; lnote="THWACK  THE SPECIAL METER CHARGES"; lnoteT=90; }
    else if(sp==VS_BELT){
        int b=djBeltNo(); static char t[32] EWRAM_BSS, l0[32] EWRAM_BSS, l1[32] EWRAM_BSS, l2[32] EWRAM_BSS, l3[32] EWRAM_BSS;
        simCat(simCat(t,"BELT  "),djBelt[b]);
        simCat(simCat(l0,"TODAY  "),djDay[simDay%7]);
        simCat(simCat(l1,"NEXT CLASS  "),djNext());
        { char*e=simCat(l2,"STREAK  "); e=simCatN(e,djStreak); simCat(e," DAYS"); }
        if(b>=5) simCat(l3,"A BLACK BELT  SPECIALS COST 30 LESS");
        else { char*e=simCat(l3,"TO "); e=simCat(e,djBelt[b+1]); e=simCat(e,"  "); e=simCatN(e,skAt[b]>skPts[SK_BODY]?skAt[b]-skPts[SK_BODY]:0); simCat(e," BODY POINTS"); }
        const char* it[4]={l0,l1,l2,l3}; menu(t,it,4); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
}

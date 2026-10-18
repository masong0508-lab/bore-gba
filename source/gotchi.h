// gotchi.h - BOREAGOTCHI!'s MODE: THE HANDHELD. A keychain egg with a little LCD, and on it a pixel blob that needs you, constantly.
//   THE BLOB  hatches from an egg, then grows: BABY, CHILD, TEEN, ADULT. Who it grows into depends on how you look after it: few CARE MISTAKES
//             and some discipline make a SUPER BORE, too many snacks a CHILL BORE (half-shut eyes, very relaxed), neglect a SLOB BORE.
//   NEEDS     HUNGER and HAPPY (four hearts each), DISCIPLINE, WEIGHT. It poops, it gets sick when the poop sits, it sleeps at night (turn the light
//             off). When it needs something the ATTENTION icon lights up; leave it lit too long and that is a CARE MISTAKE. Sometimes it calls for
//             nothing at all: that is a test, and SCOLD is the answer. Neglect it badly and it goes back to its home planet (a new egg is waiting).
//   ICONS     LEFT RIGHT pick, A uses: FEED (a MEAL or a SNACK), LIGHT, PLAY (WHICH WAY? guess where it looks, 5 rounds), MEDICINE, CLEAN, STATS,
//             SCOLD, and the ATTENTION light. B leaves.
//   TIME      runs while you hold it (a tick every half second) and while you live your life (a tick every game minute), so it keeps you company.
//             While you play, a hungry or messy blob BEEPS at you now and then.
// SAVED at GO_OFF (20 bytes, and the player save's chunk 'H'). Missions (expand.h): goStage, and the GA_ events through stQEvent.
#define GO_LEN 20
_Static_assert(GO_OFF+GO_LEN<=BD_OFF,"the handheld's block runs into the board block");
enum { GS_EGG, GS_BABY, GS_CHILD, GS_TEEN, GS_ADULT };
enum { GF_SICK=1, GF_SLEEP=2, GF_DARK=4, GF_CALL=8, GF_FAKE=16, GF_ON=32 };
typedef struct { u8 stage, form; u16 age; u8 hun, hap, disc, wt, poop, fl, miss, needT, snk, meal, nm, sub; } Gotchi;
static Gotchi go EWRAM_BSS;
static const char* const goNames[8]={"BLOBERT","SNORLOCK","MEH-CHAN","DOZY","SIR SIGH","YAWNIE","PUDDLE","KEVIN"};
static const char* const goStNm[5]={"EGG","BABY","CHILD","TEEN","ADULT"};
static const char* const goFormNm[5][3]={{"EGG","",""},{"BLOBBY","",""},{"BORELET","",""},{"ANGSTY BORE","SLOUCHY BORE",""},{"SUPER BORE","SLOB BORE","CHILL BORE"}};
static u8 goSum(volatile const u8*m){ u8 s=0x47; for(int i=2;i<GO_LEN-1;i++) s=(u8)(s*5+m[i]); return s; }
static void goPack(volatile u8*m){ m[0]='G'; m[1]='O'; m[2]=go.stage; m[3]=go.form; m[4]=(u8)go.age; m[5]=(u8)(go.age>>8); m[6]=go.hun; m[7]=go.hap; m[8]=go.disc; m[9]=go.wt;
    m[10]=go.poop; m[11]=go.fl; m[12]=go.miss; m[13]=go.needT; m[14]=go.snk; m[15]=go.meal; m[16]=go.nm; m[17]=go.sub; m[18]=0; m[19]=goSum(m); }
static int goUnpack(volatile const u8*m){ if(m[0]!='G'||m[1]!='O'||m[19]!=goSum(m)||m[2]>GS_ADULT) return 0;
    go.stage=m[2]; go.form=m[3]; go.age=(u16)(m[4]|(m[5]<<8)); go.hun=m[6]; go.hap=m[7]; go.disc=m[8]; go.wt=m[9]; go.poop=m[10]; go.fl=m[11]; go.miss=m[12]; go.needT=m[13];
    go.snk=m[14]; go.meal=m[15]; go.nm=m[16]&7; go.sub=m[17]; return 1; }
static void goClear(void){ Gotchi z={0}; go=z; goStage=0; }
static void goSave(void){ goPack(SRAM_BASE+GO_OFF); }
static void goLoad(void){ goClear(); if(!goUnpack(SRAM_BASE+GO_OFF)) goClear(); goStage=(go.fl&GF_ON)?go.stage:0; }
static int goHas(void){ return (go.fl&GF_ON)&&go.stage>GS_EGG; }   // (self.h: CHECK THE BOREAGOTCHI)
static void goNew(void){ goClear(); go.fl=GF_ON; go.hun=4; go.hap=4; go.wt=5; go.nm=(u8)(rnd8()&7); goStage=0; }
static int goNeeds(void){ return go.stage>GS_EGG&&(go.hun==0||go.hap==0||go.poop||(go.fl&GF_SICK)||((go.fl&GF_SLEEP)&&!(go.fl&GF_DARK))||(go.fl&GF_FAKE)); }
// ---- one tick of its life (a game minute, or half a second in your hand) ----
static void goTick(void){
    if(!(go.fl&GF_ON)) return;
    if(go.age<65000) go.age++;
    if(go.stage==GS_EGG){ if(go.age>=20){ go.stage=GS_BABY; go.age=0; go.hun=3; go.hap=3; } goStage=go.stage; return; }
    int night=simIsNight(), sl=(go.fl&GF_SLEEP)!=0;
    if(night&&!sl&&go.stage>=GS_CHILD){ go.fl|=GF_SLEEP; } else if(!night&&sl){ go.fl&=(u8)~(GF_SLEEP|GF_DARK); }
    sl=(go.fl&GF_SLEEP)!=0;
    int rate=go.stage==GS_BABY?30:go.stage==GS_CHILD?45:60; if(sl) rate*=3;
    if(go.age%rate==0&&go.hun) go.hun--;
    if(go.age%(rate+10)==0&&go.hap) go.hap--;
    if(!sl&&go.meal&&go.age%90==45&&go.poop<3) go.poop++;
    if(go.poop&&go.age%60==30&&((int)rnd8()*100>>8)<35) go.fl|=GF_SICK;
    if(!(go.fl&GF_FAKE)&&!goNeeds()&&!sl&&go.age%150==75&&(rnd8()&1)) go.fl|=GF_FAKE;   // a test: it calls for nothing
    if(goNeeds()){ go.fl|=GF_CALL; if(++go.needT>=15){ go.needT=0; if(go.miss<99) go.miss++; if(go.fl&GF_FAKE) go.fl&=(u8)~GF_FAKE; } }
    else { go.fl&=(u8)~GF_CALL; go.needT=0; }
    // growing up
    static const u16 grow[5]={0,240,720,1440,0};
    if(go.stage<GS_ADULT&&go.age>=grow[go.stage]){ go.stage++; go.age=0;
        if(go.stage==GS_TEEN) go.form=go.miss<=2?0:1;
        if(go.stage==GS_ADULT) go.form=go.snk>go.meal&&go.snk>=6?2:(go.miss<=2&&go.disc>=50)?0:1;
        go.miss=0; }
    if(go.hun==0&&go.hap==0&&(go.fl&GF_SICK)&&go.miss>=12){ go.fl&=(u8)~GF_ON; go.stage=0; }   // badly neglected: it goes home (to its planet)
    goStage=go.stage;
}
static u8 goBeepT;
static void goMinute(void){   // sims.h simMinute: it lives on while you play, and beeps when it needs you
    if(!(go.fl&GF_ON)) return;
    goTick(); goSave();
    if(goNeeds()&&go.stage>GS_EGG&&++goBeepT>=45){ goBeepT=0; lnote="YOUR BOREAGOTCHI IS BEEPING"; lnoteT=120; sfxPlay(SFX_TICK); }
}
// ---- the LCD: 32 x 16 dots ----
#define GL_X 56
#define GL_Y 50
#define GL_P 4
static u8 goDot[16][32] EWRAM_BSS;
static void gdClear(void){ for(int y=0;y<16;y++) for(int x=0;x<32;x++) goDot[y][x]=0; }
static void gdArt(const char* const*a,int h,int x0,int y0,int fl){ for(int r=0;r<h;r++){ const char*s=a[r]; for(int q=0;s[q];q++){ if(s[q]!='#') continue; int x=x0+(fl?(int)(15-q):q), y=y0+r; if(x>=0&&x<32&&y>=0&&y<16) goDot[y][x]=1; } } }
static const char* const gaEgg[2][12]={{"....####....","..##....##..",".#..##....#.","#..####....#","#...##..#..#","#......###.#","#.#.....#..#","#.##.......#",".#....##..#.","..##.####...","....####....","............"},
                                      {"............","....####....","..##....##..",".#..##....#.","#..####....#","#...##..#..#","#......###.#","#.#.....#..#",".##.......#.","..#...##.#..","...######...","............"}};
static const char* const gaBaby[8]={"..####..",".#....#.","#.#..#.#","#......#","#..##..#",".#....#.","..####..","........"};
static const char* const gaChild[10]={"...####...","..#....#..",".#.#..#.#.",".#......#.",".#..##..#.","..#....#..","..######..","..#....#..",".##....##.","..........."};
static const char* const gaTeen[2][12]={{"....#.#.....","...######...","..#......#..",".#.##..##.#.",".#........#.",".#...##...#.","..#......#..","...######...","..#.#..#.#..","..#.#..#.#..",".##.#..#.##.","............"},
                                      {"............","...######...","..#......#..",".#.--..--.#.",".#........#.",".#..####..#.","..#......#..","...######...","..#......#..","..#......#..","..##....##..","............"}};
static const char* const gaAdult[3][14]={
  {"...#....#....","...########..","..#........#.",".#..##..##..#",".#..........#",".#...####...#","..#........#.","...########..",".##.#....#.##","#...#....#...#","....#....#...",".....#..#....","....##..##...","............."},
  {"...########..","..#........#.",".#..#....#..#",".#..........#",".#...####...#","#............#","#............#","#............#",".#..........#.","..##########..","...#......#...","..##......##..",".............",""},
  {"......#......",".....###.....","...#######...","..#.......#..",".#..--.--..#.",".#.........#.",".#...###...#.","..#.......#..","...#######...","..#.#...#.#..","..#.#...#.#..",".##.#...#.##.","............."}};
static const char* const gaPoop[4]={".#..","..#.",".##.","####"};
static const char* const gaSkull[5]={".###.","#.#.#","#####",".###.",".#.#."};
static const char* const gaZ[5]={"###.","..#.",".#..","###.","...."};
static void goScene(int fr,int look){   // the blob on the LCD (look: -1 left, 1 right, 0 as it likes)
    gdClear();
    int bob=fr&1, x=12+((fr>>1)%3==0?-2:(fr>>1)%3==1?0:2); if(look) x=12;
    if(!(go.fl&GF_ON)){ return; }
    if((go.fl&GF_SLEEP)&&(go.fl&GF_DARK)){ gdArt(gaZ,5,24,2+bob,0); return; }
    int fl=look?look<0:(fr&2)!=0;
    switch(go.stage){
    case GS_EGG: gdArt(gaEgg[bob],12,10,3,0); break;
    case GS_BABY: gdArt(gaBaby,8,x+2,7-bob,fl); break;
    case GS_CHILD: gdArt(gaChild,10,x+1,5-bob,fl); break;
    case GS_TEEN: gdArt(gaTeen[go.form&1],12,x,3-bob,fl); break;
    default: gdArt(gaAdult[go.form<3?go.form:0],14,x,1+(go.form==1?1:-bob)+1,fl); break; }
    for(int i=0;i<go.poop;i++) gdArt(gaPoop,4,28-(i&1)*4,12-(i>>1)*5,0);
    if(go.fl&GF_SICK) gdArt(gaSkull,5,1,1,0);
    if(go.fl&GF_SLEEP) gdArt(gaZ,5,24,1,0);
}
static void goLcd(int dark){   // the dots onto the screen
    u16 bg=dark?RGB(5,8,4):RGB(19,23,14), on=dark?RGB(9,12,7):RGB(4,6,3), off=dark?RGB(6,9,5):RGB(17,21,12);
    rect(GL_X-4,GL_Y-4,32*GL_P+8,16*GL_P+8,bg);
    for(int y=0;y<16;y++) for(int x=0;x<32;x++) rect(GL_X+x*GL_P,GL_Y+y*GL_P,GL_P-1,GL_P-1,goDot[y][x]?on:off);
}
static const char* const goIconNm[8]={"FEED","LIGHT","PLAY","MEDICINE","CLEAN","STATS","SCOLD","ATTENTION"};
static void goIcon(int i,int x,int y,int on,int lit){   // 9 x 7 icons above and below the LCD
    static const char* const ic[8][7]={
      {"...#.....","..#.##...",".#####...","#######..","#######..",".#####...","........."},   // an apple
      {"...###...","..#...#..","..#...#..","...#.#...","...###...","...###...","........."},   // a bulb
      {".#######.","#..#....#","#.###.#.#","#..#..#.#","#.......#",".#######.","........."},   // a controller
      {"...###...","...#.#...",".###.###.",".#.....#.",".###.###.","...#.#...","...###..."},   // a cross
      {"..#.#.#..","...#.#...","..#####..","...###...","...###...","..#####..","........."},   // a brush (the duck)
      {"#........","#..#.....","#..#..#..","#.##..#..","#.##.##..","#########","........."},   // a meter
      {".#####...",".#...#...",".#.#.#...",".#####...","...#.....","...###...","........."},   // a finger wagging
      {"...#.....","..###....","..###....",".#####...",".#####...","#######..","...#....."}};  // a bell
    u16 c=on?RGB(31,31,31):lit?RGB(31,26,6):RGB(22,12,20);
    rect(x-3,y-3,24,20,RGB(8,2,8));   // (its cell, so the last highlight goes)
    for(int r=0;r<7;r++) for(int q=0;q<9;q++) if(ic[i][r][q]=='#') rect(x+q*2,y+r*2,2,2,c);
    if(on){ rect(x-3,y-3,24,20,RGB(31,31,31)); rect(x-2,y-2,22,18,RGB(20,6,16)); for(int r=0;r<7;r++) for(int q=0;q<9;q++) if(ic[i][r][q]=='#') rect(x+q*2,y+r*2,2,2,c); }
}
static void goBody(void){   // the keychain egg (drawn once)
    for(int y=0;y<SH;y++){ int g=y*12/SH; rect(0,y,SW,1,RGB(2+g/3,1,4+g/2)); }
    for(int y=8;y<158;y++){ int dy=y-84, half=(int)(isqrtI(98*98-dy*dy*98*98/(76*76))); if(half<=0) continue; u16 c=RGB(26-(y-8)/12,10+(y-8)/20,22-(y-8)/14); rect(120-half,y,half*2,1,c); rect(120-half,y,3,1,RGB(31,20,28)); }
    disc(64,24,6,RGB(24,24,26)); disc(64,24,3,RGB(4,2,6));   // the keyring
    s2rr(GL_X-10,GL_Y-30,32*GL_P+20,16*GL_P+68,RGB(8,2,8));
    text(120-tw("BOREAGOTCHI",1)/2,GL_Y+16*GL_P+38,"BOREAGOTCHI",RGB(31,22,28),1);
    disc(96,146,5,RGB(31,26,6)); disc(120,150,5,RGB(31,26,6)); disc(144,146,5,RGB(31,26,6));   // the three buttons
}
static void goIcons(int sel){ for(int i=0;i<8;i++){ int x=GL_X-4+(i&3)*34, y=i<4?GL_Y-26:GL_Y+16*GL_P+8; goIcon(i,x,y,i==sel,i==7&&(go.fl&GF_CALL)); } }
static const char*goMa; static const char*goMb; static u8 goMT;   // the message on show (redrawn with the rest) and how long it stays
static void goMsgDraw(void){ rect(0,0,SW,9,RGB(2,1,4)); if(goMa) text(120-tw(goMa,1)/2,1,goMa,RGB(31,26,6),1);
    rect(GL_X-6,GL_Y+16*GL_P+26,32*GL_P+12,10,RGB(8,2,8)); if(goMb) text(120-tw(goMb,1)/2,GL_Y+16*GL_P+27,goMb,WHITE,1); }
static void goMsg(const char*a,const char*b){ goMa=a; goMb=b; goMT=150; goMsgDraw(); }   // a line at the top, one under the LCD
static void goStats(void){   // STATS: hearts and numbers on the LCD's place
    u16 prev=keyNow();
    for(int page=0;page<2;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        rect(GL_X-4,GL_Y-4,32*GL_P+8,16*GL_P+8,RGB(19,23,14));
        if(page==0){ text(GL_X,GL_Y,"HUNGER",RGB(4,6,3),1); text(GL_X,GL_Y+30,"HAPPY",RGB(4,6,3),1);
            for(int i=0;i<4;i++){ u16 c=i<go.hun?RGB(4,6,3):RGB(15,18,11); disc(GL_X+8+i*14,GL_Y+18,5,c); c=i<go.hap?RGB(4,6,3):RGB(15,18,11); disc(GL_X+8+i*14,GL_Y+48,5,c); } }
        else { char b[32]; char*e=slCat(b,goNames[go.nm]); text(GL_X,GL_Y,b,RGB(4,6,3),1);
            e=slCat(b,goStNm[go.stage]); if(go.stage>=GS_TEEN){ e=slCat(e,"  "); slCat(e,goFormNm[go.stage][go.form]); } text(GL_X,GL_Y+11,b,RGB(4,6,3),1);
            e=slCat(b,"AGE "); e=slNum(e,go.age/60); slCat(e," HOURS"); text(GL_X,GL_Y+22,b,RGB(4,6,3),1);
            e=slCat(b,"WEIGHT "); e=slNum(e,go.wt); slCat(e," G"); text(GL_X,GL_Y+33,b,RGB(4,6,3),1);
            e=slCat(b,"DISCIPLINE "); e=slNum(e,go.disc); slCat(e,"%"); text(GL_X,GL_Y+44,b,RGB(4,6,3),1);
            e=slCat(b,"CARE MISTAKES "); slNum(e,go.miss); text(GL_X,GL_Y+55,b,RGB(4,6,3),1); }
        uiPresentRect(GL_X-4,GL_Y-4,GL_X+32*GL_P+4,GL_Y+16*GL_P+4);
        if(pr&(K_A|K_RIGHT|K_DOWN)) page++; if(pr&(K_B|K_START)) break;
    }
}
static int goPlay(void){   // WHICH WAY? it looks left or right: guess first. 5 rounds, 3 to win
    int won=0; u16 prev=keyNow();
    for(int r=0;r<5;r++){
        int guess=0;
        for(int t=0;!guess;t++){ u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&K_LEFT) guess=-1; if(pr&K_RIGHT) guess=1; if(pr&K_B) return -1;
            if((t&15)==0){ goScene(t>>4,0); goLcd(0); { static char b[20]; char*e=slCat(b,"ROUND "); e=slNum(e,r+1); e=slCat(e,"  WINS "); slNum(e,won); goMsg("WHICH WAY? LEFT OR RIGHT",b); } uiPresent(); } else vsync(); }
        int way=(rnd8()&1)?1:-1; goScene(0,way); goLcd(0); int ok=way==guess; won+=ok;
        goMsg(ok?"YES!":"NOPE",ok?"IT LOOKED YOUR WAY":"IT LOOKED THE OTHER WAY"); uiPresent(); sfxPlay(ok?SFX_STICK:SFX_BONK);
        for(int i=0;i<50;i++) vsync(); prev=keyNow(); }
    return won;
}
static void goAct(int i){
    if(!(go.fl&GF_ON)) return;
    if(go.stage==GS_EGG&&i!=5){ goMsg("IT IS STILL AN EGG","GIVE IT A MINUTE"); return; }
    int sl=(go.fl&GF_SLEEP)!=0;
    if(sl&&i!=1&&i!=5&&i!=3){ goMsg("SHHH  IT IS ASLEEP",0); return; }
    switch(i){
    case 0: { static const char* const fd[2]={"A MEAL","A SNACK"}; int c=menu("FEED",fd,2); goBody(); if(c<0) return;
        if(c==0){ if(go.hun>=4){ goMsg("IT REFUSES","IT IS FULL"); return; } go.hun++; if(go.wt<99) go.wt++; if(go.meal<250) go.meal++; goMsg("NOM NOM","A MEAL"); stQEvent(QE_GOTCHI,GA_FEED); }
        else { if(go.hap<4) go.hap++; if(go.wt<98) go.wt+=2; if(go.snk<250) go.snk++; goMsg("CRUNCH","A SNACK  IT LOOKS VERY RELAXED"); stQEvent(QE_GOTCHI,GA_SNACK); }
        sfxPlay(SFX_MUNCH); break; }
    case 1: go.fl^=GF_DARK; goMsg(go.fl&GF_DARK?"LIGHTS OUT":"LIGHTS ON",sl?"GOOD NIGHT":"IT IS NOT SLEEPY"); stQEvent(QE_GOTCHI,GA_LIGHT); break;
    case 2: { int w=goPlay(); goBody(); if(w<0) return; if(w>=3){ if(go.hap<4) go.hap++; if(go.wt>1) go.wt--; goMsg("IT WON  SO HAPPY",0); voxPlay(V_yahoo); } else goMsg("IT LOST  STILL FUN",0); stQEvent(QE_GOTCHI,GA_PLAY); break; }
    case 3: if(!(go.fl&GF_SICK)){ goMsg("IT IS NOT SICK","IT SPITS IT OUT"); return; } go.fl&=(u8)~GF_SICK; goMsg("ALL BETTER",0); stQEvent(QE_GOTCHI,GA_MED); break;
    case 4: if(!go.poop){ goMsg("NOTHING TO CLEAN",0); return; } go.poop=0; goMsg("FLUSHED",0); sfxPlay(SFX_FLUSH); stQEvent(QE_GOTCHI,GA_CLEAN); break;
    case 5: goStats(); goBody(); break;
    case 6: if(go.fl&GF_FAKE){ go.fl&=(u8)~GF_FAKE; go.disc=(u8)(go.disc+25>100?100:go.disc+25); goMsg("IT KNOWS BETTER NOW","DISCIPLINE UP"); }
            else { if(go.hap) go.hap--; goMsg("IT DID NOTHING WRONG","IT IS SAD NOW"); }
            stQEvent(QE_GOTCHI,GA_SCOLD); break;
    case 7: goMsg(go.fl&GF_CALL?"IT WANTS SOMETHING":"ALL GOOD",go.fl&GF_CALL?(go.hun==0?"IT IS HUNGRY":go.hap==0?"IT IS BORED":go.poop?"IT MADE A MESS":(go.fl&GF_SICK)?"IT IS SICK":(go.fl&GF_SLEEP)?"THE LIGHT IS ON":"IT IS TESTING YOU"):0); break;
    }
    goTick(); goSave();
}
// ---- THE HANDHELD (expand.h exModeRun) ----
XPF static int goRun(void){
    objHideAll(); REG_DISPCNT=0x0403;
    if(!(go.fl&GF_ON)){ goNew(); goSave(); }
    goBody(); int sel=0, fr=0; u16 prev=keyNow(); static char nb[28]; goMa=goMb=0; goMT=0;
    goScene(0,0); goLcd(0); goIcons(sel);
    if(go.stage==GS_EGG) goMsg("A NEW EGG","KEEP IT CLOSE"); else goMT=0;
    uiPresent();
    for(int t=0;;t++){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_B|K_START)) break;
        int redraw=0;
        if(pr&K_RIGHT){ sel=(sel+1)&7; redraw=1; } if(pr&K_LEFT){ sel=(sel+7)&7; redraw=1; }
        if(pr&K_A){ goAct(sel); redraw=1; prev=keyNow(); }
        if((t%30)==0){ fr++; goTick(); if((t%600)==0) goSave(); redraw=1; if(goNeeds()&&(fr&7)==0) sfxPlay(SFX_TICK); }
        if(goMT&&!--goMT) redraw=1;
        if(!redraw){ vsync(); continue; }
        goScene(fr,0); goLcd((go.fl&GF_SLEEP)&&(go.fl&GF_DARK)); goIcons(sel);
        if(!goMT){ char*e=slCat(nb,goNames[go.nm]); e=slCat(e,"  "); slCat(e,go.stage>=GS_TEEN?goFormNm[go.stage][go.form]:goStNm[go.stage]); goMa=nb; goMb=goIconNm[sel]; }
        goMsgDraw();
        if(!(go.fl&GF_ON)){ goMsg("IT WENT BACK TO ITS HOME PLANET","A NEW EGG WILL COME"); uiPresent(); for(int i=0;i<180;i++) vsync(); goNew(); goBody(); }
        uiPresent();
    }
    goSave(); return go.stage;
}
static void sgEncGotchi(SlW*w){ goSave(); volatile u8*m=SRAM_BASE+GO_OFF; for(int i=0;i<GO_LEN;i++) slwPut(w,m[i]); }

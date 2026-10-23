// lunrado.h - LUNRADO: the moonbase. The SPACEPORT's A SEAT ON THE ROCKET now flies you to it (space.h exLaunch, after the orbit).
//   THE APPROACH   the ship is damaged: three systems inside (FUEL VALVE, GUIDANCE, LANDING GEAR: stop the cursor in the green; LOGIC is your
//                  mechanical skill: a wider zone a level) and the panels outside (the spacewalk). Each fix, inside or out, makes the landing safer.
//   THE LANDING    smooth, HARD (hurts: health, energy, comfort) or the ship EXPLODES. boom % = 4 + 9 a part left broken - 2 a LOGIC level (3 to 70),
//                  hard % = 50 - 7 a part fixed - 3 a LOGIC level. An explosion sends you home, SHUTS THE SPACEPORT (Town.pad[2] bit 0, saved with the town)
//                  and the spaceport counter offers FUND THE SPACEPORT (300000) to open it again.
//   THE PLACE      the moonbase is a HIDDEN lot of the town (a SPACEPORT-kind lot named LUNRADO, made on the first landing: not on the map, not under the cursor, never in the cab list; only the rocket goes there). Its room has a lunar lander behind the counter; the spaceport's has the rocket (venues.h venueDrawShip). Its counter:
//                  LUNAR RATIONS, LOW GRAVITY BOUNCE, MOONBASE SHIFT and RETURN TO EARTH (a safe flight home to the spaceport).
#define LN_CT 250        // trvFind's pseudo kind for the moonbase lot
#define LN_FUND 300000
static int lnLot(const NbLot*L){ return L->on&&L->kind==LKIND_COMM&&L->type==CT_SPACE&&L->name[0]=='L'&&L->name[1]=='U'&&L->name[2]=='N'; }
static int lnrHere(void){ return nbOk&&nbT.cur<NB_LOTS&&lnLot(&nbT.lot[nbT.cur]); }
static int lnrShut(void){ return nbOk&&(nbT.pad[2]&1); }
static void lnrSetShut(int v){ if(!nbOk) return; if(v) nbT.pad[2]|=1; else nbT.pad[2]&=(u8)~1; nbSave(); }
static int lnrLotGet(void){   // the moonbase lot, made once there is a landing
    if(!nbOk) return -1;
    for(int i=0;i<NB_LOTS;i++) if(lnLot(&nbT.lot[i])){ NbLot*L=&nbT.lot[i]; if(L->w<9||L->h<7){ if(L->slot>=0) slDelete(L->slot); L->slot=-1; L->w=9; L->h=7; nbSave(); }   // (an older, smaller moonbase becomes the mall; its room is built again)
        return i; }
    int fi=-1; for(int i=0;i<NB_LOTS&&fi<0;i++) if(!nbT.lot[i].on) fi=i;
    if(fi>=0){ nbLotAdd(fi,"LUNRADO",0,0,9,7,LKIND_COMM,CT_SPACE); nbSave(); return fi; }   // a HIDDEN lot: it takes no land and shows nowhere on the map (lnLot: nbAt, the map and the thumbnails skip it); only the rocket goes there
    for(int i=0;i<NB_LOTS;i++){ NbLot*L=&nbT.lot[i];   // no free land: a lot nobody built on and nobody lives on
        if(L->on&&L->kind==LKIND_RES&&i!=nbT.home&&i!=nbT.cur&&L->slot<0&&!nbLives(i)){
            L->kind=LKIND_COMM; L->type=CT_SPACE; const char*nm="LUNRADO"; int k=0; for(;nm[k]&&k<NB_NAME;k++) L->name[k]=nm[k]; L->name[k]=0; nbSave(); return i; } }
    return -1;
}
static u8 lnEv; static const char* const lnEvNm[4]={"A METEOR SHOWER TONIGHT","THE EARTHRISE FESTIVAL IS ON","THE MOON FAIR IS IN TOWN","IT IS LOW GRAVITY WEEK"};
static void lnWait(int frames){ u16 p0=keyNow(); for(int i=0;i<frames;i++){ vsync(); u16 k=keyNow(); if((k&~p0)&(K_A|K_B|K_START)) break; p0=k; } }
static void lnStars(int seed){ rect(0,0,SW,SH,RGB(0,0,3)); for(int i=0;i<50;i++){ u32 h=swHash(i,seed); px((int)(h%SW),(int)((h>>8)%SH),RGB(24,24,28)); } }
static void lnMoon(int y){ rect(0,y,SW,SH-y,RGB(15,15,16)); for(int i=0;i<7;i++){ u32 h=swHash(i,5); disc((int)(h%SW),y+6+(int)((h>>8)%20),3+(int)(h&3),RGB(11,11,12)); } }
static int lnConsole(void){   // inside: three systems, stop the cursor in the green. 0..3 fixed
    static const char* const nm[3]={"FUEL VALVE","GUIDANCE","LANDING GEAR"};
    int zw=20+skLvl(SK_LOGIC)*7, fixed=0;
    for(int i=0;i<3;i++){
        int zx=20+rnd8()%(200-zw), x=20, dir=1, sp=2+i, res=-1;
        u16 prev=keyNow();
        for(int t=0;res<0&&t<900;t++){
            u16 k=keyNow(), pr=k&~prev; prev=k;
            x+=dir*sp; if(x>=220){ x=220; dir=-1; } if(x<=20){ x=20; dir=1; }
            if(pr&K_A) res=(x>=zx&&x<=zx+zw);
            if(pr&K_B) res=0;
            rect(0,0,SW,SH,RGB(2,3,8)); rect(0,0,SW,12,RGB(24,4,3)); text(6,3,"WARNING  SYSTEM DAMAGE",WHITE,1);
            text((SW-tw(nm[i],2))/2,34,nm[i],GOLD,2);
            text((SW-tw("A  STOP THE CURSOR IN THE GREEN",1))/2,60,"A  STOP THE CURSOR IN THE GREEN",RGB(17,29,31),1);
            rect(20,86,200,16,RGB(6,6,10)); rect(zx,86,zw,16,RGB(6,24,8)); rect(x-1,82,3,24,WHITE);
            { char b[16]; char*e=slCat(b,"REPAIRED "); e=slNum(e,fixed); slCat(e," OF 3"); text(6,SH-12,b,GOLD,1); }
            present(); vsync(); }
        if(res<0) res=0;
        rect(0,0,SW,SH,RGB(2,3,8)); const char*m=res?"FIXED":"BOTCHED"; text((SW-tw(m,3))/2,50,m,res?RGB(10,30,12):RGB(31,10,8),3); present();
        if(res){ fixed++; sfxPlay(SFX_STICK); voxPlay(V_yahoo); } else voxPlay(V_nah); lnWait(50); }
    return fixed;
}
static void lnrReturnHome(void){ int h=trvPlace(CT_SPACE); if(h>=0) trvGo(h); }
static void lnrBoom(void){   // the ship explodes: back to Earth, the spaceport is shut until it is funded again
    for(int i=0;i<40;i++){ lnStars(3); int r=i*5; disc(120,90,r,i<20?RGB(31,26,6):RGB(31,12,3)); disc(120,90,r/2,i<14?RGB(31,31,22):RGB(31,20,4)); present(); vsync(); if(i==3) sfxPlay(SFX_THUNDER); }
    rect(0,0,SW,SH,RGB(31,31,31)); present(); vsync(); vsync(); lnStars(9);
    text((SW-tw("THE SHIP EXPLODED",2))/2,40,"THE SHIP EXPLODED",RGB(31,10,6),2);
    text((SW-tw("YOU WAKE UP BACK ON EARTH",1))/2,66,"YOU WAKE UP BACK ON EARTH",WHITE,1);
    text((SW-tw("THE SPACEPORT IS SHUT",1))/2,80,"THE SPACEPORT IS SHUT",GOLD,1);
    text((SW-tw("FUND IT AGAIN  300000 SIMOLEONS",1))/2,92,"FUND IT AGAIN  300000 SIMOLEONS",GOLD,1); present(); voxPlay(V_shriek); lnWait(300);
    lnrSetShut(1); lhp=HP_MAX/4; vAdd(&sNrg,-50); vAdd(&sCom,-40); vAdd(&sSoc,-20);
    lnrReturnHome(); toast("YOU LOST THE SHIP  FUND THE SPACEPORT AGAIN");
}
// called by exLaunch after the orbit. orbitOk: the orbit repair worked (a head start)
static void lnrApproach(int orbitOk){
    for(int i=0;i<240;i++) simMinute();   // (six hours to the moon)
    lnStars(1); lnMoon(130); disc(200,40,12,RGB(18,24,31));
    text((SW-tw("APPROACHING LUNRADO",2))/2,20,"APPROACHING LUNRADO",WHITE,2);
    text((SW-tw("ALARMS ARE GOING OFF  REPAIR THE SHIP",1))/2,50,"ALARMS ARE GOING OFF  REPAIR THE SHIP",RGB(31,10,8),1);
    text((SW-tw("INSIDE: FUEL  GUIDANCE  GEAR    OUTSIDE: PANELS",1))/2,64,"INSIDE: FUEL  GUIDANCE  GEAR    OUTSIDE: PANELS",GOLD,1);
    text((SW-tw("YOUR LOGIC IS YOUR MECHANICAL SKILL",1))/2,78,"YOUR LOGIC IS YOUR MECHANICAL SKILL",RGB(17,29,31),1); present(); voxPlay(V_nah); lnWait(240);
    int fixed=lnConsole();
    swDone=0; exSpacewalk(1); fixed+=swDone;   // 0..6
    if(orbitOk&&fixed<6) fixed++;
    int lg=skLvl(SK_LOGIC), boom=4+(6-fixed)*9-lg*2, hard=50-fixed*7-lg*3;
    if(boom<3) boom=3; if(boom>70) boom=70; if(hard<0) hard=0;
    int r=rnd8()*100>>8, res=r<boom?2:r<boom+hard?1:0;
    for(int y=10;y<=118;y+=2){ lnStars(1); lnMoon(130); int sh=(res==1&&y>90)?(int)(rnd8()&3)-1:0;   // the descent
        rect(112+sh,y,16,12,RGB(28,28,29)); rect(110+sh,y+12,20,2,RGB(20,20,22)); rect(108+sh,y+14,3,4,RGB(20,20,22)); rect(129+sh,y+14,3,4,RGB(20,20,22));
        if(y<110) rect(117+sh,y+18,6,(int)(rnd8()&7)+3,RGB(31,22,6));
        { char b[24]; char*e=slCat(b,"PARTS FIXED "); e=slNum(e,fixed); slCat(e," OF 6"); text(6,4,b,GOLD,1); }
        if(res==2&&y>=60) break; present(); vsync(); vsync(); }
    if(res==2){ lnrBoom(); return; }
    stQEvent(QE_LAUNCH,1);
    if(res==1){ int d=30+(6-fixed)*7; hpLose(d); vAdd(&sNrg,-40); vAdd(&sCom,-35); vAdd(&lfood,-10); voxPlay(V_shriek); sfxPlay(SFX_THUNDER);
        rect(0,60,SW,40,RGB(2,3,8)); text((SW-tw("HARD LANDING",3))/2,66,"HARD LANDING",RGB(31,12,6),3); text((SW-tw("THAT HURT  YOUR BODY TOOK IT",1))/2,88,"THAT HURT  YOUR BODY TOOK IT",WHITE,1); present(); lnWait(200); }
    else { moodEvent(M_PROMO); voxPlay(V_yahoo); rect(0,60,SW,40,RGB(2,3,8)); text((SW-tw("SMOOTH LANDING",3))/2,66,"SMOOTH LANDING",RGB(10,30,12),3); text((SW-tw("WELCOME TO LUNRADO",1))/2,88,"WELCOME TO LUNRADO",WHITE,1); present(); lnWait(200); }
    int l=lnrLotGet(); if(l>=0){ NbLot*L=&nbT.lot[l]; if(L->slot>=0) slDelete(L->slot); L->slot=-1; }   // (every landing builds the resort again: three pods drawn by luck)
    lnEv=(u8)(rnd8()&3); if(l>=0&&trvGo(l)) toast(lnEvNm[lnEv]);
}
static int lnrCanDo(int sp){   // venueDo asks before it takes the money
    if(sp==VS_LAUNCH&&lnrShut()){ lnote="THE SPACEPORT IS SHUT  FUND IT AGAIN"; lnoteT=100; voxPlay(V_nah); return 0; }
    if(sp==VS_FUND){ if(!lnrShut()){ lnote="THE SPACEPORT IS ALREADY OPEN"; lnoteT=70; return 0; } if(simMoney<LN_FUND){ lnote="YOU NEED 300000 SIMOLEONS"; lnoteT=80; voxPlay(V_nah); return 0; } }
    return 1;
}
static void lnrDo(int sp){
    if(sp==VS_FUND){ simMoney-=LN_FUND; lnrSetShut(0); lnote="THE SPACEPORT IS OPEN AGAIN"; lnoteT=120; voxPlay(V_yahoo); moodEvent(M_PROMO); }
    else if(sp==VS_PHOTO){ static const char* const c[5]={"EARTH LOOKS TINY  YOU LOOK GREAT","YOU AND THE BLUE MARBLE","SMILE  YOU ARE 384000 KM FROM HOME","THE PHOTOGRAPHER SAYS  ONE MORE","EARTHRISE BEHIND YOU  PERFECT"};
        vAdd(&sSoc,12+(lnEv==1?12:0)); moodEvent(M_COMBO); lstun=90; lnote=c[rnd8()%5]; lnoteT=120; voxPlay(V_yeha); }
    else if(sp==VS_TOUR){ static const char* const c[6]={"THE CRATER IS ICE  NOT CHEESE","DUST HERE NEVER SETTLES  IT SLIDES","THE DOME HOLDS 40 THOUSAND TOURISTS","EVERY STEP LASTS TWO SECONDS","THE FLAG WAS STUCK IN BY A TOURIST","THE GUIDE HAS NEVER BEEN TO EARTH"};
        vAdd(&sSoc,15); vAdd(&sNrg,-6); skGain(SK_LOGIC,lnEv==0?2:1); moodEvent(M_SKILL); lstun=180; lnote=c[rnd8()%6]; lnoteT=120; }
    else if(sp==VS_ROCK){ static const char* const c[8]={"A ROCK  IT IS GREY","A ROCK  IT SPARKLES  A LITTLE","A KEYCHAIN WITH DUST IN IT","A SNOW GLOBE  ON THE MOON","A MAGNET  YOUR FRIDGE WILL LOVE IT","A TINY FLAG ON A ROCK","A TEA TOWEL  IT SAYS LUNRADO","A T SHIRT  MY PARENTS WENT TO THE MOON"};
        if((rnd8()&15)==0){ simMoneyAdd(150); lnote="IT WAS A REAL MOON ROCK  A COLLECTOR PAYS 150"; voxPlay(V_yahoo); } else lnote=c[rnd8()&7]; moodEvent(M_WANT); lnoteT=130; }
    else if(sp==VS_HOME){ for(int i=0;i<240;i++) simMinute(); lnote="BACK TO EARTH"; lnoteT=100; lnrReturnHome(); }
}
// ---- THE MOON RESORT: a domed lunar holiday town, built again (and reshuffled) on every landing (nbTemplate -> lnrBuild; the lot is 9 x 7 cells = the room's 36 x 28) ----
//   GROUND FLOOR  the dome. In its middle the LUNAR ICE CRATER: a pond you can swim in, a paved rim, benches and a photo statue. Four pods stand in the corners: LUNARWEED
//                 (the dispensary, always, in a north corner: the weed.h counter and menu) and three of MOON MARKET, LOW-G ARCADE, STAR LOUNGE, OBSERVATORY and the MOON SPA,
//                 drawn by the luck of the landing: no two visits are the same. The spaceport desk (its lunar menu: rations, bounce, a shift, an EARTHRISE PHOTO, a CRATER TOUR,
//                 a MOON ROCK souvenir, the flight home) stands south of the crater with the lander behind it. Stairs up on the east side.
//   SECOND FLOOR  the SKYWALK: a cross of glass bridges over the crater (open air under them) with four terraces at the ends: SKY LOUNGE, the OBSERVATORY, EARTHRISE DECK.
//   EVENTS        every landing has something on (lnEv): a meteor shower, the Earthrise festival, a moon fair or low gravity week; the photo, tour and souvenir lines know it.
static void lnPut(int x,int y,char c,const char*look){ if(x<=0||y<=0||x>=MW-1||y>=MH-1||lifeMap[y][x]!='.') return; gPut(x,y,c); if(look) wallMap[y][x]=(u8)catFind(look); }
static void lnUnit(int a,int y0,int b,int y1,int fl,int wp,int top){ gRoom(a,y0,b,y1,fl,wp); gPut((a+b)/2,top?y0:y1,'D'); }   // a pod: a walled room, its door in the wall that faces the crater
static void lnPod(int kind,int a,int y0,int b,int y1,int top){
    static const u8 fls[6]={2,2,3,4,6,5}; lnUnit(a,y0,b,y1,fls[kind],(kind&1)?6:11,top);
    #define LNY(k) (top?(y1-1-(k)):(y0+1+(k)))
    int m=(a+b)/2;
    if(kind==1){ for(int x=a+1;x<b;x+=2) lnPut(x,LNY(0),'b',(x&2)?"TALL BOOKSHELF":"WHITE BOOKSHELF"); lnPut(a+1,LNY(2),'F',"VENDING MACHINE"); lnPut(b-1,LNY(2),'F',"VENDING MACHINE"); lnPut(a+3,LNY(4),'h',"BAR STOOL"); lnPut(b-3,LNY(4),'h',"BAR STOOL"); lnPut(m,LNY(2),'l',"GLOBE"); }   // MOON MARKET
    else if(kind==2){ for(int x=a+1;x<b;x++) lnPut(x,LNY(0),'v',(x&1)?"ARCADE CABINET":"PINBALL"); for(int x=a+1;x<b;x+=2) lnPut(x,LNY(2),'v',"ARCADE CABINET"); lnPut(a+1,LNY(4),'v',"CONSOLE AND TV"); lnPut(b-1,LNY(4),'v',"CONSOLE AND TV"); }   // LOW-G ARCADE
    else if(kind==3){ lnPut(a+1,LNY(0),'l',"PINK LAVA LAMP"); lnPut(b-1,LNY(0),'l',"GREEN LAVA LAMP"); lnPut(a+1,LNY(2),'C',"VELVET SOFA"); lnPut(a+1,LNY(3),'C',"VELVET SOFA"); lnPut(b-1,LNY(2),'C',"GREY COUCH"); lnPut(b-1,LNY(3),'C',"GREY COUCH"); lnPut(m,LNY(2),'G',"HOOKAH"); lnPut(m-2,LNY(4),'h',"BAR STOOL"); lnPut(m+2,LNY(4),'h',"BAR STOOL"); lnPut(m,LNY(0),'l',"NEON BORE SIGN"); }   // STAR LOUNGE
    else if(kind==4){ lnPut(m,LNY(0),'l',"GLOBE"); for(int x=a+1;x<b;x+=2) if(x!=m) lnPut(x,LNY(0),'b',"TALL BOOKSHELF"); lnPut(a+1,LNY(3),'p',"TALL CACTUS"); lnPut(b-1,LNY(3),'p',"TALL CACTUS"); lnPut(m-1,LNY(4),'h',"BAR STOOL"); lnPut(m+1,LNY(4),'h',"BAR STOOL"); }   // OBSERVATORY
    else { lnPut(a+1,LNY(1),'N',"SUN LOUNGER"); lnPut(a+3,LNY(1),'N',"STRIPED LOUNGER"); lnPut(b-3,LNY(1),'N',"SUN LOUNGER"); lnPut(b-1,LNY(1),'N',"STRIPED LOUNGER"); lnPut(a+1,LNY(4),'p',"POTTED PALM"); lnPut(b-1,LNY(4),'p',"FICUS"); lnPut(m,LNY(3),'p',"BEACH UMBRELLA"); }   // MOON SPA
    #undef LNY
}
static void lnrUpper(int rx0,int ry0,int rx1,int ry1){   // the second floor: the SKYWALK over the crater (live while this runs)
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ lifeMap[y][x]='.'; floorMap[y][x]=FL_AIR; wallMap[y][x]=0; }
    for(int y=ry0;y<=ry1;y++) for(int x=18;x<=21;x++) floorMap[y][x]=2;                 // the long bridge, north to south
    for(int y=15;y<=18;y++) for(int x=rx0;x<=rx1;x++) floorMap[y][x]=2;                 // the long bridge, west to east
    for(int y=14;y<=19;y++) for(int x=rx0;x<=rx0+5;x++) floorMap[y][x]=3;               // the terraces at the ends
    for(int y=14;y<=19;y++) for(int x=rx1-5;x<=rx1;x++) floorMap[y][x]=3;
    for(int y=ry0;y<=ry0+3;y++) for(int x=16;x<=23;x++) floorMap[y][x]=6;
    for(int y=ry1-3;y<=ry1;y++) for(int x=16;x<=23;x++) floorMap[y][x]=5;
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(floorMap[y][x]!=FL_AIR){   // a railing wherever the walkway meets the air
        int e=0; for(int d=0;d<4;d++){ int nx=x+(d==0)-(d==1), ny=y+(d==2)-(d==3); if(nx<0||ny<0||nx>=MW||ny>=MH||floorMap[ny][nx]==FL_AIR) e=1; }
        if(e){ lifeMap[y][x]='w'; wallMap[y][x]=13; } }
    gPut(rx1-3,17,'~'); wallMap[17][rx1-3]=0; floorMap[17][rx1-3]=3;                        // the stairs down
    lnPut(19,16,'l',"GLOBE");                                                            // a telescope where the bridges cross
    lnPut(rx0+1,15,'C',"VELVET SOFA"); lnPut(rx0+1,18,'C',"VELVET SOFA"); lnPut(rx0+4,16,'p',"POTTED PALM"); lnPut(rx0+4,17,'l',"PINK LAVA LAMP");   // SKY LOUNGE
    lnPut(rx1-1,16,'N',"STRIPED LOUNGER"); lnPut(rx1-1,17,'N',"STRIPED LOUNGER"); lnPut(rx1-4,15,'p',"FICUS");
    lnPut(19,ry0+1,'l',"GLOBE"); lnPut(17,ry0+2,'N',0); lnPut(22,ry0+2,'N',0); lnPut(20,ry0+1,'l',"TOTEM POLE");   // the OBSERVATORY terrace
    lnPut(17,ry1-1,'N',"SUN LOUNGER"); lnPut(18,ry1-1,'N',"STRIPED LOUNGER"); lnPut(21,ry1-1,'N',"SUN LOUNGER"); lnPut(22,ry1-1,'N',"STRIPED LOUNGER"); lnPut(20,ry1-2,'p',"BEACH UMBRELLA");   // EARTHRISE DECK
}
static void lnrBuild(int x0,int y0,int x1,int y1){
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3;
    gBox(x0,y0,x1,y1,7); gRoom(rx0,ry0,rx1,ry1,2,5); gPut(cx,ry1,'D');   // the dome and its door
    static const int sx0[4]={3,27,3,27}, sy0[4]={7,7,22,22}, sx1[4]={12,36,12,36}, sy1[4]={14,14,29,29};   // the four corner pods
    int dn=rnd8()&1; dispBuild(dn?26:2,6,dn?37:13,17);                                                      // LUNARWEED: always in a north corner (its door and counter face the crater)
    u8 t[5]={1,2,3,4,5}; for(int i=4;i>0;i--){ int j=rnd8()%(i+1), q=t[i]; t[i]=t[j]; t[j]=(u8)q; }          // the luck of the landing: three of the five other pods
    int n=0; for(int s=0;s<4;s++){ if(s==dn) continue; lnPod(t[n++],sx0[s],sy0[s],sx1[s],sy1[s],s>=2); }
    for(int y=8;y<=29;y++) for(int x=13;x<=26;x++){ if(lifeMap[y][x]!='.') continue; int dx=2*x-39, dy=2*y-35;     // the crater: ice pond, paved rim
        if(dx*dx*64+dy*dy*169<=169*64) floorMap[y][x]=FL_W0; else if(dx*dx*100+dy*dy*225<=225*100) floorMap[y][x]=FL_COPING; }
    lnPut(19,12,'l',"TOTEM POLE"); lnPut(12,17,'N',0); lnPut(27,18,'N',0); lnPut(14,12,'Z',0); lnPut(25,12,'Z',0); lnPut(14,23,'Z',0); lnPut(25,23,'Z',0);   // the photo statue, benches, planters
    lnPut(rx0+1,16,'F',"VENDING MACHINE"); lnPut(rx1-1,20,'F',"VENDING MACHINE");
    for(int x=rx0+3;x<=rx1-3;x+=5) lnPut(x,ry1-1,'Z',0);
    int dy=ry1-5; for(int x=cx-2;x<=cx+2;x++) gPut(x,dy,'@');   // the spaceport desk: the tourist menu, the lander behind it
    gPut(rx1-3,17,'^'); floorMap[17][rx1-3]=2;                  // the stairs up, on the east side
    if(flGoF(1,0)){ lnrUpper(rx0,ry0,rx1,ry1); if(!flGoF(0,0)) flGoF(0,1); }   // the skywalk; (if it did not fit in the floor store the stairs lead to a blank floor)
    else gPut(rx1-3,17,'.');
}

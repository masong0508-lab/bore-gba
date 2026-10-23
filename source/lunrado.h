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
    int l=lnrLotGet(); if(l>=0&&trvGo(l)) toast("YOU ARE ON LUNRADO");
}
static int lnrCanDo(int sp){   // venueDo asks before it takes the money
    if(sp==VS_LAUNCH&&lnrShut()){ lnote="THE SPACEPORT IS SHUT  FUND IT AGAIN"; lnoteT=100; voxPlay(V_nah); return 0; }
    if(sp==VS_FUND){ if(!lnrShut()){ lnote="THE SPACEPORT IS ALREADY OPEN"; lnoteT=70; return 0; } if(simMoney<LN_FUND){ lnote="YOU NEED 300000 SIMOLEONS"; lnoteT=80; voxPlay(V_nah); return 0; } }
    return 1;
}
static void lnrDo(int sp){
    if(sp==VS_FUND){ simMoney-=LN_FUND; lnrSetShut(0); lnote="THE SPACEPORT IS OPEN AGAIN"; lnoteT=120; voxPlay(V_yahoo); moodEvent(M_PROMO); }
    else if(sp==VS_HOME){ for(int i=0;i<240;i++) simMinute(); lnote="BACK TO EARTH"; lnoteT=100; lnrReturnHome(); }
}
// ---- THE MOONBASE MALL: a sprawling two-storey strip mall (nbTemplate -> lnrBuild; the lot is 9 x 7 cells = the room's 36 x 28) ----
//   GROUND FLOOR  a long concourse under the dome: the LUNARWEED dispensary (the weed.h counter and its menu) between the MOON MARKET and the LOW-G ARCADE, the
//                 STAR LOUNGE on the end; in the middle the spaceport desk (its lunar menu) with the lander behind it, a food court, planters, vending machines,
//                 and the stairs up on the right.
//   SECOND FLOOR  a gallery round an open atrium (you look down on the concourse): the MOON ARCHIVE, the LUNARWEED LOUNGE (couches, a bong, a hookah) and the MOON GYM.
static void lnPut(int x,int y,char c,const char*look){ if(x<=0||y<=0||x>=MW-1||y>=MH-1||lifeMap[y][x]!='.') return; gPut(x,y,c); if(look) wallMap[y][x]=(u8)catFind(look); }
static void lnUnit(int a,int y0,int b,int y1,int fl,int wp){ gRoom(a,y0,b,y1,fl,wp); gPut((a+b)/2,y1,'D'); }   // a shop: a walled room with its door in the front wall
static void lnrUpper(int rx0,int ry0,int rx1,int ry1){   // the second floor (live while this runs)
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ lifeMap[y][x]='.'; floorMap[y][x]=FL_AIR; wallMap[y][x]=0; }
    gRoom(rx0,ry0,rx1,ry1,2,5);
    int uy0=ry0, uy1=ry0+7, a0=rx0, a1=rx0+11, a2=rx0+22, a3=rx1;   // three shops along the back
    lnUnit(a0,uy0,a1,uy1,2,6); lnUnit(a1,uy0,a2,uy1,4,11); lnUnit(a2,uy0,a3,uy1,3,5);
    for(int y=ry0+12;y<=ry0+20;y++) for(int x=rx0+9;x<=rx1-9;x++) floorMap[y][x]=FL_AIR;   // the atrium: nothing under your feet, the concourse far below
    gLine(rx0+8,ry0+11,rx1-8,ry0+11,'w',13); gLine(rx0+8,ry0+21,rx1-8,ry0+21,'w',13); gLine(rx0+8,ry0+11,rx0+8,ry0+21,'w',13); gLine(rx1-8,ry0+11,rx1-8,ry0+21,'w',13);   // its railing
    gPut(rx1-3,ry0+12,'~'); floorMap[ry0+12][rx1-3]=2;   // the stairs down, over the ones below
    for(int x=a0+1;x<a1;x+=2) lnPut(x,uy0+1,'b',(x&2)?"TALL BOOKSHELF":"WHITE BOOKSHELF");   // the MOON ARCHIVE
    lnPut(a0+1,uy0+4,'C',"GREY COUCH"); lnPut(a0+1,uy0+5,'C',"GREY COUCH"); lnPut(a1-1,uy0+4,'l',"GREEN LAVA LAMP"); lnPut(a0+4,uy0+5,'h',"BAR STOOL");
    lnPut((a1+a2)/2,uy0+1,'l',"NEON BORE SIGN"); lnPut(a1+1,uy0+1,'l',"GREEN LAVA LAMP"); lnPut(a2-1,uy0+1,'l',"PINK LAVA LAMP");   // the LUNARWEED LOUNGE
    lnPut(a1+1,uy0+3,'C',"VELVET SOFA"); lnPut(a1+1,uy0+4,'C',"VELVET SOFA"); lnPut(a1+3,uy0+3,'G',"BONG"); lnPut(a2-1,uy0+4,'C',"GREY COUCH"); lnPut(a2-1,uy0+5,'C',"GREY COUCH");
    lnPut(a2-3,uy0+4,'G',"HOOKAH"); lnPut((a1+a2)/2-2,uy0+5,'h',"BAR STOOL"); lnPut((a1+a2)/2+2,uy0+5,'h',"BAR STOOL"); lnPut(a1+1,uy0+6,'p',"POTTED PALM");
    for(int x=a2+2;x<a3-1;x+=2) lnPut(x,uy0+1,'m',"PUNCHING BAG");   // the MOON GYM
    lnPut(a2+1,uy0+5,'l',"TOTEM POLE"); lnPut(a3-1,uy0+5,'l',"TOTEM POLE");
    for(int x=rx0+2;x<=rx1-2;x+=7){ lnPut(x,ry0+9,'Z',0); lnPut(x,ry1-2,'Z',0); }   // planters on the gallery
    lnPut(rx0+2,ry0+14,'C',"VELVET SOFA"); lnPut(rx0+2,ry0+15,'C',"VELVET SOFA"); lnPut(rx1-2,ry0+16,'C',"GREY COUCH"); lnPut(rx1-2,ry0+17,'C',"GREY COUCH");
    lnPut(rx0+3,ry0+19,'p',"POTTED PALM"); lnPut(rx1-3,ry0+20,'p',"FICUS");
}
static void lnrBuild(int x0,int y0,int x1,int y1){
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3;
    gBox(x0,y0,x1,y1,7); gRoom(rx0,ry0,rx1,ry1,2,5); gPut(cx,ry1,'D');   // the dome: the concourse hall, its door at the front
    int u0=rx0, u1=rx0+8, u2=rx0+16, u3=rx0+24, uy0=ry0, uy1=ry0+7;      // four shops along the back, sharing walls
    dispBuild(u1-1,uy0-1,u2+1,uy1+3);                                    // LUNARWEED: the dispensary (weed.h), the counter and its jars; its walls are the shops' next to it
    lnUnit(u0,uy0,u1,uy1,2,6); lnUnit(u2,uy0,u3,uy1,3,11); lnUnit(u3,uy0,rx1,uy1,4,5);
    for(int x=u0+1;x<u1;x+=2) lnPut(x,uy0+1,'b',(x&2)?"TALL BOOKSHELF":"WHITE BOOKSHELF");   // the MOON MARKET: shelves, vending machines, a few stools
    lnPut(u0+1,uy0+3,'F',"VENDING MACHINE"); lnPut(u0+1,uy0+4,'F',"VENDING MACHINE"); lnPut(u1-1,uy0+3,'F',"VENDING MACHINE"); lnPut(u0+3,uy0+5,'h',"BAR STOOL"); lnPut(u0+5,uy0+5,'h',"BAR STOOL");
    for(int x=u2+1;x<u3;x++) lnPut(x,uy0+1,'v',(x&1)?"ARCADE CABINET":"PINBALL");           // the LOW-G ARCADE
    for(int x=u2+1;x<u3;x+=2) lnPut(x,uy0+3,'v',"ARCADE CABINET"); lnPut(u3-1,uy0+5,'v',"CONSOLE AND TV"); lnPut(u2+1,uy0+5,'v',"CONSOLE AND TV");
    lnPut(u3+1,uy0+1,'l',"PINK LAVA LAMP"); lnPut(rx1-1,uy0+1,'l',"GREEN LAVA LAMP");               // the STAR LOUNGE
    for(int y=uy0+3;y<=uy0+4;y++){ lnPut(u3+1,y,'C',"VELVET SOFA"); lnPut(rx1-1,y,'C',"GREY COUCH"); }
    lnPut(u3+4,uy0+3,'G',"HOOKAH"); lnPut(u3+3,uy0+5,'h',"BAR STOOL"); lnPut(u3+5,uy0+5,'h',"BAR STOOL"); lnPut(rx1-1,uy0+6,'p',"POTTED PALM");
    int dy=ry1-7; for(int x=cx-2;x<=cx+2;x++) gPut(x,dy,'@');   // the spaceport desk in the middle of the concourse: its lunar menu, the lander behind it
    for(int x=rx0+3;x<=rx1-3;x+=6){ lnPut(x,uy1+2,'Z',0); lnPut(x+2,ry1-1,'Z',0); }              // planters down the concourse
    lnPut(rx0+1,ry0+13,'F',"VENDING MACHINE"); lnPut(rx1-1,ry0+17,'F',"VENDING MACHINE");
    for(int i=0;i<2;i++){ lnPut(rx0+4,ry0+14+i*4,'K',0); lnPut(rx0+3,ry0+14+i*4,'N',0); lnPut(rx1-4,ry0+14+i*4,'K',0); lnPut(rx1-3,ry0+14+i*4,'N',0); }   // the food court
    gPut(rx1-3,ry0+12,'^'); floorMap[ry0+12][rx1-3]=2;           // the stairs up
    if(flGoF(1,0)){ lnrUpper(rx0,ry0,rx1,ry1); if(!flGoF(0,0)) flGoF(0,1); }   // the second floor; (if it did not fit in the floor store the stairs lead to a blank one)
    else gPut(rx1-3,ry0+12,'.');
}

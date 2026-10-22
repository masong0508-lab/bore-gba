// lunrado.h - LUNRADO: the moonbase. The SPACEPORT's A SEAT ON THE ROCKET now flies you to it (space.h exLaunch, after the orbit).
//   THE APPROACH   the ship is damaged: three systems inside (FUEL VALVE, GUIDANCE, LANDING GEAR: stop the cursor in the green; LOGIC is your
//                  mechanical skill: a wider zone a level) and the panels outside (the spacewalk). Each fix, inside or out, makes the landing safer.
//   THE LANDING    smooth, HARD (hurts: health, energy, comfort) or the ship EXPLODES. boom % = 4 + 9 a part left broken - 2 a LOGIC level (3 to 70),
//                  hard % = 50 - 7 a part fixed - 3 a LOGIC level. An explosion sends you home, SHUTS THE SPACEPORT (Town.pad[2] bit 0, saved with the town)
//                  and the spaceport counter offers FUND THE SPACEPORT (300000) to open it again.
//   THE PLACE      the moonbase is a lot of the town (a SPACEPORT-kind lot named LUNRADO, made on the first landing; the cab never goes there). Its counter:
//                  LUNAR RATIONS, LOW GRAVITY BOUNCE, MOONBASE SHIFT and RETURN TO EARTH (a safe flight home to the spaceport).
#define LN_CT 250        // trvFind's pseudo kind for the moonbase lot
#define LN_FUND 300000
static int lnLot(const NbLot*L){ return L->on&&L->kind==LKIND_COMM&&L->type==CT_SPACE&&L->name[0]=='L'&&L->name[1]=='U'&&L->name[2]=='N'; }
static int lnrHere(void){ return nbOk&&nbT.cur<NB_LOTS&&lnLot(&nbT.lot[nbT.cur]); }
static int lnrShut(void){ return nbOk&&(nbT.pad[2]&1); }
static void lnrSetShut(int v){ if(!nbOk) return; if(v) nbT.pad[2]|=1; else nbT.pad[2]&=(u8)~1; nbSave(); }
static int lnrLotGet(void){   // the moonbase lot, made once there is a landing
    if(!nbOk) return -1;
    for(int i=0;i<NB_LOTS;i++) if(lnLot(&nbT.lot[i])) return i;
    int fi=-1; for(int i=0;i<NB_LOTS&&fi<0;i++) if(!nbT.lot[i].on) fi=i;
    if(fi>=0) for(int sz=4;sz>=3;sz--) for(int y=0;y+sz<=NB_H;y++) for(int x=0;x+sz<=NB_W;x++){
        int ok=1; for(int yy=y;yy<y+sz&&ok;yy++) for(int xx=x;xx<x+sz;xx++) if(nbAt(xx,yy)>=0||NB_GR(nbT.cell[yy][xx])==NT_WATER){ ok=0; break; }
        if(!ok) continue;
        nbLotAdd(fi,"LUNRADO",x,y,sz,sz,LKIND_COMM,CT_SPACE);
        for(int yy=y;yy<y+sz;yy++) for(int xx=x;xx<x+sz;xx++) nbT.cell[yy][xx]=(u8)NT_PLAZA;
        nbSave(); return fi; }
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

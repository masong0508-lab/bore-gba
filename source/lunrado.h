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
// ---- THE ROCKET'S TANK: trips to LUNRADO. Town.pad[0] bits 2-7 (bits 0 and 1 are the town's own): 0 = a fresh tank (10 trips), else trips left + 1. A paid seat burns one; empty, no seat is sold ----
#define LN_FUEL10 50000
#define LN_FUEL35 250000
#define LN_TANKMAX 60
static char lnFb[44] EWRAM_BSS;
static int lnrFuel(void){ if(!nbOk) return 10; int v=nbT.pad[0]>>2; return v?v-1:10; }
static void lnrSetFuel(int n){ if(!nbOk) return; if(n<0) n=0; if(n>LN_TANKMAX) n=LN_TANKMAX; nbT.pad[0]=(u8)((nbT.pad[0]&3)|((n+1)<<2)); nbSave(); }
static void lnrBurn(void){ int f=lnrFuel(); if(f>0) lnrSetFuel(f-1); }
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
static u8 lwEnc; static u8 lnEv; static const char* const lnEvNm[4]={"A METEOR SHOWER TONIGHT","THE EARTHRISE FESTIVAL IS ON","THE MOON FAIR IS IN TOWN","IT IS LOW GRAVITY WEEK"};
static void lnWait(int frames){ u16 p0=keyNow(); for(int i=0;i<frames;i++){ vsync(); u16 k=keyNow(); if((k&~p0)&(K_A|K_B|K_START)) break; p0=k; } }
static void lnStars(int seed){   // deep space: a faint gradient, a smudge of Milky Way, stars of three brightnesses (a few with a glint)
    for(int y=0;y<SH;y+=2) rect(0,y,SW,2,exMix(RGB(0,0,2),RGB(2,2,9),(y*13)>>7));
    for(int i=0;i<30;i++){ u32 h=swHash(i,seed+40); int x=(int)(h%SW), y=20+x/3+(int)((h>>9)%14); px(x,y,RGB(5,5,12)); px(x+1,y,RGB(4,4,9)); }
    for(int i=0;i<64;i++){ u32 h=swHash(i,seed); int x=(int)(h%SW), y=(int)((h>>8)%SH), b=(int)(h>>20)&3; px(x,y,b==0?RGB(31,31,31):b==1?RGB(22,22,28):RGB(13,13,19));
        if(((h>>24)&15)==0){ u16 g=RGB(11,11,18); px(x-1,y,g); px(x+1,y,g); px(x,y-1,g); px(x,y+1,g); } }
}
static void lnMoon(int y){   // the lunar ground from row y down: a bright ridge, grey fading to shadow, craters with a lit rim and a dark floor, rocks
    int hgt=SH-y; if(hgt<4) return;
    for(int j=0;j<hgt;j+=2) rect(0,y+j,SW,2,exMix(RGB(22,22,24),RGB(8,8,10),j*16/hgt));
    rect(0,y,SW,1,RGB(29,29,30));
    for(int i=0;i<9;i++){ u32 h=swHash(i,5); int cx=(int)(h%SW), cy=y+4+(int)((h>>8)%(hgt>10?hgt-8:3)), r=3+(int)(h&3)+(cy-y)/9;
        disc(cx,cy,r,RGB(8,8,10)); disc(cx+1,cy+1,r>1?r-1:1,RGB(12,12,14)); rect(cx-r,cy-1,2,2,RGB(27,27,28)); px(cx-r+1,cy-r+2,RGB(27,27,28)); px(cx-r+2,cy-r+1,RGB(27,27,28)); }
    for(int i=0;i<12;i++){ u32 h=swHash(i,23); int rx=(int)(h%SW), ry=y+3+(int)((h>>8)%(hgt>8?hgt-4:2)); rect(rx,ry,3,2,RGB(11,11,13)); px(rx,ry,RGB(26,26,27)); }
}
static void lnEarthSmall(int cx,int cy,int r){   // the Earth far away: a disc of ocean with land, cloud and a dark limb
    disc(cx,cy,r+1,RGB(8,18,31)); disc(cx,cy,r,RGB(3,9,24)); disc(cx-r/3,cy-r/4,r/3,RGB(4,17,7)); disc(cx+r/3,cy+r/3,r/4,RGB(4,17,7)); rect(cx-r/2,cy+1,r,1,RGB(26,27,29)); rect(cx-r/3,cy-r/2,r/2,1,RGB(26,27,29));
    for(int k=0;k<r;k+=2) rect(cx+r/2+k/3,cy-r+k+2,r/3,2,RGB(0,2,8));   // (the night side)
}
static void spLander(int x,int y,int fl){   // the lander: x,y the middle of the cabin. gold-foiled descent stage, cabin with a window, four legs, a thruster flame fl rows long
    u16 g=RGB(24,19,7), m=RGB(26,26,28), d=RGB(13,13,16), hl=RGB(31,31,31);
    line(x-7,y+7,x-13,y+17,d); line(x+7,y+7,x+13,y+17,d); line(x-5,y+8,x-9,y+17,m); line(x+5,y+8,x+9,y+17,m);
    rect(x-16,y+17,6,2,m); rect(x+10,y+17,6,2,m); rect(x-11,y+17,4,2,d); rect(x+7,y+17,4,2,d);
    rect(x-8,y+3,16,6,g); rect(x-6,y+2,12,1,g); rect(x-8,y+3,16,1,RGB(30,26,11)); rect(x-8,y+8,16,1,RGB(14,10,3)); rect(x-4,y+4,2,3,RGB(18,13,4)); rect(x+2,y+4,2,3,RGB(18,13,4));
    rect(x-2,y+9,4,3,d);
    rect(x-6,y-6,12,9,m); rect(x-5,y-8,10,2,m); rect(x-6,y-6,2,9,hl); rect(x+4,y-6,2,9,d); rect(x-3,y-5,6,4,RGB(3,9,22)); px(x-2,y-4,RGB(16,24,31)); rect(x-6,y-1,12,1,RGB(18,18,20));
    line(x+5,y-8,x+9,y-14,m); disc(x+10,y-14,1,hl);
    for(int r=0;r<fl;r++){ int w=3-r*3/(fl+1); if(w<1) w=1; rect(x-w,y+12+r,w*2,1,r<fl/3?RGB(31,31,22):r<fl*2/3?RGB(31,24,6):RGB(31,12,4)); }
}
static void spGlyph(int i,int cx,int cy){   // a picture per system, 28 px wide
    u16 c=RGB(17,29,31), d=RGB(2,3,7);
    if(i==0){ rect(cx-14,cy+2,28,6,RGB(6,12,18)); rect(cx-14,cy+2,28,1,c); rect(cx-14,cy+7,28,1,RGB(3,6,10)); disc(cx,cy-6,8,c); disc(cx,cy-6,6,d); rect(cx-1,cy-14,3,16,c); rect(cx-8,cy-7,17,3,c); }
    else if(i==1){ disc(cx,cy,13,c); disc(cx,cy,11,d); disc(cx,cy,6,c); disc(cx,cy,4,d); rect(cx-14,cy,29,1,c); rect(cx,cy-14,1,29,c); disc(cx,cy,1,RGB(31,12,8)); }
    else { rect(cx-2,cy-13,5,18,c); line(cx-12,cy+11,cx-2,cy+1,c); line(cx+12,cy+11,cx+2,cy+1,c); line(cx-11,cy+11,cx-1,cy+1,c); line(cx+11,cy+11,cx+1,cy+1,c); rect(cx-15,cy+11,11,3,c); rect(cx+5,cy+11,11,3,c); }
}
static void spPanel(int t){   // the cockpit: a dark console, scan lines, a bezel, rivets
    rect(0,0,SW,SH,RGB(11,13,19)); rect(4,4,SW-8,SH-8,RGB(2,3,7));
    for(int y=16;y<SH-5;y+=4) rect(5,y,SW-10,1,RGB(3,4,9));
    for(int k=0;k<4;k++){ int rx=k&1?SW-3:2, ry=k&2?SH-3:2; px(rx,ry,RGB(24,24,26)); px(rx+1,ry,RGB(8,8,10)); }
    (void)t;
}
static int lnConsole(void){   // inside: three systems, stop the cursor in the green. 0..3 fixed
    static const char* const nm[3]={"FUEL VALVE","GUIDANCE","LANDING GEAR"}, * const why[3]={"THE TANK IS VENTING","THE SHIP IS DRIFTING","THE LEGS WILL NOT LOCK"};
    int zw=20+skLvl(SK_LOGIC)*7, fixed=0;
    for(int i=0;i<3;i++){
        int zx=20+rnd8()%(200-zw), x=20, dir=1, sp=2+i, res=-1, acc=0; s16 tr[5]={20,20,20,20,20};
        u16 prev=keyNow(); u32 vb=vblN;
        for(int t=0;res<0&&t<900;t++){
            u16 k=keyNow(), pr=k&~prev; prev=k; int dt=(int)(vblN-vb); vb=vblN; if(dt<1) dt=1; if(dt>6) dt=6;   // (the cursor moves by the clock, not by how long a picture took)
            acc+=sp*dt; { int st=acc/2; acc-=st*2; while(st-->0){ x+=dir; if(x>=220){ x=220; dir=-1; } if(x<=20){ x=20; dir=1; } } }
            for(int q=4;q>0;q--) tr[q]=tr[q-1]; tr[0]=(s16)x;
            if(pr&K_A) res=(x>=zx&&x<=zx+zw);
            if(pr&K_B) res=0;
            spPanel(t); rect(5,5,SW-10,10,((t>>3)&1)?RGB(26,4,3):RGB(13,2,2)); text(10,7,"WARNING  SYSTEM DAMAGE",WHITE,1); disc(SW-12,10,2,((t>>3)&1)?RGB(31,28,8):RGB(12,6,2));
            rect(12,24,36,36,RGB(5,8,14)); rect(12,24,36,1,RGB(14,22,31)); spGlyph(i,30,43);
            text(58,26,nm[i],GOLD,2); text(58,44,why[i],RGB(31,12,8),1); text(58,54,"A  STOP THE CURSOR IN THE GREEN",((t>>4)&1)?RGB(22,31,31):RGB(14,24,28),1);
            int mx=20,my=88,mh=16; rect(mx-4,my-4,208,mh+8,RGB(17,21,30)); rect(mx-2,my-2,204,mh+4,RGB(2,3,7)); rect(mx,my,200,mh,RGB(4,5,10));
            for(int q=0;q<=20;q++) rect(mx+q*10,my+mh+3,1,(q%5==0)?5:3,RGB(12,16,22));
            rect(zx,my,zw,mh,RGB(4,17,7)); rect(zx,my,zw,2,RGB(12,30,14)); rect(zx,my+mh-2,zw,2,RGB(2,10,4)); rect(zx+zw/2,my,1,mh,RGB(10,26,12));
            for(int q=4;q>0;q--) rect(tr[q]-1,my+2,2,mh-4,exMix(RGB(4,5,10),RGB(14,18,26),5-q));
            rect(x-1,my-5,3,mh+10,WHITE); line(x-4,my-7,x+4,my-7,WHITE); line(x-3,my-6,x+3,my-6,WHITE); line(x-4,my+mh+7,x+4,my+mh+7,WHITE);
            for(int j=0;j<3;j++){ disc(96+j*24,SH-18,5,RGB(17,21,30)); disc(96+j*24,SH-18,4,j<fixed?RGB(6,28,10):j==i?((t>>4)&1?RGB(30,24,6):RGB(8,8,10)):RGB(4,5,10)); }
            text(10,SH-21,"REPAIRED",RGB(17,29,31),1); present(); }
        if(res<0) res=0;
        for(int f=0;f<36;f++){ spPanel(f); u16 c=res?RGB(6,28,10):RGB(31,10,8); int w=f*6; rect(120-w,50,w*2,60,exMix(RGB(2,3,7),c,f<14?f:14));
            if(res){ line(96,84,112,98,WHITE); line(97,84,113,98,WHITE); line(112,98,146,64,WHITE); line(113,98,147,64,WHITE); } else { line(100,66,140,96,WHITE); line(101,66,141,96,WHITE); line(140,66,100,96,WHITE); line(141,66,101,96,WHITE); if(f<12) for(int q=0;q<10;q++){ int a=(q*13+f)&63; px(120+exS64[(a+16)&63]*(f*5)/127,80+exS64[a]*(f*3)/127,RGB(31,24,8)); } }
            const char*m=res?"FIXED":"BOTCHED"; text((SW-tw(m,3))/2,20,m,res?RGB(10,31,12):RGB(31,12,8),3); present(); if(f==2){ if(res){ sfxPlay(SFX_STICK); voxPlay(V_yahoo); } else voxPlay(V_nah); } }
        if(res) fixed++; }
    return fixed;
}
static void lnrReturnHome(void){ int h=trvPlace(CT_SPACE); if(h>=0) trvGo(h); }
static void lnrBoom(void){   // the ship explodes: back to Earth, the spaceport is shut until it is funded again
    for(int i=0;i<46;i++){ lnStars(3); int r=i*4, cx=120, cy=88;
        if(i>3){ int rr=r-12; for(int a=0;a<64;a+=1){ int x=cx+rr*exS64[(a+16)&63]/127, y=cy+rr*exS64[a&63]*3/4/127; px(x,y,exMix(RGB(31,31,31),RGB(4,4,10),i*16/46)); px(x+1,y,exMix(RGB(31,26,12),RGB(4,4,10),i*16/46)); } }   // the shock wave
        { int fr=i<24?r*3/4:(46-i)*5/2; if(fr>0){ disc(cx,cy,fr,i<20?RGB(31,26,6):RGB(31,12,3)); disc(cx,cy,fr*2/3,i<14?RGB(31,31,22):RGB(31,20,4)); disc(cx,cy,fr/3,RGB(31,31,31)); } }
        for(int k=0;k<24;k++){ int a=(k*29+7)&63, dist=i*(2+k%4)+(k%5)*3; if(dist<2) continue; int x=cx+dist*exS64[(a+16)&63]/127, y=cy+dist*exS64[a&63]*3/4/127+i*i/40; u16 c=(k&1)?RGB(31,18,5):RGB(18,18,20); rect(x,y,2,2,c); if(k%3==0) px(x-1,y-1,RGB(31,31,22)); }
        present(); if(i==3) sfxPlay(SFX_THUNDER); }
    for(int k=0;k<7;k++){ rect(0,0,SW,SH,exMix(RGB(31,31,31),RGB(0,0,3),k*16/7)); present(); }
    lnStars(9); for(int k=0;k<40;k++){ u32 h=swHash(k,31); rect((int)(h%SW),(int)((h>>8)%SH),2,1,(k&1)?RGB(20,10,4):RGB(14,14,16)); }   // the debris drifting
    exDarken(14,28,212,76,0); rect(14,28,212,1,RGB(31,10,6)); rect(14,103,212,1,RGB(31,10,6));
    text((SW-tw("THE SHIP EXPLODED",2))/2,36,"THE SHIP EXPLODED",RGB(31,10,6),2);
    text((SW-tw("YOU WAKE UP BACK ON EARTH",1))/2,62,"YOU WAKE UP BACK ON EARTH",WHITE,1);
    text((SW-tw("THE SPACEPORT IS SHUT",1))/2,76,"THE SPACEPORT IS SHUT",GOLD,1);
    text((SW-tw("FUND IT AGAIN  300000 SIMOLEONS",1))/2,88,"FUND IT AGAIN  300000 SIMOLEONS",GOLD,1); present(); voxPlay(V_shriek); lnWait(300);
    lnrSetShut(1); lhp=HP_MAX/4; vAdd(&sNrg,-50); vAdd(&sCom,-40); vAdd(&sSoc,-20);
    lnrReturnHome(); toast("YOU LOST THE SHIP  FUND THE SPACEPORT AGAIN");
}
static void lnLanded(int smooth,int t){   // the lander down on the Moon: dust settling, the flag going up after a smooth landing, smoke and sparks after a hard one
    lnStars(1); lnMoon(128); lnEarthSmall(204,30,12);
    int lx=120+(smooth?0:3), ly=110; spLander(lx,ly,0);
    int d=t<40?t:40; for(int k=0;k<14;k++){ int a=(k*11)&63, x=lx+exS64[(a+16)&63]*(d+8)/127, y=128-(d*(k%3+1))/6; if(y<100) continue; disc(x,y,2+(40-d)/12,exMix(RGB(14,14,15),RGB(8,8,10),d*16/40)); }
    if(smooth){ int fh=t<30?t:30; rect(168,127-fh,1,fh,RGB(24,24,26)); if(fh>=30){ for(int q=0;q<12;q++) rect(169+q,97+exSn(t*2+q*8,48)/70,1,7,q<6?RGB(30,5,4):RGB(30,30,31)); } }
    else { for(int k=0;k<6;k++){ int sx=lx-4+(k*5)%12, sy=ly-8-((t+k*9)%30); disc(sx,sy,2+(t+k)%3,RGB(6,6,8)); } if((t&3)==0) for(int q=0;q<4;q++) px(lx-8+rnd8()%16,ly+4+rnd8()%12,RGB(31,26,8)); }
}
// called by exLaunch after the orbit. orbitOk: the orbit repair worked (a head start)
static void lnrApproach(int orbitOk){
    for(int i=0;i<240;i++) simMinute();   // (six hours to the moon)
    static const char* const lg[5]={"T+6:00  LUNAR ORBIT","ALARMS ARE GOING OFF","FUEL  GUIDANCE  GEAR  PANELS","REPAIR THE SHIP","LOGIC IS YOUR MECHANICAL SKILL"};
    lnStars(1); lnEarthSmall(30,34,15);
    { int cy=SH+200-70; disc(120,cy,203,RGB(6,6,9)); disc(120,cy,201,RGB(23,23,25)); disc(120,cy,199,RGB(16,16,18));
      for(int k=0;k<16;k++){ u32 h=swHash(k,41); int bx=(int)(h%SW), by=SH-70+6+(int)((h>>8)%58), r=3+(int)(h&3); int dx=bx-120, dy=by-cy; if(dx*dx+dy*dy<192*192){ disc(bx,by,r,RGB(10,10,12)); disc(bx+1,by+1,r>1?r-1:1,RGB(13,13,15)); rect(bx-r,by-1,2,2,RGB(26,26,27)); } } }
    for(int k=0;k<10;k++){ u32 h=swHash(k,77); rect(150+(int)(h%80),SH-62+(int)((h>>8)%60),2,2,RGB(3,3,5)); }
    exDarken(70,6,164,70,6); rect(70,6,164,1,RGB(14,26,31)); rect(70,75,164,1,RGB(14,26,31));
    text(76,10,"MISSION LOG",RGB(17,29,31),1);
    u16 p0=keyNow(); int shown=0, tot=0; for(int i=0;i<5;i++){ int l=0; while(lg[i][l]) l++; tot+=l; }
    for(int f=0;f<300;f++){
        if(shown<tot) shown+=1;
        int done=0; for(int i=0;i<5;i++){ int l=0; while(lg[i][l]) l++; int left=shown-done; if(left<=0) break; char b[40]; int m=left<l?left:l; for(int q=0;q<m;q++) b[q]=lg[i][q]; b[m]=0;
            rect(76,22+i*10,154,9,exMix(RGB(1,2,5),RGB(1,2,5),0)); text(76,22+i*10,b,i==1?RGB(31,10,8):i==3?GOLD:WHITE,1); done+=l; }
        disc(SW-14,12,3,((f>>3)&1)?RGB(31,10,8):RGB(9,3,3)); disc(SW-14,12,1,((f>>3)&1)?RGB(31,28,22):RGB(9,3,3));
        present(); u16 k=keyNow(); if(f>30&&((k&~p0)&(K_A|K_B|K_START))) break; p0=k; }
    int fixed=lnConsole();
    swDone=0; exSpacewalk(1); fixed+=swDone;   // 0..6
    if(orbitOk&&fixed<6) fixed++;
    int lg2=skLvl(SK_LOGIC), boom=4+(6-fixed)*9-lg2*2, hard=50-fixed*7-lg2*3;
    if(boom<3) boom=3; if(boom>70) boom=70; if(hard<0) hard=0;
    int r=rnd8()*100>>8, res=r<boom?2:r<boom+hard?1:0;
    u32 vb=vblN; int y=10;
    while(y<=112){   // the descent: the Moon coming up, the thruster working, dust kicked up near the ground
        int dt=(int)(vblN-vb); vb=vblN; if(dt<1) dt=1; if(dt>6) dt=6; y+=dt;   // (by the clock: about 2 rows a frame at 60 pictures a second)
        lnStars(1); lnMoon(128); lnEarthSmall(204,30,12);
        int sh=(res==1&&y>90)?(int)(rnd8()&3)-1:0, fl=y<104?5+(int)(rnd8()&5):2+(int)(rnd8()&3);
        spLander(120+sh,y,fl);
        if(y>84){ int dd=y-84; for(int q=0;q<12;q++){ int a=(q*5)&63, x=120+exS64[(a+16)&63]*dd/20+(int)(rnd8()&7)-3, yy=127-(int)(rnd8()&(dd>>1)); if(yy<110) continue; px(x,yy,RGB(18,18,20)); px(x+1,yy,RGB(14,14,16)); } }
        { char b[24]; char*e=slCat(b,"PARTS FIXED "); e=slNum(e,fixed); slCat(e," OF 6"); text(6,4,b,GOLD,1); e=slCat(b,"ALT "); e=slNum(e,(128-(y+18))*5>0?(128-(y+18))*5:0); slCat(e," M"); text(SW-6-tw(b,1),4,b,RGB(17,29,31),1); }
        if(res==2&&y>=60) break; present(); }
    if(res==2){ lnrBoom(); return; }
    stQEvent(QE_LAUNCH,1);
    if(res==1){ int d=30+(6-fixed)*7; hpLose(d); vAdd(&sNrg,-40); vAdd(&sCom,-35); vAdd(&lfood,-10); voxPlay(V_shriek); sfxPlay(SFX_THUNDER);
        for(int f=0;f<60;f++){ lnLanded(0,f); if(f>=8){ exDarken(20,6,200,44,0); rect(20,6,200,1,RGB(31,12,6)); text((SW-tw("HARD LANDING",2))/2,12,"HARD LANDING",RGB(31,12,6),2); text((SW-tw("THAT HURT  YOUR BODY TOOK IT",1))/2,38,"THAT HURT  YOUR BODY TOOK IT",WHITE,1); } present(); }
        lnWait(160); }
    else { moodEvent(M_PROMO); voxPlay(V_yahoo);
        for(int f=0;f<60;f++){ lnLanded(1,f); if(f>=8){ exDarken(20,6,200,44,0); rect(20,6,200,1,RGB(10,30,12)); text((SW-tw("SMOOTH LANDING",2))/2,12,"SMOOTH LANDING",RGB(10,30,12),2); text((SW-tw("WELCOME TO LUNRADO",1))/2,38,"WELCOME TO LUNRADO",WHITE,1); } present(); }
        lnWait(160); }
    int l=lnrLotGet(); if(l>=0){ NbLot*L=&nbT.lot[l]; if(L->slot>=0) slDelete(L->slot); L->slot=-1; }   // (every landing builds the resort again: three pods drawn by luck)
    lnEv=(u8)(rnd8()&3); if(l>=0&&trvGo(l)){ toast(lnEvNm[lnEv]); if((rnd8()&15)==0) lwEnc=1; }
}
static int lnrCanDo(int sp){   // venueDo asks before it takes the money
    if(sp==VS_LAUNCH&&!lnrShut()&&!exSeatFree()&&lnrFuel()<=0){ lnote="OUT OF FUEL  REFUEL AT THE COUNTER"; lnoteT=110; voxPlay(V_nah); return 0; }   // (a seat the mission booked needs none)
    if((sp==VS_SYSJOB||sp==VS_SATJOB)&&stage<AG_TEEN){ lnote="KIDS WAIT OUTSIDE"; lnoteT=60; voxPlay(V_nah); return 0; }
    if((sp==VS_SYSJOB||sp==VS_SATJOB)&&sNrg<20){ lnote="TOO TIRED  EAT OR REST FIRST"; lnoteT=80; voxPlay(V_nah); return 0; }
    if(sp==VS_REFUEL10||sp==VS_REFUEL35){ int c=sp==VS_REFUEL10?LN_FUEL10:LN_FUEL35;
        if(lnrFuel()>=LN_TANKMAX){ lnote="THE TANK IS FULL"; lnoteT=70; return 0; }
        if(simMoney<c){ lnote=sp==VS_REFUEL10?"YOU NEED 50000 SIMOLEONS":"YOU NEED 250000 SIMOLEONS"; lnoteT=80; voxPlay(V_nah); return 0; } }
    if(sp==VS_LAUNCH&&lnrShut()){ lnote="THE SPACEPORT IS SHUT  FUND IT AGAIN"; lnoteT=100; voxPlay(V_nah); return 0; }
    if(sp==VS_FUND){ if(!lnrShut()){ lnote="THE SPACEPORT IS ALREADY OPEN"; lnoteT=70; return 0; } if(simMoney<LN_FUND){ lnote="YOU NEED 300000 SIMOLEONS"; lnoteT=80; voxPlay(V_nah); return 0; } }
    return 1;
}
static void lnrDo(int sp){
    if(sp==VS_SYSJOB){ exTake=EXT_SYSJOB; lnote="TO THE CONSOLE"; lnoteT=60; }
    else if(sp==VS_SATJOB){ exTake=EXT_SATJOB; lnote="SUIT UP"; lnoteT=60; }
    else if(sp==VS_GAUGE){ int f=lnrFuel(); char*e=simCat(lnFb,"THE TANK HOLDS  "); e=simCatN(e,f); simCat(e,f==1?" TRIP":" TRIPS"); lnote=lnFb; lnoteT=110; }
    else if(sp==VS_REFUEL10||sp==VS_REFUEL35){ int c=sp==VS_REFUEL10?LN_FUEL10:LN_FUEL35, n=sp==VS_REFUEL10?10:35;
        simMoney-=c; lnrSetFuel(lnrFuel()+n); sfxPlay(SFX_CASH); voxPlay(V_yahoo); moodEvent(M_PAY);
        char*e=simCat(lnFb,"REFUELLED  "); e=simCatN(e,lnrFuel()); simCat(e," TRIPS IN THE TANK"); lnote=lnFb; lnoteT=130; }
    else if(sp==VS_FUND){ simMoney-=LN_FUND; lnrSetShut(0); lnote="THE SPACEPORT IS OPEN AGAIN"; lnoteT=120; voxPlay(V_yahoo); moodEvent(M_PROMO); }
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
// ---- LUNAR WEED: bought at LUNARWEED, taken a hit at a time ----
//   A HIT        a little more of every bar (food, energy, hygiene, comfort, social, fun, happy). Twenty hits in one session (each hit keeps the high going for an hour)
//                and you are PLATINUM (the aspiration meter pinned up) for as long as you keep it up.
//   THE BILL     every fifth hit takes days off your life, and the tolerance grows with every hit you ever took: 1 day a block at first, one more every 15 hits.
//                The days come out of your age (the days set on OPTIONS > TIME > AGES; a stage that lasts for ever or an AGING that is OFF counts 30 days) and the stages go by:
//                elder, then 14 days and you die of smoke and moon dust.
//   THE SEED     very rarely (landing; or walking about the base) a stranger offers a lunar seed (very, very rarely two). Plant it in a GROW PLANTER (weed.h, stage 6),
//                30 days later it is ready and pays 50000.
//   SAVED        in the town record: Town.spare + the free bits of pad[1] and pad[2] hold 22 bits: hits (7), hits in this block (3), hits in your pocket (5),
//                seeds (2), the plant's days (5). (neighborhood.h nbPack keeps `spare`.)
#define LW_ELDER 14
static u8 lwSess, lwPlat; static short lwHigh; static u32 lwFb;
static u32 lwRd(void){ if(!nbOk) return lwFb; return (u32)nbT.spare|((u32)(nbT.pad[1]>>1)<<8)|((u32)(nbT.pad[2]>>1)<<15); }
static void lwWr(u32 v){ if(!nbOk){ lwFb=v; return; } nbT.spare=(u8)v; nbT.pad[1]=(u8)((nbT.pad[1]&1)|(((v>>8)&127)<<1)); nbT.pad[2]=(u8)((nbT.pad[2]&1)|(((v>>15)&127)<<1)); nbSave(); }
static int lwG(u32 v,int o,int n){ return (int)((v>>o)&((1u<<n)-1)); }
static u32 lwP(u32 v,int o,int n,int x){ u32 m=((1u<<n)-1)<<o; return (v&~m)|(((u32)x<<o)&m); }
static int lwSeedsN(void){ return lwG(lwRd(),15,2); }
static int lwStamp(void){ return (((int)simDay>>1)%31)+1; }   // the pot keeps the day it was planted in 2 day steps (1..31; 0 = a plant from the old one-counter version)
static int lwUnits(int b){ int s=b>>3; if(!s) return 0; return ((((int)simDay>>1)%31)-(s-1)+31)%31; }   // 2 day steps since planted; 15 = ready (30 days)
static int lwDaysOf(int b){ return (b&7)==7?31:lwUnits(b)>=15?31:lwUnits(b)*2+1; }   // 1 planted .. 31 ready, from the pot's byte
static void lwLock(void){   // AUTOMATIC: every game minute and on every room load, each lunar pot on the live floor: a ready plant locks as ready (stage 7); an old-version plant takes its age from the old counter
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){ if(lifeMap[y][x]!='u') continue; int b=wallMap[y][x], st=b&7; if(st!=6) continue;
        if(!(b>>3)){ u32 v=lwRd(); int p=lwG(v,17,5); int u=p>1?(p-1)/2:0; if(p) lwWr(lwP(v,17,5,0)); wallMap[y][x]=(u8)(6|((((((int)simDay>>1)%31)-u+31)%31+1)<<3)); continue; }
        if(lwUnits(b)>=15) wallMap[y][x]=7; } }

static int lwNeed(int st){ static const u8 pct[4]={0,200,100,50}; int d=oStageDays(st); if(!d||!xo[XO_AGING]) return 30; int n=d*pct[xo[XO_AGING]]/100; return n<1?1:n; }
static int lwDaysLeft(void){ if(stage>=AG_ELDER) return LW_ELDER-(int)ageDays; int d=lwNeed(stage)-(int)ageDays; for(int s=stage+1;s<AG_ELDER;s++) d+=lwNeed(s); return d+LW_ELDER; }
static char lwMsg[72] EWRAM_BSS;
static void lwLose(int n){   // the days come out of your age; the stages go by; at the end of the elder's days: dead
    int dead=0;
    for(int i=0;i<n&&!dead;i++){
        if(gGrow) break;
        if(stage>=AG_ELDER){ if(ageDays<250) ageDays++; if(ageDays>=LW_ELDER) dead=1; }
        else { ageDays++; if(ageDays>=lwNeed(stage)){ gGrow=1; simQueue("MOON DUST AGED YOU A STAGE"); } } }
    ageSave();
    if(dead){ static const char* const dn[4]={"DIED OF SMOKE AND MOON DUST  THE MOON SENDS AN INVOICE","YOUR LUNGS FILED FOR DIVORCE  YOU LOST","TOO MANY HITS  THE MOON KEEPS THE REST","BREATHED IN THE WHOLE MOON  RIP"};
        die(SFX_DEATH,0); lnote=dn[rnd8()&3]; lnoteT=0x7fff; }
}
static void lwHit(void){
    u32 v=lwRd(); int stock=lwG(v,10,5);
    if(stock<=0){ lnote="NO LUNAR WEED LEFT"; lnoteT=60; return; }
    int tier=lwG(v,0,7), blk=lwG(v,7,3); stock--; if(tier<127) tier++; blk++;
    int lose=0; if(blk>=5){ blk=0; lose=1+tier/15; }
    v=lwP(v,0,7,tier); v=lwP(v,7,3,blk); v=lwP(v,10,5,stock); lwWr(v);
    vAdd(&lfood,2); vAdd(&sNrg,2); vAdd(&sHyg,2); vAdd(&sCom,2); vAdd(&sSoc,2); moodFun=moodClamp(moodFun+2*MOOD_ONE); moodHap=moodClamp(moodHap+2*MOOD_ONE);   // very slightly more of every bar
    lchill=900; lstun=70; lsp=0; lgrind=0; voxChain(V_lighter_spark,V_spark_inhale,(rnd8()&1)?V_after_smoke_cough:V_after_smoke_cough_2);
    if(lwSess<250) lwSess++; lwHigh=60;
    if(lwSess>=20&&!lwPlat){ lwPlat=1; simQueue("PLATINUM  THE MOON LOVES YOU  KEEP GOING"); }
    static const char* const fl[6]={"A LUNAR HIT  TASTES LIKE A COIN","A LUNAR HIT  THE DUST TICKLES","A LUNAR HIT  YOU CAN SEE EARTH FROM HERE","A LUNAR HIT  GRAVITY IS A SUGGESTION","A LUNAR HIT  YOU COUGH IN SLOW MOTION","A LUNAR HIT  THE STARS SAY HI"};
    lnote=fl[rnd8()%6]; lnoteT=90;
    if(lose){ static const char* const tk[6]={"THE MOON TAKES A DAY  AS A TIP","A DAY OF YOUR LIFE BLEW OUT INTO THE VOID","MOON DUST CHARGES RENT  IN DAYS","YOUR LUNGS FILE A COMPLAINT  IN DUST","SOMEONE ON EARTH JUST GOT YOUR DAYS","THE DUST AND THE SMOKE ARE FRIENDS NOW"};
        char*e=simCat(lwMsg,tk[rnd8()%6]); e=simCat(e,"  -"); e=simCatN(e,lose); e=simCat(e,lose>1?" DAYS  ":" DAY  "); e=simCatN(e,lwDaysLeft()-lose>0?lwDaysLeft()-lose:0); simCat(e," LEFT"); lwLose(lose); if(!ldead){ lnote=lwMsg; lnoteT=150; } }
}
static void lwMinute(void){   // every game minute (weed.h wdHour)
    if(lwHigh>0&&--lwHigh==0){ lwSess=0; if(lwPlat){ lwPlat=0; simQueue("THE HIGH FADES  THE MOON WANTS YOU BACK"); } }
    if(lwPlat&&simMeter<920) simMeter=920;   // PLATINUM while you keep it up
    if(lnrHere()&&!lwEnc&&((rnd8()<<8|rnd8())%700)==0) lwEnc=1;   // a stranger, now and then
    lwLock();   // the pots keep their own clocks: a ready plant locks as ready, on any floor you are on, at once
}
static void lwEncounter(void){
    static const char* const who[4]={"A TOURIST IN A TOO BIG SPACE SUIT","A MAN WHO SAYS HE IS THE MOON MAYOR","A KID WHO SHOULD BE ON EARTH","A ROBOT PRETENDING TO BE A TOURIST"};
    static const char* const ask[3]={"TAKE THE SEED","NO THANKS","WHAT IS IT"};
    for(;;){
        int c=menu(who[(int)(simDay+simMin)&3],ask,3); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
        if(c==2){ static const char* const w[3]={"PSST  A LUNAR SEED  30 DAYS  50 GRAND","THEY SAY IT GROWS ON EARTH TOO","DO NOT TELL THE MAYOR  OR DO"}; toast(w[rnd8()%3]); continue; }
        if(c==0){ u32 v=lwRd(); int s=lwG(v,15,2);
            if(s>=3){ toast("YOUR POCKETS ARE FULL OF SEEDS"); return; }
            int g=(rnd8()<20)?2:1; s=s+g>3?3:s+g; lwWr(lwP(v,15,2,s)); voxPlay(V_yahoo); lnote=g==2?"TWO SEEDS  THEY MUST LIKE YOU":"YOU GOT A LUNAR SEED"; lnoteT=140; }
        return; }
}
static void lwTick(void){ if(lwEnc&&lstun<=0&&!ldead){ lwEnc=0; lwEncounter(); } }   // every logic step (weed.h wdTick)
static void lwDrawPlant(int sx,int sy,int d,int t){   // the lunar plant in its pot: it grows taller with the days, blue-green, glowing when ready
    int h=3+d*21/31, top=sy-7-h;
    rect(sx,top,1,h,RGB(4,16,18));
    for(int k=0,y=sy-10;y>top+2;k++,y-=5){ int side=(k&1)?1:-1; wdFan(sx+side*3,y,y>top+8,(k&1)?RGB(8,24,26):RGB(5,18,22),RGB(3,12,16)); }
    if(d>=31){ rect(sx-1,top-3,3,3,((t>>1)&1)?RGB(20,31,31):RGB(9,28,31)); if((t>>2)&1) px(sx+3,top-5,RGB(24,31,31)); }
    else if(d>=20) rect(sx,top-1,1,2,RGB(12,28,30));
}
static void lwPlanter(int x,int y){   // R next to a planter that holds the lunar plant
    int d=lwDaysOf(wallMap[y][x]), ready=d>=31; if(ready) wallMap[y][x]=7;
    char tl[40]; { char*e=simCat(tl,"LUNAR PLANT  DAY "); e=simCatN(e,d>30?30:d>0?d-1:0); simCat(e," OF 30"); }
    const char* it[3]; int id[3], n=0;
    if(ready){ it[n]="HARVEST AND SELL  \xC2\xA7" "50000"; id[n++]=0; } else { it[n]="CHECK ON IT"; id[n++]=1; }
    it[n]="TALK TO IT"; id[n++]=2;
    int c=menu(tl,it,n); liveInvalidate(); camSnap=1; if(c<0) return;
    lstun=60; lsp=0; lgrind=0;
    if(id[c]==0){ simMoneyAdd(50000); wpSet(x,y,WP_EMPTY,0,0); moodEvent(M_PAY); voxPlay(V_yahoo); lnote="THE MOON MAYOR CRIES  YOU SELL IT FOR 50000"; lnoteT=170; }
    else if(id[c]==1){ static const char* const w[3]={"IT HUMS  QUIETLY","THE LEAVES POINT AT EARTH","NOT YET  PATIENCE IS A VIRTUE"}; lnote=w[rnd8()%3]; lnoteT=100; }
    else { static const char* const w[4]={"YOU TELL IT ABOUT THE DUST","IT SEEMS TO LIKE ZERO GRAVITY","YOU CALL IT A GOOD MOON GIRL","IT GROWS A LITTLE PROUDER  SOMEHOW"}; lnote=w[rnd8()&3]; lnoteT=110; moodEvent(M_CHILL); }
}
static void lwPlantSeed(int x,int y){
    u32 v=lwRd(); int s=lwG(v,15,2); if(s<=0){ toast("NO LUNAR SEED"); return; }
    wallMap[y][x]=(u8)(6|(lwStamp()<<3)); lwWr(lwP(v,15,2,s-1)); lnote="PLANTED  30 DAYS  DO NOT FORGET IT"; lnoteT=140; lstun=50;
}
static void lwCounter(void){   // R at the LUNARWEED counter
    if(stage<AG_ADULT){ lnote="21 AND UP  THIS IS THE MOON  NOT A PLAYGROUND"; lnoteT=110; voxPlay(V_disagree); return; }
    for(;;){
        char tt[40]; char lb[3][36]; const char* it[7]; int id[7], n=0; int stock=lwG(lwRd(),10,5);
        char*e=simCat(tt,"LUNARWEED  \xC2\xA7"); simCatMoney(e,simMoney,1);
        it[n]="LUNAR PRE-ROLL  \xC2\xA7" "12  3 HITS"; id[n++]=0;
        it[n]="LUNAR EIGHTH  \xC2\xA7" "40  10 HITS"; id[n++]=1;
        e=simCat(lb[0],"TAKE A HIT  "); e=simCatN(e,stock); simCat(e," LEFT"); it[n]=lb[0]; id[n++]=2;
        it[n]="ROLL AND SMOKE HERE  \xC2\xA7" "6"; id[n++]=3;
        it[n]="CHECK YOUR LUNGS"; id[n++]=4;
        it[n]="THE EARTH MENU"; id[n++]=5;
        it[n]="ASK THE BUDTENDER"; id[n++]=6;
        int c=menu(tt,it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
        if(c<0) return;
        int k=id[c];
        if(k<=1||k==3){ int price=k==0?12:k==1?40:6, hits=k==0?3:k==1?10:1;
            if(simMoney<price){ toast("NOT ENOUGH SIMOLEONS"); voxPlay(V_nah); continue; }
            if(stock+hits>31){ toast("YOUR POCKETS CANNOT HOLD MORE"); continue; }
            simMoney-=price; u32 v=lwRd(); lwWr(lwP(v,10,5,stock+hits)); sfxPlay(SFX_CASH);
            if(k==3){ lwHit(); return; }
            toast(k==0?"A LUNAR PRE-ROLL  3 HITS":"A LUNAR EIGHTH  10 HITS"); }
        else if(k==2){ lwHit(); return; }
        else if(k==4){ u32 v=lwRd(); int tier=lwG(v,0,7), blk=lwG(v,7,3);
            char a[28], b[28], c2[28], d[28]; char*q;
            q=simCat(a,"HITS EVER  "); simCatN(q,tier); q=simCat(b,"DAYS LEFT  "); simCatN(q,lwDaysLeft()>0?lwDaysLeft():0);
            q=simCat(c2,"NEXT BILL IN  "); q=simCatN(q,5-blk); simCat(q," HITS"); q=simCat(d,"THEY TAKE  "); q=simCatN(q,1+tier/15); simCat(q," A TIME");
            const char* ii[4]={a,b,c2,d}; menu("YOUR LUNGS",ii,4); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
        else if(k==5){ wdCounter(); return; }
        else { static const char* const w[6]={"THE MOON HAS NO WEATHER  JUST VIBES","EVERY PUFF IS A SMALL STEP FOR YOU  A GIANT LEAP FOR YOUR LUNGS","NO REFUNDS  NO ATMOSPHERE","A STRANGER MAY OFFER YOU A SEED  SAY YES  THEN SAY NOTHING","30 DAYS AND A PLANT PAYS 50 GRAND  DO NOT TELL THE MAYOR","TOLERANCE IS JUST YOUR LUNGS GETTING TO KNOW THE MOON"}; toast(w[rnd8()%6]); }
    }
}

// ---- MOONBASE JOBS (the counter's SYSTEMS CHECK JOB and SATELLITE REPAIR JOB): repeatable work outside the missions; expand.h exTakeRun hands over the screen ----
//   SYSTEMS CHECK  the three-gauge repair game: 350 a gauge, 300 more for all three.   SATELLITE  the real spacewalk: 500 a panel, 400 more for all three.   Both cost 12 energy.
static void lnJobRun(int t){
    int pay=0;
    if(t==EXT_SYSJOB){ int f=lnConsole(); pay=f*350+(f==3?300:0); if(f>0) skGain(SK_LOGIC,1); }
    else { swDone=0; int ok=exSpacewalk(1); pay=swDone*500+(ok?400:0); }
    vAdd(&sNrg,-12);
    if(pay>0){ simMoneyAdd(pay); moodEvent(M_PAY); char*e=simCat(lnFb,"JOB DONE  \xC2\xA7"); simCatN(e,pay); toast(lnFb); }
    else toast("NO PAY  THE MOON WANTS RESULTS");
    simsSave();
}

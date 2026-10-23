// space.h - BORE-OUT MAN!'s two screens (expand.h takes the life game over for them):
//   THE LAUNCH     the SPACEPORT's A SEAT ON THE ROCKET: the pad, the gantry, a countdown, LIFTOFF (smoke, a shaking screen, the sky going
//                  black as you climb), orbit. Then the spacewalk for real, then splashdown (four hours went by).
//   THE SPACEWALK  you float by the station with the Earth below: the pad fires your thrusters (you drift: there is nothing to stop you), hold A
//                  next to a broken panel to fix it. Fix them all before the oxygen runs out. Training (the SPACEWALK TRAINING) has two panels.
// Drawing: the launch redraws the screen every other frame (a cutscene). The spacewalk draws its backdrop once; after that only the little
// boxes that change (you, the panels, the readout) are drawn again and copied: the backdrop can be worked out for any pixel (swBg).
static int swDone;   // the panels the last spacewalk fixed (lunrado.h counts them)
static u32 swHash(int x,int y){ return ((u32)x*73856093u^(u32)y*19349663u)*2654435761u; }
static int swNoise(int x,int y){   // smooth-ish land and cloud blobs for the Earth: 3 octaves of value noise
    int v=0; for(int o=0;o<3;o++){ int s=16>>o, gx=x/s, gy=y/s, fx=x%s, fy=y%s;
        int a=(int)(swHash(gx,gy)>>24), b=(int)(swHash(gx+1,gy)>>24), c=(int)(swHash(gx,gy+1)>>24), d=(int)(swHash(gx+1,gy+1)>>24);
        int t=a+(b-a)*fx/s, u=c+(d-c)*fx/s; v+=(t+(u-t)*fy/s)>>o; }
    return v;   // about 0..448
}
static u16 swBg(int x,int y){   // the spacewalk's backdrop at one pixel: the station, the Earth, the stars
    if(y>=42&&y<56&&((x>=52&&x<96)||(x>=144&&x<188))) return ((x&3)==0||y==42||y==55)?RGB(4,6,14):(((x>>2)+(y>>2))&1)?RGB(8,12,26):RGB(10,15,29);   // solar panels
    if(x>=96&&x<144&&y>=48&&y<51) return RGB(16,16,18);                                                         // the truss
    if(x>=106&&x<134&&y>=36&&y<62){ if(x==106||x==133||y==36||y==61) return RGB(10,10,12); if(x>=117&&x<123&&y>=43&&y<48) return RGB(4,10,20); return (x&7)==0?RGB(20,20,22):RGB(25,25,26); }   // the module and its window
    if(x>=119&&x<121&&y>=24&&y<36) return RGB(22,22,24);                                                        // the mast
    if(y>=19&&y<24&&x>=112+(23-y)&&x<128-(23-y)) return RGB(28,28,29);                                          // the dish
    int dx=x-120, dy=y-262, d2=dx*dx+dy*dy;
    if(d2<150*150){ int n=swNoise(x+40,y*2), cl=swNoise(x*2+300,y*3+90);
        if(cl>330) return RGB(28,29,30);                                                                        // clouds
        if(n>250) return n>300?RGB(14,20,8):RGB(10,18,6);                                                      // land
        return d2>140*140?RGB(6,12,26):RGB(3,8,22); }                                                          // ocean (lighter at the rim)
    if(d2<155*155) return RGB(14,22,31);                                                                        // the air, a thin blue line
    u32 h=swHash(x,y); if((h>>24)<3) return (h&0x100)?RGB(28,28,30):RGB(16,16,20);                            // stars
    return RGB(0,0,3);
}
static void swRestore(int x0,int y0,int x1,int y1){ if(x0<0) x0=0; if(y0<0) y0=0; if(x1>SW) x1=SW; if(y1>SH) y1=SH; for(int y=y0;y<y1;y++){ u16*d=&fb[y*SW]; for(int x=x0;x<x1;x++) d[x]=swBg(x,y); } }
static void swCopy(int x0,int y0,int x1,int y1){ if(x0<0) x0=0; if(y0<0) y0=0; if(x1>SW) x1=SW; if(y1>SH) y1=SH; x0&=~1; x1=(x1+1)&~1; if(x1>SW) x1=SW; if(x1>x0&&y1>y0) vramCopy(x0,y0,x1,y1); }
static void swGuy(int x,int y,int fl){   // the astronaut, 7 x 9
    static const char* const g[9]={"..WWW..",".WGGGW.",".WGGGW.","WWWWWWW","WBWWWBW","WBWWWBW",".WWWWW.",".WW.WW.",".WW.WW."};
    for(int r=0;r<9;r++) for(int q=0;q<7;q++){ char c=g[r][fl?6-q:q]; if(c=='.') continue; px(x+q,y+r,c=='W'?RGB(30,30,31):c=='G'?RGB(31,24,6):RGB(16,16,18)); }
}
// THE SPACEWALK. real 1 = in orbit (3 panels), 0 = training (2). 1 = fixed them all
XPF static int exSpacewalk(int real){
    static const short spX[3]={62,178,118}, spY[3]={47,47,15};
    int nsp=real?3:2; u8 fixd[3]={0,0,0}; int prog[3]={0,0,0};
    s32 ax=120<<8, ay=100<<8, vx=0, vy=0; int fl=0, o2=1000, done=0, t=0, res=-1, hud=1;
    for(int y=0;y<SH;y++) for(int x=0;x<SW;x++) fb[y*SW+x]=swBg(x,y);
    { const char*t1=real?"FIX THE BROKEN PANELS":"TRAINING  FIX THE PANELS"; text((SW-tw(t1,2))/2,66,t1,WHITE,2); text((SW-tw("PAD  THRUSTERS    HOLD A  REPAIR",1))/2,86,"PAD  THRUSTERS    HOLD A  REPAIR",GOLD,1); }
    present(); { u16 p0=keyNow(); for(int i=0;i<150;i++){ vsync(); u16 k=keyNow(); if((k&~p0)&(K_A|K_START)) break; p0=k; } }
    swRestore(0,60,SW,96); present();
    u16 prev=keyNow(); int ox=(int)(ax>>8), oy=(int)(ay>>8);
    while(res<0){
        u16 k=keyNow(), pr=k&~prev; prev=k; t++;
        if(pr&K_B){ res=0; break; }
        int thr=0;
        if(k&K_LEFT){ vx-=7; fl=1; thr=1; } if(k&K_RIGHT){ vx+=7; fl=0; thr=1; } if(k&K_UP){ vy-=7; thr=1; } if(k&K_DOWN){ vy+=7; thr=1; }
        vx-=vx>>7; vy-=vy>>7; if(vx>512) vx=512; if(vx<-512) vx=-512; if(vy>512) vy=512; if(vy<-512) vy=-512;
        ax+=vx; ay+=vy;
        if(ax<(4<<8)){ ax=4<<8; vx=-vx/2; } if(ax>(228<<8)){ ax=228<<8; vx=-vx/2; } if(ay<(12<<8)){ ay=12<<8; vy=-vy/2; } if(ay>(146<<8)){ ay=146<<8; vy=-vy/2; }   // the tether
        if((t&31)==0){ o2-=real?20:25; hud=1; } if(thr&&(t&63)==0){ o2-=4; hud=1; }   // (the thrusters cost a little air too)
        int gx=(int)(ax>>8), gy=(int)(ay>>8), near=-1;
        for(int i=0;i<nsp;i++){ int dx=gx+3-spX[i], dy=gy+4-spY[i]; if(!fixd[i]&&dx*dx+dy*dy<12*12) near=i; }
        if(near>=0&&(k&K_A)){ if(++prog[near]>=50){ fixd[near]=1; done++; sfxPlay(SFX_STICK); hud=1; } else if((prog[near]&7)==0) sfxPlay(SFX_TICK); }
        if(done>=nsp) res=1; if(o2<=0) res=0;
        // draw: the old astronaut away, the panels, the new astronaut, the readout
        swRestore(ox-3,oy-7,ox+11,oy+13);
        for(int i=0;i<nsp;i++){ int x=spX[i]-3, y=spY[i]-3; swRestore(x-2,y-2,x+9,y+9);
            if(fixd[i]) rect(x,y,6,6,RGB(8,28,10)); else { rect(x,y,6,6,((t>>3)&1)?RGB(31,6,4):RGB(18,3,2)); if((swHash(t,i)>>28)<5){ px(x+(int)(swHash(i,t)>>29),y-1,RGB(31,30,12)); px(x+6,y+(int)(swHash(t,i+7)>>29),RGB(31,30,12)); } } }
        if(thr&&(t&1)){ px(gx+(fl?7:-1),gy+5,RGB(31,28,8)); px(gx+3,gy+9,RGB(31,20,6)); }
        swGuy(gx,gy,fl);
        if(near>=0&&prog[near]){ rect(gx-2,gy-5,11,3,RGB(2,2,4)); rect(gx-1,gy-4,prog[near]*9/50,1,RGB(10,30,12)); }
        if(hud){ rect(0,0,SW,10,RGB(1,2,6)); char b[32]; char*e=slCat(b,"O2 "); e=slNum(e,o2>0?o2/10:0); slCat(e,"%"); text(4,2,b,o2<250?RGB(31,8,6):WHITE,1);
            e=slCat(b,"PANELS "); e=slNum(e,done); e=slCat(e," OF "); slNum(e,nsp); text(SW-4-tw(b,1),2,b,GOLD,1); text((SW-tw(real?"IN ORBIT":"SIMULATOR",1))/2,2,real?"IN ORBIT":"SIMULATOR",RGB(17,29,31),1); }
        vsync();
        { int x0=(ox<gx?ox:gx)-3, x1=(ox>gx?ox:gx)+11, y0=(oy<gy?oy:gy)-7, y1=(oy>gy?oy:gy)+13; swCopy(x0,y0,x1,y1); }
        for(int i=0;i<nsp;i++) swCopy(spX[i]-5,spY[i]-6,spX[i]+6,spY[i]+6);
        if(hud){ swCopy(0,0,SW,10); hud=0; }
        ox=gx; oy=gy;
    }
    swDone=done;
    rect(30,62,180,34,RGB(2,3,8)); rect(30,62,180,1,GOLD); rect(30,95,180,1,GOLD);
    const char*m1=res?"ALL FIXED":"MISSION CONTROL REELS YOU IN", *m2=res?"GROUND CONTROL IS CHEERING":"BETTER LUCK NEXT TIME";
    text((SW-tw(m1,2))/2,67,m1,res?RGB(10,30,12):RGB(31,10,8),2); text((SW-tw(m2,1))/2,84,m2,WHITE,1); present();
    if(res){ voxPlay(V_yahoo); moodEvent(M_PROMO); } else voxPlay(V_nah);
    { u16 p0=keyNow(); for(int i=0;i<200;i++){ vsync(); u16 k=keyNow(); if((k&~p0)&(K_A|K_B|K_START)) break; p0=k; } }
    return res;
}
// THE LAUNCH
static void lnSky(int alt,int night,int shake){   // the sky by height: day blue (or night) to black; stars come out above 200
    for(int y=0;y<SH;y++){ int h=alt+(SH-y)/2; int t=h>420?16:h*16/420;
        int r0=night?2:9, g0=night?3:18, b0=night?10:30; int r=r0-r0*t/16, g=g0-g0*t/16, b=b0-(b0-3)*t/16;
        rect(0,y,SW,1,RGB(r,g,b)); }
    if(alt>150||night) for(int i=0;i<60;i++){ u32 h=swHash(i,77); int x=(int)(h%SW), y=(int)((h>>8)%SH)+(alt>>3)%SH; y%=SH; px(x+shake,y,(alt>300||night)?RGB(28,28,30):RGB(14,16,22)); }
}
static void lnRocket(int cx,int y,int t,int fire,int sh){   // y = the bottom of the rocket
    cx+=sh;
    for(int r=0;r<10;r++){ int w=1+r*5/9; rect(cx-w,y-62+r,w*2,1,RGB(30,6,4)); }   // the nose
    rect(cx-6,y-52,12,52,RGB(29,29,30)); rect(cx-6,y-52,2,52,RGB(22,22,24)); rect(cx+4,y-52,2,52,RGB(31,31,31));
    rect(cx-6,y-30,12,3,RGB(30,6,4)); rect(cx-2,y-46,4,4,RGB(4,10,22));                // a stripe, the window
    text(cx-2,y-24,"B",RGB(4,6,14),1); text(cx-2,y-17,"O",RGB(4,6,14),1); text(cx-2,y-10,"R",RGB(4,6,14),1);
    for(int r=0;r<10;r++){ rect(cx-6-r/2,y-10+r,3+r/3,1,RGB(26,5,3)); rect(cx+6+r/2-(3+r/3),y-10+r,3+r/3,1,RGB(26,5,3)); }   // the fins
    if(fire){ int n=fire>2?28:fire*9; for(int r=0;r<n;r++){ int w=4-r/8+(int)((swHash(t,r)>>30)); if(w<1) w=1; u16 c=r<6?RGB(31,31,20):r<14?RGB(31,26,6):RGB(31,14,4); rect(cx-w,y+r,w*2,1,c); } }
}
static void lnrApproach(int orbitOk); static void lnrBurn(void);   // lunrado.h
XPF static void exLaunch(void){
    int tourist=!exSeatFree(), night=simIsNight(), padY=134; if(tourist) lnrBurn();   // (a seat the mission booked burns no fuel)
    static s16 smX[24] EWRAM_BSS, smY[24] EWRAM_BSS; static u8 smR[24] EWRAM_BSS, smL[24] EWRAM_BSS; int smN=0;
    char who[40]; { char*e=slCat(who,tourist?"SPACE TOURIST ":"MAJOR "); slCat(e,hhPName); }
    u16 prev=keyNow(); int t=0, alt=0, skip=0;
    // the countdown (10 to 1, half a second each), then liftoff and the climb
    for(int phase=0;phase<3&&!skip;){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(t>60&&(pr&(K_B|K_START))) skip=1;
        int cnt=10-t/30;
        if(phase==0&&cnt<=0){ phase=1; t=0; sfxPlay(SFX_THUNDER); voxPlay(V_yahoo); }
        if(phase==1&&t>150){ phase=2; t=0; }
        if(phase==2&&t>100) phase=3;
        if(phase==0&&t%30==0) sfxPlay(cnt<=3?SFX_POP:SFX_TICK);
        if(phase>=1){ int v=phase==1?t/12+1:14; alt+=v; }
        int sh=phase==1&&t<90?((int)(swHash(t,3)>>30)-2):0;
        int ry=alt<60?padY-alt:padY-60, ground=alt<60?0:alt-60;   // the camera follows once the rocket is up
        if(phase<=1&&(t&1)){ int i=smN<24?smN++:(int)(swHash(t,9)%24); smX[i]=(s16)(120+(int)(swHash(t,i)%40)-20); smY[i]=(s16)(padY+2); smR[i]=3; smL[i]=(u8)(phase?60:20); }
        if(t&1){ t++; vsync(); continue; }   // (a frame drawn every other one)
        lnSky(alt,night,sh);
        if(ground<SH){ int gy=padY+ground; if(gy<SH){ rect(0,gy+4,SW,SH-gy,night?RGB(3,6,3):RGB(8,16,6)); rect(80,gy,80,5,RGB(10,10,12)); }   // the ground and the pad
            int gt=gy-80; if(gt<SH){ for(int x=86;x<=96;x+=10) rect(x+sh,gt,2,80,RGB(14,14,16)); for(int y=gt;y<gy;y+=8) line(86+sh,y,96+sh,y+8,RGB(14,14,16));   // the gantry
                if(phase==0&&cnt>2) rect(97+sh,gt+30,17,2,RGB(14,14,16)); } }   // its arm, gone at T minus 2
        lnRocket(120,ry,t,phase==0?(cnt<=3?1:0):phase==1&&t<20?2:3,sh);
        for(int i=0;i<smN;i++) if(smL[i]){ smL[i]--; if(smR[i]<14&&(t&3)==0) smR[i]++; smX[i]+=(s16)((smX[i]<120)?-1:1); int yy=smY[i]+ground; if(yy-smR[i]>=SH) continue; disc(smX[i]+sh,yy,smR[i],smL[i]>30?RGB(26,26,26):RGB(18,18,20)); }
        if(phase==0){ char b[4]; slNum(b,cnt); text((SW-tw(b,3))/2,18,b,WHITE,3); text((SW-tw(who,1))/2,52,who,GOLD,1); }
        else if(phase==1&&t<70) text((SW-tw("LIFTOFF!",3))/2,18,"LIFTOFF!",GOLD,3);
        else if(phase==2) text((SW-tw("ORBIT REACHED",2))/2,24,"ORBIT REACHED",RGB(17,29,31),2);
        present(); t++;
    }
    stQEvent(QE_LAUNCH,1);
    int ok=exSpacewalk(1); if(ok) stQEvent(QE_SPACEWALK,2);
    lnrApproach(ok);   // lunrado.h: the damaged ship, the repairs, the landing on LUNRADO (or the explosion)
    simsSave();
}

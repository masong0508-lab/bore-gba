// space.h - BORE-OUT MAN!'s two screens (expand.h takes the life game over for them):
//   THE LAUNCH     the SPACEPORT's A SEAT ON THE ROCKET: the pad, the gantry, a countdown, LIFTOFF (smoke, a shaking screen, the sky going
//                  black as you climb), orbit. Then the spacewalk for real, then splashdown (four hours went by).
//   THE SPACEWALK  you float by the station with the Earth below: the pad fires your thrusters (you drift: there is nothing to stop you), hold A
//                  next to a broken panel to fix it. Fix them all before the oxygen runs out. Training (the SPACEWALK TRAINING) has two panels.
// Drawing: the launch redraws the screen every other frame (a cutscene). The spacewalk draws its backdrop once; after that only the little
// boxes that change (you, the panels, the readout) are drawn again and copied: the backdrop can be worked out for any pixel (swBg).
static int swDone;   // the panels the last spacewalk fixed (lunrado.h counts them)
static u32 swHash(int x,int y){ return ((u32)x*73856093u^(u32)y*19349663u)*2654435761u; }
static int swNoise(int x,int y){   // smooth-ish land and cloud blobs for the Earth: 3 octaves of value noise (cells of 16, 8, 4: shifts, no division: this ran for three seconds a picture)
    int v=0; for(int o=0;o<3;o++){ int sh=4-o, m=(1<<sh)-1, gx=x>>sh, gy=y>>sh, fx=x&m, fy=y&m;
        int a=(int)(swHash(gx,gy)>>24), b=(int)(swHash(gx+1,gy)>>24), c=(int)(swHash(gx,gy+1)>>24), d=(int)(swHash(gx+1,gy+1)>>24);
        int t=a+(((b-a)*fx)>>sh), u=c+(((d-c)*fx)>>sh); v+=(t+(((u-t)*fy)>>sh))>>o; }
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
static void swGuy(int x,int y,int fl){   // the astronaut, 11 x 14: a white suit with a shaded side, a gold visor with a glint, a pack, boots
    static const char* const g[14]={"...WWWWW...","..WWWWWWw..",".WWGGGGGWw.",".WWGHGGGWw.",".WWGGGGGWw.","..WWWWWWw..","OWWWWRRWWwO","OWWWWWWWwwO","WWWBWWWBwwW","WBWWWWWWwBW","WB.WWWWWw.B","...WWWWWw..","..WWW.WWw..","..BBB.BBB.."};
    for(int r=0;r<14;r++) for(int q=0;q<11;q++){ char c=g[r][fl?10-q:q]; if(c=='.') continue;
        px(x+q,y+r,c=='W'?RGB(30,30,31):c=='w'?RGB(19,19,23):c=='G'?RGB(31,24,6):c=='H'?RGB(31,31,26):c=='R'?RGB(31,6,4):c=='O'?RGB(30,16,3):RGB(12,12,15)); }
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
        for(int i=0;i<nsp;i++){ int dx=gx+5-spX[i], dy=gy+7-spY[i]; if(!fixd[i]&&dx*dx+dy*dy<14*14) near=i; }
        if(near>=0&&(k&K_A)){ if(++prog[near]>=50){ fixd[near]=1; done++; sfxPlay(SFX_STICK); hud=1; } else if((prog[near]&7)==0) sfxPlay(SFX_TICK); }
        if(done>=nsp) res=1; if(o2<=0) res=0;
        // draw: the old astronaut away, the panels, the new astronaut, the readout
        swRestore(ox-8,oy-8,ox+19,oy+22);
        for(int i=0;i<nsp;i++){ int x=spX[i]-3, y=spY[i]-3; swRestore(x-2,y-2,x+9,y+9);
            if(fixd[i]) rect(x,y,6,6,RGB(8,28,10)); else { rect(x,y,6,6,((t>>3)&1)?RGB(31,6,4):RGB(18,3,2)); if((swHash(t,i)>>28)<5){ px(x+(int)(swHash(i,t)>>29),y-1,RGB(31,30,12)); px(x+6,y+(int)(swHash(t,i+7)>>29),RGB(31,30,12)); } } }
        if(thr){ int jx=fl?11:-1, jd=fl?1:-1; if(k&(K_LEFT|K_RIGHT)) for(int q=0;q<3+(int)(swHash(t,1)>>30);q++) px(gx+jx+jd*q,gy+7,q<1?RGB(31,31,22):q<2?RGB(31,24,6):RGB(31,12,4));
            if(k&(K_UP|K_DOWN)){ int dd=(k&K_UP)?1:-1, jy=(k&K_UP)?14:-1; for(int q=0;q<3+(int)(swHash(t,2)>>30);q++){ px(gx+4,gy+jy+dd*q,q<1?RGB(31,31,22):q<2?RGB(31,24,6):RGB(31,12,4)); px(gx+6,gy+jy+dd*q,q<1?RGB(31,31,22):q<2?RGB(31,24,6):RGB(31,12,4)); } } }
        swGuy(gx,gy,fl);
        if(near>=0&&prog[near]){ rect(gx-2,gy-6,15,4,RGB(2,2,4)); rect(gx-1,gy-5,prog[near]*13/50,2,RGB(10,30,12)); }
        if(hud){ rect(0,0,SW,10,RGB(1,2,6)); char b[32]; char*e=slCat(b,"O2 "); e=slNum(e,o2>0?o2/10:0); slCat(e,"%"); text(4,2,b,o2<250?RGB(31,8,6):WHITE,1);
            e=slCat(b,"PANELS "); e=slNum(e,done); e=slCat(e," OF "); slNum(e,nsp); text(SW-4-tw(b,1),2,b,GOLD,1); text((SW-tw(real?"IN ORBIT":"SIMULATOR",1))/2,2,real?"IN ORBIT":"SIMULATOR",RGB(17,29,31),1); }
        vsync();
        { int x0=(ox<gx?ox:gx)-8, x1=(ox>gx?ox:gx)+19, y0=(oy<gy?oy:gy)-8, y1=(oy>gy?oy:gy)+22; swCopy(x0,y0,x1,y1); }
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
// ---- THE LAUNCH: look. A pad at dawn (floodlit at night): hills, a service tower whose arm swings back, a shaded rocket with two boosters,
// billowing smoke, a telemetry panel; then the climb through cloud, the boosters let go (a timing prompt), the sky going black and the Earth coming up. ----
static u16 spSkyCol(int alt,int night,int y){   // the sky's colour at screen row y: day blue (or night indigo), black with height
    int h=alt+((SH-y)>>1), t=h>1370?16:(h*12)>>10; int r0=night?2:9, g0=night?3:18, b0=night?10:30;   // (h*39>>10 is h*16/420: no division)
    return RGB(r0-((r0*t)>>4),g0-((g0*t)>>4),b0-(((b0-3)*t)>>4));
}
static u8 spStX[70], spStY[70];   // the stars, worked out once
static void spSky(int alt,int night,int sh,int t){
    for(int y=0;y<SH;y+=2) rect(0,y,SW,2,spSkyCol(alt,night,y));   // (two rows a band: the steps of the gradient are a colour apart)
    if(alt>400||night){ if(!spStX[1]) for(int i=0;i<70;i++){ u32 h=swHash(i,77); spStX[i]=(u8)(h%SW); spStY[i]=(u8)((h>>8)%SH); }
        u16 br=(alt>900||night)?RGB(28,28,30):RGB(14,16,22); int n=(alt>900||night)?70:40;
        for(int i=0;i<n;i++){ int y=spStY[i]+(alt>>3); while(y>=SH) y-=SH; px(spStX[i]+sh,y,(((t>>3)+i)&7)<6?br:RGB(12,12,18)); } }
}
static void spCone(int px0,int py0,int tx,int ty){   // a floodlight's beam: a triangle of dithered light from the lamp down to the ground
    if(ty<=py0) return; for(int y=py0;y<ty&&y<SH;y++){ int k=y-py0, span=ty-py0, cx=px0+(tx-px0)*k/span, half=1+k*9/span; for(int x=cx-half;x<=cx+half;x++) if(((x+y)&1)==0) px(x,y,RGB(11,10,7)); }
}
static void spScenery(int alt,int night,int hy,int sh,int t,int arm){   // hy: the ground line; the whole scene drops away as the rocket climbs
    if(hy>=SH+50) return;
    { int fade=alt<600?600-alt:0; if(fade) for(int k=0;k<36;k+=2){ int y=hy-k; if(y<0||y>=SH) continue; int f=((36-k)*(36-k)*fade*21)>>20; if(f>0) rect(0,y-1,SW,2,exMix(spSkyCol(alt,night,y),night?RGB(9,8,19):RGB(31,22,13),f>16?16:f)); } }
    if(alt<1200){ if(night){ int my=hy-86+alt/12; disc(206,my,7,RGB(28,28,26)); disc(209,my-2,6,spSkyCol(alt,night,my)); } else { disc(46,hy-20,10,RGB(31,26,12)); disc(46,hy-20,7,RGB(31,31,23)); } }
    u16 far=exMix(spSkyCol(alt,night,hy-24),RGB(2,3,9),night?10:8), near=exMix(far,RGB(1,2,3),9);
    static u8 hF[SW], hN[SW]; if(!hF[1]) for(int x=0;x<SW;x++){ hF[x]=(u8)(14+(exSn(x*2+7,150)+127)/22+(exSn(x*5,60)+127)/60); hN[x]=(u8)(5+(exSn(x*3+90,110)+127)/40); }   // (the hills never change: worked out once)
    for(int x=0;x<SW;x++){ int y0=hy-hF[x], y1=hy+2; if(y0<0) y0=0; if(y1>SH) y1=SH; for(int y=y0;y<y1;y++) fb[y*SW+x]=far; }
    for(int x=0;x<SW;x++){ int y0=hy-hN[x], y1=hy+2; if(y0<0) y0=0; if(y1>SH) y1=SH; for(int y=y0;y<y1;y++) fb[y*SW+x]=near; }
    if(hy<SH){ rect(0,hy+1,SW,SH-hy,night?RGB(2,5,3):RGB(6,13,5)); rect(0,hy+1,SW,2,night?RGB(3,7,4):RGB(8,17,6));
        rect(60,hy+1,120,9,RGB(11,11,13)); rect(60,hy+1,120,1,RGB(20,20,22)); for(int x=60;x<180;x+=8) rect(x,hy+7,4,2,RGB(25,20,3)); rect(108,hy+1,24,4,RGB(2,2,3));   // the pad, its hazard stripes and the flame trench
        for(int i=0;i<6;i++){ int lx=10+i*44+((i*13)&7); rect(lx,hy+14+(i&1)*5,12,1,RGB(4,9,4)); }
        if(night){ int ly=hy-40; for(int s=0;s<2;s++){ int lx=s?190:50; rect(lx,ly,2,40,RGB(8,8,10)); rect(lx-2,ly-2,6,3,RGB(28,28,24)); spCone(lx+1,ly,s?150:90,hy); } } }
    int gt=hy-82; if(gt<SH){ u16 st=RGB(16,16,18), hl=RGB(22,22,24);   // the service tower
        for(int x=86;x<=96;x+=10) rect(x+sh,gt,2,82,st); for(int y=gt;y<hy;y+=8) line(86+sh,y,96+sh,y+8,st);
        for(int y=gt+10;y<hy;y+=24) rect(84+sh,y,14,2,hl);
        if(arm>0) rect(97+sh,gt+34,arm,3,hl);   // the arm that holds the crew hatch: it swings away in the last seconds
        if(((t>>3)&1)||night) disc(92+sh,gt-2,1,RGB(31,6,4)); }
}
static void spRocket(int cx,int y,int t,int fire,int sh,int boost){   // y = the bottom of the rocket; fire 0 off, 1 a little, 2 full, 3 cruising
    cx+=sh;
    static const u8 nw[12]={1,2,3,3,4,5,5,6,6,7,7,7};
    if(boost) for(int b=0;b<2;b++){ int bx=b?cx+8:cx-12; rect(bx,y-44,5,44,RGB(26,26,28)); rect(bx+(b?3:0),y-44,2,44,RGB(19,19,22)); rect(bx+1,y-44,1,44,RGB(31,31,31));
        for(int r=0;r<6;r++) rect(bx+(r<3?1:0)+(b?0:0),y-50+r,5-(r<3?2:0),1,RGB(30,6,4)); rect(bx,y-30,5,2,RGB(28,5,3)); rect(bx+1,y-1,3,2,RGB(8,8,10)); }
    for(int r=0;r<12;r++){ int w=nw[r]; rect(cx-w,y-72+r,w*2,1,RGB(30,6,4)); px(cx-w+1,y-72+r,RGB(31,18,14)); px(cx+w-2,y-72+r,RGB(20,3,2)); }   // the nose
    static const u16 shade[8]={RGB(17,17,20),RGB(23,23,25),RGB(28,28,30),RGB(31,31,31),RGB(30,30,31),RGB(26,26,28),RGB(21,21,24),RGB(16,16,19)};
    for(int i=0;i<14;i++) rect(cx-7+i,y-60,1,60,shade[i*8/14]);
    rect(cx-7,y-46,14,3,RGB(29,6,4)); rect(cx-7,y-43,14,1,RGB(17,3,2)); rect(cx-7,y-24,14,2,RGB(29,6,4));
    rect(cx-2,y-56,5,5,RGB(3,9,22)); px(cx-1,y-55,RGB(14,22,31)); rect(cx-1,y-38,3,3,RGB(3,9,22));
    text(cx-2,y-20,"B",RGB(4,6,14),1); text(cx-2,y-13,"O",RGB(4,6,14),1); text(cx-2,y-6,"R",RGB(4,6,14),1);
    for(int r=0;r<12;r++){ int w=3+r/3; rect(cx-7-r/2-1,y-12+r,w,1,RGB(26,5,3)); rect(cx+7+r/2-w+2,y-12+r,w,1,RGB(26,5,3)); }   // the fins
    rect(cx-5,y,4,3,RGB(7,7,9)); rect(cx+1,y,4,3,RGB(7,7,9));
    if(fire){ int n=fire>2?30:fire*10;
        for(int b=-1;b<=1;b+=2) for(int r=0;r<n;r++){ int w=3-r/10+(int)(swHash(t,r+b)>>30); if(w<1) w=1; u16 c=r<7?RGB(31,31,22):r<16?RGB(31,26,7):RGB(31,13,4); rect(cx+b*3-w/2,y+3+r,w,1,c); }
        disc(cx,y+4,5+fire,RGB(31,22,7)); disc(cx,y+4,3,RGB(31,31,24)); }
}
static void spEarthLimb(int rise,int t){   // the Earth coming up under the rocket: a bright rim, ocean, slowly turning land and cloud
    int cy=SH+200-rise; disc(120,cy,203,RGB(6,13,28)); disc(120,cy,201,RGB(12,22,31)); disc(120,cy,199,RGB(2,7,22));
    for(int k=0;k<26;k++){ int bx=SW-((t/3+k*43)%(SW+40))+20, by=SH-rise+4+(k*11)%52, dx=bx-120, dy=by-cy; if(dx*dx+dy*dy<190*190){ disc(bx,by,3+(k&1)+(k%5==0),k%3==2?RGB(27,28,29):RGB(4,17,6)); if(k%3==0) disc(bx+2,by+1,1,RGB(10,20,6)); if(k%3==2) rect(bx-2,by+1,6,1,RGB(22,23,26)); } }
}
static void spHud(int phase,int cnt,int t,int alt,const char*who,int night){
    char b[28]; int w=84, h=phase==0?30:30; exDarken(3,3,w,h,6); rect(3,3,w,1,RGB(14,26,31));
    if(phase==0){ char*e=slCat(b,"T-00:"); if(cnt<10) e=slCat(e,"0"); slNum(e,cnt>0?cnt:0); text(7,6,b,cnt<=3?RGB(31,12,8):WHITE,1);
        const char*st=cnt>7?"FUELLING":cnt>4?"ALL SYSTEMS GO":cnt>2?"ENGINE START":"IGNITION"; text(7,15,st,RGB(10,30,12),1); }
    else { int sec=(phase==1?t:t+150)/60; char*e=slCat(b,"T+00:"); if(sec<10) e=slCat(e,"0"); slNum(e,sec); text(7,6,b,WHITE,1);
        e=slCat(b,"ALT "); e=slNum(e,alt/12); slCat(e," KM"); text(7,15,b,RGB(17,29,31),1); e=slCat(b,"VEL "); e=slNum(e,alt*31/10); text(7,24,b,GOLD,1); }
    (void)who; (void)night;
}
static void lnrApproach(int orbitOk); static void lnrBurn(void);   // lunrado.h
XPF static void exLaunch(void){
    int tourist=!exSeatFree(), night=simIsNight(), padY=134; if(tourist) lnrBurn();   // (a seat the mission booked burns no fuel)
    static s16 smX[28] EWRAM_BSS, smY[28] EWRAM_BSS; static u8 smR[28] EWRAM_BSS, smL[28] EWRAM_BSS; int smN=0;
    char who[40]; { char*e=slCat(who,tourist?"SPACE TOURIST ":"MAJOR "); slCat(e,hhPName); }
    u16 prev=keyNow(); int t=0, alt=0, skip=0, stg=0, stgT=0;   // stg: 0 the boosters are on, 1 let go cleanly, 2 let go roughly
    u32 vb0=vblN;   // THE CLOCK: t counts real frames since the phase began, however long a picture takes to draw (about one a frame and a half: a cinematic at 30)
    for(int phase=0;phase<3&&!skip;){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(t>60&&(pr&(K_B|K_START))) skip=1;
        int tn=(int)(vblN-vb0), dt=tn-t; if(dt<1) dt=1; if(dt>6) dt=6; t+=dt;
        int cnt=10-t/30;
        if(phase==0&&cnt<=0){ phase=1; t=0; vb0=vblN; dt=1; sfxPlay(SFX_THUNDER); voxPlay(V_yahoo); }
        if(phase==1&&t>150){ phase=2; t=0; vb0=vblN; dt=1; }
        if(phase==2&&t>100) phase=3;
        if(phase==0&&((t-dt)/30!=t/30)) sfxPlay(cnt<=3?SFX_POP:SFX_TICK);
        if(phase>=1) alt+=(phase==1?t/12+1:14)*dt;
        if(phase==2&&!stg&&(pr&K_A)&&t>=14){ stg=(t>=26&&t<=36)?1:2; stgT=t; sfxPlay(SFX_POP); if(stg==1){ simMoneyAdd(500); voxPlay(V_yahoo); } }   // THE STAGING: A at the right moment lets the boosters go cleanly (+500)
        if(phase==2&&!stg&&t>=48){ stg=2; stgT=t; sfxPlay(SFX_POP); }
        int sh=phase==1&&t<90?((int)(swHash(t,3)>>30)-2):0;
        int ry=alt<30?padY-alt:padY-30, ground=alt<30?0:alt-30;   // the camera follows once the rocket is up
        if(phase==1||(phase==0&&cnt<=4)){ int i=smN<28?smN++:(int)(swHash(t,9)&15)+((int)(t&1)<<3); smX[i]=(s16)(120+(int)(swHash(t,i)&31)-16); smY[i]=(s16)(padY+3); smR[i]=3; smL[i]=(u8)(phase?70:24); }
        int hy=padY+4+ground, arm=phase==0?(cnt>4?17:cnt>3?11:cnt>2?5:0):0;
        spSky(alt,night,sh,t);
        spScenery(alt,night,hy,sh,t,arm);
        if(phase>=1&&hy<SH+20&&t<110){ int g=hy-1, w=40+(t>60?0:(60-t)/2); rect(120-w,g,2*w,4,RGB(13,7,3)); rect(120-w*2/3,g,w*4/3,4,RGB(22,12,4)); rect(120-w/3,g,w*2/3,4,RGB(31,22,8)); rect(120-w/6,g+1,w/3,2,RGB(31,31,22)); }   // the flame lights the pad
        { int fire=phase==0?(cnt<=3?1:0):phase==1&&t<20?2:3; if(phase==0&&cnt<=1) fire=2; spRocket(120,ry,t,fire,sh,!stg); }
        if(alt>120&&alt<1400) for(int i=0;i<5;i++){ int cy=((i*53+alt*(1+i%3)/3)%(SH+40))-20, cx=(i*71+13)%SW; disc(cx,cy,5,RGB(29,30,31)); disc(cx+6,cy+1,4,RGB(27,28,30)); disc(cx-6,cy+2,3,RGB(26,27,29)); }   // cloud rushing past
        if(phase>=2&&alt>600) for(int i=0;i<5;i++){ int sx=108+i*6+(int)(swHash(t,i)>>30), sy=ry+10+(int)(swHash(i,t)%60); line(sx,sy,sx,sy+8,RGB(22,24,31)); }   // speed streaks
        if(stg){ int d=t-stgT; if(d<70){ int fx=d*2; for(int b=0;b<2;b++){ int bx=(b?136:100)+(b?fx:-fx), by=ry-30+d*d/20; rect(bx,by,5,32,RGB(26,26,28)); rect(bx+(b?3:0),by,2,32,RGB(19,19,22)); rect(bx,by-4,5,4,RGB(30,6,4)); if(d<30) disc(bx+2,by+34,2+(d&1),RGB(31,22,8)); } } }
        if(phase==2&&alt>1800){ int rise=(alt-1800)*2/3; if(rise>74) rise=74; spEarthLimb(rise,t); }
        for(int i=0;i<smN;i++) if(smL[i]){ smL[i]=(u8)(smL[i]>dt?smL[i]-dt:0); if(smR[i]<14) smR[i]++; smX[i]+=(s16)((smX[i]<120)?-dt:dt); int yy=smY[i]+ground-((70-smL[i])>>3); int r=smR[i]; if(yy-r>=SH) continue; disc(smX[i]+sh,yy,r,smL[i]>30?RGB(24,24,26):RGB(17,17,20)); if(r>6) disc(smX[i]+sh-r/3,yy-r/3,r*2/3,smL[i]>30?RGB(29,29,30):RGB(21,21,23)); }
        spHud(phase,cnt,t,alt,who,night);
        if(phase==0){ char b[4]; slNum(b,cnt); text((SW-tw(b,3))/2,8,b,WHITE,3); text(SW-4-tw(who,1),4,who,GOLD,1); }
        else if(phase==1&&t<70) text((SW-tw("LIFTOFF!",3))/2,6,"LIFTOFF!",GOLD,3);
        else if(phase==2){
            if(!stg&&t>=14){ int bw=120, bx=(SW-bw)/2, by=SH-30; exDarken(bx-6,by-12,bw+12,32,4); rect(bx-6,by-12,bw+12,1,GOLD);
                text((SW-tw("A  LET THE BOOSTERS GO",1))/2,by-9,"A  LET THE BOOSTERS GO",WHITE,1); rect(bx,by+2,bw,7,RGB(3,4,9)); rect(bx+(26-14)*bw/34,by+2,11*bw/34,7,RGB(6,24,8)); int mx=bx+(t-14)*bw/34; if(mx>bx+bw) mx=bx+bw; rect(mx,by,2,11,WHITE); }
            else if(stg&&t-stgT<60){ const char*m=stg==1?"CLEAN STAGING  +500":"ROUGH STAGING"; text((SW-tw(m,2))/2,SH-26,m,stg==1?RGB(10,30,12):RGB(31,16,6),2); }
            if(alt>1800&&t>60) text((SW-tw("ORBIT REACHED",2))/2,8,"ORBIT REACHED",RGB(17,29,31),2); }
        present();
    }
    stQEvent(QE_LAUNCH,1);
    int ok=exSpacewalk(1); if(ok) stQEvent(QE_SPACEWALK,2);
    lnrApproach(ok);   // lunrado.h: the damaged ship, the repairs, the landing on LUNRADO (or the explosion)
    simsSave();
}

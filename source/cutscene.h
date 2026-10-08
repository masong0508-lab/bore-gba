// cutscene.h - CUTSCENES: a short scripted scene, drawn by code (no art files): a backdrop, up to two figures, a caption box and a few effects.
// csPlay(scene): A types the caption out / moves on, START skips the whole scene. Scenes are tables of beats (CsBeat) at the bottom.
// Used by TV SHOW & TELL (story.h plays one when a chapter ends). Needs from main.c: fb, rect, disc, line, text, tw, present, keyNow, sfxPlay, rnd8, objHideAll.
// Layout: black bar 0..11, the picture 12..115, the caption panel 116..159. Fades use the hardware brightness blend (BLDCNT / BLDY), so they cost nothing.
enum { CB_BLACK, CB_STAGE, CB_HOME, CB_MIRROR, CB_SITE, CB_HOSP, CB_BACK, CB_BIG, CB_FLAT, CB_RATE };            // backdrops
enum { CA_NONE, CA_MISSY, CA_MAME, CA_HOST, CA_CREW, CA_DOC };                                   // who stands there
enum { CP_STAND, CP_SWAY, CP_DANCE, CP_SING, CP_HEAD, CP_RUN, CP_CLIMB, CP_FLAIL, CP_LIE };      // what they are doing
enum { CF_SHAKE=1, CF_FLASH=2, CF_FADEIN=4, CF_FADEOUT=8, CF_STROBE=16, CF_IRIS=32, CF_SICK=64, CF_AUTO=128 };
typedef struct { u8 bg, a, pa, ax, b, pb, bx, fx, sfx, dur; const char* who; const char* t[3]; } CsBeat;   // ax / bx: x position / 4. sfx: SFX id + 1. dur: frames (no caption: how long; AUTO caption: the wait after it)
typedef struct { const CsBeat* b; u8 n; } CsScene;

static int csOx, csOy;   // the shake
static void csR(int x,int y,int w,int h,u16 c){ rect(x+csOx,y+csOy,w,h,c); }
static void csD(int x,int y,int r,u16 c){ disc(x+csOx,y+csOy,r,c); }
static void csLn(int x0,int y0,int x1,int y1,u16 c){ line(x0+csOx,y0+csOy,x1+csOx,y1+csOy,c); }
static int csWv(int t,int per){ int p=t%per, h=per/2, v=p<h?p:per-p; return v*16/h-8; }   // a triangle wave, -8 .. 8
static void csGrad(int y0,int h,int r0,int g0,int b0,int r1,int g1,int b1){ for(int i=0;i<h;i++){ int t=h>1?i*256/(h-1):0; csR(0,y0+i,SW,1,RGB(r0+(r1-r0)*t/256,g0+(g1-g0)*t/256,b0+(b1-b0)*t/256)); } }

static void csFig(int x,int y,int who,int pose,int t){   // one person, 34 px tall, feet at (x,y)
    static const u16 dr[6]={0,RGB(5,6,13),RGB(4,17,22),RGB(14,4,18),RGB(27,17,2),RGB(29,29,31)};      // dress / suit / vest / coat
    static const u16 hr[6]={0,RGB(5,4,4),RGB(7,4,2),RGB(16,14,12),RGB(18,18,18),RGB(9,6,4)};          // hair
    u16 sk=RGB(28,21,16), cl=dr[who], Hh=hr[who], dk=RGB(3,2,3);   // (cl, not D: main.c defines D as a macro)
    if(pose==CP_LIE){ csR(x-10,y-6,17,6,cl); csR(x+7,y-4,9,2,sk); csD(x-14,y-4,4,sk); csR(x-18,y-8,5,5,Hh); csR(x-15,y-5,1,1,dk); return; }
    int lean=0, bob=0, lh=-6, lv=8, rh=6, rv=8, ls=0, hd=0, open=0;
    switch(pose){
    case CP_SWAY:  lean=csWv(t,70)/2; lh=-7+csWv(t,50)/3; rh=7-csWv(t,50)/3; ls=csWv(t,70)/4; break;
    case CP_DANCE: bob=(csWv(t,16)+8)/6; lean=csWv(t,40)/2; lh=-9; lv=-9+csWv(t,16)/2; rh=9; rv=-9-csWv(t,16)/2; ls=csWv(t,16)/3; break;
    case CP_SING:  lean=csWv(t,60)/4; rh=2; rv=-8; lh=-10; lv=-3+csWv(t,30)/3; open=1; break;
    case CP_HEAD:  lean=-1; hd=2; lh=-4; lv=-9; rh=4; rv=-9; break;
    case CP_RUN:   lean=3; lh=-5+csWv(t,10)/2; rh=5-csWv(t,10)/2; lv=rv=2; ls=csWv(t,10); break;
    case CP_CLIMB: lh=-4; rh=4; lv=-8+csWv(t,24)/2; rv=-8-csWv(t,24)/2; ls=csWv(t,24)/2; break;
    case CP_FLAIL: lean=csWv(t,8)/3; lh=-9; lv=-8+csWv(t,6); rh=9; rv=-8-csWv(t,6); ls=csWv(t,6)/2; break;
    }
    int sy=y-21+bob;
    csR(x-3+ls,y-8,2,8,sk); csR(x+1-ls,y-8,2,8,sk); csR(x-4+ls,y-1,3,1,dk); csR(x+1-ls,y-1,3,1,dk);                                       // legs and shoes
    csLn(x+lean-4,sy,x+lean+lh,sy+lv,sk); csLn(x+lean-3,sy,x+lean+lh+1,sy+lv,sk); csLn(x+lean+4,sy,x+lean+rh,sy+rv,sk); csLn(x+lean+3,sy,x+lean+rh-1,sy+rv,sk);   // arms
    for(int i=0;i<14;i++){ int w=8+i*6/13; csR(x+lean*(14-i)/14-w/2,y-22+i+bob,w,1,cl); }                                                 // the dress / suit, a trapezoid
    if(who==CA_MISSY){ csR(x+lean-6,y-30+bob+hd,2,4,Hh); csR(x+lean+4,y-30+bob+hd,2,4,Hh); csR(x+lean-1,y-35+bob+hd,3,2,Hh); }
    if(who==CA_MAME){ csR(x+lean-5,y-29+bob+hd,2,10,Hh); csR(x+lean+3,y-29+bob+hd,2,10,Hh); }                              // long hair
    csD(x+lean,y-29+bob+hd,5,Hh); csD(x+lean,y-27+bob+hd,4,sk);                                                                           // hair, face
    csR(x+lean-2,y-28+bob+hd,1,1,dk); csR(x+lean+1,y-28+bob+hd,1,1,dk);
    if(open) csR(x+lean-1,y-25+bob+hd,2,2,RGB(18,2,3)); else csR(x+lean-1,y-25+bob+hd,2,1,RGB(18,6,6));
}

static char* csNum(char*b,int v){ char d[12]; int n=0; if(v<=0) d[n++]='0'; while(v>0){ d[n++]=(char)('0'+v%10); v/=10; } while(n>0) *b++=d[--n]; *b=0; return b; }   // a number as text (the approval board)
static void csBg(int bg,int t,int fx){   // the picture area: y 12 .. 115
    switch(bg){
    case CB_STAGE: case CB_BIG: {
        int big=bg==CB_BIG;
        if(big) csGrad(12,104,3,2,10,10,5,22); else csGrad(12,104,6,3,9,14,6,14);
        int on=!(fx&CF_STROBE)||((t>>2)&1); u16 cone=on?(big?RGB(18,17,24):RGB(20,17,13)):RGB(8,5,12);
        for(int i=0;i<78;i++){ int w=8+i*2; csR(120-w/2,12+i,w,1,cone); }
        if(fx&CF_STROBE){ for(int i=0;i<6;i++){ int c=(t>>3)+i; csR(24+i*38,12,6,3,(c&3)==0?RGB(31,8,8):(c&3)==1?RGB(8,26,31):(c&3)==2?RGB(31,28,6):RGB(20,8,31)); } }
        if(!big){ csR(0,12,34,82,RGB(18,3,5)); csR(206,12,34,82,RGB(18,3,5)); for(int i=0;i<5;i++){ csR(5+i*7,12,1,82,RGB(10,1,3)); csR(209+i*7,12,1,82,RGB(10,1,3)); } }
        csR(0,96,SW,20,big?RGB(7,5,10):RGB(13,8,4)); csR(0,96,SW,2,big?RGB(14,11,20):RGB(21,14,7));
        for(int i=0;i<15;i++){ int hx=8+i*16+((i*7)&5), hy=110+(i&1); csD(hx,hy+4,5,RGB(1,1,3)); if(big&&((i*5+(t>>4))%7)==0) csR(hx-1,hy-3,2,2,RGB(31,31,14)); else if(!big&&(i%4==1)&&((t>>3)&1)) csR(hx,hy-2,1,2,RGB(28,28,31)); }   // the audience
        if(big){ csR(118,106,6,5,RGB(26,24,20)); csR(118,111,1,4,RGB(26,24,20)); csR(123,111,1,4,RGB(26,24,20)); csR(118,102,1,4,RGB(26,24,20)); }   // the empty chair, front row centre
        break; }
    case CB_HOME:
        csGrad(12,98,3,4,11,5,6,16); csR(0,110,SW,6,RGB(5,3,3));
        csR(180,20,34,30,RGB(2,3,8)); csR(181,21,32,28,RGB(4,8,18)); csD(198,32,6,RGB(27,27,22)); csD(201,31,5,RGB(4,8,18)); csR(196,21,2,28,RGB(2,3,8));   // window and moon
        csR(14,86,50,24,RGB(6,3,6)); csR(10,78,10,32,RGB(6,3,6)); csR(58,78,10,32,RGB(6,3,6));                                                     // sofa
        csR(150,92,36,3,RGB(9,6,3)); csR(154,95,3,15,RGB(9,6,3)); csR(180,95,3,15,RGB(9,6,3));                                                      // bar cart
        for(int i=0;i<6;i++){ csR(161+i,81+i,12-2*i,1,RGB(24,29,31)); } csR(166,87,2,5,RGB(24,29,31)); csR(163,92,8,1,RGB(24,29,31)); csR(166,83,2,2,RGB(6,20,6));   // the martini glass, an olive
        break;
    case CB_MIRROR:
        csR(0,12,SW,104,RGB(8,12,14)); for(int x=0;x<SW;x+=12) csR(x,12,1,104,RGB(6,9,11)); for(int y=12;y<116;y+=12) csR(0,y,SW,1,RGB(6,9,11));
        csR(66,14,108,84,RGB(22,22,16)); for(int i=0;i<78;i++) csR(69,17+i,102,1,RGB(10-i*4/78,14-i*5/78,18-i*4/78));
        csFig(120,94,CA_MISSY,CP_HEAD,t);   // the reflection (the beat's pose is ignored: she hangs her head)
        csR(40,98,160,18,RGB(24,24,26)); csR(40,98,160,2,RGB(30,30,31)); csR(116,88,8,10,RGB(16,16,19)); csR(112,86,16,3,RGB(16,16,19));
        break;
    case CB_SITE:
        csGrad(12,70,26,12,6,6,8,18); csR(0,82,SW,34,RGB(10,7,4)); csR(0,82,SW,2,RGB(15,11,6));
        for(int i=0;i<4;i++){ csR(14+i*30,50,3,34,RGB(18,13,6)); } csR(10,50,100,3,RGB(18,13,6)); csR(10,64,100,2,RGB(18,13,6)); csR(10,76,100,2,RGB(18,13,6));   // the house frame
        csR(128,28,3,88,RGB(14,14,16)); csR(172,28,3,88,RGB(14,14,16)); for(int y=40;y<110;y+=22){ csR(128,y,47,2,RGB(14,14,16)); csLn(131,y,172,y+22,RGB(12,12,14)); } csR(124,56,55,3,RGB(19,13,6));   // scaffold tower and a plank
        csR(196,40,44,76,RGB(6,5,4)); for(int i=0;i<9;i++) csR(198+i*5,36+(i&1)*3,3,80,RGB(9,7,4));                                                     // the fence
        for(int i=0;i<4;i++) if((((t>>2)+i*5)&7)<2){ int fx2=206+((i*13)&24), fy=50+((i*11)&31); csR(fx2,fy,2,2,RGB(31,31,31)); csR(fx2-1,fy,4,1,RGB(31,31,26)); }   // flashbulbs
        break;
    case CB_HOSP: case CB_FLAT:
        csGrad(12,104,22,27,24,17,23,20); csR(0,110,SW,6,RGB(13,15,14));
        csR(110,80,100,10,RGB(29,29,30)); csR(110,90,100,6,RGB(14,18,22)); csR(106,70,4,40,RGB(20,22,22)); csR(210,76,4,34,RGB(20,22,22));             // the bed
        csR(14,30,34,30,RGB(2,3,4)); { int py=45; for(int x=0;x<32;x++){ int ph=(x+t/2)%32; int y=py; if(bg!=CB_FLAT){ if(ph==14) y=py-9; else if(ph==15) y=py+6; } csR(15+x,y,1,1,bg==CB_FLAT?RGB(31,8,6):RGB(8,31,12)); } }   // the monitor
        csR(28,60,4,50,RGB(16,18,18)); csR(20,106,20,4,RGB(16,18,18)); csR(226,24,2,40,RGB(20,22,22)); csR(222,24,10,12,RGB(24,29,31));
        break;
    case CB_RATE: {   // the approval board: her line climbs for years, then falls off a cliff (drawn a bit more every frame)
        csR(0,12,SW,104,RGB(2,3,6)); csR(14,18,212,92,RGB(1,2,4));
        for(int gy=30;gy<=90;gy+=15) csR(20,gy,200,1,RGB(4,6,9));
        csR(20,24,1,78,RGB(10,12,14)); csR(20,102,201,1,RGB(10,12,14));
        int n=t*3; if(n>200) n=200; int py=92; u16 col=RGB(8,28,12);
        for(int i=0;i<n;i++){
            int y; if(i<110) y=92-i*52/110-((i%9)==0?2:0); else if(i<134) y=40+(i-110)*54/24; else y=94+((i>>2)&1);
            col=i<110?RGB(8,28,12):RGB(31,6,6);
            if(i>0) csLn(20+i-1,py,20+i,y,col); csR(20+i,y,1,1,col); py=y;
        }
        csD(20+(n>0?n-1:0),py,2,col);
        text(24,14,"APPROVAL",RGB(20,24,28),1);
        { char b[20]; char*e=csNum(b,(100-py)*70); e[0]='K'; e[1]=0; text(176,14,b,col,1); }
        break; }
    case CB_BACK:
        csGrad(12,104,14,6,10,8,3,7); csR(0,106,SW,10,RGB(8,5,4));
        csR(58,18,124,66,RGB(8,8,12)); csR(61,21,118,60,RGB(14,20,26)); for(int i=0;i<12;i++) csD(63+i*10,19+((i*3)&1),2,(((t>>4)+i)&3)?RGB(31,28,10):RGB(20,16,5));   // the mirror and its bulbs
        csR(46,84,148,6,RGB(16,10,7)); csR(40,90,4,16,RGB(16,10,7)); csR(196,90,4,16,RGB(16,10,7));
        csD(214,28,9,RGB(28,28,28)); csD(214,28,7,RGB(5,5,8)); csLn(214,28,214,22,RGB(28,28,28)); csLn(214,28,218,30,RGB(28,28,28));          // the clock
        break;
    default: csR(0,12,SW,104,0); break;
    }
}

static int csLen(const char*s){ int n=0; while(s[n]) n++; return n; }
static int csIsq(int v){ int w=0; while((w+1)*(w+1)<=v) w++; return w; }

static void csDraw(const CsBeat*b,int t,int shown){   // one frame of one beat (shown: how many letters of the caption are typed)
    csOx=csOy=0; if(b->fx&CF_SHAKE){ csOx=(rnd8()%5)-2; csOy=(rnd8()%5)-2; }
    rect(0,0,SW,SH,0);
    csBg(b->bg,t,b->fx);
    int ay=110, by=110;
    if(b->bg==CB_HOSP||b->bg==CB_FLAT){ if(b->pb==CP_LIE) by=98; }
    if(b->bg==CB_SITE){ if(b->pb==CP_CLIMB) by=110-(t/3>48?48:t/3); if(b->pb==CP_FLAIL){ by=62+t*t/20; if(by>110) by=110; } if(b->pb==CP_LIE) by=111; }
    if(b->a) csFig(b->ax*4,ay,b->a,b->pa,t);
    if(b->b) csFig(b->bx*4,by,b->b,b->pb,t);
    if(b->fx&CF_SICK){ int mx=b->bx*4, my=by-26; for(int k=0;k<9;k++) if(t>k*2) csR(mx+5+k*3,my+k*k/3-3,2,2,k&1?RGB(13,24,4):RGB(18,28,6)); }
    if(b->fx&CF_IRIS){ int r=130-t*2; if(r<0) r=0; int cx=b->bx*4, cy=by-27;
        for(int y=12;y<116;y++){ int dy=y-cy, v=r*r-dy*dy; if(v<=0){ rect(0,y,SW,1,0); continue; } int w=csIsq(v); if(cx-w>0) rect(0,y,cx-w,1,0); if(cx+w<SW) rect(cx+w,y,SW-cx-w,1,0); } }
    rect(0,0,SW,12,0); rect(0,116,SW,44,RGB(2,3,8)); rect(0,116,SW,1,RGB(14,11,3));
    text(205,3,"START SKIP",RGB(8,9,11),1);
    int y0=b->who?129:124; if(b->who) text(12,119,b->who,GOLD,1);
    static char buf[64]; int left=shown;
    for(int i=0;i<3&&b->t[i];i++){ int n=csLen(b->t[i]); int k=left<n?left:n; if(k<=0) break; for(int j=0;j<k;j++) buf[j]=b->t[i][j]; buf[k]=0; text(12,y0+i*9,buf,b->who?WHITE:RGB(22,26,31),1); left-=n; if(left<=0) break; }
}

// ---- the scenes (TV SHOW & TELL): 0-3 close chapters 1-4, 4 is the news that opens chapter 5, 5 closes the story ----
static const CsBeat csS0[]={
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,CF_FADEIN,0,0,0,{"The final act takes a bow to polite, uncertain applause.","At the judges' table, Missy Jeanne stopped pretending to","take notes an hour ago."}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,"Dex",{"Missy, any last words for our finalists?",0,0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,"Missy",{"Last words? Oh, Dex. Sweetheart. I","have SO many words.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_SWAY,37,0,SFX_BONK+1,0,0,{"She stands. The chair does not.",0,0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_SWAY,37,CF_SHAKE,0,0,"Missy",{"You call that a talent show? Let me show","you a TALENT show!",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_CLIMB,37,0,0,0,"Dex",{"Missy, please sit down. Missy. Are we still live? Tell me","we're not still live.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_CLIMB,37,0,SFX_POP+1,0,0,{"They are still live. She hauls herself onto the stage","with one heel in her hand, and the crowd realizes it","isn't part of the show."}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_DANCE,37,CF_STROBE,0,0,0,{"And then she dances. The kind of dancing that gets a","performer discovered, or gets a crowd to hold up their","phones. This is the second kind."}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_SING,37,CF_STROBE|CF_SHAKE,0,0,"Missy",{"La la LAAA! Somebody turn up my monitor!",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_DANCE,37,CF_STROBE|CF_SHAKE,0,0,0,{"The spins get wider. The singing gets louder. The floor","begins to tilt in a way that floors really shouldn't.",0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,37,CF_SHAKE,0,0,"Missy",{"Oh. Oh, no. I think I'm going to...",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,37,CF_SICK|CF_SHAKE,SFX_GROAN+1,70,0,{"Three cameras catch it from three different angles.",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,37,CF_IRIS|CF_AUTO,SFX_GASP+1,70,"Missy",{"...Ow.",0,0}},
 {CB_BLACK,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN|CF_FADEOUT,0,0,0,{"By morning the clip had been replayed eleven million","times. Her agent stopped answering at nine.",0}},
};
static const CsBeat csS1[]={
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,22,CF_FADEIN,0,0,0,{"Day seven. 11:04 p.m. Fifty-six minutes from a full","week, and the house has never been this quiet.",0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,22,0,0,0,0,{"On the bar cart sits a glass she poured on day one and","couldn't bring herself to drink or dump. It has been","waiting for her ever since."}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,22,0,0,0,"Missy",{"One. Just one. It's practically a technicality.",0,0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,22,0,0,0,"Missy",{"Nobody would even know.",0,0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,22,0,SFX_TICK+1,0,0,{"Her phone lights up on the table. It's the network.",0,0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,22,0,0,0,0,{"'Missy, we're going in another direction. Please don't","take it personally.'",0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,22,0,0,0,"Missy",{"Another direction. Everybody is always going in another","direction. Funny how it's always the one away from me.",0}},
 {CB_HOME,CA_NONE,CP_STAND,0,CA_MISSY,CP_RUN,22,0,0,10,0,{"She makes it as far as the bathroom.",0,0}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN,0,0,0,{"The woman in the mirror has Missy's face and","none of her sparkle.",0}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_SHAKE,0,0,"Missy",{"Who is that?",0,0}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_SHAKE,SFX_CRY+1,0,"Missy",{"...That's me. That's the woman from every","headline, every clip, every meltdown. I did that.","Nobody did that TO me."}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,0,0,0,"Missy",{"I can't keep being her. I don't even know","how to stop being her.",0}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,0,0,0,0,{"She is still on the cold tile when the clock rolls past","midnight. One week. Seven days. She doesn't feel proud.","She just feels awake."}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,0,0,0,"Missy",{"Okay. Not for the network. Not for the fans. For me.","And for Mamesy, who still picks up when I call.",0}},
 {CB_MIRROR,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEOUT,0,0,0,{"Tomorrow she will try again.","Tomorrow, she'll try harder.",0}},
};
static const CsBeat csS2[]={
 {CB_SITE,CA_CREW,CP_STAND,10,CA_MISSY,CP_STAND,32,CF_FADEIN,0,0,0,{"The new house rises behind twelve-foot walls and a very","expensive gate. After months of lenses in the hedges,","Missy has learned exactly what 'private' costs."}},
 {CB_SITE,CA_CREW,CP_STAND,10,CA_MISSY,CP_STAND,32,0,0,0,0,{"Her crew has opinions, selfies, and a worrying","relationship with the blueprints.",0}},
 {CB_SITE,CA_CREW,CP_STAND,10,CA_MISSY,CP_STAND,32,0,0,0,"Hal",{"Ma'am, I'm going to ask you one more time to stay","off the scaffolding.",0}},
 {CB_SITE,CA_CREW,CP_STAND,10,CA_MISSY,CP_STAND,32,0,0,0,"Missy",{"It's my house, Hal. I'll stand wherever I want.",0,0}},
 {CB_SITE,CA_CREW,CP_HEAD,10,CA_MISSY,CP_STAND,32,0,SFX_HIT+1,0,0,{"Behind her, a panel cracks. Someone 'accidentally'","tips a bucket of mix across the new floor. Hours of","work, gone in one splash."}},
 {CB_SITE,CA_CREW,CP_HEAD,10,CA_MISSY,CP_STAND,32,0,0,0,"Hal",{"Don't touch that. Missy, don't touch that.",0,0}},
 {CB_SITE,CA_CREW,CP_HEAD,10,CA_MISSY,CP_FLAIL,32,CF_SHAKE,0,0,"Missy",{"Do you people want to get paid or not?! Give it","here. I'll do it myself!",0}},
 {CB_SITE,CA_CREW,CP_HEAD,10,CA_MISSY,CP_CLIMB,32,0,0,40,0,{"Her temper climbs faster than she does.",0,0}},
 {CB_SITE,CA_CREW,CP_FLAIL,10,CA_MISSY,CP_CLIMB,32,0,0,0,"Hal",{"Come down! That platform isn't rated for- Missy!",0,0}},
 {CB_SITE,CA_CREW,CP_FLAIL,10,CA_MISSY,CP_CLIMB,32,0,0,0,0,{"Fifteen feet up, she wedges the beam into place with","her bare hands. For one glorious second, she is winning.",0}},
 {CB_SITE,CA_CREW,CP_FLAIL,10,CA_MISSY,CP_CLIMB,32,0,SFX_NEARLY+1,0,"Missy",{"See? Easy! You just have to-",0,0}},
 {CB_SITE,CA_NONE,CP_STAND,0,CA_MISSY,CP_FLAIL,32,CF_SHAKE,SFX_SCREAM+1,48,0,{0,0,0}},
 {CB_SITE,CA_NONE,CP_STAND,0,CA_MISSY,CP_LIE,32,CF_FLASH,SFX_HIT+1,50,0,{0,0,0}},
 {CB_SITE,CA_NONE,CP_STAND,0,CA_MISSY,CP_LIE,32,CF_FADEOUT,0,0,0,{"Beyond the fence the flashbulbs keep popping, at","the gate, at the hedge, at nothing. For once, nobody","is taking her picture."}},
};
static const CsBeat csS3[]={
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,CF_FADEIN,0,0,0,{"Day nine. The machines do the breathing now, and","Mamesy does the talking.",0}},
 {CB_HOSP,CA_DOC,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Dr. Okafor",{"Ms. Jeanne, we need to talk about next steps. Her scans","haven't changed. I'm so sorry, I know how hard this is.",0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,0,0,"Mamesy",{"One more night. Please. She's stubborn. She has never","once missed an entrance in her life.",0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,0,0,0,{"Mamesy pulls the chair close and takes her","sister's hand. It is the first time she has let","herself cry in front of anyone."}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Mamesy",{"Hey, Mish. You still owe me a duet. And a rematch at","that card game you cheat at.",0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,SFX_TICK+1,0,0,{"The monitor keeps its slow green rhythm.","Somewhere between the third beep and the fourth,","something changes."}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,CF_AUTO,0,50,0,{"Her fingers move.",0,0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Missy",{"...Mame? Why is the ceiling so clean?",0,0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,CF_SHAKE,0,0,"Mamesy",{"You absolute idiot. You absolute, glorious idiot.",0,0}},
 {CB_HOSP,CA_DOC,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Dr. Okafor",{"A concussion, some scrapes... and by every","measure she shouldn't be awake. Missy, do you","remember what happened?"}},
 {CB_HOSP,CA_DOC,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Missy",{"There was scaffolding. And a lot of martinis, I think?","Honestly, it's all a bit of a blur.",0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Mamesy",{"Then let it stay a blur.",0,0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Missy",{"Is that rain? Somebody open the","window. I want to hear it.",0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,0,{"She listens to the rain as if it were a symphony. The","whole world has been turned up.",0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Missy",{"Everything's so bright. Was it always this beautiful?",0,0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Mamesy",{"Always was. You just had your eyes closed.",0,0}},
 {CB_HOSP,CA_MAME,CP_STAND,20,CA_MISSY,CP_LIE,41,CF_FADEOUT,0,0,"Missy",{"Then I'm done closing them. Mame, let's go win","everybody back. The right way.",0}},
};
static const CsBeat csS4[]={
 {CB_BACK,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,27,CF_FADEIN,0,0,0,{"Showtime minus forty minutes. The dressing room smells","like hairspray and second chances.",0}},
 {CB_BACK,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,27,0,0,0,"Missy",{"Where's Mame? She swore front row, center, and the","loudest whistle in the building.",0}},
 {CB_BACK,CA_HOST,CP_STAND,47,CA_MISSY,CP_STAND,27,0,0,0,"Dex",{"She'll be here. Breathe, Missy. Everything is perfect.","Nothing can possibly go wrong.",0}},
 {CB_BACK,CA_HOST,CP_STAND,47,CA_MISSY,CP_STAND,27,0,SFX_TICK+1,0,0,{"Then the phone rings. It isn't Mamesy.",0,0}},
 {CB_BACK,CA_HOST,CP_STAND,47,CA_MISSY,CP_HEAD,27,0,0,0,0,{"'Ms. Jeanne? This is the Route 9 Highway Patrol. There","has been a collision.'",0}},
 {CB_BACK,CA_HOST,CP_STAND,47,CA_MISSY,CP_HEAD,27,CF_SHAKE,SFX_GASP+1,0,0,{"The words arrive in the wrong order. Truck. Wet road.","Nothing anyone could have done.",0}},
 {CB_BACK,CA_HOST,CP_HEAD,47,CA_MISSY,CP_HEAD,27,0,0,0,"Dex",{"Missy? Missy, sit down. Please sit down.",0,0}},
 {CB_BACK,CA_HOST,CP_HEAD,47,CA_MISSY,CP_HEAD,27,0,0,0,0,{"In the mirror, a woman in a sparkling gown is being told","something that cannot be told.",0}},
 {CB_BACK,CA_HOST,CP_STAND,47,CA_MISSY,CP_HEAD,27,0,0,0,"Dex",{"We can hold the show. Say the word and we hold it. All","night, if we have to.",0}},
};
static const CsBeat csS5[]={
 {CB_BIG,CA_HOST,CP_STAND,17,CA_MISSY,CP_STAND,43,CF_FADEIN,0,0,0,{"The house lights fall. Eleven million people","watched her worst night. Tonight a few thousand","will watch what comes next."}},
 {CB_BIG,CA_HOST,CP_STAND,17,CA_MISSY,CP_STAND,43,0,0,0,"Dex",{"Ladies and gentlemen, a woman who needs no","introduction. Which is lucky, because she skipped every","rehearsal. Missy Jeanne!"}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,43,0,0,0,0,{"She had a speech. A good one, with a joke about","the bars and a sparkle in every sentence. She","rehearsed it for weeks."}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,43,0,0,0,0,{"She doesn't remember a single word.",0,0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,43,0,0,0,"Missy",{"I was going to tell you I'm sorry. For all of it. For","two years of it. I am.",0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,43,0,0,0,"Missy",{"But I came to sing for someone tonight. She was","supposed to be here. She was always going to be here.",0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,43,0,0,0,"Missy",{"She'd say this song is far too sentimental. She would","sing every word anyway.",0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_SING,43,0,SFX_POP+1,0,0,{"Here Today, by Paul McCartney.",0,0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_SING,43,CF_AUTO,0,140,0,{"She doesn't hit every note. She doesn't hide","behind a single one.",0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_SING,43,CF_AUTO,0,150,0,{"In the front row, dead center, one seat stays empty.","Nobody asks to sit in it.",0}},
 {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_STAND,43,0,0,0,"Missy",{"Thanks for coming, Mame.",0,0}},
 {CB_BLACK,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN|CF_FADEOUT,0,0,0,{"Some nights she still pours two glasses of water. One","for her. One for the front row.",0}},
};
// CH4 LOSS: the plug pressure hit 100 for the second time (story.h stPlugLose plays this, then the game goes back to your last save). CB_FLAT = the hospital with a flat green line.
static const CsBeat csS6[]={
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,CF_FADEIN,0,0,0,{"The pressure reached a hundred. There were no more","nights left to give her.",0}},
 {CB_HOSP,CA_DOC,CP_STAND,20,CA_MISSY,CP_LIE,41,0,0,0,"Dr. Okafor",{"Ms. Jeanne, the hospital has made its decision.","I am so sorry. I fought for every night I could.",0}},
 {CB_HOSP,CA_DOC,CP_STAND,20,CA_MISSY,CP_LIE,41,CF_SHAKE,SFX_CRY+1,0,"Mamesy",{"No. Please. One more night. She has never once","missed an entrance.",0}},
 {CB_HOSP,CA_DOC,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,0,0,"Dr. Okafor",{"I'll give you a few minutes with her.",0,0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,SFX_TICK+1,0,0,{"Mamesy takes her sister's hand. The monitor keeps","its slow green rhythm.",0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,0,0,"Mamesy",{"Hey, Mish. You still owe me a duet.",0,0}},
 {CB_HOSP,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,SFX_TICK+1,0,0,{"The machines are switched off, one at a time. The","room gets very quiet.",0}},
 {CB_FLAT,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,CF_AUTO,SFX_DEATH+1,70,0,{"The green line goes flat.",0,0}},
 {CB_FLAT,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,CF_SHAKE,SFX_CRY+1,0,"Mamesy",{"Mish? ...Mish.",0,0}},
 {CB_FLAT,CA_MAME,CP_HEAD,20,CA_MISSY,CP_LIE,41,0,0,0,0,{"Missy Jeanne is gone. For the last time, nobody is","taking her picture.",0}},
 {CB_BLACK,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN|CF_FADEOUT,0,0,0,{"Some stories do not get a second night.",0,0}},
};
// OPENING: the night it all started, live on TV (STORY MODE > TV SHOW & TELL plays this once, before chapter 1). Drunk judging, a gasp, the rush to the stage, the sick, the approval board falling off a cliff.
static const CsBeat csS7[]={
 {CB_BLACK,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN,0,0,0,{"Present day. Studio 9. Live, in front of four million","viewers and one very patient host.",0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_STAND,37,CF_FADEIN,0,0,0,{"For six years Missy Jeanne was the nation's favorite","judge. Sharp, sparkling, never once late for a cue.",0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,0,{"Tonight her water glass has been refilled eleven times.","Nobody has the heart to tell the crew it isn't water.",0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,"Dex",{"Missy, thoughts on that last performance?",0,0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,"Missy",{"Thoughts? Sweetheart, I had a lovely nap.","Wake me when somebody sings.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_SWAY,37,0,0,0,0,{"The audience laughs. They think it's a bit. Dex, who has","worked beside her for six years, does not laugh.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_SWAY,37,CF_SHAKE,SFX_GASP+1,0,"Missy",{"Who told you that you could sing, honey?","Whoever it was, they were lying to you.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_SWAY,37,0,0,0,0,{"A gasp rolls through the studio. In the control room a","producer says 'Stay on her,' very quietly. Nobody argues.",0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,0,0,0,"Dex",{"We're going to a break. Missy? We are going to a break.",0,0}},
 {CB_STAGE,CA_HOST,CP_STAND,15,CA_MISSY,CP_SWAY,37,CF_SHAKE,0,0,"Missy",{"I don't want a break. I want the STAGE.",0,0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_RUN,37,CF_SHAKE,SFX_BONK+1,0,0,{"She stands too fast and the studio tips sideways. With","the total confidence of the truly gone, she goes anyway.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_RUN,44,CF_SHAKE,0,10,0,{0,0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_RUN,38,CF_SHAKE,0,10,0,{0,0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_RUN,33,CF_SHAKE,0,10,0,{0,0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,30,0,0,0,"Missy",{"Everybody... watch me.",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,30,0,0,0,0,{"The spotlight finds her. So does the sudden, terrible","heat of every light in the building.",0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,30,CF_SHAKE,0,0,"Missy",{"Oh. That's... oh, no.",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,30,CF_SICK|CF_SHAKE,SFX_GROAN+1,80,0,{0,0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,30,0,0,0,0,{"Camera two pushes in. Nobody in the control room","says cut.",0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,30,CF_AUTO,0,60,0,{"Three seconds of dead silence.",0,0}},
 {CB_STAGE,CA_NONE,CP_STAND,0,CA_MISSY,CP_HEAD,30,0,SFX_POP+1,0,0,{"Then somebody in the third row laughs. Then a phone","comes up. Then four hundred phones.",0}},
 {CB_STAGE,CA_HOST,CP_HEAD,15,CA_MISSY,CP_HEAD,30,0,0,0,"Dex",{"We are... experiencing technical difficulties.",0,0}},
 {CB_RATE,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN,0,0,0,{"By midnight the clip had left the building, the city and","the country. Her approval rating went with it.",0}},
 {CB_RATE,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,0,SFX_TICK+1,0,0,{"Six years of goodwill, gone in eleven seconds. The","sponsors left first. The fans left next.",0}},
 {CB_BLACK,CA_NONE,CP_STAND,0,CA_NONE,CP_STAND,0,CF_FADEIN|CF_FADEOUT,0,0,0,{"Her phone buzzed forty-one times before sunrise. Only","one voicemail was worth hearing: 'Mish. Pick up. Please.'",0}},
};
static const CsScene csScenes[8]={ {csS0,sizeof(csS0)/sizeof(csS0[0])}, {csS1,sizeof(csS1)/sizeof(csS1[0])}, {csS2,sizeof(csS2)/sizeof(csS2[0])}, {csS3,sizeof(csS3)/sizeof(csS3[0])}, {csS4,sizeof(csS4)/sizeof(csS4[0])}, {csS5,sizeof(csS5)/sizeof(csS5[0])}, {csS6,sizeof(csS6)/sizeof(csS6[0])}, {csS7,sizeof(csS7)/sizeof(csS7[0])} };
static const char* const csNames[8]={ "CH1 END  THE BARS", "CH2 END  THE SWEATER", "CH3 END  THE FALL", "CH4 END  WAKING UP", "CH5 START  THE NEWS", "CH5 END  HERE TODAY", "CH4 LOSS  THE PLUG", "OPENING  THE NIGHT" };

#ifndef CS_HOST
static void csPlay(int id){   // play scene id; returns when it ends or START skips it
    volatile u16*bc=(volatile u16*)0x04000050; volatile u16*bl=(volatile u16*)0x04000054;
    const CsScene*sc=&csScenes[id]; clipAll(); objHideAll();
    u16 prev=keyNow(); int skip=0;
    for(int bi=0;bi<sc->n&&!skip;bi++){
        const CsBeat*b=&sc->b[bi]; int total=0; for(int i=0;i<3&&b->t[i];i++) total+=csLen(b->t[i]);
        int t=0, shown=0, rest=0; if(b->sfx) sfxPlay(b->sfx-1);
        for(;;){
            u16 k=keyNow(), pr=k&~prev; prev=k;
            if(pr&K_START){ skip=1; break; }
            if(total){ if(shown<total){ shown=t/2; if(shown>total) shown=total; if(pr&K_A) shown=total; } else rest++;
                if(shown>=total&&((b->fx&CF_AUTO)?rest>(b->dur?b->dur:60):(rest>8&&(pr&K_A)))) break; }
            else if(t>=(b->dur?b->dur:60)) break;
            *bc=0x00C4; *bl=0; if((b->fx&CF_FADEIN)&&t<16){ *bl=16-t; } else if((b->fx&CF_FLASH)&&t<14){ *bc=0x0084; *bl=14-t; }
            csDraw(b,t,shown); present(); t++;
        }
        if(!skip&&(b->fx&CF_FADEOUT)){ *bc=0x00C4; for(int i=0;i<=16;i++){ *bl=i; csDraw(b,t,total); present(); } for(int i=0;i<14;i++){ present(); } }
    }
    *bc=0x0400; *bl=0; objHideAll(); clipAll();
    while(keyNow()&(K_A|K_B|K_START)) vsync();   // let go before the next screen reads the keys
}
#endif

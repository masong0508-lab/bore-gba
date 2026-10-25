// bortua3d.h - BORTUA FIGHTER's BALLZ MODE: the same ladder of nine as the arcade mode, but fought in real 3D (bortua3dcore.h has the fighters, the blows and the camera;
// this file draws them and runs the match). Every fighter is 21 shaded balls hung on a skeleton (like Ballz 3D, 1994), painted far to near in the screen buffer with
// plain rectangles: no sprites, so the other mode's art stays out of the way. A frame is worked out after the last one went up in the vblank; the fight itself runs on the
// timer in fixed 60 Hz steps, so a slow frame costs picture, never speed.
//   OPTIONS (INPUT page)  CAMERA  SIDE (square on, you on the left: the classic view) / CHASE (over your shoulder) / HIGH (the whole ring from above); SELECT changes it mid-fight
//                         MOVEMENT  SCREEN (the pad moves you as the screen sees it: right is right, up is into the screen) / ORBIT (up closes in, down backs off, left and right circle them)
//   BUTTONS               see bortua3dcore.h; START pauses.   RULES as the arcade mode: 30 second rounds, best of three, K.O., RING OUT or more life left at TIME.
#define BZ_HUDH 26
static Bz bzF[2]; static BzCam bzCam; static u16 bzM[2][5][3];     // the fighters, the camera, and each one's five materials (skin, top, bottom, hair, shoes) as dark / mid / light
typedef struct { s16 x, y, r; s16 z; u8 who, ball; } BzSp;
static BzSp bzSp[2*BZ_NBALL]; static u8 bzOrd[2*BZ_NBALL]; static int bzNSp;
typedef struct { s32 x, z; int y, t; } BzSpark;
static BzSpark bzSpk[2];
static int bzStop, bzShake, bzFlash, bzCamSel, bzMoveSel;
static void bzMaterials(int i){   // the colours of fighter i from its sprite palette (bortua.h made them): the three shades of each material
    const u16*p=bfF[i].pal; u16 c[5]; c[0]=p[3]; c[1]=p[6]; c[2]=p[9]; c[3]=p[12]; c[4]=RGB(5,4,8);
    for(int m=0;m<5;m++) bfRamp(bzM[i][m],c[m]);
}
static void bzSetFighter(int i,int stage){   // fighter i of the match: who it is, how it is built
    const Bf*b=&bfF[i]; Bz*f=&bzF[i]; (void)stage;
    f->pw=b->pw; f->sp=b->sp; f->d=b->d; f->body=b->body==BFB_HEAVY?1:b->body==BFB_SLIM?2:0; f->wins=0;
    bzMaterials(i);
}
static void bzBegin(void){   // a round's start: on their marks, the camera beside them
    bzReset(&bzF[0],-900,0,64); bzReset(&bzF[1],900,0,192);
    bzSpk[0].t=bzSpk[1].t=0; bzStop=bzShake=bzFlash=0; bzCallT=0;
    bzCamUpdate(&bzCam,bzCamSel,&bzF[0],&bzF[1],1);
}
// ---- the picture ----
static u16 bzSky(int k){ return RGB(2+(k*7>>8),2+(k*5>>8),9+(k*13>>8)); }
static int bzCamS, bzCamC;                                 // the camera's sin and cos, once a frame
static void bzBackdrop(int top){   // sky, the crowd's skyline, and the ring: every pixel of the floor written once (a row is up to seven spans: void, rim, mat, the painted circle and back)
    const BzCam*c=&bzCam; bzCamS=fsin(c->yaw); bzCamC=fcos(c->yaw);
    int yh=c->yh; if(yh<top) yh=top; if(yh>SH-2) yh=SH-2; int y0=(yh+1)&~1;
    for(int y=top;y<y0;y+=4){ int k=(y-top)*256/(y0-top+1); int h=y0-y<4?y0-y:4; rect(0,y,SW,h,bzSky(k)); }
    for(int x=0;x<SW;x+=8){ int hh=3+((x*7+(x>>3)*13)&7); rect(x,y0-hh,8,hh,RGB(3,2,6)); }   // (the crowd, as a dark skyline)
    int ox=(int)(-c->x), oz=(int)(-c->z), cxx=(ox*bzCamC-oz*bzCamS)>>8, czz=(ox*bzCamS+oz*bzCamC)>>8, vk=65536/(SH-y0+1);
    static const u16 rim=RGB(26,20,6), mat=RGB(5,8,18), cir=RGB(8,11,24);
    const int R1=(BZ_R+170)*(BZ_R+170), R2=BZ_R*BZ_R, R3=(BZ_R*55/100)*(BZ_R*55/100);
    for(int y=y0;y<SH;y+=2){
        int zc=c->f*c->h/(y+1-yh); if(zc>32000) zc=32000; int dz=zc-czz, dz2=dz*dz, q1=R1-dz2, q2=R2-dz2, q3=R3-dz2, k=((y-y0)*vk)>>16;
        u16 vc=RGB(1+(k>>7),1+(k>>7),4+(k>>6));
        if(q1<=0){ rect(0,y,SW,2,vc); continue; }
        int inv=(c->f<<16)/zc, h1=bzSqrt(q1), xa=120+(((cxx-h1)*inv)>>16), xb=120+(((cxx+h1)*inv)>>16);
        rect(0,y,xa,2,vc); rect(xb,y,SW-xb,2,vc);
        if(q2<=0){ rect(xa,y,xb-xa,2,rim); continue; }
        int h2=bzSqrt(q2), xc=120+(((cxx-h2)*inv)>>16), xd=120+(((cxx+h2)*inv)>>16);
        rect(xa,y,xc-xa,2,rim); rect(xd,y,xb-xd,2,rim);
        if(q3<=0){ rect(xc,y,xd-xc,2,mat); continue; }
        int h3=bzSqrt(q3), xe=120+(((cxx-h3)*inv)>>16), xf=120+(((cxx+h3)*inv)>>16);
        rect(xc,y,xe-xc,2,mat); rect(xe,y,xf-xe,2,cir); rect(xf,y,xd-xf,2,mat); }
}
static void bzShadow(const Bz*f){
    const BzCam*c=&bzCam; int sx,sy,iv; if(f->fall||!bzProjI(c,bzCamS,bzCamC,f->x,0,f->z,&sx,&sy,&iv)) return;
    int k=256-f->y*256/2600; if(k<40) k=40;
    int rx=(520*iv>>16)*k>>8, ry=rx*c->h*(iv>>8)/(c->f*256); if(ry<1) ry=1; if(rx<2) return;
    for(int dy=-ry;dy<=ry;dy++){ int hw=rx*bzSqrt(ry*ry-dy*dy)/ry; rect(sx-hw,sy+dy,hw*2+1,1,RGB(1,2,6)); }
}
#define BZ_RMAX 44
static u8 bzHW[BZ_RMAX+1][BZ_RMAX+1] EWRAM_BSS; static u8 bzHWok;   // a disc's half widths: row dy of radius r (the sqrt is done once, here)
static void bzDisc(int cx,int cy,int r,u16 col){ if(r<1) r=1; if(r>BZ_RMAX) r=BZ_RMAX; discLut(cx,cy,r,col,bzHW[r]); }   // (discLut: main.c, in IWRAM)
static void bzBallDraw(int x,int y,int r,const u16*m){   // a sphere: a dark rim, the body, a lit patch
    if(r<=4){ bzDisc(x,y,r,m[1]); return; }
    if(r>BZ_RMAX) r=BZ_RMAX;
    bzDisc(x,y,r,m[0]); bzDisc(x-r/6,y-r/6,r*4/5,m[1]); if(r>=9) bzDisc(x-r*2/5,y-r*2/5,r*2/5,m[2]);
}
static void bzSpark(const BzSpark*k){
    int sx,sy,iv; if(k->t<=0||!bzProjI(&bzCam,bzCamS,bzCamC,k->x,k->y,k->z,&sx,&sy,&iv)) return;
    int r=((8-k->t)*70*iv>>16)+3; u16 c=k->t>5?WHITE:RGB(31,24,4);
    for(int i=0;i<8;i++){ int a=i*32, rr=i&1?r:r*2/3; int px=sx+(fsin(a)*rr>>8), py=sy+(fcos(a)*rr>>8); bzDisc(px,py,2,c); }
    bzDisc(sx,sy,r/3+1,c);
}
static void bzAddBalls(int w){   // fighter w's balls into the draw list
    BzBall bl[BZ_NBALL]; bzBalls(&bzF[w],bl);
    for(int i=0;i<BZ_NBALL;i++){ int sx,sy,iv; if(bl[i].r<=0||!bzProjI(&bzCam,bzCamS,bzCamC,bl[i].x,bl[i].y,bl[i].z,&sx,&sy,&iv)) continue;
        BzSp*q=&bzSp[bzNSp]; q->x=(s16)sx; q->y=(s16)sy; q->z=(s16)(-(iv>>5)); q->r=(s16)((bl[i].r*iv)>>16); q->who=(u8)w; q->ball=bl[i].col; bzNSp++; }   // (z: bigger = nearer: sorted far first)
}
static void bzDrawList(void){   // far to near: an insertion sort of indices (no structs move)
    for(int i=0;i<bzNSp;i++){ int j=i; while(j>0&&bzSp[bzOrd[j-1]].z>bzSp[i].z){ bzOrd[j]=bzOrd[j-1]; j--; } bzOrd[j]=(u8)i; }
    for(int n=0;n<bzNSp;n++){ const BzSp*s=&bzSp[bzOrd[n]]; const u16*m=bzM[s->who][s->ball];
        if(bzF[s->who].flash>0){ static const u16 fl[3]={RGB(20,20,22),RGB(28,28,29),RGB(31,31,31)}; m=fl; }
        bzBallDraw(s->x,s->y+(bzShake&1),s->r,m); }
}
static void bzScene(void){
    cY0=BZ_HUDH; cH=SH-BZ_HUDH;      // (the HUD above the scene stays as it is)
    bzBackdrop(BZ_HUDH);
    bzShadow(&bzF[0]); bzShadow(&bzF[1]);
    bzNSp=0; bzAddBalls(0); bzAddBalls(1); bzDrawList();
    for(int i=0;i<2;i++) bzSpark(&bzSpk[i]);
    cY0=0; cH=SH;
}
// ---- the HUD (drawn into the top rows of the screen buffer; the scene never touches them) ----
static int bzShownT=-1;
static void bzBar(int x,int w,int rev){ if(w<=0) return; int xx=rev?x-w:x; rect(xx,11,w,2,RGB(31,30,16)); rect(xx,13,w,2,RGB(31,25,4)); rect(xx,15,w,2,RGB(25,16,2)); }
static void bzHudBars(int secs){
    for(int i=0;i<2;i++){ const Bz*f=&bzF[i]; int x=i?132:8, w=100, fill=f->hp*w/100, tr=f->trail*w/100;
        rect(x-2,9,w+4,10,RGB(1,1,2)); rect(x-1,10,w+2,8,RGB(22,17,5)); rect(x,11,w,6,RGB(5,2,4));
        if(tr>0){ if(i) rect(x+w-tr,11,tr,6,RGB(24,3,3)); else rect(x,11,tr,6,RGB(24,3,3)); }
        bzBar(i?x+w:x,fill,i);
        for(int r=0;r<2;r++){ int cx=i?x+w-5-r*10:x+5+r*10; rect(cx-3,21,7,4,RGB(1,1,2)); rect(cx-2,22,5,2,r<f->wins?RGB(31,24,4):RGB(6,5,8)); } }
    if(secs!=bzShownT){ rect(110,6,20,18,RGB(1,1,2)); rect(111,7,18,16,RGB(6,3,10)); char b[4]; slNum(b,secs); text(120-tw(b,2)/2,8,b,secs<=5?RGB(31,8,6):WHITE,2); bzShownT=secs; }
}
static void bzHud(int secs){   // a round's HUD from scratch: the strip, the names (outlined text: once a round), the bars
    for(int y=0;y<BZ_HUDH;y++) rect(0,y,SW,1,RGB(2+(y>>3),1,5+(y>>2)));
    for(int i=0;i<2;i++){ int x=i?132:8, w=100; bfText(i?x+w-tw(bfF[i].nm,1):x,1,bfF[i].nm,WHITE,1); }
    bzShownT=-1; bzHudBars(secs);
}
static void bzBanner(int y,const char*t,u16 c,int sc){ int x=120-tw(t,sc)/2; text(x+1,y+1,t,RGB(1,1,3),sc); text(x,y,t,c,sc); }
static void bzPresent(void){ vsync(); REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR; REG_DMA3CNT=(SW*SH/2)|0x84000000u; }
// ---- the pad ----
static void bzPad(u16 k,BzIn*in,u16 pr){
    int ux=(k&K_RIGHT?1:0)-(k&K_LEFT?1:0), uy=(k&K_UP?1:0)-(k&K_DOWN?1:0); padFour(&ux,&uy);
    in->mx=in->mz=0;
    if(ux||uy){ if(bzMoveSel) bzOrbitVec(&bzF[0],&bzF[1],uy,ux,&in->mx,&in->mz); else bzScreenVec(&bzCam,uy,ux,&in->mx,&in->mz); }
    in->a=(k&K_A)!=0; in->b=(k&K_B)!=0; in->guard=(k&K_R)!=0; in->pa=(pr&K_A)!=0; in->pb=(pr&K_B)!=0; in->pj=(pr&K_L)!=0;
}
// ---- one match (best of three). 1 = you won, 0 = you lost, -1 = you quit ----
static int bzMatch(int stage){
    Bz*p=&bzF[0], *o=&bzF[1]; (void)stage; p->wins=o->wins=0;
    for(int round=1;;round++){
        bzBegin(); bzCallT=0; bzCall=0;
        int phase=0, pt=0, secs=30, winner=-1; const char*why=0; u16 prev=keyNow();
        char rname[12]; { char*e=slCat(rname,round==3?"FINAL ROUND":"ROUND "); if(round<3) slNum(e,round); }
        bzHud(30); bzScene(); bzPresent();
        voxSay(hhPUid,V_lets_fight);
        tmStart(); u16 tl=R_TM2D; int acc=0, done=0;
        for(;!done;){
            u16 now=R_TM2D, dt=(u16)(now-tl); tl=now; acc+=dt;
            int steps=acc/TICKS_FRAME; acc-=steps*TICKS_FRAME; if(steps>3){ steps=3; acc=0; }
            u16 k=keyNow(), pr=k&~prev; prev=k;
            if(pr&K_SEL){ bzCamSel=(bzCamSel+1)%BC_N; }
            if((pr&K_START)&&phase==1){ static const char* const pm[2]={"KEEP FIGHTING","QUIT BALLZ MODE"}; int q=menu("PAUSED",pm,2); if(q==1) return -1;
                prev=keyNow(); bzHud(secs); tmStart(); tl=R_TM2D; acc=0; continue; }
            for(int st=0;st<steps&&!done;st++){ if(st) pr=0;
                if(bzShake>0) bzShake--;
                if(bzStop>0){ bzStop--; continue; }
                pt++;
                BzIn pin, ain={0}; bzPad(k,&pin,pr);
                p->ev=o->ev=0;
                if(phase==0){ if(pt>=100){ phase=1; pt=0; } bzStep(p,o,&ain); bzStep(o,p,&ain); if(pt==60) sfxPlay(SFX_BELL); }
                else if(phase==1){
                    if(pt%60==59&&secs>0) secs--;
                    bzAi(o,p,&ain); bzStep(p,o,&pin); bzStep(o,p,&ain);
                    if(o->st==BS_KO&&o->hp==0){ winner=0; why="K.O."; } else if(p->st==BS_KO&&p->hp==0){ winner=1; why="K.O."; }
                    else if(o->fall&&o->y<-500){ winner=0; why="RING OUT"; } else if(p->fall&&p->y<-500){ winner=1; why="RING OUT"; }
                    else if(secs==0){ winner=p->hp>=o->hp?0:1; why="TIME UP"; }
                    if(winner>=0){ phase=2; pt=0; if(winner==0){ p->wins++; if(o->hp<=0&&p->hp==100) why="PERFECT"; } else o->wins++; sfxPlay(SFX_BELL); bzHud(secs); }
                } else { bzStep(p,o,&ain); bzStep(o,p,&ain);
                    if(pt==80){ Bz*wf=winner?o:p; if(wf->st!=BS_KO&&!wf->fall){ wf->st=BS_WIN; wf->t=0; } voxSay(hhPUid,winner?V_lost_the_fight:V_win_the_fight); }
                    if(pt>=200) done=1; }
                // what the step did, in sound and shake and sparks
                for(int w=0;w<2;w++){ Bz*a=&bzF[w]; int e=a->ev;
                    if(e&BZE_HIT){ sfxPlay(e&BZE_KO?SFX_KO:SFX_HIT); BzSpark*s=&bzSpk[w]; s->x=a->hx; s->z=a->hz; s->y=a->hy; s->t=8; bzStop=(e&BZE_BIG)?5:2; bzShake=(e&BZE_BIG)?8:4; }
                    if(e&BZE_BLOCK) sfxPlay(SFX_TICK);
                    if(e&BZE_LAND) sfxPlay(SFX_LAND);
                    if(e&BZE_THROW) sfxPlay(SFX_BONK); }
                for(int w=0;w<2;w++) if(bzSpk[w].t>0) bzSpk[w].t--;
                if(bzCallT>0) bzCallT--;
                bzCamUpdate(&bzCam,bzCamSel,p,o,0);
            }
            if(done) break;
            bzScene();
            const char*bt=phase==0?(pt<60?rname:"FIGHT!"):(phase==2&&pt<170)?why:bzCallT?bzCall:0;
            if(bt){ if(phase==0) bzBanner(60,bt,RGB(31,26,6),3); else if(phase==2){ bzBanner(52,bt,RGB(31,26,6),3); bzBanner(86,winner?"YOU LOSE THE ROUND":"YOU WIN THE ROUND",WHITE,1); } else bzBanner(52,bt,RGB(31,20,6),2); }
            bzHudBars(secs);
            bzPresent();
        }
        if(p->wins>=2) return 1;
        if(o->wins>=2) return 0;
    }
}
// ---- the cards between fights: the ring, the fighters standing on it, the camera turning slowly ----
static int bzT2Sc=1;   // the size of a card's second line (the CONTINUE? count is big)
static void bzCardDraw(const char*t1,const char*t2,const char*t3,int both){
    cY0=0; cH=SH; bzBackdrop(0); bzShadow(&bzF[0]); if(both) bzShadow(&bzF[1]);
    bzNSp=0; bzAddBalls(0); if(both) bzAddBalls(1); bzDrawList();
    bfTextC(5,t1,RGB(31,26,6),2); if(t2) bfTextC(bzT2Sc>1?40:26,t2,bzT2Sc>1?WHITE:RGB(17,29,31),bzT2Sc); if(t3) bfTextC(150,t3,DIMC,1);
}
// shows a card until a key (1 = A, 0 = B, -1 = it timed out after `frames`, when that is not 0). ko: you lie flat on the ring; win: you celebrate
static int bzCard(const char*t1,const char*t2,const char*t3,int both,int frames,int ko,int win){
    u16 prev=keyNow(); bzBegin();
    if(ko){ bzF[0].st=BS_KO; bzF[0].t=40; } if(win){ bzF[0].st=BS_WIN; bzF[0].yaw=128; }
    for(int c=0;;c++){ u16 k=keyNow(), pr=k&~prev; prev=k;
        bzF[0].cnt++; bzF[1].cnt++;
        bzCardDraw(t1,t2,t3,both); bzPresent();
        if(c>20&&(pr&K_A)) return 1; if(c>20&&(pr&K_B)) return 0; if(frames&&c>=frames) return -1; }
}
// ---- BALLZ MODE ----
XPF static int bz3dRun(void){
    objHideAll(); zoomOff(); u16 al=R_BLDAL; REG_DISPCNT=0x0403; R_BLDCNT=0; R_BLDY=0;
    bfLoad(); bzMoveSel=xo[XO_BZMOVE];
    if(!bzHWok){ for(int r=1;r<=BZ_RMAX;r++) for(int d=0;d<=r;d++) bzHW[r][d]=(u8)bzSqrt(r*r+r-d*d); bzHWok=1; }
    int stage=0, won=0;
    bfSetYou(&bfF[0]);
    for(;;){
        bfSetThem(&bfF[1],stage); bzSetFighter(0,stage); bzSetFighter(1,stage);
        { char b[24]; char*e=slCat(b,stage==8?"FINAL STAGE":"STAGE "); if(stage<8) slNum(e,stage+1);
          char vs[40]; e=slCat(vs,bfF[0].nm); e=slCat(e,"  VS  "); slCat(e,bfLadder[stage].nm);
          bzCamSel=BC_SIDE; if(!bzCard(b,vs,"A FIGHT   B QUIT",1,0,0,0)) break; }
        bzCamSel=xo[XO_BZCAM];
        int r=bzMatch(stage); if(r<0) break;
        if(r==1){ won++; if(bfWins<250) bfWins++; bfSave(); stQEvent(QE_KO,1);
            if(stage==8){ bfChamp=1; bfSave(); voxPlay(V_yahoo); bzCamSel=BC_SIDE; bzCard("BORTUA CHAMPION","NOBODY IS BORED OF YOU NOW",0,0,420,0,1); break; }
            stage++; }
        else {   // CONTINUE?: a countdown, like the arcade, with you flat on the ring
            int cont=0;
            for(int n=9;n>=0&&!cont;n--){ char b[4]; slNum(b,n); sfxPlay(SFX_TICK); bzCamSel=BC_SIDE; bzT2Sc=3; int q=bzCard("CONTINUE?",b,"A CONTINUE   B GIVE UP",0,60,1,0); bzT2Sc=1; if(q==1) cont=1; else if(q==0) n=-1; }
            if(!cont) break; }
    }
    R_BLDAL=al; objHideAll(); REG_DISPCNT=0x0403;
    hhSlotsFree(); hhExtraDone();
    return won;
}
// ---- the mode shelf: ARCADE MODE (2D, bortua.h) or BALLZ MODE (3D, this file) ----
XPF static int bfModePick(void){
    static const char* const it[2]={"ARCADE MODE   2D","BALLZ MODE   3D"};
    int q=menu("BORTUA FIGHTER",it,2);
    if(q==0) return bfRun();
    if(q==1) return bz3dRun();
    return 0;
}

// cscam.h - CUTSCENE CAMERA (module 2): zoom + pan over the picture area (y 12..115). Everything the scenes draw goes through csR / csD / csLn, so backdrops and figures all follow the camera.
// z is 8.8 fixed point (256 = the wide shot, 512 = 2x, 768 = 3x). csCamAim picks the shot by itself: the speaker gets a medium close-up, a SHAKE beat a tight one, SICK / IRIS
// beats an extreme close-up, two figures a two-shot. (csshot.h adds hand-made shot lists on top.)
// LIKE A FILM, not a slideshow: the camera is a dolly. Inside one framing size it PANS smoothly after what it frames (a spring: it eases off, glides and settles, and it follows
// a figure that walks or runs, from where the figure is NOW). A new size of shot, a new place or a move too big to pan is a CUT. Zooms are never animated (scaling the pixel art in
// steps makes it swim): a change of size is always a cut. Positions are kept in 1/16 px, so a slow pan moves the picture by a pixel at a time, not in jumps.
// Needs from cutscene.h: CsBeat, CB_* CA_* CF_* CP_*. Needs from main.c: rect, disc, line.
static int csCz=256, csCxF=120<<4, csCyF=64<<4, csVx, csVy, csCbg=-1, csCtz=256;   // zoom; the centre (1/16 px); the pan's speed (1/16 px a frame); the backdrop; the framing's size
static void csCamReset(void){ csCz=csCtz=256; csCxF=120<<4; csCyF=64<<4; csVx=csVy=0; csCbg=-1; }
static int csCamX(int x){ return ((((x<<4)-csCxF)*csCz)>>12)+120; }
static int csCamY(int y){ return ((((y<<4)-csCyF)*csCz)>>12)+64; }
static void csCamClamp(void){   // never show past the edge of the picture
    int hw=(120<<12)/csCz, hh=(52<<12)/csCz;
    if(csCxF<hw) csCxF=hw; if(csCxF>(240<<4)-hw) csCxF=(240<<4)-hw; if(csCyF<(12<<4)+hh) csCyF=(12<<4)+hh; if(csCyF>(116<<4)-hh) csCyF=(116<<4)-hh;
}
static void csCamCut(int z,int x16,int y16){ csCz=csCtz=z; csCxF=x16; csCyF=y16; csVx=csVy=0; csCamClamp(); }
static int csSpring(int*p,int*v,int goal){ int d=goal-*p; if(d>-24&&d<24&&*v>-8&&*v<8){ *p+=d>>2; *v=0; return d; } *v=((*v)*12>>4)+(d>>5); *p+=*v; return d; }   // eases off, glides, settles (no overshoot)
static void csCamPan(int z,int x16,int y16){   // the same size of shot: pan there; too far to pan (more than ~90 px across the screen): cut
    if(z!=csCtz){ csCamCut(z,x16,y16); return; }
    int dx=x16-csCxF, dy=y16-csCyF; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
    if(((dx*z)>>12)>90||((dy*z)>>12)>60){ csCamCut(z,x16,y16); return; }
    csSpring(&csCxF,&csVx,x16); csSpring(&csCyF,&csVy,y16); csCamClamp();
}
static int csFigNowX(int who,int x); static int csFigNowY(int who,int y);   // csart.h: where a figure is right now (walking to a new spot), else x / y
static int csFeet(const CsBeat*b,int k,int t);   // cutscene.h: the feet line of the beat's first (k 0) or second (k 1) figure
struct CsShot_; static const struct CsShot_* csShotFind(int t);   // csshot.h: a hand-made shot in use (it moves the camera itself)
static void csCamR(int x,int y,int w,int h,u16 c){
    if(csCz==256){ rect(x,y,w,h,c); return; }
    int x0=csCamX(x), y0=csCamY(y), x1=csCamX(x+w), y1=csCamY(y+h);
    if(w>0&&x1<=x0) x1=x0+1;
    if(h>0&&y1<=y0) y1=y0+1;
    rect(x0,y0,x1-x0,y1-y0,c);
}
static void csCamD(int x,int y,int r,u16 c){
    if(csCz==256){ disc(x,y,r,c); return; }
    int cx=csCamX(x), cy=csCamY(y), rr=(r*csCz+128)>>8;
    if(cx+rr<0||cx-rr>=SW||cy+rr<12||cy-rr>=116) return;
    disc(cx,cy,rr,c);
}
static void csCamL(int x0,int y0,int x1,int y1,u16 c){
    if(csCz==256){ line(x0,y0,x1,y1,c); return; }
    int ax=csCamX(x0), ay=csCamY(y0), bx=csCamX(x1), by=csCamY(y1), n=(csCz+128)>>8;
    int steep=(by>ay?by-ay:ay-by)>=(bx>ax?bx-ax:ax-bx);
    for(int i=0;i<n;i++){ int o=i-n/2; if(steep) line(ax+o,ay,bx+o,by,c); else line(ax,ay+o,bx,by+o,c); }   // a line as thick as the zoom
}
static int csWho(const char*s){ if(!s) return 0; if(s[0]=='M') return s[1]=='a'?CA_MAME:CA_MISSY; if(s[0]=='D') return s[1]=='r'?CA_DOC:CA_HOST; if(s[0]=='H') return CA_CREW; return 0; }   // the speaker's name -> who
static void csCamAim(const CsBeat*b,int t){
    if(b->bg!=csCbg){ csCbg=b->bg; csCtz=-1; }                                                   // a new place: whatever comes next is a cut
    if(csShotFind(t)) return;                                                                    // a hand-made shot has the camera (csShotApply)
    int tz=256, tx=120, ty=64, id=csWho(b->who), fx=0, fy=110, found=0;
    int xa=csFigNowX(b->a,b->ax*4), ya=csFigNowY(b->a,csFeet(b,0,t)), xb=csFigNowX(b->b,b->bx*4), by=csFigNowY(b->b,csFeet(b,1,t));   // (from where they are now: the camera follows a walk)
    if(id){ if(b->a==id){ fx=xa; fy=ya; found=1; } else if(b->b==id){ fx=xb; fy=by; found=1; } }
    if(b->bg==CB_RATE||b->bg==CB_BLACK){ }                                                       // text and a graph: always the wide shot
    else {
        if(b->fx&(CF_SICK|CF_IRIS)){ if(b->b){ tx=xb; ty=by-26; tz=768; } }                     // the sick moment: extreme close-up on her face
        else if(found){ int hot=(b->fx&CF_SHAKE)!=0; tx=fx; ty=fy-(hot?26:20); tz=hot?768:576; } // the speaker: medium close-up (tight when it is heated)
        else if(id&&b->bg==CB_MIRROR){ tx=120; ty=68; tz=640; }                                 // the mirror: her reflection
        else if((b->fx&CF_SHAKE)&&b->b){ tx=xb; ty=by-18; tz=512; }
        else if(b->a&&b->b){ tx=(xa+xb)/2; ty=(ya+by)/2-14; }
        else if(b->b){ tx=xb; ty=by-14; }
        // Long narration over the figures cuts from the wide shot to a close-up once its typing is done; after a spoken line has been typed out and a beat has
        // gone by, the picture cuts to the other person's reaction (shot / reverse shot, as a film cuts a conversation).
        { int tot=0; for(int i=0;i<3&&b->t[i];i++){ int n=0; while(b->t[i][n]) n++; tot+=n; }
          if(!id&&tot>=36&&(b->a||b->b)&&!(b->fx&(CF_SICK|CF_IRIS))&&t>=tot){ if(b->b){ tx=xb; ty=by-20; tz=512; } else { tx=xa; ty=ya-20; tz=512; } }
          else if(found&&b->a&&b->b&&t>=tot*2+24){ if(b->a==id){ tx=xb; ty=by-20; } else { tx=xa; ty=ya-20; } tz=448; } }
    }
    csCamPan(tz,tx<<4,ty<<4);
}

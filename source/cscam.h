// cscam.h - CUTSCENE CAMERA (module 2): zoom + pan over the picture area (y 12..115). Everything the scenes draw goes through csR / csD / csLn, so backdrops and figures all follow the camera.
// z is 8.8 fixed point (256 = the wide shot, 512 = 2x, 768 = 3x). The camera eases toward each beat's shot (a CUT when the backdrop changes). csCamAim picks the shot by itself:
//   the speaker gets a medium close-up, a SHAKE beat a tight one, SICK / IRIS beats an extreme close-up, the rest a slow push-in. (Module 5 can add hand-made shot lists on top.)
// Needs from cutscene.h: CsBeat, CB_* CA_* CF_* CP_*. Needs from main.c: rect, disc, line.
static int csCz=256, csCx=120, csCy=64, csCbg=-1;
static void csCamReset(void){ csCz=256; csCx=120; csCy=64; csCbg=-1; }
static int csCamX(int x){ return (((x-csCx)*csCz)>>8)+120; }
static int csCamY(int y){ return (((y-csCy)*csCz)>>8)+64; }
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
static int csEase(int c,int g){ int d=g-c; if(!d) return c; int s=d/4; if(!s) s=d>0?1:-1; return c+s; }
static void csCamAim(const CsBeat*b,int t){
    int tz=256, tx=120, ty=64, id=csWho(b->who), fx=0, fy=110, found=0, by=110;
    if(b->bg==CB_SITE){ if(b->pb==CP_CLIMB) by=110-(t/3>48?48:t/3); if(b->pb==CP_FLAIL){ by=62+t*t/20; if(by>110) by=110; } if(b->pb==CP_LIE) by=111; }   // (where csDraw puts the second figure)
    if((b->bg==CB_HOSP||b->bg==CB_FLAT)&&(b->pb==CP_LIE||b->pb==CP_STIR)) by=98;
    if(id){ if(b->a==id){ fx=b->ax*4; fy=110; found=1; } else if(b->b==id){ fx=b->bx*4; fy=by; found=1; } }
    if(b->bg==CB_RATE||b->bg==CB_BLACK){ }                                                       // text and a graph: always the wide shot
    else {
        if(b->fx&(CF_SICK|CF_IRIS)){ if(b->b){ tx=b->bx*4; ty=by-26; tz=768; } }                // the sick moment: extreme close-up on her face
        else if(found){ int hot=(b->fx&CF_SHAKE)!=0; tx=fx; ty=fy-(hot?26:20); tz=hot?768:576; } // the speaker: medium close-up (tight when it is heated)
        else if(id&&b->bg==CB_MIRROR){ tx=120; ty=68; tz=640; }                                 // the mirror: her reflection
        else if((b->fx&CF_SHAKE)&&b->b){ tx=b->bx*4; ty=by-18; tz=512; }
        else if(b->a&&b->b){ tx=(b->ax*4+b->bx*4)/2; ty=by-14; }
        else if(b->b){ tx=b->bx*4; ty=by-14; }
        tz+=(t>200?200:t)/2;                                                                     // a slow push-in the longer the beat runs
    }
    if(b->bg!=csCbg){ csCbg=b->bg; csCz=tz; csCx=tx; csCy=ty; }                                  // a new backdrop: cut
    else { csCz=csEase(csCz,tz); csCx=csEase(csCx,tx); csCy=csEase(csCy,ty); }
    int hw=(120*256)/csCz, hh=(52*256)/csCz;                                                     // never show past the edge of the picture
    if(csCx<hw) csCx=hw; if(csCx>240-hw) csCx=240-hw; if(csCy<12+hh) csCy=12+hh; if(csCy>116-hh) csCy=116-hh;
}

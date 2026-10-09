// csfx.h - CUTSCENE MOTION STREAKS (module 3): a few short, thin, faint streaks on the fast moments only (a sprint, a fall). Not a burst, not white: each streak just
// lightens what is already behind it (or darkens it when that is bright), like a hint of motion blur. Drawn behind the figures. Needs cscam.h (step 2); reads fb directly.
static void csTint(int x0,int y0,int w,int h,int k){   // world coords; tints the pixels already drawn
    int sx0=csCamX(x0+csOx), sy0=csCamY(y0+csOy), sx1=csCamX(x0+csOx+w), sy1=csCamY(y0+csOy+h);
    if(sx1<=sx0) sx1=sx0+1; if(sy1<=sy0) sy1=sy0+1;
    if(sx0<0) sx0=0; if(sx1>SW) sx1=SW; if(sy0<12) sy0=12; if(sy1>116) sy1=116;
    for(int y=sy0;y<sy1;y++) for(int x=sx0;x<sx1;x++){ u16*p=&fb[y*SW+x]; int r=*p&31, g=(*p>>5)&31, b=(*p>>10)&31, d=(r+g+b)>48?-3:k;
        r+=d; g+=d; b+=d; if(r<0)r=0; if(g<0)g=0; if(b<0)b=0; if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; *p=(u16)(r|(g<<5)|(b<<10)); }
}
static void csSprint(int who,int x,int y,int t){   // a runner at (x,y): four short trailing streaks behind her (running on the spot: on the side away from the middle, as if toward it)
    int sd=x>120?1:-1; { CsPS*p=&csPS[who]; if(csFrameNo-p->fn<=3&&p->x!=x){ sd=p->x<x?-1:1; x=p->x; } if(csFrameNo-p->fn<=3) y=p->y; }   // (running to a new spot: where she is now, last frame)
    for(int i=0;i<4;i++){ int yy=y-5-i*7-((i+t/5)%3), off=7+((i*5)&7)+((t*2+i*9)&7), len=9+((i*7)%9);
        csTint(sd>0?x+off:x-off-len,yy,len,1,4); }
}
static void csFall(int x,int y){   // a falling figure (head at y-34): three faint streaks above it, longer the further it has fallen
    int len=(110-y)/2; if(len>22) len=22; if(len<4) return;
    for(int i=0;i<3;i++){ int xx=x-5+i*5, l=len-((i&1)?4:0); csTint(xx,y-34-l,1,l,4); }
}
static void csSpeed(const CsBeat*b,int t,int by){
    if(b->bg==CB_RATE||b->bg==CB_BLACK) return;
    if(b->a&&b->pa==CP_RUN) csSprint(b->a,b->ax*4,110,t);
    if(b->b&&b->pb==CP_RUN) csSprint(b->b,b->bx*4,by,t);
    if(b->bg==CB_SITE&&b->b&&b->pb==CP_FLAIL&&by<108) csFall(b->bx*4,by);
}

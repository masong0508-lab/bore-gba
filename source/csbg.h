// csbg.h - CUTSCENE BACKDROP LIGHTING (module 5): a light pass over each backdrop so the wide shots have depth: a vignette on every picture, the stage's light pool and drifting dust,
// moonlight through the window at home, a sweeping glare on the mirror, haze and dust on the building site, the heart monitor's green (or red) glow, the dressing-room bulbs' glow.
// Cheap on purpose: the vignette darkens a few edge rows with packed 15-bit maths, glows add a shifted share of "what is missing" to chosen colour channels, and glows are skipped in tight close-ups.
// Needs cscam.h (step 2). csBgFx runs right after csBg (behind the figures), csVig runs last (over everything, before the black bars).
#define CSG_ALL 0x7FFF
#define CSG_WARM 0x03FF
#define CSG_COOL 0x7FE0
static int csGi(int v){ int w=0; while((w+1)*(w+1)<=v) w++; return w; }
static u16 csGm(int s){ return s==1?0x3DEF:s==2?0x1CE7:0x0C63; }                                       // the bits that survive a shift of s
static void csGlowRow(int sx0,int sx1,int sy,int s,u16 ch){                                              // screen row: add a share of the missing light to the channels in ch
    if(sy<12||sy>=116) return; if(sx0<0) sx0=0; if(sx1>SW) sx1=SW; u16 m=(u16)(csGm(s)&ch);
    for(u16*p=&fb[sy*SW+sx0];sx0<sx1;sx0++,p++){ u16 c=*p; *p=(u16)(c+((((~c)&0x7FFF)>>s)&m)); }
}
static void csGlowBox(int wx,int wy,int w,int h,int s,u16 ch){                                           // a rectangle of world pixels
    int x0=csCamX(wx), x1=csCamX(wx+w), y0=csCamY(wy), y1=csCamY(wy+h); if(x1<=x0) x1=x0+1; if(y1<=y0) y1=y0+1;
    if((x1-x0)*(y1-y0)>7000) return; for(int y=y0;y<y1;y++) csGlowRow(x0,x1,y,s,ch);
}
static void csGlow(int wx,int wy,int rx,int ry,int s,u16 ch){                                            // an ellipse of world pixels
    int cx=csCamX(wx), cy=csCamY(wy), sx=csCamX(wx+rx)-cx, sy=csCamY(wy+ry)-cy; if(sx<1) sx=1; if(sy<1) sy=1;
    if(sx*sy*3>6000) return;
    for(int y=-sy;y<=sy;y++){ int w=sx*csGi(16*(sy*sy-y*y))/(4*sy); csGlowRow(cx-w,cx+w+1,cy+y,s,ch); }
}
static void csMote(int wx,int wy,int s){ int n=(csCz+128)>>8; if(n<1) n=1; int x=csCamX(wx); for(int i=0;i<n;i++) csGlowRow(x,x+n,csCamY(wy)+i,s,CSG_ALL); }
static void csBgFx(int bg,int t,int fx){
    if(csCz>=640) return;                                                                                // a tight close-up: nobody looks at the wall
    switch(bg){
    case CB_STAGE: case CB_BIG: { u16 ch=bg==CB_BIG?CSG_COOL:CSG_WARM;
        csGlow(120,103,64,6,bg==CB_BIG?2:3,bg==CB_BIG?ch:CSG_ALL); csGlow(120,103,36,4,bg==CB_BIG?2:3,bg==CB_BIG?ch:CSG_ALL);                                            // the light pool on the boards
        for(int i=0;i<6;i++) csMote(100+((i*37+t/3)%40),22+((i*29+t/4)%70),1);                           // dust drifting in the beam
        break; }
    case CB_STUDIO:
        csGlow(120,73,58,3,3,CSG_ALL); csGlow(120,73,30,2,2,CSG_WARM);                                  // the pool of light on the boards
        for(int i=0;i<6;i++) csMote(100+((i*37+t/3)%40),22+((i*29+t/4)%48),1);                           // dust drifting in the beam
        break;
    case CB_HOME:
        for(int y=52;y<112;y++) csGlowBox(178-(y-52)*3/2,y,32,1,3,CSG_COOL);                              // moonlight from the window across the floor
        for(int i=0;i<4;i++) csMote(40+((i*53+t/5)%120),30+((i*31+t/6)%60),2);
        break;
    case CB_MIRROR: { int bx=60+((t*2)%150); for(int y=14;y<98;y+=1){ int x=bx-(y-14)/2; if(x>66&&x<166) csGlowBox(x,y,7,1,2,CSG_ALL); } break; }   // a glare sweeping the glass
    case CB_SITE:
        csGlowBox(0,68,SW,16,3,CSG_ALL);                                                                // dusty haze at the horizon
        for(int i=0;i<6;i++) csMote(20+((i*41+t/2)%200),40+((i*23+t/5)%50),2);
        break;
    case CB_HOSP: csGlow(31,45,26,22,(((t/2)%32)<4)?1:2,0x03E0); break;                                  // the monitor's green, flaring on the beat
    case CB_FLAT: csGlow(31,45,26,22,2,0x001F); break;                                                   // the same glow, red
    case CB_BACK: if(csMood<=0) for(int i=0;i<12;i++) if(((t>>4)+i)&3) csGlow(63+i*10,19,5,5,2,CSG_WARM); break;      // the bulbs round the mirror
    default: break;
    }
}
static void csDk(int y,int x0,int x1,int s){                                                             // darken a screen row segment by 1/2, 1/4 or 1/8
    u16*p=&fb[y*SW+x0]; u16 m=csGm(s); u32 mm=(u32)m|((u32)m<<16);
    if(((uintptr_t)p&2)||((x1-x0)&1)){ for(;x0<x1;x0++,p++){ u16 c=*p; *p=(u16)(c-((c>>s)&m)); } return; }
    for(u32*q=(u32*)p;x0<x1;x0+=2,q++){ u32 v=*q; *q=v-((v>>s)&mm); }
}
static void csVig(int bg){
    if(bg==CB_RATE||bg==CB_BLACK) return;
    if(csMood>0){ int s=csMood>1?1:2; for(int y=12;y<116;y++) csDk(y,0,SW,s); }                       // cutscene redo 12: the lights go down
    else if(csMood<0){ for(int y=12;y<116;y++) csGlowRow(0,SW,y,3,CSG_ALL); }                           // ...or the whole world turns up
    for(int y=12;y<116;y++){ int edge=(y<14||y>=114);
        if(edge) csDk(y,10,230,3);
        csDk(y,0,4,2); csDk(y,4,10,3); csDk(y,230,236,3); csDk(y,236,240,2);
    }
}

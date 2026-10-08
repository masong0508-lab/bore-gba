// csshot.h - CUTSCENE SHOT LISTS (module 6): hand-made camera moves for the key moments, laid over the automatic framing of cscam.h.
// A scene beat with no entry here keeps the automatic camera. An entry takes over a beat from its t0 frame: the camera starts at (z0,x0,y0), glides in a straight line to (z1,x1,y1) over
// dur frames and holds there. Several entries on one beat make a cut list: the one with the latest t0 that has started is the one in use (each entry's first frame is a hard cut).
// z is 8.8 fixed point (256 = wide, 512 = 2x). anc: 0 = x is a world x, 1 = x is an offset from the beat's second figure (b), 2 = from the first figure (a). The usual edge limit still applies.
// Needs cscam.h (csCz csCx csCy), cutscene.h (CsBeat). csCurSc / csCurBi say which scene and beat csPlay is on (csPlay sets them).
typedef struct { u8 sc, bi, anc; short t0, dur, z0, x0, y0, z1, x1, y1; } CsShot;
static int csCurSc=-1, csCurBi=0;
static const CsShot csShots[]={
    // the sick moment, THE BARS (scene 0, beat 11): three cameras, three angles: side-on, closer, then her face and a slow push
    {0,11,1,  0, 24, 400,-34,84, 460,-24,84},
    {0,11,1, 24, 24, 640, 10,82, 700,  6,82},
    {0,11,1, 48, 70, 900,  0,84,1024,  0,86},
    // the sick moment, THE OPENING (scene 7, beats 17-19): a slow push in, held tight, then a long pull back from the silence
    {7,17,1,  0, 80, 440,  0,80, 940,  0,86},
    {7,18,1,  0,150, 940,  0,86,1024,  0,86},
    {7,19,1,  0,120,1024,  0,86, 520,  0,76},
    // the fall, THE SWEATER (scene 2, beats 11-13): the camera drops with her, a hard cut to the ground when she lands, then it backs away to the fence and the flashbulbs
    {2,11,1,  0, 30, 300,  0,44, 420,  0,92},
    {2,12,1,  0, 50, 420, -6,85, 560, -6,90},
    {2,13,1,  0,170, 560, -6,90, 300, 10,72},
    // the flat line, THE PLUG (scene 6, beats 7-9): a push toward the monitor, a cut to the bedside, then a slow pull back out of the room
    {6,7,0,   0, 70, 256,120,64, 440, 76,52},
    {6,8,0,   0, 80, 420,130,86, 520,134,88},
    {6,9,0,   0,220, 480,134,86, 256,120,64},
    // the empty chair, HERE TODAY (scene 5, beat 9): from the singer, a slow drift down the front row to the one seat nobody takes
    {5,9,0,   0,170, 330,172,80, 700,121,97},
    // the mirror, THE BARS' NIGHT at home (scene 1, beat 8): the first look, creeping in on the reflection
    {1,8,0,   0,150, 300,120,60, 640,120,72},
};
static void csShotApply(const CsBeat*b,int t){
    const CsShot*s=0; int n=(int)(sizeof(csShots)/sizeof(csShots[0]));
    for(int i=0;i<n;i++){ const CsShot*c=&csShots[i]; if(c->sc==csCurSc&&c->bi==csCurBi&&t>=c->t0&&(!s||c->t0>=s->t0)) s=c; }
    if(!s) return;
    int u=t-s->t0, d=s->dur>0?s->dur:1; if(u>d) u=d;
    int ax=s->anc==1?b->bx*4:s->anc==2?b->ax*4:0;
    csCz=s->z0+(s->z1-s->z0)*u/d; csCx=ax+s->x0+(s->x1-s->x0)*u/d; csCy=s->y0+(s->y1-s->y0)*u/d;
    int hw=(120*256)/csCz, hh=(52*256)/csCz;                                                     // never show past the edge of the picture (as cscam.h)
    if(csCx<hw) csCx=hw;
    if(csCx>240-hw) csCx=240-hw;
    if(csCy<12+hh) csCy=12+hh;
    if(csCy>116-hh) csCy=116-hh;
}

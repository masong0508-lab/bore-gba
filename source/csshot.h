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
    // bars end shots (cutscene redo 2): a push in on the shout (beat 4), then from the extreme close-up a slow pull out as the iris closes on "...Ow." (beat 12)
    {0,4,1,   0, 60, 330,  0,70, 560,  0,76},
    {0,12,1,  0, 70,1024,  0,86, 700,  0,82},
    // the sick moment, THE OPENING (scene 7, beats 17-19): a slow push in, held tight, then a long pull back from the silence
    {7,17,1,  0, 80, 440,  0,80, 940,  0,86},
    {7,18,1,  0,150, 940,  0,86,1024,  0,86},
    {7,19,1,  0,120,1024,  0,86, 520,  0,76},
    // opening end shots (cutscene redo 1): the laughs and phones pull wide, Dex gets a slow push, the approval board pushes in on the cliff then drifts back
    {7,20,1,  0,100, 520,  0,76, 256,  0,64},
    {7,21,2,  0,120, 256, 60,70, 420, 40,84},
    {7,22,0,  0,200, 256,120,64, 380,150,74},
    {7,23,0,  0,140, 380,150,74, 256,120,64},
    // the fall, THE SWEATER (scene 2, beats 11-13): the camera drops with her, a hard cut to the ground when she lands, then it backs away to the fence and the flashbulbs
    {2,11,1,  0, 30, 300,  0,44, 420,  0,92},
    {2,12,1,  0, 50, 420, -6,85, 560, -6,90},
    {2,13,1,  0,170, 560, -6,90, 300, 10,72},
    // fall shots (cutscene redo 4): open tight and pull wide (0), the bucket (4), the shout (6), the camera climbs with her (7-9), a tight cut-in on "Easy!" (10)
    {2,0,0,   0,150, 300, 60,70, 256,120,64},
    {2,4,2,   0, 90, 300, 10,84, 380, 30,84},
    {2,6,1,   0, 50, 450,  0,78, 520,  0,76},
    {2,7,1,   0,140, 330,  0,84, 330,  0,62},
    {2,8,1,   0, 60, 300,-10,56, 330,  0,50},
    {2,9,1,   0,120, 330,  0,50, 520,  0,46},
    {2,10,1,  0, 60, 760,  0,46, 900,  0,46},
    // the flat line, THE PLUG (scene 6, beats 7-9): a push toward the monitor, a cut to the bedside, then a slow pull back out of the room
    {6,7,0,   0, 70, 256,120,64, 440, 76,52},
    {6,8,0,   0, 80, 420,130,86, 520,134,88},
    {6,9,0,   0,220, 480,134,86, 256,120,64},
    // plug shots (cutscene redo 8): THE PLUG (scene 6): a slow push into the room (0), the doctor (1), a push on Mamesy's plea (2), the doctor leaves (3), the held hand (4-5), a hard cut to the machines being switched off (6), a tight cut-in on "Mish? ...Mish." (8)
    {6,0,0,   0,150, 256,120,64, 320,130,72},
    {6,1,2,   0, 90, 360, 10,82, 440, 10,82},
    {6,2,2,   0, 70, 450,  0,80, 620,  0,80},
    {6,3,2,   0, 70, 380, 10,82, 320, 30,80},
    {6,4,1,   0,120, 330,-40,86, 560,-30,88},
    {6,5,1,   0,100, 560,-30,88, 760,-20,88},
    {6,6,0,   0, 60, 560, 40,56, 700, 36,50},
    {6,8,1,   0, 50, 900,  0,92,1024,  0,92},
    // the empty chair, HERE TODAY (scene 5, beat 9): from the singer, a slow drift down the front row to the one seat nobody takes
    {5,9,0,   0,170, 330,172,80, 700,121,97},
    // here today shots (cutscene redo 9): a slow push in and drift back on the singer for each sung line, tight pushes on the two high lines, then a long drift to the empty seat and a pull out to the wide shot
    {5,10,1,  0,132, 330,0,84, 450,0,84},
    {5,11,1,  0,161, 450,0,84, 330,0,84},
    {5,12,1,  0,208, 330,0,84, 450,0,84},
    {5,13,1,  0,157, 450,0,84, 330,0,84},
    {5,14,1,  0,243, 330,0,84, 450,0,84},
    {5,15,1,  0,223, 450,0,84, 330,0,84},
    {5,16,1,  0,197, 330,0,84, 450,0,84},
    {5,17,1,  0,238, 450,0,84, 330,0,84},
    {5,18,1,  0,132, 330,0,84, 450,0,84},
    {5,19,1,  0,271, 420,0,82, 700,0,82},
    {5,20,1,  0,162, 330,0,84, 450,0,84},
    {5,21,1,  0,278, 450,0,84, 330,0,84},
    {5,22,1,  0,214, 420,0,82, 700,0,82},
    {5,23,1,  0,229, 450,0,84, 330,0,84},
    {5,24,1,  0,74, 330,0,84, 450,0,84},
    {5,25,1,  0,90, 450,0,84, 330,0,84},
    {5,26,1,  0,312, 330,0,84, 450,0,84},
    {5,27,1,  0,272, 450,0,84, 330,0,84},
    {5,28,0,  0,296, 330,172,80, 568,127,95},
    {5,29,0,  0,40, 568,127,95, 600,121,97},
    {5,30,0,  0,310, 600,121,97, 296,120,68},
    {5,31,0,  0,41, 296,120,68, 256,120,64},
    // waking shots (cutscene redo 7): WAKING UP (scene 3): a slow push into the room (0), Mamesy's plea (4), the held hand (7), her grief (8), the monitor (11-12), a hard cut to the fingers (14), Missy's face (16), the shout (17), the coffee and lawyer (21), "I heard you" (28), the rain (29-30), a last push (33)
    {3,0,0,   0,150, 256,120,64, 300,120,70},
    {3,4,2,   0, 90, 560,  0,80, 620,  0,80},
    {3,7,1,   0,120, 330,-40,86, 560,-30,88},
    {3,8,2,   0,120, 520,  0,80, 700,  0,82},
    {3,11,0,  0,100, 300, 60,60, 560, 36,50},
    {3,12,0,  0, 50, 560, 36,50, 760, 31,46},
    {3,14,1,  0, 50, 900, 10,94,1024, 10,94},
    {3,16,1,  0,100, 760,-14,92, 900,-14,92},
    {3,17,2,  0, 60, 560,  0,80, 620,  0,80},
    {3,21,1,  0, 80, 520,-14,90, 700,-14,92},
    {3,28,1,  0,120, 640,-14,92, 780,-14,92},
    {3,29,0,  0,120, 500,150,80, 300,130,70},
    {3,30,0,  0,150, 300,130,70, 256,120,64},
    {3,33,0,  0,200, 256,120,64, 420,164,86},
    // news shots (cutscene redo 6): THE NEWS (scene 4): a slow push into the dressing room (0), the mirror rehearsal (2), her laugh (8), the heartfelt line (9), a wide two-shot for the jinx (11), the clock and the late call (15-17), the ring and the news (19-22), a long pull out into silence (23), the cry (25), the mirror line (27), the offer (28), the one minute (30), the last pull (31)
    {4,0,0,   0,150, 256,120,64, 320,108,76},
    {4,2,1,   0, 90, 330,  0,80, 420,  0,82},
    {4,8,1,   0, 90, 640,  0,82, 760,  0,82},
    {4,9,1,   0,140, 420,  0,80, 620,  0,82},
    {4,11,0,  0, 60, 256,150,64, 300,150,76},
    {4,15,1,  0,100, 330,  0,80, 520,  0,82},
    {4,17,1,  0,120, 500,  0,80, 700,  0,82},
    {4,19,1,  0, 80, 800,  0,82, 900,  0,82},
    {4,21,1,  0, 60,1024,  0,82,1024,  0,82},
    {4,22,1,  0,120,1024,  0,82, 700,  0,80},
    {4,23,0,  0, 80, 700,108,80, 256,120,64},
    {4,24,2,  0, 60, 420,  0,80, 520,  0,82},
    {4,25,1,  0,120, 760,  0,82, 900,  0,84},
    {4,27,1,  0,140, 700,  0,82, 380,  0,80},
    {4,28,2,  0,100, 520,  0,82, 640,  0,82},
    {4,30,1,  0,120, 620,  0,82,1024,  0,82},
    {4,31,0,  0,200, 520,120,76, 256,120,64},
    // the mirror, THE BARS' NIGHT at home (scene 1, beat 8): the first look, creeping in on the reflection
    {1,8,0,   0,150, 300,120,60, 640,120,72},
    // sweater shots (cutscene redo 3): push to the glass (1), slow push on "Nobody would know" (3), track her run (7), tight on the crying line (10), pull out on the cold tile (12)
    {1,1,0,   0,120, 256,120,64, 600,166,86},
    {1,3,1,   0, 90, 330,  0,70, 520,  0,76},
    {1,7,1,   0, 10, 360,-20,72, 300, 60,72},
    {1,10,0,  0,120, 760,120,66, 900,120,68},
    {1,12,0,  0,150, 900,120,68, 256,120,64},
};
static void csShotApply(const CsBeat*b,int t){
    const CsShot*s=0; int n=(int)(sizeof(csShots)/sizeof(csShots[0]));
    for(int i=0;i<n;i++){ const CsShot*c=&csShots[i]; if(c->sc==csCurSc&&c->bi==csCurBi&&t>=c->t0&&(!s||c->t0>=s->t0)) s=c; }
    if(!s) return;
    int u=t-s->t0, d=s->dur>0?s->dur:1; if(u>d) u=d;
    int ax=s->anc==1?b->bx*4:s->anc==2?b->ax*4:0;
    // JUMP CUTS instead of glides: the move from the first framing to the last is cut into 2 hard cuts (3 for a long one), each a still shot
    int n=d>=150?3:2, k=u>=d?n-1:u*n/d;
    csCz=s->z0+(s->z1-s->z0)*k/(n-1); csCx=ax+s->x0+(s->x1-s->x0)*k/(n-1); csCy=s->y0+(s->y1-s->y0)*k/(n-1);
    int hw=(120*256)/csCz, hh=(52*256)/csCz;                                                     // never show past the edge of the picture (as cscam.h)
    if(csCx<hw) csCx=hw;
    if(csCx>240-hw) csCx=240-hw;
    if(csCy<12+hh) csCy=12+hh;
    if(csCy>116-hh) csCy=116-hh;
}

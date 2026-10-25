// bortua3dcore.h - BORTUA FIGHTER's 3D MODE ("BALLZ"): the PURE part (no hardware: tested on a PC by tools/test_bortua3d.sh).
// Needs before it: u8 u16 u32 s16 s32, fsin() / fcos() (feel.h: 256 = a full turn, results x256) and rnd8().
//
// The fighters are made of BALLS (like Ballz 3D): 17 shaded spheres each, hung on a small skeleton that is posed from a handful of key poses (a stance, a guard, a jab, a kick,
// a palm, a throw, a hit, a jump, a win; a walk cycle is worked out from the way the fighter is moving). The fight happens on a round ring in real 3D: positions are in
// MILLIMETRES (x and z on the ground, y up), a fighter's yaw is 0..255 (0 = looking along +z), the camera is a pinhole with its horizon at a screen row (no pitch: a high
// camera is a low horizon and a tall camera, which is all a ring needs). Blows are not on a line: they have a reach and an ARC (a fighter who steps sideways is not hit by a
// jab), so the ring is round and the fight is too. Everything here is integer maths.
//
// CONTROLS (bortua3d.h turns the pad into a BzIn):  D-pad moves (MOVEMENT option: ORBIT = UP closes in, DOWN backs off, LEFT / RIGHT circle the opponent;
//   SCREEN = the pad as the camera sees it)   A punch   B kick   A + B close up a throw   L jump (A / B in the air: a jump kick)   R guard
//   forward + A the BORE PALM, back + B the SPIN KICK  (forward / back = towards / away from the opponent, whichever movement option is on)
#ifndef BORTUA3DCORE_H
#define BORTUA3DCORE_H
#define BZ_R 2600          // the ring's radius (mm)
#define BZ_SEP 520         // the closest two standing fighters get (mm)
#define BZ_BODY 260        // how far round the middle of a fighter a blow still counts as touching (mm)
enum { BS_IDLE, BS_WALK, BS_GUARD, BS_ATK, BS_HIT, BS_BLOCK, BS_JUMP, BS_KO, BS_WIN, BS_THROWN };
enum { BM_JAB, BM_KICK, BM_PALM, BM_SPIN, BM_THROW, BM_JKICK, BM_N };
typedef struct { const char*nm; u8 start, act, rec, dmg, stun, kb, lvl; u16 reach; u8 arc; } BzMv;   // frames of wind-up, of the hit, of recovery; damage; the stun it causes; knock-back; level (0 high 1 mid 2 low 3 throw); reach (mm); arc (+-, of 256)
static const BzMv bzMv[BM_N]={
    {"JAB",3,3,8,6,14,2,0,950,40}, {"KICK",6,4,12,9,18,4,1,1200,40}, {"BORE PALM",9,4,16,15,26,10,1,1000,40},
    {"SPIN KICK",11,5,18,14,26,12,0,1250,70}, {"THROW",3,2,22,18,40,14,3,700,50}, {"JUMP KICK",4,8,6,8,18,4,0,1050,45} };
#define BZE_BLOCK 1     // events a step reports (cleared by the caller): a blow was blocked / landed / landed hard / knocked out / a throw / a swing starts / a landing / a special's call
#define BZE_HIT   2
#define BZE_BIG   4
#define BZE_KO    8
#define BZE_THROW 16
#define BZE_SWING 32
#define BZE_LAND  64
#define BZE_CALL  128
typedef struct { s32 x, z; int y, vy; s32 kx, kz, jx, jz;    // position (mm), knock-back velocity and the drift of a jump (mm per frame)
                 int yaw, spin, hp, trail, trailT, st, t, stun, mv, wins, cnt, flash, combo, mx, mz, arm, jatk, fall, lvl, done;
                 int aiT, aiDo, aiSide; int ev; int hx, hy, hz;   /* where the last blow landed */
                 u8 pw, sp, d, body; } Bz;
typedef struct { int mx, mz; u8 a, b, pa, pb, pj, guard; } BzIn;   // the move vector (256 = full speed), buttons held (a, b, guard), buttons pressed this frame (pa, pb, pj)
static const char* bzCall; static int bzCallT; static char bzCallB[12];   // the word over the fight ("COUNTER", "3 HITS", "THROW", ...)
// ---- small maths ----
static inline int bzSqrt(int v){ if(v<=0) return 0; unsigned x=(unsigned)v, r=0, b=1u<<30; while(b>x) b>>=2; while(b){ if(x>=r+b){ x-=r+b; r=(r>>1)+b; } else r>>=1; b>>=2; } return (int)r; }
static inline int bzWrap(int a){ return ((a+128)&255)-128; }
static int bzAng(int dx,int dz){   // the angle (0..255) whose (sin, cos) points along (dx, dz): 0 = +z, 64 = +x
    if(!dx&&!dz) return 0;
    int a=dx>=0?(dz>=0?0:64):(dz<0?128:192);
    for(int s=32;s>=1;s>>=1){ int t=a+s; if(fsin(t)*dz-fcos(t)*dx<=0) a=t; }   // (inside one quarter the sign of the cross product is monotonic)
    return a&255;
}
static inline int bzDistXZ(const Bz*a,const Bz*b){ int dx=(int)(b->x-a->x), dz=(int)(b->z-a->z); return bzSqrt(dx*dx+dz*dz); }
static void bzNum(char*d,const char*suffix,int n){ char t[6]; int k=0; if(n<=0) t[k++]='0'; while(n>0&&k<5){ t[k++]=(char)('0'+n%10); n/=10; } int o=0; while(k) d[o++]=t[--k]; while(*suffix) d[o++]=*suffix++; d[o]=0; }
// ---- the key poses: local joints (mm; right is +x, forward +z), for the RIGHT limbs where it matters (the left ones are a mirror) ----
typedef struct { s16 e[2][3], h[2][3], k[2][3], f[2][3]; s16 lean, drop; } BzPose;   // elbows, hands, knees, feet [0 left, 1 right]; chest lean (mm forward) and how far the pelvis drops
#define BZP(eLx,eLy,eLz,hLx,hLy,hLz,eRx,eRy,eRz,hRx,hRy,hRz,kLx,kLy,kLz,fLx,fLy,fLz,kRx,kRy,kRz,fRx,fRy,fRz,ln,dr) \
    {{{eLx,eLy,eLz},{eRx,eRy,eRz}},{{hLx,hLy,hLz},{hRx,hRy,hRz}},{{kLx,kLy,kLz},{kRx,kRy,kRz}},{{fLx,fLy,fLz},{fRx,fRy,fRz}},ln,dr}
enum { PZ_IDLE, PZ_GUARD, PZ_JAB_A, PZ_JAB_B, PZ_KICK_A, PZ_KICK_B, PZ_PALM, PZ_SPIN, PZ_THROW, PZ_HIT, PZ_JUMP, PZ_WIN, PZ_N };
static const BzPose bzPz[PZ_N]={
 /* IDLE  */ BZP(-300,1150,170, -200,1380,380,  300,1170,150, 220,1400,340,  -170,520,90, -190,90,70,  170,520,-60, 210,90,-170,  30,0),
 /* GUARD */ BZP(-230,1330,260, -100,1600,330,  230,1330,260, 100,1600,330,  -170,500,80, -190,90,70,  170,500,-70, 210,90,-170,  70,80),
 /* JABA  */ BZP(-300,1150,170, -200,1380,380,  320,1180,100, 200,1330,200,  -170,520,90, -190,90,70,  170,520,-60, 210,90,-170,  0,0),
 /* JABB  */ BZP(-300,1200,150, -150,1500,330,  230,1420,520, 110,1470,900,  -170,500,110, -190,90,90,  170,500,-70, 210,90,-150,  120,0),
 /* KICKA */ BZP(-300,1150,170, -200,1380,380,  300,1170,150, 220,1400,340,  -170,520,90, -190,90,70,  170,920,360, 180,480,200,  -40,0),
 /* KICKB */ BZP(-420,1250,60, -520,1250,60,  420,1250,-60, 520,1250,-60,  -170,520,40, -190,90,20,  170,1000,480, 180,1080,950,  -150,0),
 /* PALM  */ BZP(-240,1340,500, -110,1400,880,  240,1340,500, 110,1400,880,  -170,480,260, -180,90,230,  170,480,140, 190,90,-40,  160,0),
 /* SPIN  */ BZP(-450,1300,0, -650,1280,-60,  450,1300,0, 650,1280,-60,  -170,520,-30, -190,90,-40,  170,1000,480, 180,1080,950,  -150,0),
 /* THROW */ BZP(-300,1250,420, -130,1250,720,  300,1250,420, 130,1250,720,  -170,500,150, -190,90,120,  170,500,-80, 210,90,-190,  140,0),
 /* HIT   */ BZP(-340,1180,30, -420,1200,-80,  340,1180,30, 420,1200,-80,  -170,500,-20, -190,90,-40,  170,520,-120, 210,90,-230,  -200,20),
 /* JUMP  */ BZP(-300,1150,170, -200,1380,380,  300,1170,150, 220,1400,340,  -170,800,260, -180,430,120,  170,760,300, 180,380,160,  20,0),
 /* WIN   */ BZP(-390,1640,30, -300,1990,40,  390,1640,30, 300,1990,40,  -170,520,90, -190,90,70,  170,520,-60, 210,90,-170,  -30,0) };
static void bzMirror(BzPose*p){ for(int a=0;a<4;a++){ s16*arr=(s16*)p+a*6; for(int i=0;i<3;i++){ s16 t=arr[i]; arr[i]=(s16)-arr[3+i]; arr[3+i]=(s16)-t; } } }   // swap left and right (and flip x)
static void bzLerp(BzPose*d,const BzPose*a,const BzPose*b,int k){   // k 0..256
    s16*dd=(s16*)d; const s16*aa=(const s16*)a, *bb=(const s16*)b;
    for(int i=0;i<(int)(sizeof(BzPose)/2);i++) dd[i]=(s16)(aa[i]+(bb[i]-aa[i])*k/256);
}
// the key poses of a move: wind-up, the blow
static const u8 bzMvA[BM_N]={PZ_JAB_A,PZ_KICK_A,PZ_PALM,PZ_SPIN,PZ_THROW,PZ_KICK_A};
static const u8 bzMvB[BM_N]={PZ_JAB_B,PZ_KICK_B,PZ_PALM,PZ_SPIN,PZ_THROW,PZ_KICK_B};
// ---- the skeleton -> balls ----
#define BZ_NBALL 21
typedef struct { s32 x; int y; s32 z; int r; u8 col; } BzBall;   // world position, radius (mm), colour index: 0 skin 1 top 2 bottom 3 hair 4 shoes
// pose of a fighter right now (local joints), and how far the body is rolled over (rot, 0..64 = lying on its back) and how low the pelvis is
static void bzPoseOf(const Bz*f,BzPose*o,int*rot,int*py){
    *rot=0; *py=950;
    const BzPose*idle=&bzPz[PZ_IDLE];
    switch(f->st){
    case BS_GUARD: *o=bzPz[PZ_GUARD]; break;
    case BS_BLOCK: bzLerp(o,&bzPz[PZ_GUARD],&bzPz[PZ_HIT],40); break;
    case BS_JUMP: if(f->jatk){ const BzMv*m=&bzMv[BM_JKICK]; int k=f->t<m->start?f->t*256/m->start:256; BzPose a=bzPz[PZ_JUMP]; if(f->arm) bzMirror(&a); bzLerp(o,&a,&bzPz[PZ_KICK_B],k); if(f->arm) bzMirror(o); } else *o=bzPz[PZ_JUMP]; break;
    case BS_ATK: { const BzMv*m=&bzMv[f->mv]; BzPose a=bzPz[bzMvA[f->mv]], b=bzPz[bzMvB[f->mv]], id=*idle; int t=f->t, half=m->start/2;
        if(f->arm&&f->mv!=BM_PALM&&f->mv!=BM_SPIN&&f->mv!=BM_THROW){ bzMirror(&a); bzMirror(&b); bzMirror(&id); }
        if(t<=half) bzLerp(o,&id,&a,half?t*256/half:256);
        else if(t<m->start) bzLerp(o,&a,&b,(t-half)*256/(m->start-half));
        else if(t<m->start+m->act) *o=b;
        else bzLerp(o,&b,&id,(t-m->start-m->act)*256/m->rec);
        if(f->arm&&f->mv!=BM_PALM&&f->mv!=BM_SPIN&&f->mv!=BM_THROW) bzMirror(o);   // (the mirrored copy of a mirrored pose is the pose again: the swap above already did the work)
        break; }
    case BS_HIT: { int k=f->stun-f->t; k=k<4?k*64:256; bzLerp(o,idle,&bzPz[PZ_HIT],k); break; }
    case BS_THROWN: { *o=bzPz[PZ_HIT]; int p=f->stun>0?(f->stun-f->t)*256/f->stun:256; *rot=(p*256/256)&255; *py=950+((p<128?p:256-p)*2); break; }
    case BS_KO: { *o=bzPz[PZ_HIT]; int k=f->t<14?f->t*64/14:64; *rot=k; *py=950-(950-250)*k/64; break; }
    case BS_WIN: *o=bzPz[PZ_WIN]; break;
    case BS_WALK: { *o=*idle;
        int fm=(f->mx*fsin(f->yaw)+f->mz*fcos(f->yaw))>>8, sm=(f->mx*fcos(f->yaw)-f->mz*fsin(f->yaw))>>8;   // forward and sideways parts of the move (-256..256)
        int ph=(f->cnt*9)&255; for(int s=0;s<2;s++){ int p=(ph+s*128)&255, sw=(fsin(p)*260)>>8, lift=fcos(p)>0?(fcos(p)*70)>>8:0;
            o->f[s][2]=(s16)(o->f[s][2]+(sw*fm>>8)); o->f[s][0]=(s16)(o->f[s][0]+(sw*sm>>8)); o->f[s][1]=(s16)(o->f[s][1]+lift); o->k[s][2]=(s16)(o->k[s][2]+(sw*fm>>9)); o->k[s][1]=(s16)(o->k[s][1]+lift); }
        break; }
    default: *o=*idle; { int bob=(fsin(f->cnt*5)*14)>>8; o->drop=(s16)(o->drop+bob+14); }
    }
    *py-=o->drop;
}
static void bzBalls(const Bz*f,BzBall*out){
    BzPose p; int rot, py; bzPoseOf(f,&p,&rot,&py);
    int sc=f->body==1?290:f->body==2?232:256;   // standard / heavy / slim: the balls' sizes
    int lean=p.lean, cz=lean+20;
    // 0 head  1 hair  2 chest  3 belly  4 pelvis | 5,6 upper arms  7,8 elbows  9,10 forearms  11,12 hands | 13,14 thighs  15,16 knees  17,18 shins  19,20 feet
    static const u16 rr[BZ_NBALL]={190,135,275,0,245, 105,105,100,100,95,95,115,115, 165,165,135,135,130,130,140,140};
    static const u8 cl[BZ_NBALL]={0,3,1,1,2, 1,1,0,0,0,0,0,0, 2,2,2,2,2,2,4,4};
    int B[BZ_NBALL][3];
    B[0][0]=0; B[0][1]=py+760; B[0][2]=cz+lean/3+40;
    B[1][0]=0; B[1][1]=B[0][1]+105; B[1][2]=B[0][2]-80;
    B[2][0]=0; B[2][1]=py+440; B[2][2]=cz;
    B[3][0]=0; B[3][1]=py+215; B[3][2]=cz/2;
    B[4][0]=0; B[4][1]=py; B[4][2]=0;
    for(int s=0;s<2;s++){
        int shx=s?330:-330, shy=py+520, hpx=s?140:-140;
        B[5+s][0]=(shx+p.e[s][0])/2; B[5+s][1]=(shy+p.e[s][1])/2; B[5+s][2]=(cz+p.e[s][2])/2;
        B[7+s][0]=p.e[s][0]; B[7+s][1]=p.e[s][1]; B[7+s][2]=p.e[s][2];
        B[9+s][0]=(p.e[s][0]+p.h[s][0])/2; B[9+s][1]=(p.e[s][1]+p.h[s][1])/2; B[9+s][2]=(p.e[s][2]+p.h[s][2])/2;
        B[11+s][0]=p.h[s][0]; B[11+s][1]=p.h[s][1]; B[11+s][2]=p.h[s][2];
        B[13+s][0]=(hpx+p.k[s][0])/2; B[13+s][1]=(py+p.k[s][1])/2; B[13+s][2]=p.k[s][2]/2;
        B[15+s][0]=p.k[s][0]; B[15+s][1]=p.k[s][1]; B[15+s][2]=p.k[s][2];
        B[17+s][0]=(p.k[s][0]+p.f[s][0])/2; B[17+s][1]=(p.k[s][1]+p.f[s][1])/2; B[17+s][2]=(p.k[s][2]+p.f[s][2])/2;
        B[19+s][0]=p.f[s][0]; B[19+s][1]=p.f[s][1]; B[19+s][2]=p.f[s][2]; }
    // (the poses' limbs are measured from a standing body: with the pelvis lower, or higher, they come with it. The thigh's top is the hip, so it follows too)
    if(py!=950){ int dy=py-950; for(int i=5;i<BZ_NBALL;i++) B[i][1]+=dy; for(int i=13;i<15;i++) B[i][1]=(py+B[i+2][1])/2; }
    int c=fcos(rot), s=fsin(rot), yawA=f->yaw+f->spin, S=fsin(yawA), C=fcos(yawA);
    for(int i=0;i<BZ_NBALL;i++){
        int lx=B[i][0], ly=B[i][1]-py, lz=B[i][2];
        if(rot){ int ny=(c*ly+s*lz)>>8, nz=(-s*ly+c*lz)>>8; ly=ny; lz=nz; }
        int wx=(lz*S+lx*C)>>8, wz=(lz*C-lx*S)>>8;
        out[i].x=f->x+wx; out[i].z=f->z+wz; out[i].y=f->y+py+ly; out[i].r=rr[i]*sc/256; out[i].col=cl[i];
        if(f->y>-300&&out[i].y<out[i].r) out[i].y=out[i].r; }   // (nothing sinks into the floor; off the ring there is no floor)
}
// ---- the camera ----
typedef struct { int yaw; s32 x, z; int h, yh, f; } BzCam;   // yaw (0..255), position, height (mm), the screen row of the horizon, the focal length (px)
enum { BC_SIDE, BC_CHASE, BC_HIGH, BC_N };
static inline int bzProj(const BzCam*c,s32 X,int Y,s32 Z,int*sx,int*sy,int*zc){
    int rx=(int)(X-c->x), rz=(int)(Z-c->z), S=fsin(c->yaw), C=fcos(c->yaw);
    int zz=(rx*S+rz*C)>>8, xx=(rx*C-rz*S)>>8; if(zz<150) return 0;
    *zc=zz; *sx=120+(c->f*xx)/zz; *sy=c->yh+(c->f*(c->h-Y))/zz; return 1;
}
// the same with ONE division (the picture does a few hundred of these a frame, and the GBA has no divide instruction): inv is 65536 * focal length / depth, r (a size in mm) comes back in pixels
static inline int bzProjI(const BzCam*c,int S,int C,s32 X,int Y,s32 Z,int*sx,int*sy,int*inv){
    int rx=(int)(X-c->x), rz=(int)(Z-c->z), zz=(rx*S+rz*C)>>8, xx=(rx*C-rz*S)>>8; if(zz<150) return 0;
    int iv=(c->f<<16)/zz; *inv=iv; *sx=120+((xx*iv)>>16); *sy=c->yh+(((c->h-Y)*iv)>>16); return 1;
}
static void bzCamUpdate(BzCam*c,int mode,const Bz*p,const Bz*o,int snap){
    int dx=(int)(o->x-p->x), dz=(int)(o->z-p->z), d=bzSqrt(dx*dx+dz*dz), L=bzAng(dx,dz);
    s32 ax=(p->x+o->x)/2, az=(p->z+o->z)/2; int ty, th, tyh, tf, dd;
    if(mode==BC_CHASE){ ty=L; th=1750; tyh=70; tf=170; dd=2300+d/6; ax=p->x; az=p->z; }   // behind you, looking over your shoulder at them
    else if(mode==BC_HIGH){ ty=(L-64)&255; th=3300; tyh=24; tf=185; dd=3700+d/2; }        // from above, off to the side: the whole ring
    else { ty=(L-64)&255; th=1450; tyh=70; tf=190; dd=2900+d/2; }                         // SIDE: square on to the line between them (you on the left), the classic view
    s32 tx=ax-(s32)(fsin(ty)*dd>>8), tz=az-(s32)(fcos(ty)*dd>>8);
    if(mode==BC_CHASE){ tx+=(s32)(fcos(ty)*380>>8); tz-=(s32)(fsin(ty)*380>>8); }
    if(snap){ c->yaw=ty; c->x=tx; c->z=tz; c->h=th; c->yh=tyh; c->f=tf; return; }
    int dy=bzWrap(ty-c->yaw); if(dy>7) dy=7; if(dy<-7) dy=-7; c->yaw=(c->yaw+dy)&255;
    c->x+=(tx-c->x)/6; c->z+=(tz-c->z)/6; c->h+=(th-c->h)/8; c->yh+=(tyh-c->yh)/6; c->f+=(tf-c->f)/8;
}
// ---- the pad -> a move vector ----
static void bzOrbitVec(const Bz*f,const Bz*o,int uy,int ux,int*mx,int*mz){   // ORBIT: relative to the opponent (uy +1 closes in, ux +1 is the fighter's right)
    int a=bzAng((int)(o->x-f->x),(int)(o->z-f->z)), S=fsin(a), C=fcos(a);
    *mx=uy*S+ux*C; *mz=uy*C-ux*S; if(uy&&ux){ *mx=*mx*181>>8; *mz=*mz*181>>8; }
}
static void bzScreenVec(const BzCam*c,int uy,int ux,int*mx,int*mz){   // SCREEN: the pad as the camera sees it (uy +1 into the screen, ux +1 to the right)
    int S=fsin(c->yaw), C=fcos(c->yaw);
    *mx=uy*S+ux*C; *mz=uy*C-ux*S; if(uy&&ux){ *mx=*mx*181>>8; *mz=*mz*181>>8; }
}
// ---- one fighter's step ----
static int bzFree(const Bz*f){ return f->st==BS_IDLE||f->st==BS_WALK||f->st==BS_GUARD; }
static void bzReset(Bz*f,s32 x,s32 z,int yaw){
    f->x=x; f->z=z; f->y=f->vy=0; f->kx=f->kz=f->jx=f->jz=0; f->yaw=yaw&255; f->spin=0; f->hp=f->trail=100; f->trailT=0; f->st=BS_IDLE; f->t=f->stun=0; f->mv=0; f->cnt=0; f->flash=0; f->combo=0;
    f->mx=f->mz=0; f->arm=0; f->jatk=0; f->fall=0; f->lvl=0; f->done=0; f->aiT=f->aiDo=f->aiSide=0; f->ev=0; f->hx=f->hy=f->hz=0;
}
static int bzLand(Bz*a,Bz*o,int mv){   // 1 = the blow connected (hit or blocked)   // a blow connects (or not)
    const BzMv*m=&bzMv[mv]; int dx=(int)(o->x-a->x), dz=(int)(o->z-a->z), d=bzSqrt(dx*dx+dz*dz), ang=bzAng(dx,dz);
    if(d>m->reach+BZ_BODY) return 0;
    if(bzWrap(ang-a->yaw)>m->arc||bzWrap(ang-a->yaw)<-m->arc) return 0;   // (a sidestep beats a straight blow)
    if(o->st==BS_KO||o->st==BS_THROWN) return 0;
    if(m->lvl==3&&(o->y>120||o->st==BS_JUMP)) return 0;
    int air=o->y>260; if(m->lvl==0&&air&&mv!=BM_JKICK&&o->y>600) return 0;   // (high blows sail under a high jump)
    static const s16 hy[4]={1500,1200,500,1200}; a->hx=(int)o->x; a->hz=(int)o->z; a->hy=(int)o->y+hy[m->lvl]; a->lvl=m->lvl;
    int guard=o->st==BS_GUARD||o->st==BS_BLOCK;
    if(guard&&m->lvl!=3){ int back=bzWrap((ang+128)-o->yaw); if(back>70||back<-70) guard=0; } else guard=0;   // (a guard only covers the front)
    int S=fsin(ang), C=fcos(ang);
    if(guard){ o->st=BS_BLOCK; o->t=8; o->kx=S*m->kb*5>>8; o->kz=C*m->kb*5>>8; a->ev|=BZE_BLOCK; return 1; }
    int dmg=m->dmg*a->pw/100;
    if(o->st==BS_ATK&&o->t<bzMv[o->mv].start){ dmg=dmg*3/2; bzCall="COUNTER"; bzCallT=40; a->ev|=BZE_CALL; }
    if(o->st==BS_HIT){ o->combo++; bzNum(bzCallB," HITS",o->combo+1); bzCall=bzCallB; bzCallT=40; a->ev|=BZE_CALL; } else o->combo=0;
    if(o->hp>dmg) o->hp-=dmg; else o->hp=0;
    o->trailT=24; o->flash=3;
    o->st=m->lvl==3?BS_THROWN:BS_HIT; o->t=o->stun=m->stun; o->kx=S*m->kb*13>>8; o->kz=C*m->kb*13>>8; o->jatk=0;
    a->ev|=BZE_HIT; if(m->dmg>=14) a->ev|=BZE_BIG;
    if(m->lvl==3){ bzCall="THROW"; bzCallT=40; a->ev|=BZE_CALL|BZE_THROW; o->yaw=(a->yaw+128)&255; }
    if(mv==BM_PALM||mv==BM_SPIN){ bzCall=m->nm; bzCallT=45; a->ev|=BZE_CALL; }
    if(o->hp==0){ o->st=BS_KO; o->t=0; o->kx=S*(m->kb+6)*13>>8; o->kz=C*(m->kb+6)*13>>8; a->ev|=BZE_BIG|BZE_KO; }
    return 1;
}
static void bzStep(Bz*f,Bz*o,const BzIn*in){
    int dx=(int)(o->x-f->x), dz=(int)(o->z-f->z), dist=bzSqrt(dx*dx+dz*dz), ang=bzAng(dx,dz), S=fsin(ang), C=fcos(ang);
    f->cnt++; if(f->flash>0) f->flash--;
    if(f->trailT>0) f->trailT--; else if(f->trail>f->hp) f->trail--; if(f->trail<f->hp) f->trail=f->hp;
    int turn=0;   // how fast it may turn to face the opponent this frame (0 = not at all)
    if(f->st==BS_KO){ f->t++; }
    else if(f->st==BS_WIN){ f->t++; turn=4; }
    else if(f->st==BS_HIT||f->st==BS_BLOCK||f->st==BS_THROWN){ if(--f->t<=0){ f->st=f->y>0?BS_JUMP:BS_IDLE; f->combo=0; f->jatk=0; } }
    else if(f->st==BS_ATK){ const BzMv*m=&bzMv[f->mv]; f->t++;
        if(f->t<=m->start) turn=3;
        if(f->t==1) f->ev|=BZE_SWING;
        if(f->t>m->start&&f->t<=m->start+m->act&&!f->done) f->done=bzLand(f,o,f->mv);
        if(f->mv==BM_SPIN){ int span=m->start+m->act; f->spin=f->t<span?f->t*256/span:0; }
        if(f->t>=m->start+m->act+m->rec){ f->st=BS_IDLE; f->spin=0; } }
    else if(f->st==BS_JUMP){ turn=3;
        if(!f->jatk&&(in->pa||in->pb)){ f->jatk=1; f->done=0; f->mv=BM_JKICK; f->t=0; f->arm=in->pb?1:0; f->ev|=BZE_SWING; }
        if(f->jatk){ f->t++; if(f->t>bzMv[BM_JKICK].start&&f->t<=bzMv[BM_JKICK].start+bzMv[BM_JKICK].act&&!f->done) f->done=bzLand(f,o,BM_JKICK); } }
    else {   // free to act
        turn=7; int fdot=(in->mx*S+in->mz*C)>>8, fwd=fdot>100, back=fdot<-100, mv=in->mx||in->mz;
        f->mx=in->mx; f->mz=in->mz;
        int ab=(in->pa&&in->b)||(in->pb&&in->a);
        if(ab&&dist<=bzMv[BM_THROW].reach+BZ_BODY){ f->st=BS_ATK; f->mv=BM_THROW; f->t=0; f->jatk=0; f->done=0; }
        else if(in->pa){ f->st=BS_ATK; f->mv=fwd?BM_PALM:BM_JAB; f->t=0; f->done=0; f->arm=fwd?0:(f->arm^1); }
        else if(in->pb){ f->st=BS_ATK; f->mv=back?BM_SPIN:BM_KICK; f->t=0; f->done=0; f->arm=f->arm^1; }
        else if(in->pj&&f->y==0){ f->st=BS_JUMP; f->vy=62; f->jatk=0; f->jx=in->mx*f->sp/128; f->jz=in->mz*f->sp/128; }
        else if(in->guard) f->st=BS_GUARD;
        else if(mv) f->st=BS_WALK; else f->st=BS_IDLE;
        if(f->st==BS_WALK){ int spd=f->sp*2; if(back) spd=spd*4/5; f->x+=in->mx*spd>>8; f->z+=in->mz*spd>>8; }
    }
    if(turn){ int dy=bzWrap(ang-f->yaw); if(dy>turn) dy=turn; if(dy<-turn) dy=-turn; f->yaw=(f->yaw+dy)&255; }
    if(f->st==BS_JUMP){ f->y+=f->vy; f->vy-=3; f->x+=f->jx; f->z+=f->jz; if(f->y<=0){ f->y=0; f->vy=0; f->jx=f->jz=0; f->st=BS_IDLE; f->jatk=0; f->ev|=BZE_LAND; } }
    else if(f->st==BS_THROWN){ int p=f->stun>0?(f->stun-f->t)*256/f->stun:256; int q=p<128?p:256-p; f->y=q*5; f->x+=f->kx; f->z+=f->kz; f->kx=f->kx*14/16; f->kz=f->kz*14/16; if(f->t<=1) f->y=0; }
    else if(f->kx||f->kz){ f->x+=f->kx; f->z+=f->kz; f->kx=f->kx*13/16; f->kz=f->kz*13/16; if(f->kx>-4&&f->kx<4) f->kx=0; if(f->kz>-4&&f->kz<4) f->kz=0; }
    // off the ring: over the edge and down
    { int rr=bzSqrt((int)(f->x/4)*(int)(f->x/4)+(int)(f->z/4)*(int)(f->z/4))*4;
      if(!f->fall){
          if(bzFree(f)||f->st==BS_ATK||f->st==BS_JUMP){ if(rr>BZ_R-120){ f->x=f->x*(BZ_R-120)/rr; f->z=f->z*(BZ_R-120)/rr; } }   // (an invisible wall for anyone who is on their feet)
          else if(rr>BZ_R+120&&f->y<=0){ f->fall=1; f->st=BS_KO; f->t=0; f->vy=0; f->kx=f->kx/2; f->kz=f->kz/2; } }   // (a blow carried you over the edge: down you go)
      else { f->y+=f->vy; f->vy-=4; f->x+=f->kx; f->z+=f->kz; f->kx=f->kx*13/16; f->kz=f->kz*13/16; if(f->y<-1600) f->y=-1600; } }
    // keep the two apart
    if(dist<BZ_SEP&&f->st!=BS_KO&&o->st!=BS_KO&&f->y<300&&o->y<300){ int push=(BZ_SEP-dist)/2+1; f->x-=S*push>>8; f->z-=C*push>>8; o->x+=S*push>>8; o->z+=C*push>>8; }
}
// ---- the opponent's pad: approach, circle, poke, block what it sees coming, sometimes a special (uses the same orbit vector as the player's ORBIT movement) ----
static void bzAi(Bz*f,const Bz*o,BzIn*in){
    in->mx=in->mz=0; in->a=in->b=in->pa=in->pb=in->pj=in->guard=0;
    if(o->st==BS_KO||o->st==BS_WIN||f->st==BS_KO) return;
    int dx=(int)(o->x-f->x), dz=(int)(o->z-f->z), d=bzSqrt(dx*dx+dz*dz), lv=f->d, uy=0, ux=0;
    if(o->st==BS_ATK&&o->t<=bzMv[o->mv].start&&d<=bzMv[o->mv].reach+BZ_BODY+300&&f->aiDo!=2&&f->aiDo!=11){
        int r=rnd8()*100>>8; if(r<14+lv*8){ f->aiDo=2; f->aiT=10+(rnd8()&7); } else if(r<26+lv*8){ f->aiDo=11; f->aiSide=(rnd8()&1)?1:-1; f->aiT=9+(rnd8()&5); } }   // (it sees a blow coming: guard, or step out of its line)
    if(f->aiT>0) f->aiT--;
    else { int r=rnd8()*100>>8; f->aiT=(18-lv)+(rnd8()&15); f->aiSide=(rnd8()&1)?1:-1;
        if(d>2300) f->aiDo=r<80?1:(lv>=2?6:1);
        else if(d>1500) f->aiDo=r<42?1:r<58?3:(r<68&&lv>=2)?4:r<84?8:0;
        else f->aiDo=r<28?7:r<46?3:r<56?8:(r<64&&lv>=1)?9:(r<70&&lv>=3)?5:r<80?0:r<92?10:2; }
    switch(f->aiDo){
        case 1: uy=1; break;                                         // walk in
        case 2: in->guard=1; break;                                  // guard
        case 3: in->b=in->pb=1; f->aiDo=0; break;                    // kick
        case 4: uy=1; in->a=in->pa=1; f->aiDo=0; break;              // the palm
        case 5: uy=-1; in->b=in->pb=1; f->aiDo=0; break;             // the spin kick
        case 6: uy=1; in->pj=1; f->aiDo=0; break;                    // jump in
        case 7: in->a=in->pa=1; f->aiDo=0; break;                    // jab
        case 8: ux=f->aiSide; if(d>1300) uy=1; break;                // circle
        case 9: in->a=in->b=1; in->pa=1; f->aiDo=0; break;           // throw
        case 10: uy=-1; break;                                       // back off
        case 11: ux=f->aiSide; break;                                // out of the line
    }
    bzOrbitVec(f,o,uy,ux,&in->mx,&in->mz);
}
#endif

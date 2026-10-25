// test_bortua3d.c - PC test of BORTUA FIGHTER's 3D mode core (source/bortua3dcore.h): maths, blows (reach, arc, guard, throw), the ring, the camera, and AI fights.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef short s16; typedef int s32;
#include "fsin.h"
static u32 lr=12345; static int rnd8(void){ lr=lr*1664525u+1013904223u; return (int)(lr>>24); }
#include "../source/bortua3dcore.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } }while(0)
static void mk(Bz*f,int x,int z,int yaw,int d){ bzReset(f,x,z,yaw); f->pw=100; f->sp=17; f->d=(u8)d; f->body=0; }
static BzIn none(void){ BzIn i; memset(&i,0,sizeof i); return i; }
static void run(Bz*a,Bz*b,int n){ BzIn z=none(); for(int i=0;i<n;i++){ a->ev=b->ev=0; bzStep(a,b,&z); bzStep(b,a,&z); } }
int main(void){
    // angles
    for(int a=2;a<254;a++){ if(a%64<2||a%64>62) continue; int g=bzAng(fsin(a)*100,fcos(a)*100); int d=bzWrap(g-a); CHECK(d>=-2&&d<=2,"bzAng %d -> %d",a,g); }
    CHECK(bzSqrt(144)==12&&bzSqrt(0)==0&&bzSqrt(99999)==316,"sqrt");
    // a jab in front lands; one to the side misses; a guard blocks; a throw works up close, not at range
    { Bz a,b; mk(&a,0,-500,0,0); mk(&b,0,300,128,0); BzIn in=none(); in.pa=in.a=1; a.ev=0; bzStep(&a,&b,&in); in=none(); for(int i=0;i<12;i++){ bzStep(&a,&b,&in); bzStep(&b,&a,&in); }
      CHECK(b.hp<100&&b.hp==100-bzMv[BM_JAB].dmg,"jab damage hp=%d",b.hp); }
    { Bz a,b; mk(&a,0,-500,0,0); mk(&b,1300,-300,128,0); BzIn in=none(); in.pa=in.a=1; bzStep(&a,&b,&in); in=none(); for(int i=0;i<12;i++){ bzStep(&a,&b,&in); bzStep(&b,&a,&in); }
      CHECK(b.hp==100,"a jab at 1300 mm off to the side must miss, hp=%d",b.hp); }
    { Bz a,b; mk(&a,0,-500,0,0); mk(&b,0,300,128,0); BzIn g=none(); g.guard=1; BzIn in=none(); in.pa=in.a=1; bzStep(&a,&b,&in); bzStep(&b,&a,&g); in=none();
      for(int i=0;i<12;i++){ bzStep(&a,&b,&in); bzStep(&b,&a,&g); }
      CHECK(b.hp==100,"a guard must block a jab, hp=%d",b.hp); }
    { Bz a,b; mk(&a,0,-300,0,0); mk(&b,0,400,128,0); BzIn in=none(); in.a=1; in.pb=1; bzStep(&a,&b,&in); in=none(); for(int i=0;i<30;i++){ bzStep(&a,&b,&in); bzStep(&b,&a,&in); }
      CHECK(b.hp<100,"a throw up close must land, hp=%d",b.hp); }
    { Bz a,b; mk(&a,0,-1800,0,0); mk(&b,0,1800,128,0); BzIn in=none(); in.a=1; in.pb=1; bzStep(&a,&b,&in);
      CHECK(a.st!=BS_ATK||a.mv!=BM_THROW,"no throw out of reach"); }
    // a K.O.: enough palms
    { Bz a,b; mk(&a,0,-400,0,0); mk(&b,0,300,128,0); b.hp=10; BzIn in=none(); in.mz=256; in.pa=in.a=1; bzStep(&a,&b,&in); in=none(); int ko=0; for(int i=0;i<40;i++){ a.ev=0; bzStep(&a,&b,&in); bzStep(&b,&a,&in); if(a.ev&BZE_KO) ko=1; }
      CHECK(b.st==BS_KO&&ko&&b.hp==0,"a palm on 10 hp must K.O. (st=%d hp=%d)",b.st,b.hp); }
    // the ring: a fighter walking outwards stops at the edge
    { Bz a,b; mk(&a,0,0,64,0); mk(&b,0,-1500,0,0); BzIn in=none(), z0=none(); in.mx=256; for(int i=0;i<300;i++){ a.ev=0; bzStep(&a,&b,&in); in.mx=256; bzStep(&b,&a,&z0); }
      int r=bzSqrt((int)(a.x/4)*(int)(a.x/4)+(int)(a.z/4)*(int)(a.z/4))*4; CHECK(r<=BZ_R,"walked out of the ring r=%d",r); }
    // knocked far out: falls
    { Bz a,b; mk(&a,0,2200,128,0); mk(&b,0,1700,0,0); a.kz=130; a.st=BS_HIT; a.t=a.stun=30; run(&a,&b,60); CHECK(a.fall||a.y<0,"a fighter knocked off the ring must fall (y=%d)",a.y); }
    // AI fights, many: no fighter leaves the ring, hp in range, a result in time, balls sane, the camera projects both
    int ko=0, tim=0; int bad=0;
    for(int seed=1;seed<=60;seed++){ lr=seed*7919u; Bz a,b; mk(&a,-1000,0,64,seed%4); mk(&b,1000,0,192,(seed*3)%5); a.body=seed%3; b.body=(seed/3)%3; b.sp=12+seed%8; BzCam cam; int mode=seed%3;
        bzCamUpdate(&cam,mode,&a,&b,1); int f;
        for(f=0;f<5400;f++){ BzIn ia,ib; bzAi(&a,&b,&ia); bzAi(&b,&a,&ib); a.ev=b.ev=0; bzStep(&a,&b,&ia); bzStep(&b,&a,&ib); bzCamUpdate(&cam,mode,&a,&b,0);
            Bz*fs[2]={&a,&b};
            for(int q=0;q<2;q++){ Bz*x=fs[q]; int r=bzSqrt((int)(x->x/4)*(int)(x->x/4)+(int)(x->z/4)*(int)(x->z/4))*4;
                if(!x->fall&&(bzFree(x)||x->st==BS_ATK||x->st==BS_JUMP)&&r>BZ_R){ if(bad++<3) printf("seed %d f %d: fighter %d out at r=%d st=%d\n",seed,f,q,r,x->st); }
                if(x->hp<0||x->hp>100){ if(bad++<3) printf("hp %d\n",x->hp); } }
            if(f%20==0&&!a.fall&&!b.fall){ BzBall bl[BZ_NBALL]; for(int q=0;q<2;q++){ bzBalls(fs[q],bl); for(int i=0;i<BZ_NBALL;i++){ int sx,sy,zc; if(bl[i].y<0&&!fs[q]->fall) { if(bad++<3) printf("ball below floor\n"); }
                        if(!bzProj(&cam,bl[i].x,bl[i].y,bl[i].z,&sx,&sy,&zc)){ if(bad++<3) printf("seed %d f %d mode %d: ball %d (fighter %d st %d) behind the camera: cam %d,%d yaw %d  ball %d,%d,%d\n",seed,f,mode,i,q,fs[q]->st,(int)cam.x,(int)cam.z,cam.yaw,(int)bl[i].x,bl[i].y,(int)bl[i].z); } } } }
            if(a.st==BS_KO&&a.t>40) break; if(b.st==BS_KO&&b.t>40) break; if(a.fall&&a.y<-1500) break; if(b.fall&&b.y<-1500) break; }
        if(f<5400) ko++; else tim++; }
    printf("60 AI fights: %d ended in a K.O. or ring-out, %d went to the time limit, %d anomalies\n",ko,tim,bad);
    CHECK(!bad,"anomalies in the fights"); CHECK(ko>=40,"too few fights end in a result (%d)",ko);
    // pose lerp / mirror sanity
    { BzPose p=bzPz[PZ_JAB_B]; BzPose q=p; bzMirror(&q); bzMirror(&q); CHECK(!memcmp(&p,&q,sizeof p),"mirror twice must be the identity"); }
    printf(fails?"FAILED: %d\n":"ok: bortua3dcore\n",fails); return fails?1:0;
}

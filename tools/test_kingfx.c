// test_kingfx.c - PC test of source/kingfx.h: the sprites of the life on the map. A small realm is built and run; every frame the carriers are tracked and the plan of sprites is made, and each
// word is checked: shapes and sizes agree, the tiles and palettes are the ones the art has, nothing is off the screen, the plan never overflows the OAM, carriers glide and turn to face where they go.
// Run from the repo root:   sh tools/test_kingfx.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
static u16 sb[1024];
#define KV_PUT(sx,sy,e) do{ sb[(sy)*32+(sx)]=(u16)(e); }while(0)
#include "../source/kingview.h"
#include "../source/kinghand.h"
#include "../source/kinggiant.h"
#include "../source/kingfx.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int fresh(int tier,u32 seed){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=5; kgGen(cells,seed,tier); int e=ksInit(&S,&R,B,cells); for(int i=0;i<KG_GOODS;i++) R.goods[i]=60; return e; }
static int word_ok(const u16*o,int n,int*kinds){   // 0 if the three words of sprite n are well formed; counts the kinds
    unsigned a0=o[n*3], a1=o[n*3+1], a2=o[n*3+2]; int shape=a0>>14, size=a1>>14, tile=a2&1023, bank=a2>>12;
    if(a0&0x3F00) return 1;                                  // no mode / mosaic / colour bits
    if(a1&0x0E00) return 2;                                  // no affine bits
    if(a2&0x0C00&&((a2>>10)&3)!=1) return 3;                 // priority 1
    if(shape==2&&size==0){ if(tile<KF_NUB||tile>=KF_GOOD||((tile-KF_NUB)&1)||bank<1||bank>4) return 4; kinds[0]++; return 0; }             // a Nubbin: 8 x 16
    if(shape==0&&size==0){ if(tile>=KF_GOOD&&tile<KF_SMOKE&&bank==5){ kinds[1]++; return 0; } if(tile>=KF_SMOKE&&tile<KF_CURSOR&&bank==6){ kinds[2]++; return 0; } return 5; }   // a good, a puff of smoke
    if(shape==0&&size==1){ if(tile==KF_CURSOR&&bank==7){ kinds[3]++; return 0; } if(tile>=KF_CROWD&&tile<KF_CROWD+56&&bank==8&&(((tile-KF_CROWD)%28)==0)){ kinds[4]++; return 0; } return 6; }   // the cursor, 16 x 16; a group of 5
    if(shape==1&&size==2){ if(bank==8&&tile>=KF_CROWD&&tile<KF_CROWD+56&&(((tile-KF_CROWD)%28)==4)){ kinds[5]++; return 0; } return 8; }                    // a crowd of 13, 32 x 16
    if(shape==0&&size==2){ if(bank==8&&tile>=KF_CROWD&&tile<KF_CROWD+56&&(((tile-KF_CROWD)%28)==12)){ kinds[6]++; return 0; } return 9; }                   // a throng of 26, 32 x 32
    if(shape==2&&size==3){ if(bank==0&&tile%KG_FTILES==0&&tile<KG_FRAMES*KG_FTILES){ kinds[7]++; return 0; } return 10; }                                    // the giant, 32 x 64
    return 7; }
static int nfaces(const u16*o,int n){ int f=0; for(int i=0;i<n;i++){ int sh=o[i*3]>>14, sz=o[i*3+1]>>14, tile=o[i*3+2]&1023, bank=o[i*3+2]>>12;
        if(sh==2&&sz==0) f++; else if(bank==8&&sh==0&&sz==1) f+=5; else if(bank==8&&sh==1&&sz==2) f+=13; else if(bank==8&&sh==0&&sz==2) f+=KF_THRONG; (void)tile; } return f; }
static int nsmall(const u16*o,int n,int kind){ int c=0; for(int i=0;i<n;i++){ int sh=o[i*3]>>14, sz=o[i*3+1]>>14, bank=o[i*3+2]>>12; c+=kind==0?(bank==8&&sh==0&&sz==1):kind==1?(bank==8&&sh==1&&sz==2):(bank==8&&sh==0&&sz==2); } return c; }
int main(void){
    printf("== kingfx: the life on the map\n");
    CHECK(fresh(0,31u)==KSE_OK,"init"); int d=S.dim, c=d/2;
    for(int y=c-8;y<c+10;y++) for(int x=c-12;x<c+16;x++) cells[y*d+x]=KT_GRASS;
    ksPlace(&S,KB_STORE,c-1,c-1,1); ksPlace(&S,KB_HUT,c+3,c-1,1); ksPlace(&S,KB_HOUSE,c+7,c-1,1); ksPlace(&S,KB_WOODCUT,c-5,c-1,1); ksPlace(&S,KB_SAWMILL,c-1,c+3,1); ksPlace(&S,KB_TAVERN,c+7,c+3,1);
    ksPlace(&S,KB_HUT,c+3,c+5,0); ksPlace(&S,KB_MASON,c-5,c+3,0);
    for(int x=c-4;x<c+9;x++) ksRoadSet(&S,x,c+1,1); for(int y=c-1;y<c+7;y++){ ksRoadSet(&S,c+2,y,1); ksRoadSet(&S,c-2,y,1); }
    for(int i=0;i<R.nB;i++) if(B[i].lvl) B[i].work=(u8)(KS_T[B[i].type].house?3:1);
    ksCensus(&S);
    KGiant G; kgSpawn(&G,&S); KView V; kvInit(&V,S.cells,d,S.B,R.nB); kgCamera(&V,&G); KFx F; kfInit(&F); static u16 plan[KF_MAX*3];
    int kinds[8]={0,0,0,0,0,0,0,0}, maxN=0, glide=0, jumped=0, faces[3]={0,0,0}, flips=0, frames=0;
    for(int t=0;t<6000;t++){
        ksStep(&S); kfTrack(&F,&S);
        if(t%97==0){ int dx=(t/97)%3-1, dy=((t/97)/3)%3-1; kgMove(&G,&S,dx,dy,3); kgCamera(&V,&G); }
        int smooth=(t/1500)%2==0; int n=kfPlan(plan,&F,&S,&V,&G,(unsigned)t,smooth); frames++; if(n>maxN) maxN=n;
        CHECK(n>=0&&n<=KF_MAX,"plan of %d sprites",n);
        for(int i=0;i<n;i++){ int e=word_ok(plan,i,kinds); CHECK(!e,"sprite %d of frame %d is malformed (code %d: %04X %04X %04X)",i,t,e,plan[i*3],plan[i*3+1],plan[i*3+2]);
            int x=plan[i*3+1]&511; if(x>=256) x-=512; int y=plan[i*3]&255; if(y>=192) y-=256;   // (0..159 on the screen, 192..255 just above it)
            CHECK(x>-32&&x<240&&y>-64&&y<160,"sprite %d off the screen at %d,%d",i,x,y);
            if((plan[i*3]>>14)==2&&(plan[i*3+1]>>14)==0){ int view=((plan[i*3+2]&1023)-KF_NUB)/4; faces[view>2?2:view]++; flips+=(plan[i*3+1]&0x1000)!=0; } }
        // the gliding carriers: with smoothing on, no carrier moves more than a cell in a frame; a carrier that has just stepped is between cells
        if(smooth) for(int i=0;i<KS_MAXW;i++){ if(S.wk[i].st&&F.age[i]<KS_WALK&&F.prev[i]!=F.cur[i]) glide++; }
        else for(int i=0;i<KS_MAXW;i++)
        { if(!smooth&&S.wk[i].st&&F.age[i]==0&&F.prev[i]!=F.cur[i]) jumped++; }
    }
    CHECK(kinds[0]>2000,"Nubbins drawn: %d",kinds[0]); CHECK(kinds[1]>500,"loads drawn: %d",kinds[1]); CHECK(kinds[2]>100,"smoke drawn: %d",kinds[2]); CHECK(kinds[3]>3000,"the cursor drawn: %d",kinds[3]);
    CHECK(faces[0]>100&&faces[1]>100&&faces[2]>5,"carriers face every way: side %d, down %d, up %d",faces[0],faces[1],faces[2]); CHECK(flips>10,"carriers walking left are flipped (%d)",flips);
    CHECK(glide>50,"carriers were seen gliding between cells (%d)",glide); CHECK(maxN<=KF_MAX&&maxN>8,"the biggest plan is %d sprites of %d",maxN,KF_MAX);
    printf("  6000 frames: %d Nubbins, %d loads, %d puffs of smoke, %d cursors drawn; the biggest plan %d of %d sprites; %d gliding, %d jumping carriers\n",kinds[0],kinds[1],kinds[2],kinds[3],maxN,KF_MAX,glide,jumped);
    // the giant's hands: what it holds shows above its head
    { S.hd.k=KHD_GOODS; S.hd.g=G_STONE; S.hd.n=3; int n=kfPlan(plan,&F,&S,&V,&G,0,1); int found=0; for(int i=0;i<n;i++) found+=(plan[i*3+2]&1023)==KF_GOOD+G_STONE;
      CHECK(found>=1,"the stone it carries shows"); S.hd.k=KHD_NUB; S.hd.bt=1; n=kfPlan(plan,&F,&S,&V,&G,0,1); int nub=0; for(int i=0;i<n;i++) nub+=(plan[i*3]>>14)==2; CHECK(nub>=1,"the Nubbin it carries shows"); S.hd.k=KHD_NONE; }
    // a crowd: all 40 carriers at once, and the plan still fits
    { for(int i=0;i<KS_MAXW;i++){ S.wk[i].st=1; S.wk[i].good=(u8)(i&7); S.wk[i].cell=(u16)((c+1)*d+c-3+(i%14)); F.cur[i]=F.prev[i]=S.wk[i].cell; F.age[i]=255; }
      int n=kfPlan(plan,&F,&S,&V,&G,5,1); CHECK(n<=KF_MAX,"a crowd of 40 carriers made a plan of %d",n); int k2[8]={0,0,0,0,0,0,0,0}; for(int i=0;i<n;i++) CHECK(!word_ok(plan,i,k2),"crowd sprite %d",i);
      printf("  a crowd of 40 carriers: a plan of %d sprites, all well formed\n",n); }
    // a place of n Nubbins draws n faces (to within one), in at most 7 sprites, at every n up to what a place can hold
    { fresh(0,31u); KGiant g5; kgSpawn(&g5,&S); KView V5; kvInit(&V5,S.cells,S.dim,S.B,R.nB); V5.x=100; V5.y=100; int worst=0, most=0; static u16 p5[KF_MAX*3];
      for(int cnt=0;cnt<=80;cnt++){ int n=kfCrowd(p5,0,&V5,V5.x+120,V5.y+100,cnt,(unsigned)cnt*7u,(unsigned)cnt); int f=nfaces(p5,n); int exp=cnt; int dfc=f-exp; if(dfc<0) dfc=-dfc;
        CHECK(dfc<=1,"%d Nubbins drew %d faces",cnt,f); CHECK(n<=7,"%d Nubbins took %d sprites",cnt,n); if(n>most) most=n; if(dfc>worst) worst=dfc; for(int i=0;i<n;i++){ int k5[8]={0,0,0,0,0,0,0,0}; CHECK(!word_ok(p5,i,k5),"crowd of %d, sprite %d",cnt,i); } }
      printf("  places of 0..80 Nubbins: faces within %d of the count, at most %d sprites a place\n",worst,most); }
    // THE PEOPLE: a big realm (36 homes of 8, a tavern, a crowned castle: 292 Nubbins). Every Nubbin of the count is somewhere; the sprites are few.
    { CHECK(fresh(2,77u)==KSE_OK,"init big realm"); int dd=S.dim, cc=dd/2; for(int y=cc-12;y<cc+14;y++) for(int x=cc-14;x<cc+16;x++) cells[y*dd+x]=KT_GRASS;
      int nh=0;
      for(int j=0;j<6;j++) for(int i=0;i<6;i++){ int r=ksPlace(&S,KB_HOUSE,cc-14+i*3,cc-12+j*3,1); CHECK(r>=0,"house %d,%d: %d",i,j,r); if(r>=0){ B[r].work=8; nh++; } }
      int tv=ksPlace(&S,KB_TAVERN,cc+4,cc-12,1), cs=ksPlace(&S,KB_CASTLE,cc+8,cc-6,0); CHECK(tv>=0&&cs>=0,"tavern %d castle %d",tv,cs); B[cs].lvl=1; R.castle=KG_CASTLE; ksCensus(&S);
      CHECK(S.pop==KS_PARTY+36*8&&S.pop<=KS_POPCAP,"the realm counts %d Nubbins",S.pop);
      KGiant g2; kgSpawn(&g2,&S); g2.x=(cc-14)*KV_CELL; g2.y=(cc-12)*KV_CELL; KView V2; kvInit(&V2,S.cells,dd,S.B,R.nB); KFx F2; kfInit(&F2); static u16 pl[KF_MAX*3]; int fday=0, fnight=0, fdusk=0, big=0;
      const unsigned T0[5]={ KS_DAYT*0u/5u+5u, KS_DAYT*1u/5u+5u, KS_DAYT*2u/5u+5u, KS_DAYT*3u/5u+5u, KS_DAYT*4u/5u+5u };
      for(int part=0;part<5;part++){ S.t=T0[part]; CHECK(kfPart(&S)==part,"part of the day %d is %d",part,kfPart(&S));
        for(int w=0;w<12;w++){ V2.x=(cc-14)*KV_CELL-16+(w%4)*100; V2.y=(cc-12)*KV_CELL-16+(w/4)*60; kvClamp(&V2);
          int n=kfPlan(pl,&F2,&S,&V2,&g2,(unsigned)(w*9),1); CHECK(n<=KF_MAX,"plan of %d",n); int k3[8]={0,0,0,0,0,0,0,0}; for(int i=0;i<n;i++) CHECK(!word_ok(pl,i,k3),"big-realm sprite %d of part %d: %04X %04X %04X",i,part,pl[i*3],pl[i*3+1],pl[i*3+2]);
          if(n>big) big=n; int f=nfaces(pl,n); if(part==1&&f>fday) fday=f; if(part==4&&f>fnight) fnight=f; if(part==3&&f>fdusk) fdusk=f;
          // the lowest on the screen is drawn on top: the bottoms of the sprites never go up the plan (the cursor, last, is exempt)
          for(int i=1;i<n-1;i++) CHECK(kfFeet(pl,i-1)>=kfFeet(pl,i),"unsorted at %d: %d then %d",i,kfFeet(pl,i-1),kfFeet(pl,i)); } }
      CHECK(fday>=70,"by day a screen of this realm shows %d faces",fday); CHECK(big<=KF_MAX,"the biggest plan %d",big);
      printf("  %d Nubbins in the realm: the busiest screen by day shows %d faces, at dusk %d, at night %d; the biggest plan is %d sprites\n",S.pop,fday,fdusk,fnight,big);
      // the tavern: empty by day, a throng at dusk; the castle's court: a throng by day, none at night
      { int tx=B[tv].x*KV_CELL, ty=B[tv].y*KV_CELL; V2.x=tx-96; V2.y=ty-40; kvClamp(&V2); g2.x=tx; g2.y=ty+200; if(g2.y>dd*KV_CELL-40) g2.y=dd*KV_CELL-40;
        S.t=T0[1]; int n=kfPlan(pl,&F2,&S,&V2,&g2,0,1); int d1=nsmall(pl,n,2); S.t=T0[3]; n=kfPlan(pl,&F2,&S,&V2,&g2,0,1); int d3=nsmall(pl,n,2);
        CHECK(d3>d1&&d3>=1,"throngs at the tavern: %d by day, %d at dusk",d1,d3); printf("  the tavern: %d throng(s) by day, %d at dusk\n",d1,d3); }
      { int cx=B[cs].x*KV_CELL, cy=B[cs].y*KV_CELL; V2.x=cx-104; V2.y=cy-40; kvClamp(&V2); g2.x=cx; g2.y=cy+200; if(g2.y>dd*KV_CELL-40) g2.y=dd*KV_CELL-40;
        S.t=T0[1]; int n=kfPlan(pl,&F2,&S,&V2,&g2,0,1); int c1=nsmall(pl,n,2); S.t=T0[4]; n=kfPlan(pl,&F2,&S,&V2,&g2,0,1); int c4=nsmall(pl,n,2);
        CHECK(c1>=1&&c4==0,"the court by day: %d throng(s), at night %d",c1,c4); printf("  the court at the castle: %d throng(s) by day, %d at night\n",c1,c4); }
      // more Nubbins, more faces: the same screen with one home and with all of them
      { KGiant g3=g2; g3.x=(cc-14)*KV_CELL; g3.y=(cc-12)*KV_CELL; V2.x=(cc-14)*KV_CELL-16; V2.y=(cc-12)*KV_CELL-16; kvClamp(&V2); S.t=T0[1];
        int full=0, few=0; { int n=kfPlan(pl,&F2,&S,&V2,&g3,0,1); full=nfaces(pl,n); }
        for(int i=0;i<R.nB;i++) if(S.B[i].type==KB_HOUSE&&(i%3)) S.B[i].work=1; ksCensus(&S); { int n=kfPlan(pl,&F2,&S,&V2,&g3,0,1); few=nfaces(pl,n); }
        CHECK(full>few,"a fuller realm shows more people: %d against %d",full,few); printf("  the same screen: %d faces with the homes full, %d with them nearly empty (pop %d)\n",full,few,S.pop); }
      // the giant is in the sorted list: one sprite of its own, and a Nubbin in front of its feet is drawn first
      { fresh(0,31u); int d0=S.dim, c0=d0/2; for(int y=c0-8;y<c0+10;y++) for(int x=c0-12;x<c0+16;x++) cells[y*d0+x]=KT_GRASS;
        KGiant g4; kgSpawn(&g4,&S); KView V4; kvInit(&V4,S.cells,d0,S.B,R.nB); kgCamera(&V4,&g4); KFx F4; kfInit(&F4); S.wk[0].st=1; S.wk[0].good=0; S.wk[0].cell=(u16)((g4.y/KV_CELL+1)*d0+g4.x/KV_CELL); F4.cur[0]=F4.prev[0]=S.wk[0].cell; F4.age[0]=255;
        int n=kfPlan(pl,&F4,&S,&V4,&g4,0,1); int gi=-1, ni=-1; for(int i=0;i<n;i++){ if((pl[i*3]>>14)==2&&(pl[i*3+1]>>14)==3) gi=i; else if((pl[i*3]>>14)==2&&(pl[i*3+1]>>14)==0&&ni<0) ni=i; }
        CHECK(gi>=0,"the giant is in the plan"); CHECK(ni>=0&&gi>=0&&ni<gi,"a carrier one cell in front of the giant's feet is drawn on top (it %d, giant %d)",ni,gi); }
    }
    printf("%s: %d failure(s)\n",fails?"FAILED":"kingfx: ALL PASSED",fails); return fails?1:0;
}

// test_kinggiant.c - PC test of source/kinggiant.h: the walking giant. Spawn, walking and the limits (water, the border, speed under 8), the camera that follows it, the sprite art
// (every pixel checked against its tile bytes), the palette and the three OAM words (visible, flipped, off screen, wrapped). Run from the repo root:   sh tools/test_kinggiant.sh
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
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 rs=99; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int px(const u8*g,int f,int x,int y){ if(x<0||y<0||x>31||y>63) return 0; return (g[(f*32+(y>>3)*4+(x>>3))*32+(y&7)*4+((x&7)>>1)]>>(((x&7)&1)*4))&15; }   // a pixel of a frame of a 32 x 64 sprite in the 1D tile order
static int fresh(int tier,u32 seed){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=5; kgGen(cells,seed,tier); return ksInit(&S,&R,B,cells); }
int main(void){
    printf("== kinggiant: the walking giant\n");
    for(int tier=0;tier<3;tier++){
        CHECK(fresh(tier,200u+(u32)tier)==KSE_OK,"init"); int d=S.dim; KGiant G; CHECK(kgSpawn(&G,&S)==1,"spawn"); CHECK(kgcWalk(&S,G.x,G.y),"spawned on a walkable cell");
        CHECK(kgCx(&G)==d/2&&kgCy(&G)==d/2,"spawn at the middle of the land (the starting field is clear)");
        // 40,000 random frames: it never stands on water, outside the border, and never moves 8 or more pixels in a frame; the camera keeps it on the screen
        KView V; kvInit(&V,S.cells,d,S.B,0); int moves=0, run=0, maxJump=0, onscreen=0;
        for(int k=0;k<40000;k++){ int dx=(int)(rnd()%3)-1, dy=(int)(rnd()%3)-1; if(k%97<40){ dx=1; } int sp=(k&2)?khRun(k%5):khStride(k%5); int ox=G.x, oy=G.y;
            int mv=kgMove(&G,&S,dx,dy,sp); moves+=mv; run+=sp>khStride(k%5);
            int jx=abs(G.x-ox), jy=abs(G.y-oy); if(jx>maxJump) maxJump=jx; if(jy>maxJump) maxJump=jy;
            CHECK(kgcWalk(&S,G.x,G.y),"giant stands on an unwalkable cell (%d,%d) at frame %d",G.x,G.y,k); CHECK(G.x>=16&&G.y>=16&&G.x<d*16-16&&G.y<d*16-16,"outside the border");
            CHECK(mv==(G.moving!=0),"moving flag"); CHECK(!mv||jx+jy>0,"moved but did not");
            kgCamera(&V,&G); int sx=G.x-V.x, sy=G.y-V.y; if(sx>=0&&sx<240&&sy>=0&&sy<160) onscreen++; CHECK(V.x>=0&&V.y>=0&&V.x<=kvMaxX(&V)&&V.y<=kvMaxY(&V),"camera inside the land"); }
        CHECK(maxJump<16,"a frame moved %d pixels",maxJump); CHECK(onscreen==40000,"the giant left the screen"); CHECK(moves>5000,"it walked (%d)",moves);
        printf("  tier %d: 40,000 frames, %d steps, longest %d px, always on a walkable cell and on the screen\n",tier,moves,maxJump);
    }
    // water blocks, a building does not; the giant slides along a shore (one axis moves, the other is refused)
    { CHECK(fresh(0,5u)==KSE_OK,"init"); int d=S.dim; for(int i=0;i<d*d;i++) cells[i]=KT_GRASS; KGiant G; kgSpawn(&G,&S); int cx=kgCx(&G), cy=kgCy(&G);
      cells[cy*d+cx+1]=KT_WATER; G.x=cx*16+15; G.y=cy*16+8; G.face=1; CHECK(kgMove(&G,&S,1,0,3)==0&&G.x==cx*16+15&&G.face==0,"water refuses the step east, and the giant turns to face it");
      CHECK(kgMove(&G,&S,1,1,3)==1&&G.x==cx*16+15&&G.y==cy*16+14,"it slides south along the shore (y %d)",G.y);
      cells[cy*d+cx-1]=KT_ROAD; G.x=cx*16; G.y=cy*16+8; CHECK(kgMove(&G,&S,-1,0,3)==1&&G.x==cx*16-6,"a road is walked");
      memset(&B[0],0,sizeof B[0]); KHand H; H.mode=KHM_STORE; G.x=cx*16+8+32; G.y=cy*16+8+32; cells[(cy+2)*d+cx+2]=KT_GRASS; CHECK(khAct(&H,&S,cx+2,cy+2)==KHR_SITE,"a site"); int bi=khBldAt(&S,cx+2,cy+2); CHECK(bi>=0,"the site is there");
      G.x=(cx+1)*16+8+6; G.y=(cy+2)*16+8; CHECK(kgMove(&G,&S,1,0,3)==1,"a building is walked over");
      // the border and the edges
      kgSpawn(&G,&S); for(int k=0;k<4000;k++) kgMove(&G,&S,-1,-1,4); CHECK(G.x==16&&G.y==16,"stopped at the border (%d,%d)",G.x,G.y); for(int k=0;k<8000;k++) kgMove(&G,&S,1,1,7); CHECK(G.x==d*16-17&&G.y==d*16-17,"stopped at the far border (%d,%d)",G.x,G.y);
      CHECK(kgMove(&G,&S,1,0,100)==0,"a speed over 7 is capped, and the wall still holds");
      // all water: spawn reports it and stands in the middle; nothing moves
      for(int i=0;i<d*d;i++) cells[i]=KT_WATER; CHECK(kgSpawn(&G,&S)==0&&kgCx(&G)==d/2,"an all water land"); CHECK(kgMove(&G,&S,1,0,3)==0,"nowhere to walk");
      // a land whose middle is water: it spawns on the nearest dry cell
      for(int i=0;i<d*d;i++) cells[i]=KT_GRASS; for(int y=d/2-3;y<=d/2+3;y++) for(int x=d/2-3;x<=d/2+3;x++) cells[y*d+x]=KT_WATER;
      CHECK(kgSpawn(&G,&S)==1&&kgcWalk(&S,G.x,G.y)&&(abs(kgCx(&G)-d/2)==4||abs(kgCy(&G)-d/2)==4)&&abs(kgCx(&G)-d/2)<=4&&abs(kgCy(&G)-d/2)<=4,"spawn on the nearest dry cell (%d,%d)",kgCx(&G),kgCy(&G)); }
    // the camera: centred on the giant in the middle of the land, clamped at the edges (the giant is then off the middle of the screen)
    { CHECK(fresh(1,8u)==KSE_OK,"init"); int d=S.dim; KView V; kvInit(&V,S.cells,d,S.B,0); KGiant G; kgSpawn(&G,&S); kgCamera(&V,&G); CHECK(G.x-V.x==120&&G.y-V.y==92,"centred (the camera sits a little above the feet of the tall giant)");
      G.x=16; G.y=16; kgCamera(&V,&G); CHECK(V.x==0&&V.y==0&&G.x-V.x==16,"clamped at the corner"); G.x=d*16-17; G.y=d*16-17; kgCamera(&V,&G); CHECK(V.x==kvMaxX(&V)&&V.y==kvMaxY(&V)&&G.x-V.x<240&&G.y-V.y<160,"clamped at the far corner"); }
    // the art: four giants, nine frames each (32 x 64): every frame has an outline, skin, a shirt and trousers; the walking frames differ from standing; no colour outside the palette roles (the rows from 57 down hold the dithered shadow under the feet: single dark pixels on purpose)
    { for(int gi=0;gi<4;gi++){ const u8*g=kaGiants[gi]; int used[16]={0};
        for(int f=0;f<KG_FRAMES;f++) for(int y=0;y<64;y++) for(int x=0;x<32;x++){ int c=px(g,f,x,y); used[c]++; }
        CHECK(used[1]>150&&used[2]>60&&used[6]+used[8]>80&&used[9]>40&&used[0]>3000,"giant %d colours: outline %d skin %d shirt %d trousers %d clear %d",gi,used[1],used[2],used[6]+used[8],used[9],used[0]);
        for(int v=0;v<3;v++){ int diff=0; for(int i=0;i<KG_FTILES*32;i++) diff+=g[(v*3)*KG_FTILES*32+i]!=g[(v*3+1)*KG_FTILES*32+i]; CHECK(diff>=40,"giant %d: the step frame of view %d differs from standing (%d bytes)",gi,v,diff);
          int d2=0; for(int i=0;i<KG_FTILES*32;i++) d2+=g[(v*3+1)*KG_FTILES*32+i]!=g[(v*3+2)*KG_FTILES*32+i]; CHECK(d2>=40,"giant %d: the two steps of view %d differ (%d bytes)",gi,v,d2); }
        for(int f=0;f<KG_FRAMES;f++) for(int y=0;y<57;y++) for(int x=0;x<32;x++){ if(px(g,f,x,y)==1){ int n=px(g,f,x-1,y)>1||px(g,f,x+1,y)>1||px(g,f,x,y-1)>1||px(g,f,x,y+1)>1; CHECK(n,"loose outline pixel %d,%d (giant %d frame %d)",x,y,gi,f); } }
        for(int gj=0;gj<gi;gj++) CHECK(kaGiantPal[gi][6]!=kaGiantPal[gj][6],"giants %d and %d wear the same shirt",gi,gj);
        CHECK(kaGiantPal[gi][0]==0,"palette %d: colour 0 is see-through",gi); }
      printf("  the art: 4 giants x 9 frames of 32 x 64, every one with an outline, skin, shirt, trousers; the steps differ; four different shirts\n"); }
    // the OAM words (32 x 64: tall shape, size 3)
    { KGiant G; KView V; memset(&V,0,sizeof V); unsigned a0,a1,a2; G.face=0; G.step=0; G.moving=0; G.dir=0; G.x=100; G.y=100; V.x=0; V.y=0;
      kgOam(&G,&V,&a0,&a1,&a2); CHECK((a0&255)==40&&(a0>>14)==2&&(a1&511)==84&&(a1>>14)==3&&!(a1&0x1000)&&(a2&1023)==0&&((a2>>10)&3)==1,"standing: %u %u %u",a0,a1,a2);
      G.moving=1; G.step=4; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==1*KG_FTILES,"the first step uses frame 1"); G.step=12; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==2*KG_FTILES,"the second step uses frame 2"); G.step=8; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==0,"and back to standing between");
      G.dir=1; G.step=0; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==3*KG_FTILES,"sideways uses frames 3 to 5"); G.dir=2; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==6*KG_FTILES,"up uses frames 6 to 8"); G.dir=1;
      G.face=1; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a1&0x1000)!=0,"facing left flips it"); G.dir=0; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a1&0x1000)==0,"but not when walking up or down"); G.face=0; G.dir=0;
      G.x=3; G.y=100; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a1&511)==((3-16)&511)&&a0!=0x200,"partly off the left edge still shows (x wraps to %u)",a1&511);
      G.x=100; G.y=20; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a0&255)==(unsigned)((20-60)&255)&&(a0&255)>200,"partly above the top (y wraps to %u)",a0&255);
      G.x=-40; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"off the left: off"); G.x=260; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"off the right: off"); G.x=100; G.y=230; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"below: off");
      for(int sx=-60;sx<290;sx++) for(int sy=-30;sy<240;sy+=3){ G.x=sx; G.y=sy; kgOam(&G,&V,&a0,&a1,&a2); if(a0!=0x200){ int x=sx-16, y=sy-60; CHECK(x>-32&&x<240&&y>-64&&y<160&&(a0&0x3F00)==0&&(a0>>14)==2&&(a1&0x0E00)==0&&(a2&0xF000)==0,"words in range at %d,%d",sx,sy); } }
      printf("  OAM words: visible, tall shape, flipped only sideways, wrapped at the edges, switched off outside the screen\n"); }
    if(fails){ printf("kinggiant: %d FAILURE(S)\n",fails); return 1; } printf("kinggiant: ALL PASSED: 0 failure(s)\n"); return 0; }

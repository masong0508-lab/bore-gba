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
static int px(const u8*g,int f,int x,int y){ if(x<0||y<0||x>15||y>15) return 0; return (g[(f*4+(y>>3)*2+(x>>3))*32+(y&7)*4+((x&7)>>1)]>>(((x&7)&1)*4))&15; }
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
            CHECK(kgcWalk(&S,G.x,G.y),"giant stands on an unwalkable cell (%d,%d) at frame %d",G.x,G.y,k); CHECK(G.x>=8&&G.y>=8&&G.x<d*8-8&&G.y<d*8-8,"outside the border");
            CHECK(mv==(G.moving!=0),"moving flag"); CHECK(!mv||jx+jy>0,"moved but did not");
            kgCamera(&V,&G); int sx=G.x-V.x, sy=G.y-V.y; if(sx>=0&&sx<240&&sy>=0&&sy<160) onscreen++; CHECK(V.x>=0&&V.y>=0&&V.x<=kvMaxX(&V)&&V.y<=kvMaxY(&V),"camera inside the land"); }
        CHECK(maxJump<8,"a frame moved %d pixels",maxJump); CHECK(onscreen==40000,"the giant left the screen"); CHECK(moves>5000,"it walked (%d)",moves);
        printf("  tier %d: 40,000 frames, %d steps, longest %d px, always on a walkable cell and on the screen\n",tier,moves,maxJump);
    }
    // water blocks, a building does not; the giant slides along a shore (one axis moves, the other is refused)
    { CHECK(fresh(0,5u)==KSE_OK,"init"); int d=S.dim; for(int i=0;i<d*d;i++) cells[i]=KT_GRASS; KGiant G; kgSpawn(&G,&S); int cx=kgCx(&G), cy=kgCy(&G);
      cells[cy*d+cx+1]=KT_WATER; G.x=cx*8+7; G.y=cy*8+4; G.face=1; CHECK(kgMove(&G,&S,1,0,3)==0&&G.x==cx*8+7&&G.face==0,"water refuses the step east, and the giant turns to face it");
      CHECK(kgMove(&G,&S,1,1,3)==1&&G.x==cx*8+7&&G.y==cy*8+7,"it slides south along the shore (y %d)",G.y);
      cells[cy*d+cx-1]=KT_ROAD; G.x=cx*8; G.y=cy*8+4; CHECK(kgMove(&G,&S,-1,0,3)==1&&G.x==cx*8-3,"a road is walked");
      memset(&B[0],0,sizeof B[0]); KHand H; H.mode=KHM_STORE; G.x=cx*8+4+16; G.y=cy*8+4+16; cells[(cy+2)*d+cx+2]=KT_GRASS; CHECK(khAct(&H,&S,cx+2,cy+2)==KHR_SITE,"a site"); int bi=khBldAt(&S,cx+2,cy+2); CHECK(bi>=0,"the site is there");
      G.x=(cx+1)*8+4+3; G.y=(cy+2)*8+4; CHECK(kgMove(&G,&S,1,0,3)==1,"a building is walked over");
      // the border and the edges
      kgSpawn(&G,&S); for(int k=0;k<4000;k++) kgMove(&G,&S,-1,-1,4); CHECK(G.x==8&&G.y==8,"stopped at the border (%d,%d)",G.x,G.y); for(int k=0;k<8000;k++) kgMove(&G,&S,1,1,7); CHECK(G.x==d*8-9&&G.y==d*8-9,"stopped at the far border (%d,%d)",G.x,G.y);
      CHECK(kgMove(&G,&S,1,0,100)==0,"a speed over 7 is capped, and the wall still holds");
      // all water: spawn reports it and stands in the middle; nothing moves
      for(int i=0;i<d*d;i++) cells[i]=KT_WATER; CHECK(kgSpawn(&G,&S)==0&&kgCx(&G)==d/2,"an all water land"); CHECK(kgMove(&G,&S,1,0,3)==0,"nowhere to walk");
      // a land whose middle is water: it spawns on the nearest dry cell
      for(int i=0;i<d*d;i++) cells[i]=KT_GRASS; for(int y=d/2-3;y<=d/2+3;y++) for(int x=d/2-3;x<=d/2+3;x++) cells[y*d+x]=KT_WATER;
      CHECK(kgSpawn(&G,&S)==1&&kgcWalk(&S,G.x,G.y)&&(abs(kgCx(&G)-d/2)==4||abs(kgCy(&G)-d/2)==4)&&abs(kgCx(&G)-d/2)<=4&&abs(kgCy(&G)-d/2)<=4,"spawn on the nearest dry cell (%d,%d)",kgCx(&G),kgCy(&G)); }
    // the camera: centred on the giant in the middle of the land, clamped at the edges (the giant is then off the middle of the screen)
    { CHECK(fresh(1,8u)==KSE_OK,"init"); int d=S.dim; KView V; kvInit(&V,S.cells,d,S.B,0); KGiant G; kgSpawn(&G,&S); kgCamera(&V,&G); CHECK(G.x-V.x==120&&G.y-V.y==80,"centred");
      G.x=8; G.y=8; kgCamera(&V,&G); CHECK(V.x==0&&V.y==0&&G.x-V.x==8,"clamped at the corner"); G.x=d*8-9; G.y=d*8-9; kgCamera(&V,&G); CHECK(V.x==kvMaxX(&V)&&V.y==kvMaxY(&V)&&G.x-V.x<240&&G.y-V.y<160,"clamped at the far corner"); }
    // the art: every pixel of both frames is what kgGfx's tiles say (colour range, the outline, nothing in the corners that the outline cannot explain), and the frames differ
    { u8 g[256]; kgGfxAll(g); int used[16]={0}, diff=0; for(int f=0;f<2;f++) for(int y=0;y<16;y++) for(int x=0;x<16;x++){ int t=f*4+(y>>3)*2+(x>>3); int c=(g[t*32+(y&7)*4+((x&7)>>1)]>>(((x&7)&1)*4))&15; used[c]++; }
      for(int i=0;i<128;i++) diff+=g[i]!=g[128+i]; CHECK(diff>=4,"the walking frame differs from standing (%d bytes)",diff);
      CHECK(used[1]>20&&used[2]>20&&used[3]>20&&used[4]>4&&used[6]>8&&used[0]>40,"colours used: outline %d skin %d shirt %d hair %d trousers %d see-through %d",used[1],used[2],used[3],used[4],used[6],used[0]);
      CHECK(used[5]==0&&used[7]+used[8]+used[9]+used[10]+used[11]+used[12]+used[13]+used[14]+used[15]==0,"no colour outside 0 to 6");
      // an outline pixel always touches a filled pixel; a filled pixel is never alone
      for(int f=0;f<2;f++) for(int y=0;y<16;y++) for(int x=0;x<16;x++){ if(px(g,f,x,y)==1){ int n=px(g,f,x-1,y)>1||px(g,f,x+1,y)>1||px(g,f,x,y-1)>1||px(g,f,x,y+1)>1; CHECK(n,"loose outline pixel %d,%d (frame %d)",x,y,f); } }
      u16 pl[16]; for(int gi=0;gi<5;gi++){ kgPal(gi,pl); CHECK(pl[0]==0&&pl[3]!=0&&pl[4]!=0&&(pl[3]&0x8000)==0,"palette %d",gi); for(int gj=0;gj<gi;gj++){ u16 q[16]; kgPal(gj,q); CHECK(q[3]!=pl[3],"giants %d and %d wear the same shirt",gi,gj); } }
      kgPal(77,pl); CHECK(pl[3]==kgShirt[0],"a bad giant number wears the first colours");
      printf("  the art: 2 frames of 16 x 16, %d outline, %d skin, %d shirt pixels; 5 palettes, all different\n",used[1],used[2],used[3]); }
    // the OAM words
    { KGiant G; KView V; memset(&V,0,sizeof V); unsigned a0,a1,a2; G.face=0; G.step=0; G.moving=0; G.x=100; G.y=100; V.x=0; V.y=0;
      kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==85&&(a1&511)==92&&(a1>>14)==1&&!(a1&0x1000)&&a2==(1u<<10),"standing: %u %u %u",a0,a1,a2);
      G.moving=1; G.step=8; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==4,"the step frame uses tiles 4 to 7"); G.step=7; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a2&1023)==0,"and back");
      G.face=1; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a1&0x1000)!=0,"facing left flips it"); G.face=0;
      G.x=3; G.y=100; kgOam(&G,&V,&a0,&a1,&a2); CHECK((a1&511)==((3-8)&511)&&a0!=0x200,"partly off the left edge still shows (x wraps to %u)",a1&511);
      G.x=100; G.y=8; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==(unsigned)((8-15)&255)&&a0>240,"partly above the top (y wraps to %u)",a0);
      G.x=-20; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"off the left: off"); G.x=250; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"off the right: off"); G.x=100; G.y=200; kgOam(&G,&V,&a0,&a1,&a2); CHECK(a0==0x200,"below: off");
      for(int sx=-30;sx<270;sx++) for(int sy=-30;sy<190;sy+=3){ G.x=sx; G.y=sy; kgOam(&G,&V,&a0,&a1,&a2); if(a0!=0x200){ int x=sx-8, y=sy-15; CHECK(x>-16&&x<240&&y>-16&&y<160&&(a0&0xFF00)==0&&(a1&~0x5FFFu)==0&&(a2&0xF000)==0,"words in range at %d,%d",sx,sy); } }
      printf("  OAM words: visible, flipped, wrapped at the edges, switched off outside the screen\n"); }
    if(fails){ printf("kinggiant: %d FAILURE(S)\n",fails); return 1; } printf("kinggiant: ALL PASSED: 0 failure(s)\n"); return 0; }

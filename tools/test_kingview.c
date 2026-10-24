// test_kingview.c - PC test of source/kingview.h: the hardware tile background. A 32 x 32 screenblock is simulated; the camera is moved at random (and by jumps, to the corners and
// the edges); after every move the 31 x 21 tiles the hardware would show must be exactly the tiles of the world cells under them. Then the realm is edited while it is shown and
// must catch up within 21 frames. Run from the repo root:   sh tools/test_kingview.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
static u16 sb[1024]; static long puts_=0;
#define KV_PUT(sx,sy,e) do{ if((sx)<0||(sx)>31||(sy)<0||(sy)>31){ printf("FAIL: put outside the screenblock %d %d\n",(sx),(sy)); exit(1); } sb[(sy)*32+(sx)]=(u16)(e); puts_++; }while(0)
#include "../source/kingview.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 rs=12345; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static u16 expect(const u8*cells,int d,const KBld*B,int nB,int x,int y){   // written out again, differently: by asking each building which of its four cells this is
    if(x<0||y<0||x>=d||y>=d) return 7;
    int v=cells[y*d+x]; if(!(v&KC_BLD)) return (u16)(v&7);
    for(int i=0;i<nB;i++) for(int q=0;q<4;q++) if(B[i].x+(q&1)==x&&B[i].y+(q>>1)==y) return (u16)((B[i].lvl?8+q:12+q)|(B[i].lvl?(1+B[i].type)<<12:0));
    return 3; }
static int shown(const KView*V,const KBld*B){   // how many of the tiles the screen shows are wrong
    int bad=0; int nx=V->x>>3, ny=V->y>>3;
    for(int j=0;j<KV_H;j++) for(int i=0;i<KV_W;i++){ int wx=nx+i, wy=ny+j; if(sb[((wy)&31)*32+((wx)&31)]!=expect(V->cells,V->dim,B,V->nB,wx,wy)) bad++; }
    return bad; }
int main(void){
    printf("== kingview: the hardware tile background\n");
    for(int tier=0;tier<3;tier++){
        int d=kgDim(tier); static u8 cells[128*128]; static KBld B[KS_MAXB]; KRealm R; memset(&R,0,sizeof R); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=900u+(u32)tier; R.rng=5; kgGen(cells,R.seed,tier);
        int n=0; for(int y=6;y<d-6&&n<KS_MAXB;y+=9) for(int x=6;x<d-6&&n<KS_MAXB;x+=9){ B[n].type=(u8)(n%KS_NTYPES); B[n].x=(u8)x; B[n].y=(u8)y; B[n].lvl=(u8)(n&1); B[n].work=0; B[n].stock=0; n++; }
        kgStamp(cells,tier,B,n);
        KView V; memset(sb,0xEE,sizeof sb); kvInit(&V,cells,d,B,n);
        V.x=V.y=0; kvClamp(&V); kvSync(&V); CHECK(shown(&V,B)==0,"tier %d first draw wrong",tier);
        long moves=0; int worstPuts=0;
        for(int k=0;k<20000;k++){
            long before=puts_; int r=(int)(rnd()%100);
            if(r<3){ V.x=(int)(rnd()%(u32)(d*8)); V.y=(int)(rnd()%(u32)(d*8)); }                               // a jump anywhere
            else if(r<5){ V.x=(rnd()&1)?0:kvMaxX(&V); V.y=(rnd()&1)?0:kvMaxY(&V); }                          // a corner
            else { V.x+=(int)(rnd()%19)-9; V.y+=(int)(rnd()%19)-9; }                                         // up to 9 pixels a frame, every way
            kvClamp(&V); kvSync(&V); moves++;
            if(r>=5){ int p=(int)(puts_-before); if(p>worstPuts) worstPuts=p; }
            int bad=shown(&V,B); CHECK(bad==0,"tier %d move %d: %d tiles wrong at camera %d,%d",tier,k,bad,V.x,V.y); if(bad) break;
        }
        CHECK(worstPuts<=2*(KV_W+KV_H),"a normal move wrote %d tiles (9 pixels can cross two tile edges: 2 columns and 2 rows, 104 at most)",worstPuts);
        // edits while shown: fell trees, lay roads, finish a building; one row refreshes per frame
        for(int e=0;e<400;e++){ int x=(V.x>>3)+(int)(rnd()%KV_W), y=(V.y>>3)+(int)(rnd()%KV_H); if(x<0||y<0||x>=d||y>=d||(cells[y*d+x]&KC_BLD)) continue; cells[y*d+x]=(u8)(rnd()%KT_N); }
        for(int i=0;i<n;i++) B[i].lvl=1;
        for(int f=0;f<KV_H;f++) kvRefreshRow(&V);
        CHECK(shown(&V,B)==0,"tier %d: edits did not show within %d frames",tier,KV_H);
        { int x=(V.x>>3)+3, y=(V.y>>3)+3; if(!(cells[y*d+x]&KC_BLD)){ cells[y*d+x]=KT_ROAD; kvPoke(&V,x,y); CHECK(sb[(y&31)*32+(x&31)]==kvEntry(&V,x,y)&&(sb[(y&31)*32+(x&31)]&0xFFF)==KT_ROAD,"kvPoke did not show"); } }
        printf("  tier %d (%dx%d): %ld camera moves, every visible tile right after each; worst normal move wrote %d tiles\n",tier,d,d,moves,worstPuts);
    }
    // the art: 16 tiles of 32 bytes, every nibble a palette index, the quarters of a building differ
    { u8 g[KV_TILES][32]; for(int t=0;t<KV_TILES;t++) kvTileGfx(t,g[t]); int diff=0; for(int a=0;a<KV_TILES;a++) for(int b=a+1;b<KV_TILES;b++) if(!memcmp(g[a],g[b],32)) diff++;
      CHECK(diff==0,"%d pairs of tiles are identical",diff); CHECK(kvPalAt(0,14)!=kvPalAt(1,14)&&kvPalAt(1,14)!=kvPalAt(2,14),"building banks not recoloured"); CHECK(kvPalAt(3,5)==kvPal[5],"banks change more than the roof");
      printf("  art: %d tiles, %d bytes of tile data (a charblock is 16384), %d palette banks\n",KV_TILES,KV_TILES*32,KS_NTYPES+1); }
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}

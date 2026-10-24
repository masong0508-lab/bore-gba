// test_kingover.c - PC test of source/kingover.h (step 4i): every pixel of the overview in all three sizes. Run: sh tools/test_kingover.sh
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kingover.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128]; static u16 fb[240*160];
int main(void){ printf("== kingover\n");
    for(int tier=0;tier<3;tier++){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=77u+(u32)tier; R.rng=5; kgGen(cells,R.seed,tier); CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init");
        int d=S.dim, c=d/2; for(int y=c-3;y<c+5;y++) for(int x=c-3;x<c+5;x++) S.cells[y*d+x]=KT_GRASS;
        ksPlace(&S,KB_STORE,c-1,c-1,1); ksPlace(&S,KB_HUT,c+2,c-1,0); ksRoadSet(&S,c+1,c+1,1);
        int sc=koScale(d); CHECK(koW(d)<=160&&koX0(d)>=0&&koY0(d)>=0&&(tier==0?sc==3:sc==1),"layout tier %d scale %d",tier,sc);
        for(int i=0;i<240*160;i++) fb[i]=0x1234; koDraw(fb,&S,c,c+3,1); int bad=0, outside=0;
        for(int y=0;y<160;y++) for(int x=0;x<240;x++){ u16 p=fb[y*240+x]; int cx=x-koX0(d), cy=y-koY0(d);
            if(cx<0||cy<0||cx>=koW(d)||cy>=koW(d)){ outside+=p!=0x1234; continue; }
            int gx=cx/sc, gy=cy/sc; u16 want=(gx==c&&gy==c+3)?0x7FFF:koCell(&S,gx,gy); bad+=p!=want; }
        CHECK(bad==0&&outside==0,"tier %d: %d wrong pixels, %d outside the map",tier,bad,outside);
        CHECK(koCell(&S,c-1,c-1)==koRoof[KB_STORE]&&koCell(&S,c+1,c+1)==koLand[KT_ROAD]&&koCell(&S,c+2,c-1)!=koRoof[KB_HUT],"store roof, road, a site is dimmer than a finished hut");
        koDraw(fb,&S,c,c+3,0); CHECK(fb[(koY0(d)+(c+3)*sc)*240+koX0(d)+c*sc]==koCell(&S,c,c+3),"no blink, no dot");
        printf("  tier %d: %d x %d pixels, scale %d, every pixel checked\n",tier,koW(d),koW(d),sc); }
    if(fails){ printf("kingover: %d FAILURE(S)\n",fails); return 1; } printf("kingover: ALL PASSED: 0 failure(s)\n"); return 0; }

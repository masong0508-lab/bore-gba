// test_kingcap.c - PC test of step 5f: ENDLESS PLAY. A realm must behave in year 500 as in year 5.
// 1 a soak: a whole village (every chain, the castle, crowned) run for 24,000 days in the 96 and the 128 land, checked every 2,000 days: the books, no carrier stuck, a save that stays small
//   and loads back the same, and the stockpile ceiling (KG_GOODMAX) reached and held with the books still balanced;
// 2 the counters at their limits: the day count at the top of its u32, the frame clock across its wrap, the run counters at their ceiling, the match rotation, the map stamps;
// 3 the screen's day. From the repo root:   sh tools/test_kingcap.sh
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
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<25){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S, S2; static KRealm R, R2; static KBld B[KS_MAXB], B2[KS_MAXB]; static u8 cells[128*128], c2[128*128]; static u8 buf[60000];
static void village(int tier,u32 seed){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; int d=kgDim(tier); kgGen(cells,seed,tier); int c=d/2;
    for(int y=c-12;y<=c+14;y++) for(int x=c-12;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    for(int y=c+5;y<=c+9;y++) for(int x=c-12;x<=c-5;x++) cells[y*d+x]=KT_FOREST; for(int y=c+6;y<=c+10;y++) for(int x=c-4;x<=c+6;x++) cells[y*d+x]=KT_ROCK;
    for(int y=c-6;y<=c-3;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_MEADOW; for(int y=c+5;y<=c+7;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_CRYSTAL;
    CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init"); for(int x=c-8;x<=c+27;x++) ksRoadSet(&S,x,c+1,1);
    ksPlace(&S,KB_STORE,c-8,c-1,1); for(int k=0;k<5;k++) ksPlace(&S,KB_HUT,c-5+3*k,c-1,1);
    ksPlace(&S,KB_WOODCUT,c-8,c+2,0); ksPlace(&S,KB_SAWMILL,c-5,c+2,0); ksPlace(&S,KB_QUARRY,c-2,c+2,0); ksPlace(&S,KB_MASON,c+1,c+2,0);
    ksPlace(&S,KB_SHEEP,c+20,c-1,0); ksPlace(&S,KB_WEAVER,c+10,c+2,0); ksPlace(&S,KB_CRYMINE,c+20,c+2,0); ksPlace(&S,KB_CUTTER,c+14,c+2,0); ksPlace(&S,KB_CASTLE,c+14,c-1,0);
    ksGift(&S,G_WOOD,30); ksGift(&S,G_PLANKS,10); ksGift(&S,G_CUT,4);
}
static int stuck(void){ int n=0; for(int w=0;w<KS_MAXW;w++) if(S.wk[w].st&&S.wk[w].stall>=KS_TIMEOUT) n++; return n; }
// save the realm, load it into the second simulation, and say whether it came back the same
static int roundTrip(void){
    ksPrepSave(&S); int len=kgEncode(buf,sizeof buf,&R,B,cells); if(len<=0) return -1;
    if(kgDecode(buf,len,&R2,B2,0,0)!=0||kgDecode(buf,len,&R2,B2,c2,1)!=0) return -2;
    if(R2.day!=R.day||R2.castle!=R.castle||R2.nB!=R.nB||R2.seed!=R.seed||memcmp(R2.goods,R.goods,sizeof R.goods)) return -3;
    int d=kgDim(R.tier); if(memcmp(c2,cells,(size_t)d*d)) return -4;
    if(ksInit(&S2,&R2,B2,c2)!=KSE_OK||ksCheck(&S2)!=0) return -5;
    ksInit(&S,&R,B,cells); return len;   // (the realm goes on from what it saved)
}
int main(void){
    printf("== kingcap: endless play\n");
    // ---- 1 the soak ----
    for(int tier=1;tier<=2;tier++){
        village(tier,40u+(u32)tier); int worst=0, ceilingDay=-1; const int days=24000; unsigned long long total=0;
        for(int day=0;day<days;day++){
            for(int f=0;f<KS_DAYT;f++){ ksStep(&S); S.ev=0; }
            if(ceilingDay<0&&R.goods[G_PLANKS]>=KG_GOODMAX) ceilingDay=day;
            if(day%2000==1999){ int e=ksCheck(&S); CHECK(e==0,"tier %d day %d: the books (%d)",tier,day,e); CHECK(stuck()==0,"tier %d day %d: a carrier is stuck",tier,day);
                total+=S.delivered; int len=roundTrip(); CHECK(len>0,"tier %d day %d: save and load (%d)",tier,day,len); if(len>worst) worst=len;
                for(int g=0;g<KG_GOODS;g++) CHECK(R.goods[g]<=KG_GOODMAX,"good %d above the ceiling",g); }
            if(fails>20) break; }
        CHECK(R.castle==KG_CASTLE,"tier %d: crowned",tier); CHECK(ceilingDay>=0&&R.goods[G_PLANKS]==KG_GOODMAX,"tier %d: the planks reached the ceiling and held (%d, day %d)",tier,R.goods[G_PLANKS],ceilingDay);
        CHECK(total>(unsigned long long)days*2&&S.pop>KS_PARTY&&S.timeouts==0&&S.aborts==0,"tier %d: kept working (delivered %llu, pop %d, timeouts %u, aborts %u)",tier,total,S.pop,S.timeouts,S.aborts);
        printf("  tier %d: %d days, crowned, planks at the ceiling from day %d, books balanced, nothing stuck, largest save %d bytes, %llu loads delivered\n",tier,days,ceilingDay,worst,total); }
    // ---- 2 the counters at their limits ----
    // 2a the day count: the top of the u32 is a ceiling, it saves and loads, and days still run
    village(1,52u); R.day=0xFFFFFFF0u; for(long k=0;k<KS_DAYT*40L;k++){ ksStep(&S); S.ev=0; }
    CHECK(R.day==0xFFFFFFFFu,"the day count stops at the top (%u)",(unsigned)R.day); CHECK(ksCheck(&S)==0&&S.delivered>0,"still working at the last day");
    { int len=roundTrip(); CHECK(len>0&&R2.day==0xFFFFFFFFu,"the last day saves and loads (%d)",len); }
    for(long k=0;k<KS_DAYT*3L;k++){ ksStep(&S); S.ev=0; } CHECK(R.day==0xFFFFFFFFu&&ksCheck(&S)==0,"and stays there");
    // 2b the frame clock: it wraps at a whole number of days, a day never comes early or late, the clock does not stand still
    CHECK(KS_TMAX%KS_DAYT==0&&KS_TMAX%KS_WALK==0&&(unsigned long long)KS_TMAX<=0xFFFFFFFFull&&KS_TMAX>0xFFFF0000u,"KS_TMAX is whole days (%u)",(unsigned)KS_TMAX);
    village(1,53u); { u32 day0=R.day; S.t=KS_TMAX-3*KS_DAYT-50; long frames=KS_DAYT*8L; u32 wrapped=0, prev=S.t; for(long k=0;k<frames;k++){ ksStep(&S); S.ev=0; if(S.t<prev) wrapped++; prev=S.t; }
      CHECK(wrapped==1,"the frame clock wrapped once (%u)",wrapped); CHECK(S.t<KS_TMAX,"clock %u inside its range",S.t);
      CHECK(R.day-day0==8||R.day-day0==9,"8 days of frames across the wrap gave %u days",R.day-day0); CHECK(ksCheck(&S)==0&&stuck()==0&&S.delivered>0,"books, carriers and work across the wrap"); }
    // the same village, the same frames, with and without the wrap in the middle: every day counted, and the days land on whole days
    village(1,54u); S.t=KS_TMAX-KS_DAYT*2; { int ticks=0; u32 d0=R.day; for(long k=0;k<KS_DAYT*5L;k++){ u32 b=R.day; ksStep(&S); S.ev=0; ticks+=R.day!=b; } CHECK(ticks==5&&R.day-d0==5,"5 days across the wrap, each a whole day (%d ticks)",ticks); }
    // 2c ksAge across the wrap
    village(1,55u); { S.t=10; CHECK(ksAge(&S,4)==6&&ksAge(&S,10)==0,"age without a wrap"); CHECK(ksAge(&S,KS_TMAX-5)==15,"age across the wrap (%u)",ksAge(&S,KS_TMAX-5)); S.t=0; CHECK(ksAge(&S,KS_TMAX-1)==1,"one frame across"); }
    // 2d the run counters stop at their ceiling instead of wrapping to 0
    village(1,56u); S.delivered=0xFFFFFFF0u; S.timeouts=0xFFFFFFFFu; S.aborts=0xFFFFFFFEu; for(long k=0;k<KS_DAYT*20L;k++){ ksStep(&S); S.ev=0; }
    CHECK(S.delivered==0xFFFFFFFFu,"deliveries stop at the top (%u)",S.delivered); CHECK(S.timeouts==0xFFFFFFFFu,"timeouts hold (%u)",S.timeouts); CHECK(S.aborts>=0xFFFFFFFEu,"aborts do not wrap (%u)",S.aborts); CHECK(ksCheck(&S)==0,"books with the counters at the top");
    // 2e the match rotation near its reset: it still visits every building
    village(1,57u); S.mrr=(1u<<30)-3; { long k=0; for(;k<KS_DAYT*30L;k++){ ksStep(&S); S.ev=0; } CHECK(S.mrr<(1u<<30)+100000u&&ksCheck(&S)==0&&S.delivered>0,"the rotation reset (%u)",S.mrr); }
    // 2f map stamps at the wrap: a village run across it builds every building, the maps are handed out and the carriers are never stuck
    village(1,58u); S.t=KS_TMAX-4000; { for(long k=0;k<KS_DAYT*40L;k++){ ksStep(&S); S.ev=0; } int built=0; for(int i=0;i<R.nB;i++) built+=B[i].lvl; CHECK(built==R.nB&&stuck()==0&&ksCheck(&S)==0,"%d of %d built across the wrap, no stuck carrier",built,R.nB); }
    // 2g the pile at its ceiling takes no more, and gifts are not lost from the books
    village(1,59u); R.goods[G_WOOD]=KG_GOODMAX-2; S.made[G_WOOD]=ksHeld(&S,G_WOOD); ksGift(&S,G_WOOD,50); CHECK(R.goods[G_WOOD]==KG_GOODMAX&&ksCheck(&S)==0,"a gift above the ceiling is cut, the books know (%d)",R.goods[G_WOOD]);
    // ---- 3 the screen's day ----
    CHECK(khDay(0)==0&&khDay(12)==12&&khDay(99999)==99999&&khDay(100000)==99999&&khDay(0x80000000u)==99999&&khDay(0xFFFFFFFFu)==99999,"the day on the screen is capped and never negative");
    { char l[64]; khStatus(l,0,khDay(0xFFFFFFFFu),7,0); CHECK(strstr(l,"DAY 99999")&&!strchr(l,'-'),"status line at the last day: %s",l); }
    printf("kingcap: %s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}

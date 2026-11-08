// test_kingamb.c - PC test of source/kingamb.h: the seasons, the weather and wildlife sprites, the caravan. Checks: the year turns every 8 days (spring first) and the colours blend
// from one season to the next over the last two days; the season's colours are legal 15 bit colours and the browns and the dark stay put in autumn; the ambient plan never writes a
// malformed sprite or goes past 124 entries or onto the text strip, for every season and every part of the day; the art is 11 + 4 tiles of legal digits; the caravan is here on days
// 2 to 5 of every 8 and only with a finished storehouse; the wanted good turns through all eight; a sale takes the goods out of the stockpile and out of the books together (ksLedger
// stays 0), pays the table's price (half as much again for the wanted good); winter slows a producer and the well and tavern take the loss away; with seasons off nothing changes.
// Run from the repo root:   sh tools/test_kingamb.sh
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
#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
#include "../source/kingamb.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int fresh(void){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=0; R.genVer=KG_GENVER; R.seed=5; R.rng=5; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,5,0);
    int d=kgDim(0), c=d/2; for(int y=c-8;y<c+10;y++) for(int x=c-8;x<c+10;x++) cells[y*d+x]=KT_GRASS; return ksInit(&S,&R,B,cells); }
int main(void){
    CHECK(fresh()==KSE_OK,"init");
    // the year
    for(int day=0;day<70;day++){ R.day=(u32)day; S.t=0; int sn,nx,k; kaSeason(&S,&sn,&nx,&k); CHECK(sn==((day/8)&3)&&nx==((sn+1)&3),"season of day %d is %d",day,sn); CHECK((day&7)>5||k==0,"no turn at the start of day %d",day); }
    { R.day=7; int last=-1; for(int t=0;t<KS_DAYT;t+=30){ S.t=(u32)t; int sn,nx,k; kaSeason(&S,&sn,&nx,&k); CHECK(k>=last&&k<=16,"the turn only goes forward (%d after %d)",k,last); last=k; } CHECK(last>=15,"the last day of a season has all but turned (%d)",last); R.day=6; S.t=0; int sn,nx,k; kaSeason(&S,&sn,&nx,&k); CHECK(k==0,"two days before the end it has not begun"); }
    // the colours
    for(int sn=0;sn<4;sn++) for(int c=0;c<0x8000;c+=7){ u16 o=kaSeasonC((u16)c,sn); CHECK(!(o&0x8000),"colour %04x season %d leaves bit 15",c,sn); }
    CHECK(kaSeasonC(RGB(15,9,5),KS_AUTUMN)==RGB(15,9,5),"a brown stays brown in autumn");
    CHECK(kaSeasonC(RGB(8,20,7),KS_SUMMER)==RGB(8,20,7),"summer is the art as drawn");
    { u16 g=RGB(8,20,7); u16 a=kaSeasonC(g,KS_AUTUMN); CHECK((a&31)>(g&31)&&((a>>5)&31)<=20,"autumn turns a green warm"); u16 w=kaSeasonC(g,KS_WINTER); CHECK((w&31)>(g&31)&&((w>>10)&31)>((g>>10)&31),"winter whitens it"); }
    CHECK(kaSeasonMix(RGB(8,20,7),KS_AUTUMN,KS_WINTER,0)==kaSeasonC(RGB(8,20,7),KS_AUTUMN),"mix 0 is the season");
    CHECK(kaSeasonMix(RGB(8,20,7),KS_AUTUMN,KS_WINTER,16)==kaSeasonC(RGB(8,20,7),KS_WINTER),"mix 16 is the next season");
    // the art
    for(int t=0;t<KA_AMB_N;t++){ u16 w[16]; kaAmbTile(t,w); for(int r=0;r<8;r++){ CHECK(strlen(kaAmbArt[t][r])==8,"tile %d row %d is 8 wide",t,r); } }
    // the plan, every season, every part of the day, many ticks
    { for(int day=0;day<32;day+=8) for(int part=0;part<5;part++) for(unsigned tick=0;tick<3000;tick+=17){ R.day=(u32)day+3; S.t=(u32)(part*KS_DAYT/5+10); u16 o[130*3]; int n=kaAmbient(o,0,&S,tick);
            CHECK(n<=124,"too many ambient sprites (%d)",n);
            for(int i=0;i<n;i++){ unsigned a0=o[i*3], a1=o[i*3+1], a2=o[i*3+2]; int y=a0&255; if(y>=192) y-=256; int tile=a2&1023, bank=a2>>12, prio=(a2>>10)&3;
                CHECK(!(a0&0x3F00)&&((a1&0x0E00)==0),"sprite %d malformed",i); CHECK(tile>=KA_AMB&&tile<KA_AMB+KA_AMB_N&&bank==KA_AMBPAL&&prio==0,"sprite %d tile %d bank %d prio %d",i,tile,bank,prio); CHECK(y+8<=144&&y>=-8,"sprite %d over the text strip (y %d)",i,y); } } }
    // the caravan
    CHECK(fresh()==KSE_OK,"init2"); R.day=3; CHECK(!kaCaravanHere(&S),"no storehouse, no caravan");
    ksPlace(&S,KB_STORE,R.tier?60:22,22,1); ksSeasonsOn=1;
    for(int d=0;d<40;d++){ R.day=(u32)d; int want=(d&7)>=2&&(d&7)<=5; CHECK(kaCaravanHere(&S)==want,"caravan on day %d",d); }
    { int seen=0; for(int v=0;v<8;v++){ R.day=(u32)(v*8+3); seen|=1<<kaWanted(&S); } CHECK(seen==255,"every good is wanted in turn (%02x)",seen); }
    { R.day=3; int wx,wy; CHECK(kaWagonCell(&S,&wx,&wy),"a wagon cell"); CHECK(kaNearWagon(&S,wx,wy)&&kaNearWagon(&S,wx+2,wy-2)&&!kaNearWagon(&S,wx+3,wy),"near the wagon is within two cells"); R.day=0; CHECK(!kaNearWagon(&S,wx,wy),"nobody trades with a wagon that is not there"); }
    { R.day=3; int w=kaWanted(&S), o=(w+1)&7; CHECK(kaPay(&S,o,4)==kaPrice[o]*4,"plain price"); CHECK(kaPay(&S,w,4)==kaPrice[w]*4*3/2,"the wanted good pays half as much again"); }
    // the books: a sale leaves nothing unbalanced
    { R.day=3; ksGift(&S,G_WOOD,30); CHECK(ksLedger(&S)==0,"books at the start"); int g=G_WOOD; int have=R.goods[g]; int n=have>10?10:have; R.goods[g]=(u16)(R.goods[g]-n); S.made[g]-=n; CHECK(ksLedger(&S)==0,"books after a sale (%d)",ksLedger(&S)); CHECK(R.goods[g]==have-n,"the stockpile fell by what was sold"); }
    // winter and the work
    { fresh(); ksPlace(&S,KB_STORE,22,22,1); int i=ksPlace(&S,KB_WOODCUT,26,22,1); CHECK(i>=0,"woodcutter"); ksSeasonsOn=0; R.day=24; unsigned base=ksPeriod(&S,i); ksSeasonsOn=1;
        R.day=24; unsigned win=ksPeriod(&S,i); CHECK(win>base,"winter slows a producer (%u vs %u)",win,base); S.cmf[i]=1; unsigned w1=ksPeriod(&S,i); ksSeasonsOn=0; unsigned b1=ksPeriod(&S,i); ksSeasonsOn=1; CHECK(w1>b1&&w1-b1<win-base,"a well takes some of the loss away (%u vs %u)",w1,b1);
        S.cmf[i]=2; unsigned w2=ksPeriod(&S,i); ksSeasonsOn=0; unsigned b2=ksPeriod(&S,i); ksSeasonsOn=1; CHECK(w2==b2,"a well and a tavern take all of it away (%u vs %u)",w2,b2);
        S.cmf[i]=0; R.day=16; unsigned au=ksPeriod(&S,i); CHECK(au<base,"the autumn harvest is faster (%u vs %u)",au,base); R.day=8; CHECK(ksPeriod(&S,i)==base,"summer is the table's"); ksSeasonsOn=0; R.day=24; CHECK(ksPeriod(&S,i)==base,"seasons off: winter changes nothing"); }
    if(fails){ printf("test_kingamb: %d FAILED\n",fails); return 1; }
    printf("test_kingamb: all ok\n"); return 0;
}

// test_kingdom.c - checks source/kingdom.h on a PC (no cart, no emulator). Run from the repo root:
//   gcc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -o /tmp/tk tools/test_kingdom.c && /tmp/tk
// It checks: the made land never changes (fingerprints), save then load gives the same realm, the same bytes every time, a damaged or cut save is
// refused and never reads or writes outside its buffers (even when the checksum is made to match), and the size of realms against the slots.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"

static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } }while(0)
static u32 rs=12345; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static u32 fnv(const u8*p,int n){ u32 h=2166136261u; for(int i=0;i<n;i++){ h^=p[i]; h*=16777619u; } return h; }

// the made land: fingerprints (FNV-1a of the cells) for three seeds in each of the three sizes. If one moves, saves would load a different land: make genVer 2 instead.
static const u32 SEEDS[3]={1u,12345u,0xC0FFEEu};
static const u32 GOLDEN[3][3]={ {0x99A588BAu,0xB40BB668u,0x65C25706u},{0x1C3B9BBAu,0x752D99E9u,0xFF862184u},{0x56C299B1u,0x078D4DEDu,0xCF9412A5u} };

static u8 cells[128*128], cells2[128*128];
static KRealm R, R2; static KBld B[KG_MAXB], B2[KG_MAXB];

static void scenario(int tier,int bld,int roads,int cleared){   // a realm as a player might have built it
    int d=kgDim(tier); memset(&R,0,sizeof R); R.tier=(u8)tier; R.giant=(u8)(rnd()%KG_GIANTS); R.castle=(u8)(rnd()%5); R.genVer=KG_GENVER; R.seed=rnd(); R.day=rnd()%100000; R.rng=rnd();
    for(int i=0;i<KG_GOODS;i++) R.goods[i]=(u16)(rnd()%2000);
    kgGen(cells,R.seed,tier);
    for(int k=0;k<cleared;k+=12){ int cx=(int)(rnd()%d), cy=(int)(rnd()%d); for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i++){ int x=cx+i,y=cy+j; if(x<0||y<0||x>=d||y>=d) continue; int t=cells[y*d+x]&7; if(t==KT_FOREST||t==KT_ROCK||t==KT_CRYSTAL||t==KT_MEADOW) cells[y*d+x]=KT_GRASS; } }
    for(int k=0;k<roads;){ int horiz=rnd()&1, a=(int)(rnd()%d), b=(int)(rnd()%d), len=5+(int)(rnd()%25); for(int i=0;i<len&&k<roads;i++,k++){ int x=horiz?(b+i)%d:a, y=horiz?a:(b+i)%d; if((cells[y*d+x]&7)!=KT_WATER) cells[y*d+x]=KT_ROAD; } }
    R.nB=(u16)bld; for(int i=0;i<bld;i++){ B[i].type=(u8)(rnd()%KG_BTYPES); B[i].x=(u8)(rnd()%(d-2)); B[i].y=(u8)(rnd()%(d-2)); B[i].lvl=(u8)(rnd()%4); B[i].work=(u8)(rnd()%10); B[i].stock=(u16)(rnd()%500); }
    kgStamp(cells,tier,B,bld);
}
static int same(void){   // R,B,cells == R2,B2,cells2 (field by field: the structs have padding bytes nobody sets)
    if(R.tier!=R2.tier||R.giant!=R2.giant||R.castle!=R2.castle||R.genVer!=R2.genVer||R.flags!=R2.flags||R.seed!=R2.seed||R.day!=R2.day||R.rng!=R2.rng||R.nB!=R2.nB) return 0;
    for(int i=0;i<KG_GOODS;i++) if(R.goods[i]!=R2.goods[i]) return 0;
    for(int i=0;i<R.nB;i++) if(B[i].type!=B2[i].type||B[i].x!=B2[i].x||B[i].y!=B2[i].y||B[i].lvl!=B2[i].lvl||B[i].work!=B2[i].work||B[i].stock!=B2[i].stock) return 0;
    return memcmp(cells,cells2,kgCells(R.tier))==0;
}

int main(void){
    // 1 the made land
    for(int t=0;t<3;t++) for(int s=0;s<3;s++){ kgGen(cells,SEEDS[s],t); u32 h=fnv(cells,kgCells(t));
        if(GOLDEN[t][s]==0) printf("GOLDEN[%d][%d]=0x%08Xu\n",t,s,h); else CHECK(h==GOLDEN[t][s],"the made land moved: tier %d seed %u gives %08X, expected %08X",t,SEEDS[s],h,GOLDEN[t][s]); }
    for(int t=0;t<3;t++){ int d=kgDim(t), c=d/2; kgGen(cells,777,t); for(int y=c-3;y<=c+3;y++)for(int x=c-3;x<=c+3;x++) CHECK(cells[y*d+x]==KT_GRASS,"start field not clear"); }
    // 2 round trips and the same bytes every time, in a few sizes of realm
    static u8 buf[20000], buf2[20000];
    struct { int tier,bld,roads,cleared; const char*nm; } SC[]={ {0,6,60,100,"hamlet, early"},{0,40,300,700,"hamlet, busy"},{1,10,80,150,"realm, early"},{1,60,500,800,"realm, mid"},{1,200,1500,3000,"realm, late"},{2,30,300,500,"kingdom, early"},{2,120,1200,2500,"kingdom, mid"},{2,250,3000,6000,"kingdom, late"} };
    printf("%-16s %6s  slots (a save spans 32 B + 4 B of chunk + the bytes, in 2 KB slots)\n","realm","bytes");
    for(unsigned k=0;k<sizeof SC/sizeof SC[0];k++){
        for(int rep=0;rep<30;rep++){
            scenario(SC[k].tier,SC[k].bld,SC[k].roads,SC[k].cleared);
            int n=kgEncode(buf,16300,&R,B,cells);
            if(n<0){ if(rep==0) printf("%-16s  too big for 16300: error %d\n",SC[k].nm,n); continue; }
            int n2=kgEncode(buf2,16300,&R,B,cells); CHECK(n==n2&&!memcmp(buf,buf2,n),"encoding twice gave different bytes");
            CHECK(kgDecode(buf,n,&R2,B2,0,0)==0,"check failed");
            memset(cells2,0xEE,sizeof cells2); CHECK(kgDecode(buf,n,&R2,B2,cells2,1)==0,"load failed");
            CHECK(same(),"%s: the loaded realm is not the saved one",SC[k].nm);
            if(rep==0) printf("%-16s %6d  %d\n",SC[k].nm,n,(32+4+n+2047)/2048);
        }
    }
    // 3 damage: every cut, every flipped byte (checksum left alone), and flips with the checksum made to match
    scenario(1,60,500,800); int n=kgEncode(buf,16300,&R,B,cells); CHECK(n>0,"encode");
    for(int l=0;l<n;l++) CHECK(kgDecode(buf,l,&R2,B2,0,0)!=0,"a save cut to %d bytes was accepted",l);
    int acc=0; for(int i=0;i<n;i++){ memcpy(buf2,buf,n); buf2[i]^=(u8)(1+rnd()%255); if(kgDecode(buf2,n,&R2,B2,0,0)==0) acc++; }
    CHECK(acc<=n/50,"%d of %d single-byte flips were accepted",acc,n); printf("single-byte flips accepted: %d of %d\n",acc,n);
    int okc=0, bad=0;
    for(int it=0;it<60000;it++){
        memcpy(buf2,buf,n); int hits=1+(int)(rnd()%3); for(int h=0;h<hits;h++) buf2[rnd()%(n-2)]=(u8)rnd();
        int s=kgSum16(buf2,n-2); buf2[n-2]=(u8)s; buf2[n-1]=(u8)(s>>8);   // (the slot's checksum passes: the checks in kgDecode are the only defence)
        int e=kgDecode(buf2,n,&R2,B2,0,0);
        if(e==0){ okc++; memset(cells2,0,sizeof cells2); CHECK(kgDecode(buf2,n,&R2,B2,cells2,1)==0,"checked ok but failed to load");
            int d=kgDim(R2.tier); for(int i=0;i<R2.nB;i++) CHECK(B2[i].x<d&&B2[i].y<d&&B2[i].type<KG_BTYPES,"loaded a building out of range");
            for(int i=0;i<d*d;i++) CHECK((cells2[i]&7)<KT_N,"loaded a bad terrain value");
            for(int g=0;g<KG_GOODS;g++) CHECK(R2.goods[g]<=KG_GOODMAX,"goods over the ceiling"); } else bad++;
    }
    printf("flips with a matching checksum: %d refused, %d still valid saves (a changed number is a valid save)\n",bad,okc);
    // 4 a small buffer: refused, and never written past
    { u8 g[600]; memset(g,0x5A,sizeof g); int e=kgEncode(g,200,&R,B,cells); CHECK(e==KGE_BIG,"a too small buffer gave %d",e); for(int i=200;i<600;i++) CHECK(g[i]==0x5A,"wrote past the end at %d",i); }
    // 5 the worst realm: every cell different. It must say too big (or fit), never overrun.
    { kgGen(cells,99,2); for(int i=0;i<128*128;i++) cells[i]=(u8)((kgCell(99,2,i%128,i/128)+1+(i%5))%KT_N); memset(&R,0,sizeof R); R.tier=2; R.genVer=KG_GENVER; R.seed=99; R.nB=0;
      static u8 big[40000]; memset(big,0x77,sizeof big); int e=kgEncode(big,16300,&R,B,cells); CHECK(e==KGE_BIG,"a worst-case realm gave %d",e); for(int i=16300;i<40000;i++) CHECK(big[i]==0x77,"wrote past the cap"); printf("worst case (every cell edited): %s\n",e==KGE_BIG?"too big, refused":"fits"); }
    // 6 bad arguments
    { memset(&R,0,sizeof R); R.genVer=KG_GENVER; R.tier=3; CHECK(kgEncode(buf,16300,&R,B,cells)==KGE_ARG,"tier 3 accepted"); R.tier=0; R.giant=5; CHECK(kgEncode(buf,16300,&R,B,cells)==KGE_ARG,"giant 5 accepted"); }
    printf(fails?"\n%d CHECK(S) FAILED\n":"\nall checks passed\n",fails);
    return fails?1:0;
}

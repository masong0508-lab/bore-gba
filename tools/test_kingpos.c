// test_kingpos.c - PC test of step 4j: the giant's position saved with the realm (kingdom.h format 2: KRealm gx, gy; kinggiant.h: kgPlace).
// It checks: the position round trips through kgEncode / kgDecode in all three sizes; a format 1 save (made before 4j) still loads, with no position, and saves again as format 2;
// a damaged position is refused (even with the checksum made to match); kgPlace puts the giant where it was left or, if it cannot stand there, where a new game starts it;
// and walk -> save -> load -> place gives back the very same cell (through the real simulation start). From the repo root:   sh tools/test_kingpos.sh
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
static u32 rs=4242; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static u8 cells[128*128], c2[128*128]; static KRealm R, R2; static KBld B[KS_MAXB], B2[KS_MAXB]; static KSim S, S2;
static u8 buf[20000], old[20000], hos[20000];
static int mk(int tier,u32 seed,int nb){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=5; R.day=77; R.goods[0]=10; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,tier);
    int d=kgDim(tier); R.nB=(u16)nb; for(int i=0;i<nb;i++){ B[i].type=(u8)(rnd()%7); B[i].x=(u8)(2+rnd()%(unsigned)(d-6)); B[i].y=(u8)(2+rnd()%(unsigned)(d-6)); B[i].lvl=1; B[i].work=0; B[i].stock=0; }
    return 0; }
static void fix(u8*p,int n){ int s=kgSum16(p,n-2); p[n-2]=(u8)s; p[n-1]=(u8)(s>>8); }
static int load(const u8*p,int n){ memset(&R2,0,sizeof R2); memset(B2,0,sizeof B2); memset(c2,0xEE,sizeof c2); int e=kgDecode(p,n,&R2,B2,0,0); if(e) return e; return kgDecode(p,n,&R2,B2,c2,1); }
int main(void){
    printf("== kingpos: the giant's position in the save (format 2)\n");
    static const int dims[3]={48,96,128};
    for(int tier=0;tier<3;tier++){
        int d=dims[tier]; CHECK(kgDim(tier)==d,"dim");
        // 1 round trip: no position, corners of the walkable area, the middle, 200 random cells
        int tot=0;
        for(int k=0;k<205;k++){
            mk(tier,300u+(u32)tier*7u+(u32)k,k%9);
            if(k==0){ R.gx=KG_NOPOS; R.gy=KG_NOPOS; } else if(k==1){ R.gx=1; R.gy=1; } else if(k==2){ R.gx=(u8)(d-2); R.gy=(u8)(d-2); } else if(k==3){ R.gx=(u8)(d/2); R.gy=(u8)(d/2); } else if(k==4){ R.gx=(u8)(d-1); R.gy=0; } else { R.gx=(u8)(rnd()%(unsigned)d); R.gy=(u8)(rnd()%(unsigned)d); }
            int n=kgEncode(buf,sizeof buf,&R,B,cells); CHECK(n>0,"encode gave %d",n); if(n<=0) continue; tot++;
            CHECK(buf[0]==2,"a new save is format 2 (is %d)",buf[0]); CHECK(buf[36]==R.gx&&buf[37]==R.gy,"position bytes 36, 37");
            CHECK(load(buf,n)==0,"load refused (tier %d, k %d)",tier,k);
            CHECK(R2.gx==R.gx&&R2.gy==R.gy,"position came back (%d,%d) for (%d,%d)",R2.gx,R2.gy,R.gx,R.gy);
            CHECK(R2.tier==R.tier&&R2.seed==R.seed&&R2.day==R.day&&R2.rng==R.rng&&R2.nB==R.nB&&R2.goods[0]==R.goods[0],"the rest of the realm came back");
            for(int i=0;i<R.nB;i++) CHECK(B2[i].type==B[i].type&&B2[i].x==B[i].x&&B2[i].y==B[i].y,"building %d",i);
            // 2 a format 1 save (before step 4j): header bytes 0..35, then no position
            int n1=n-2; memcpy(old,buf,36); memcpy(old+36,buf+38,(size_t)(n-38)); old[0]=1; fix(old,n1);
            CHECK(load(old,n1)==0,"a format 1 save was refused (tier %d, k %d)",tier,k);
            CHECK(R2.gx==KG_NOPOS&&R2.gy==KG_NOPOS,"a format 1 save has no position (got %d,%d)",R2.gx,R2.gy);
            CHECK(R2.tier==R.tier&&R2.seed==R.seed&&R2.day==R.day&&R2.nB==R.nB,"format 1: the realm came back");
            for(int i=0;i<R.nB;i++) CHECK(B2[i].type==B[i].type&&B2[i].x==B[i].x&&B2[i].y==B[i].y,"format 1 building %d",i);
            int n2=kgEncode(hos,sizeof hos,&R2,B2,c2); CHECK(n2==n&&hos[0]==2&&hos[36]==KG_NOPOS&&hos[37]==KG_NOPOS,"a loaded format 1 realm saves as format 2 with no position (n2 %d, n %d)",n2,n);
            // 3 hostile: the checksum is made to match, only the position (or the format) is wrong
            if(k<40){
                memcpy(hos,buf,(size_t)n); hos[36]=(u8)d; fix(hos,n); CHECK(load(hos,n)==KGE_RANGE,"x = %d accepted",d);
                memcpy(hos,buf,(size_t)n); hos[37]=(u8)d; fix(hos,n); CHECK(load(hos,n)==KGE_RANGE,"y = %d accepted",d);
                memcpy(hos,buf,(size_t)n); hos[36]=254; fix(hos,n); CHECK(load(hos,n)==KGE_RANGE,"x = 254 accepted");
                memcpy(hos,buf,(size_t)n); hos[37]=200; fix(hos,n); CHECK(load(hos,n)==KGE_RANGE,"y = 200 accepted");
                memcpy(hos,buf,(size_t)n); hos[36]=KG_NOPOS; hos[37]=3; fix(hos,n); CHECK(load(hos,n)==0,"a half-set position (255, 3) is a harmless value, the giant is simply started as new");
                memcpy(hos,buf,(size_t)n); hos[0]=3; fix(hos,n); CHECK(load(hos,n)==KGE_FMT,"format 3 accepted");
                memcpy(hos,buf,(size_t)n); hos[0]=0; fix(hos,n); CHECK(load(hos,n)==KGE_FMT,"format 0 accepted");
                for(int l=0;l<n;l++) if(kgDecode(buf,l,&R2,B2,0,0)==0){ CHECK(0,"a save cut to %d of %d bytes was accepted",l,n); break; }
                for(int l=0;l<n1;l++) if(kgDecode(old,l,&R2,B2,0,0)==0){ CHECK(0,"a format 1 save cut to %d of %d bytes was accepted",l,n1); break; }
            }
        }
        // 4 encode refuses a position off the land
        mk(tier,9u,0); R.gx=(u8)d; R.gy=1; CHECK(kgEncode(buf,sizeof buf,&R,B,cells)==KGE_ARG,"encode took x = %d",d);
        R.gx=1; R.gy=(u8)d; CHECK(kgEncode(buf,sizeof buf,&R,B,cells)==KGE_ARG,"encode took y = %d",d);
        R.gx=254; R.gy=1; CHECK(kgEncode(buf,sizeof buf,&R,B,cells)==KGE_ARG,"encode took x = 254");
        R.gx=KG_NOPOS; R.gy=KG_NOPOS; CHECK(kgEncode(buf,sizeof buf,&R,B,cells)>0,"encode refused no position");
        printf("  tier %d (%dx%d): %d realms round-tripped, each also as a format 1 save, hostile positions and formats refused\n",tier,d,d,tot);
    }
    // 5 kgPlace: where it was left, or where a new game starts it
    for(int tier=0;tier<3;tier++){
        mk(tier,555u+(u32)tier,3); int d=kgDim(tier); CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init"); KGiant G, N; kgSpawn(&N,&S);
        int dry=0, wet=-1, wx=0, wy=0; for(int y=2;y<d-2&&wet<0;y++) for(int x=2;x<d-2;x++) if(!(S.cells[y*d+x]&KC_BLD)&&(S.cells[y*d+x]&7)==KT_WATER){ wet=1; wx=x; wy=y; break; }
        for(int y=1;y<d-1;y+=3) for(int x=1;x<d-1;x+=3){ if(!kgcWalk(&S,x*8+4,y*8+4)) continue; dry++; G.x=-1; CHECK(kgPlace(&G,&S,x,y)==1,"place on (%d,%d)",x,y); CHECK(kgCx(&G)==x&&kgCy(&G)==y&&G.x==x*8+4&&G.y==y*8+4&&!G.moving&&G.step==0&&G.face==0,"stands on its cell"); }
        CHECK(dry>20,"dry cells tried (%d)",dry);
        if(wet>0){ CHECK(kgPlace(&G,&S,wx,wy)==0,"water refuses"); CHECK(G.x==N.x&&G.y==N.y,"water: it starts as a new game does"); }
        CHECK(kgPlace(&G,&S,0,5)==0&&G.x==N.x&&G.y==N.y,"the border refuses"); CHECK(kgPlace(&G,&S,5,0)==0,"top border"); CHECK(kgPlace(&G,&S,d-1,5)==0,"right border"); CHECK(kgPlace(&G,&S,5,d-1)==0,"bottom border");
        CHECK(kgPlace(&G,&S,KG_NOPOS,KG_NOPOS)==0&&G.x==N.x&&G.y==N.y,"no position starts as new"); CHECK(kgPlace(&G,&S,KG_NOPOS,4)==0&&G.x==N.x,"half a position starts as new"); CHECK(kgPlace(&G,&S,d,d)==0&&G.x==N.x,"off the land"); CHECK(kgPlace(&G,&S,-1,3)==0,"negative");
        CHECK(kgPlace(&G,&S,0,0)==0&&G.x==N.x&&G.y==N.y,"a zeroed position (an old test realm, a memset) starts as new");
        // a giant left on a building stays there (it walks over buildings)
        for(int i=0;i<S.R->nB;i++){ const KBld*b=&S.B[i]; if(kgcWalk(&S,b->x*8+4,b->y*8+4)){ CHECK(kgPlace(&G,&S,b->x,b->y)==1&&kgCx(&G)==b->x&&kgCy(&G)==b->y,"on a building"); break; } }
        // an all-water land: nowhere to stand, as for kgSpawn
        for(int i=0;i<d*d;i++) cells[i]=KT_WATER; R.nB=0; CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init water"); CHECK(kgPlace(&G,&S,d/2,d/2)==0&&G.x==(d/2)*8+4,"all water");
    }
    // 6 walk, save, load, place: the same cell (through the real simulation start)
    int cyc=0;
    for(int tier=0;tier<3;tier++){
        mk(tier,800u+(u32)tier,6); CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init"); KGiant G; kgSpawn(&G,&S);
        for(int k=1;k<=4000;k++){ int dx=(int)(rnd()%3)-1, dy=(int)(rnd()%3)-1; if(k%53<20) dx=1; if(k%71<20) dy=-1; kgMove(&G,&S,dx,dy,1+(int)(rnd()%7));
            if(k%250==0){
                R.gx=(u8)kgCx(&G); R.gy=(u8)kgCy(&G); int n=kgEncode(buf,sizeof buf,&R,B,cells); CHECK(n>0,"encode");
                CHECK(load(buf,n)==0,"load"); CHECK(ksInit(&S2,&R2,B2,c2)==KSE_OK,"init 2");
                KGiant H; int got=kgPlace(&H,&S2,R2.gx,R2.gy); CHECK(got==1&&kgCx(&H)==kgCx(&G)&&kgCy(&H)==kgCy(&G),"tier %d frame %d: saved (%d,%d), back on (%d,%d) got %d",tier,k,kgCx(&G),kgCy(&G),kgCx(&H),kgCy(&H),got); cyc++; } }
    }
    printf("  walk, save, load, place: %d times, always on the very cell it was saved on\n",cyc);
    if(fails){ printf("kingpos: FAILED: %d failure(s)\n",fails); return 1; }
    printf("kingpos: ALL PASSED: 0 failure(s)\n"); return 0;
}

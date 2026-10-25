// test_diag.c - PC test of the DIAGONALS option (source/opts.h: padTrack, padFour): 8 WAY leaves the pad alone; 4 WAY keeps, of two axes held at once, only the one pressed last.
// Run from the repo root:   sh tools/test_diag.sh
#include <stdio.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16;
#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
enum { K_A=1, K_B=2, K_SEL=4, K_START=8, K_RIGHT=16, K_LEFT=32, K_UP=64, K_DOWN=128, K_R=256, K_L=512 };
#include "../source/opts.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } }while(0)
static void pad(u16 k,u16 pr,int*ux,int*uy){ padTrack(pr); *ux=((k&K_RIGHT)?1:0)-((k&K_LEFT)?1:0); *uy=((k&K_DOWN)?1:0)-((k&K_UP)?1:0); padFour(ux,uy); }
int main(void){
    int ux,uy; optsDefaults();
    CHECK(xo[XO_DIAG]==0&&xoCnt[XO_DIAG]==2&&xoDef[XO_DIAG]==0,"8 WAY is the default of two choices");
    // 8 WAY: every combination goes through
    pad(K_RIGHT|K_DOWN,K_RIGHT|K_DOWN,&ux,&uy); CHECK(ux==1&&uy==1,"8 way: down right %d %d",ux,uy);
    pad(K_LEFT|K_UP,K_UP,&ux,&uy); CHECK(ux==-1&&uy==-1,"8 way: up left %d %d",ux,uy);
    // 4 WAY: the key pressed last wins, while the other stays held
    xo[XO_DIAG]=1;
    pad(K_RIGHT,K_RIGHT,&ux,&uy); CHECK(ux==1&&uy==0,"4 way: right alone");
    pad(K_RIGHT|K_DOWN,K_DOWN,&ux,&uy); CHECK(ux==0&&uy==1,"4 way: down pressed last: %d %d",ux,uy);
    pad(K_RIGHT|K_DOWN,0,&ux,&uy); CHECK(ux==0&&uy==1,"4 way: still down while both are held: %d %d",ux,uy);
    pad(K_RIGHT,0,&ux,&uy); CHECK(ux==1&&uy==0,"4 way: down let go, right goes on: %d %d",ux,uy);
    pad(K_RIGHT|K_UP,K_UP,&ux,&uy); CHECK(ux==0&&uy==-1,"4 way: up pressed last: %d %d",ux,uy);
    pad(K_RIGHT|K_UP,K_LEFT&0,&ux,&uy); pad(K_UP|K_LEFT,K_LEFT,&ux,&uy); CHECK(ux==-1&&uy==0,"4 way: left pressed last: %d %d",ux,uy);
    pad(K_LEFT|K_DOWN,K_LEFT|K_DOWN,&ux,&uy); CHECK(ux==-1&&uy==0,"4 way: both at once: left / right wins: %d %d",ux,uy);
    pad(0,0,&ux,&uy); CHECK(ux==0&&uy==0,"nothing held");
    // never a diagonal in 4 way, for every key combination and every order of presses
    int diag=0; for(int a=0;a<16;a++) for(int b=0;b<16;b++){ u16 k0=(u16)(((a&1)?K_RIGHT:0)|((a&2)?K_LEFT:0)|((a&4)?K_UP:0)|((a&8)?K_DOWN:0)), k1=(u16)(((b&1)?K_RIGHT:0)|((b&2)?K_LEFT:0)|((b&4)?K_UP:0)|((b&8)?K_DOWN:0));
        pad(k0,k0,&ux,&uy); diag+=ux&&uy; pad(k1,k1&~k0,&ux,&uy); diag+=ux&&uy; }
    CHECK(diag==0,"4 way made %d diagonals",diag);
    printf("  8 WAY passes every direction, 4 WAY never makes a diagonal and follows the key pressed last\n");
    printf("%s: %d failure(s)\n",fails?"FAILED":"diag: ALL PASSED",fails); return fails?1:0;
}

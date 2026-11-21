// BORE skateboard control regression: compile with host C compiler, no hardware required.
#include <assert.h>
#include <stdio.h>
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed char s8;
typedef int s32;
enum { K_LEFT=1, K_RIGHT=2, K_UP=4, K_DOWN=8, K_A=16, K_B=32 };
enum { AB_GRIP, AB_JUMP, SFX_STICK };
static int lsp,lvx,lvy,lhd,lvz,lz,lfx,lfy,lfr,lflip,lspecOn,bdOwn,stage,cview,lnoteT;
static const char *lnote;
static const int stSpd[5]={50,75,90,100,80};
static const int hdT[3][3]={{10,12,14},{8,-1,0},{6,4,2}}; // walk-only mock heading table
static int moodTop(int v){ return v; }
static int abOf(int n){ (void)n; return 2; }
static int abPct(int n,int v){ (void)n; (void)v; return 100; }
static int abBalance(void){ return 0; }
static int surfH(int x,int y){ (void)x; (void)y; return 0; }
static void sfxPlay(int n){ (void)n; }
static void padFour(int*x,int*y){ (void)x; (void)y; }
#include "../source/feel.h"
int main(void){
    feelReset(0);
    feelPush(K_A,0);
    assert(F.spd>=20);
    for(int i=0;i<36;i++){ lfr++; feelSteer(0); feelPush(K_A,0); feelVel(); }
    assert(F.spd>360&&F.spd<=F_TOP);
    int before=F.angF;
    for(int i=0;i<6;i++){ feelSteer(K_RIGHT); feelVel(); }
    assert(F.turn>0&&F.angF!=before);
    assert(lhd!=0||lvx!=0||lvy!=0);
    int coast=F.spd;
    for(int i=0;i<20;i++){ lfr++; feelSteer(0); feelPush(0,0); }
    assert(F.spd<coast&&F.spd>coast/2);
    assert(feelGradeAt(0)==3&&feelGradeAt(128)==3);
    assert(feelGradeAt(64)==0);
    feelReset(0); F.spd=200; feelSteer(K_RIGHT|K_DOWN);
    assert(F.drift==1);
    puts("BORE skating feel: PASS"); return 0;
}

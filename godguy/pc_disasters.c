// PC test of the classic disasters: the bomb, the tornado and the acid rain must really destroy what they should, and the DISASTERS law must stop the natural ones
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int alive(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive; return n; }
static int blds(void){ int n=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) n+=bld[y][x]&&bld[y][x]!=B_RUIN; return n; }
static void fresh(void){ planetSize=128; rs=909u; makeSpecies(); makeItems(); newWorld(); monOn=0; lawOff=0; memset(tor,0,sizeof tor); acidT=0; for(int t=1;t<=12000;t++){ tick=t; year=t/1200; simulate(); } }
static int crowded(int*cx,int*cy){ int best=-1,bn=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ int n=0; for(int j=0;j<nCr;j++) if(cr[j].alive&&abs(cr[j].x-cr[i].x)<=9&&abs(cr[j].y-cr[i].y)<=9) n++; if(n>bn){ bn=n; best=i; } } *cx=cr[best].x; *cy=cr[best].y; return bn; }
int main(){ int fails=0, cx, cy;
    fresh(); int n=crowded(&cx,&cy); int a0=alive(), b0=blds(); int lava0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) lava0+=world[y][x]==T_LAVA;
    nukeAt(cx,cy); int sk=0; for(int i=0;i<nCr;i++) sk+=cr[i].alive&&sick[i]==1; int lava1=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) lava1+=world[y][x]==T_LAVA;
    printf("bomb: %d creatures within 9 tiles of the target; alive %d -> %d, sick after fallout %d, lava tiles %d -> %d, buildings %d -> %d\n",n,a0,alive(),sk,lava0,lava1,b0,blds());
    if(alive()>=a0||lava1<=lava0){ fails++; printf("  FAIL: bomb\n"); }
    fresh(); int v=-1; for(int i=0;i<VIL_N;i++) if(vil[i].on&&vil[i].houses>2){ v=i; break; } if(v<0){ printf("no village\n"); return 1; }
    int bb=blds(), ac=alive(); tornadoSpawn(vil[v].x,vil[v].y); for(int t=0;t<700;t++){ tick=13000+t; tornadoTick(); } printf("tornado: buildings %d -> %d, creatures %d -> %d\n",bb,blds(),ac,alive()); if(blds()>=bb&&alive()>=ac){ fails++; printf("  FAIL: tornado\n"); }
    fresh(); bb=blds(); ac=alive(); int g0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) g0+=world[y][x]==T_GRASS||world[y][x]==T_FOREST; acidT=900; for(int t=0;t<900;t++){ tick=13000+t; acidTick(); } int g1=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) g1+=world[y][x]==T_GRASS||world[y][x]==T_FOREST;
    printf("acid rain (900 ticks): living ground %d -> %d, buildings %d -> %d, creatures %d -> %d\n",g0,g1,bb,blds(),ac,alive()); if(g1>=g0){ fails++; printf("  FAIL: acid\n"); }
    // the law: with DISASTERS off the world events never run
    fresh(); lawOff=(1u<<LW_NATDIS)|(1u<<LW_OTHERDIS); memset(tor,0,sizeof tor); acidT=0; int ev=0; for(int t=1;t<=60000;t++){ tick=20000+t; eventTick(); ev+=tor[0].on||tor[1].on||acidT>0; } printf("DISASTERS off: tornado / acid events started in 60000 ticks: %d\n",ev); if(ev){ fails++; printf("  FAIL: law\n"); }
    lawOff=0; ev=0; for(int t=1;t<=60000;t++){ tick=20000+t; eventTick(); ev+=tor[0].on||tor[1].on||acidT>0; memset(tor,0,sizeof tor); acidT=0; } printf("DISASTERS on : events seen %d\n",ev);
    printf("%s\n",fails?"DISASTERS FAIL":"DISASTERS OK"); return fails?1:0; }

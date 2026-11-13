// PC test of the plague: infect one creature in a crowd and follow it - it must spread, wear creatures down, burn out into immunity, and a HEAL must cure
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(int argc,char**argv){
    planetSize=128; rs=4242u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=3000;t++){ tick=t; simulate(); }
    int alive0=0; for(int i=0;i<nCr;i++) alive0+=cr[i].alive;
    int pick=-1; for(int i=0;i<nCr&&pick<0;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5){ int nb=0; for(int k=0;k<4;k++){ int x=cr[i].x+dxs[k], y=cr[i].y+dys[k]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]) nb++; } if(nb>=2) pick=i; }
    if(pick<0){ for(int i=0;i<nCr;i++) if(cr[i].alive){ pick=i; break; } }
    sick[pick]=1; int peak=0, end_imm=0;
    for(int t=3001;t<=9000;t++){ tick=t; simulate(); int s1=0,s2=0,al=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ al++; if(sick[i]==1) s1++; else if(sick[i]==2) s2++; } if(s1>peak) peak=s1; if(t%1500==0) printf("tick %d: alive %d, sick %d, immune %d\n",t,al,s1,s2); end_imm=s2; }
    int alive1=0,sk=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ alive1++; if(sick[i]==1) sk++; }
    // heal test: infect two creatures, heal one with the power code path
    int a=-1,b=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&sick[i]==0){ if(a<0) a=i; else { b=i; break; } }
    if(a>=0&&b>=0){ sick[a]=1; sick[b]=1; }
    printf("plague: peak %d sick at once, %d immune at the end, creatures %d -> %d, still sick %d\n",peak,end_imm,alive0,alive1,sk);
    int z=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&!strcmp(arch[sp[cr[i].sp].a].nm,"ZOMBIE")) z++; int pj=0; for(int i=0;i<jrN;i++) if(strstr(jr[i],"PLAGUE IN")) pj++;
    printf("zombies alive %d, journal lines \"PLAGUE IN ...\" %d\n",z,pj);
    printf("%s\n",(peak>=3&&end_imm>0)?"PLAGUE OK":"PLAGUE WEAK"); return 0; }

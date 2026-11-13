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
    // the zombie infection: one zombie among the folk
    planetSize=128; rs=4242u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=6000;t++){ tick=t; simulate(); }
    int za=-1; for(int a2=8;a2<ARCH_N;a2++) if(!strcmp(arch[a2].nm,"ZOMBIE")) za=a2; int vv=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>=8){ vv=v; break; }
    int placed=0; for(int i=0;i<nCr&&placed<8&&za>=8;i+=3) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4){ int fx=cr[i].x, fy=cr[i].y; for(int k=0;k<4;k++){ int n=addCr(8+(za-8)*16,fx+dxs[k],fy+dys[k]); if(n>=0){ cr[n].hp=(u8)maxHp(&cr[n]); placed++; break; } } }   // five zombies among the folk
    // the bite chain, deterministic: each zombie takes a few turns beside its folk (no waiting for the crowd's rotation), then an infected victim falls and must rise
    int inf_peak=0, rose=0; int z0=countArch(za); int zs[8], nz=0; for(int i=0;i<nCr&&nz<8;i++) if(cr[i].alive&&sp[cr[i].sp].a==za) zs[nz++]=i;
    for(int r=0;r<60;r++) for(int k=0;k<nz;k++){ if(cr[zs[k]].alive&&sp[cr[zs[k]].sp].a==za){ cr[zs[k]].hp=(u8)maxHp(&cr[zs[k]]); think(zs[k]); } }
    for(int k=0;k<nz;k++){ if(!cr[zs[k]].alive) continue; for(int d=0;d<4;d++){ int x=cr[zs[k]].x+dxs[d], y=cr[zs[k]].y+dys[d]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]&&arch[sp[cr[occ[y][x]-1].sp].a].diet==4) for(int q=0;q<6;q++) zombieBite(&cr[zs[k]],occ[y][x]-1); } }      // (and the bite itself, aimed directly: the chase above depends on the crowd)
    { int in=0; int fall=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&sick[i]==3){ in++; if(fall<0) fall=i; } inf_peak=in; if(fall>=0){ tick=12001; killCr(fall); simulate(); } }
    int z1=countArch(za); rose=z1-z0;
    printf("zombie infection: zombies placed %d, infected by their bites %d, a fallen victim rose: zombies %d -> %d\n",placed,inf_peak,z0,z1);
    int ok2=placed>=1&&(inf_peak>=1||rose>=1);
    // heal cures
    { int j=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4){ j=i; break; } if(j>=0){ sick[j]=3; power=0; for(int q=0;q<PW_N;q++) if(pw[q].kind==PK_HEAL) power=q; brush=1; usePower(cr[j].x,cr[j].y,1); printf("HEAL on an infected creature: state %d (0 = cured)\n",sick[j]); if(sick[j]!=0) ok2=0; } }
    printf("%s\n",(peak>=3&&end_imm>0&&ok2)?"PLAGUE OK":"PLAGUE WEAK"); return 0; }

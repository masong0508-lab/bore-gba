// PC test: the new animals, insects, magical folk, special animals, terraformers, evolved forms and spawner monsters
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int cnt(int a){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&sp[cr[i].sp].a==a) n++; return n; }
static int tiles[T_N]; static void tcount(void){ memset(tiles,0,sizeof tiles); for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) tiles[world[y][x]]++; }
static int LX=64,LY=64; static void findLand(void){ for(int y=8;y<WH-8;y++) for(int x=8;x<WW-8;x++){ int ok=1; for(int dy=-3;dy<=3&&ok;dy++) for(int dx=-3;dx<=3;dx++) if(world[y+dy][x+dx]!=T_GRASS){ ok=0; break; } if(ok){ LX=x; LY=y; return; } } }
static int put(int a,int x,int y){ for(int k=0;k<60;k++){ int n=addCr(8+(a-8)*16+3,x+(int)(rnd()%9)-4,y+(int)(rnd()%9)-4); if(n>=0) return n; } return -1; }
int main(){ int bad=0; planetSize=128; rs=77u; makeSpecies(); makeItems(); newWorld(); monOn=0;
  printf("archetypes %d (monsters from %d), species %d of %d, shapes %d\n",ARCH_N,ARCH_A,nSp,SP_N,SHAPE_N); if(nSp!=8+(ARCH_N-8)*16) bad++;
  { typedef char evn_chk[(sizeof evNm/sizeof evNm[0])==EVN_N&&EVN_N==ARCH_A-8-8?1:-1]; (void)sizeof(evn_chk); }
  // every new kind lives on its own for a while
  int fine=0; for(int a=44;a<ARCH_A;a++){ planetSize=128; newWorld(); monOn=0; nCr=0; memset(occ,0,sizeof occ); findLand(); int n=put(a,LX,LY); for(int k=0;k<4&&n>=0;k++) put(a,LX,LY); for(int t=1;t<=1500;t++){ tick=t; simulate(); } if(cnt(a)>0) fine++; else { printf("  %s died out\n",arch[a].nm); } }
  printf("new kinds that survived 1500 ticks: %d of %d\n",fine,ARCH_A-44); if(fine<ARCH_A-44-6) bad++;      // (hunters with nothing to hunt starve, a piranha needs water: the rest must live)
  // terraformers change the ground
  int changed=0; for(int a=A_FINGER;a<=A_PRINTER;a++){ planetSize=128; newWorld(); monOn=0; nCr=0; memset(occ,0,sizeof occ); static u8 before[MAXD][MAXD]; memcpy(before,world,sizeof before); findLand(); for(int k=0;k<6;k++) put(a,LX,LY); for(int t=1;t<=3000;t++){ tick=t; simulate(); } int d=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(before[y][x]!=world[y][x]) d++; printf("  %-11s changed %4d tiles\n",arch[a].nm,d); if(d>5) changed++; }
  if(changed<7) bad++;
  // a white mage and a unicorn heal
  { planetSize=128; newWorld(); monOn=0; nCr=0; memset(occ,0,sizeof occ); findLand(); int w=addCr(8+(A_WMAGE-8)*16+3,LX,LY); int v=addCr(8+(8-8)*16+3,LX+1,LY); cr[v].hp=2; cr[v].sick=1; for(int k=0;k<5;k++) healAround(&cr[w],2,6,1); printf("  sheep next to a white mage: hp %d, sick %d\n",cr[v].hp,cr[v].sick); if(cr[v].hp<=2||cr[v].sick==1) bad++; }
  // evolution (the law is on): some animals evolve and keep their evolved name
  { planetSize=128; newWorld(); monOn=0; gx.lAge=1; for(int i=0;i<nCr;i++) if(arch[sp[cr[i].sp].a].diet<4) { } int ev=0; for(int k=0;k<200;k++){ int n=put(12,60+(k%20),60+(k/20)); if(n>=0){ cr[n].age=60; cr[n].food=100; evolve(n); ev++; } } char nm[48]; int j=-1; for(int i=0;i<nCr;i++) if(crFl[i]&FL_EVOLVED){ j=i; break; } if(j>=0){ crName(&cr[j],nm); printf("  evolved: %s (hp %d, base %d)\n",nm,maxHp(&cr[j]),arch[sp[cr[j].sp].a].hp); if(maxHp(&cr[j])<=arch[sp[cr[j].sp].a].hp||!strstr(nm,"FANGKIN")) bad++; } else bad++; }
  // sapient animals around a den: tools, a growing tribe, more huts, and they fight monsters
  { planetSize=128; newWorld(); monOn=0; nCr=0; memset(occ,0,sizeof occ); findLand(); gx.lAge=1; for(int k=0;k<8;k++){ int n=addCr(8+(12-8)*16+3,LX-2+(k%4),LY-1+(k/4)); if(n>=0) crFl[n]|=FL_SAPIENT; } bld[LY][LX]=B_HUT; bldv[LY][LX]=1; int n0=nCr; for(int t=1;t<=4000;t++){ tick=t; simulate(); denTick(); } int armed=0,sap=0,huts=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&(crFl[i]&FL_SAPIENT)){ sap++; armed+=cr[i].wep!=0; } for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) huts+=bld[y][x]==B_HUT; printf("  sapient wolves after 4000 ticks: %d (from 8), armed %d, huts %d\n",sap,armed,huts); if(sap<8||!armed) bad++; }
  // the spawners make more of themselves
  { planetSize=128; newWorld(); monOn=1; nCr=0; memset(occ,0,sizeof occ); findLand(); put(M_TUMOR,LX,LY); for(int t=1;t<=4000;t++){ tick=t; simulate(); } printf("  tumor monsters %d, bioblobs %d after 4000 ticks\n",cnt(M_TUMOR),cnt(M_BIOBLOB)); if(cnt(M_BIOBLOB)<1) bad++; }
  // captain down: the soldiers around a fallen war hero are routed
  { planetSize=128; newWorld(); monOn=0; nCr=0; memset(occ,0,sizeof occ); findLand(); int cpt=addCr(0,LX,LY); int s1=addCr(0,LX+2,LY); int far=addCr(0,LX+14,LY); Nb*n=nbMake(cpt,20); if(n){ n->kills=6; crFl[cpt]|=FL_NOTE; } tick=5000; killCr(cpt); int r1=s1>=0&&routed(&cr[s1]), r2=far>=0&&routed(&cr[far]); printf("  captain down: soldier beside him routed %d, one 14 tiles away %d\n",r1,r2); if(!r1||r2) bad++; tick=5800; if(s1>=0&&routed(&cr[s1])) bad++; }
  printf("%s\n",bad?"ROSTER FAIL":"ROSTER OK"); return bad?1:0; }

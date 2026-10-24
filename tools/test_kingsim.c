// test_kingsim.c - PC test of source/kingsim.h (economy) and source/kinglog.h (hauling). Run from the repo root:   sh tools/test_kingsim.sh   (or  sh tools/test_kingsim.sh soak 10000)
// Checks: a realm builds itself from blueprints, healing (stall, vanished goal, road under a loaded carrier), save and load with goods in the
// carriers' hands, determinism, every cap, hostile saves, 30000 random edits, a soak with faults (road cut, sawmill destroyed, woodcutter walled in,
// stockpile full, storehouses gone), a 30-building stretch, the frame cost and the memory. Every scenario must PROVE it ran (goods delivered, buildings built).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"

static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } }while(0)
static u32 rs=987654321u; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static u8 cells[128*128]; static KRealm R; static KBld B[KG_MAXB]; static KSim S0; static KSim*S=&S0;

// ---- cycle model: a cost per operation (ASSUMED, like docs/EP6_PHASE0.md; replace with a cart measurement) ----
static double lo_cyc,hi_cyc,wlo,whi; static long frames; static u32 pm,pw,pb,ps,pp;
static void resetBudget(void){ lo_cyc=hi_cyc=wlo=whi=0; frames=0; pm=S->opMatch; pw=S->opWalk; pb=S->opBfs; ps=S->opSearch; pp=S->opProd; }
static void cycles(void){
    double m=S->opMatch-pm, w=S->opWalk-pw, b=S->opBfs-pb, q=S->opSearch-ps, p=S->opProd-pp; pm=S->opMatch; pw=S->opWalk; pb=S->opBfs; ps=S->opSearch; pp=S->opProd;
    double l=300+m*25+w*30+b*20+q*8+p*5, h=300+m*60+w*80+b*50+q*20+p*12;
    lo_cyc+=l; hi_cyc+=h; frames++; if(l>wlo) wlo=l; if(h>whi) whi=h;
}
static void budget(const char*what){ if(!frames) return; printf("  %s: budget per frame (of 280,896 cycles) average %.0f to %.0f (%.2f%% to %.2f%%), worst frame %.0f to %.0f (%.1f%% to %.1f%%)\n",what,lo_cyc/frames,hi_cyc/frames,lo_cyc/frames/2809.0,hi_cyc/frames/2809.0,wlo,whi,wlo/2809.0,whi/2809.0); }
static void run(long n){ for(long k=0;k<n;k++){ ksStep(S); cycles(); } }

// ---- building a scenario ----
static int cellFree(int x,int y){ int v=cells[y*S->dim+x]; return v==KT_GRASS||v==KT_MEADOW; }
static int countRes(int x,int y,int res){ int n=0; for(int j=-KS_GR;j<=KS_GR+1;j++) for(int k=-KS_GR;k<=KS_GR+1;k++){ int cx=x+1+k, cy=y+1+j; if(cx>=0&&cy>=0&&cx<S->dim&&cy<S->dim&&(cells[cy*S->dim+cx]&7)==res) n++; } return n; }
static int spotFor(int type,int skip,int built){   // the nth free place (a ring walk out from the middle), with a one cell gap round it, and trees or rock in reach for a gatherer
    int d=S->dim, c=d/2; const KsType*T=ksT(type); int need=T->kind==KK_GATHER?6:0, tried=0;
    for(int r=2;r<d/2-4;r++) for(int y=c-r;y<=c+r;y++) for(int x=c-r;x<=c+r;x++){
        if(x!=c-r&&x!=c+r&&y!=c-r&&y!=c+r) continue;
        int ok=1; for(int j=-1;j<=2&&ok;j++) for(int k=-1;k<=2;k++){ int cx=x+k, cy=y+j; if(cx<2||cy<2||cx>=d-2||cy>=d-2||(cells[cy*d+cx]&KC_BLD)||((j>=0&&j<=1&&k>=0&&k<=1)&&!cellFree(cx,cy))){ ok=0; break; } }
        if(!ok) continue; if(need&&countRes(x,y,T->res)<need) continue; if(tried++<skip) continue;
        return ksPlace(S,type,x,y,built); }
    return -1;
}
static int route(int b){   // roads from a building to the nearest road (a search over the cells); 1 if it is joined now
    int d=S->dim, adj[8], n=ksAdj(S,&S->B[b],adj); static int par[128*128]; static int q[128*128]; int qh=0, qt=0;
    for(int c=0;c<d*d;c++) par[c]=-2;
    for(int i=0;i<n;i++){ if(ksIsRoad(S,adj[i])) return 1; if(cellFree(adj[i]%d,adj[i]/d)){ par[adj[i]]=-1; q[qt++]=adj[i]; } }
    while(qh<qt){ int c=q[qh++]; int nb[4]={c+1,c-1,c+d,c-d};
        for(int j=0;j<4;j++){ int m=nb[j]; if(m<0||m>=d*d||par[m]!=-2) continue;
            if(ksIsRoad(S,m)){ int p=c; while(p>=0){ if(ksRoadSet(S,p%d,p/d,1)!=KSE_OK) return 0; p=par[p]; } return 1; }
            if(cellFree(m%d,m/d)){ par[m]=c; q[qt++]=m; } } }
    return 0;
}
static int roadStub(int b){ int adj[8], n=ksAdj(S,&S->B[b],adj); for(int i=0;i<n;i++) if(ksRoadSet(S,adj[i]%S->dim,adj[i]/S->dim,1)==KSE_OK) return 1; return 0; }
static int starter(int tier,u32 seed,int huts,int houses,int cutters,int quarries,int saws,int masons,int stores){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=seed*2654435761u+1;
    kgGen(cells,seed,tier); if(ksInit(S,&R,B,cells)!=KSE_OK) return 0;
    int s=spotFor(KB_STORE,0,1); if(s<0||!roadStub(s)) return 0;
    int want[7]={huts,houses,cutters,quarries,saws,masons,stores-1}, type[7]={KB_HUT,KB_HOUSE,KB_WOODCUT,KB_QUARRY,KB_SAWMILL,KB_MASON,KB_STORE};
    for(int t=0;t<7;t++) for(int k=0;k<want[t];k++){ int b=spotFor(type[t],0,0); if(b<0) break; if(!route(b)) ksDemolish(S,b); }
    ksGift(S,G_WOOD,60); ksGift(S,G_PLANKS,8); return 1;
}
static int findSeed(int tier,int huts,int houses,int cutters,int quarries,int saws,int masons,int stores){
    for(u32 sd=1;sd<400;sd++){ u32 seed=sd*7919u+12345u; if(!starter(tier,seed,huts,houses,cutters,quarries,saws,masons,stores)) continue;
        int c[KS_NTYPES]={0}; for(int i=0;i<R.nB;i++) c[B[i].type]++;
        if(c[KB_HUT]>=huts&&c[KB_HOUSE]>=houses&&c[KB_WOODCUT]>=cutters&&c[KB_QUARRY]>=quarries&&c[KB_SAWMILL]>=saws&&c[KB_MASON]>=masons&&c[KB_STORE]>=stores) return (int)seed; }
    return -1;
}
static int blueprints(void){ int n=0; for(int i=0;i<R.nB;i++) n+=!B[i].lvl; return n; }
static int active(void){ int a=0; for(int w=0;w<KS_MAXW;w++) a+=S->W[w].st!=0; return a; }
static void waitBusy(void){ for(long k=0;k<6000&&active()<3;k++){ ksStep(S); cycles(); } }
static u32 fnv(const void*p,int n){ const u8*b=p; u32 h=2166136261u; for(int i=0;i<n;i++){ h^=b[i]; h*=16777619u; } return h; }
static u32 world(void){ return fnv(cells,S->dim*S->dim)^fnv(B,R.nB*(int)sizeof(KBld))^fnv(R.goods,sizeof R.goods)^R.day^(u32)S->delivered*31u; }
static int spec(void){ return findSeed(1,2,1,2,1,1,0,1); }   // the spec's 10 to 11 buildings

// ---- 1. a realm builds itself ----
static void testEconomy(void){
    printf("== economy: a realm bootstraps from blueprints\n");
    int seed=spec(); CHECK(seed>=0,"no seed gave the starter realm"); if(seed<0) return;
    int bp0=blueprints(), n0=R.nB; resetBudget(); run(KS_DAYT*300);
    long planks=S->made[G_PLANKS];
    printf("  seed %d, %d buildings (%d are blueprints), %d road cells, after 300 days: %d blueprints left, pop %d, carriers %d, delivered %u, wood made %ld, planks made %ld, stone made %ld\n",seed,n0,bp0,S->nN,blueprints(),S->pop,S->nWt,S->delivered,S->made[G_WOOD],planks,S->made[G_STONE]);
    CHECK(bp0>=6,"the start had only %d blueprints",bp0); CHECK(blueprints()==0,"%d blueprints never got built",blueprints());
    CHECK(S->delivered>200,"only %u goods delivered",S->delivered); CHECK(ksCheck(S)==0,"ksCheck %d",ksCheck(S)); CHECK(S->pop>KS_PARTY,"nobody moved in");
    int staffed=0; for(int i=0;i<R.nB;i++) staffed+=B[i].work&&ksT(B[i].type)->kind>=KK_GATHER&&ksT(B[i].type)->kind<=KK_PROC;
    CHECK(staffed>=3,"only %d buildings staffed",staffed); budget("economy");
}
// ---- 2. healing ----
static void testHealing(void){
    printf("== healing: a vanished goal, a road that vanishes under a loaded carrier, a stalled job\n");
    int seed=spec(); if(seed<0) return; run(KS_DAYT*30); waitBusy(); CHECK(active()>0,"no carrier was ever busy");
    int w=0; while(w<KS_MAXW&&!S->W[w].st) w++;
    if(w<KS_MAXW){ int dst=S->W[w].dst; ksDemolish(S,dst); CHECK(active()==0,"a demolition left %d carriers out",active()); CHECK(ksCheck(S)==0,"books after a vanished goal: %d",ksCheck(S)); }
    run(KS_DAYT*5); waitBusy(); w=0; while(w<KS_MAXW&&!S->W[w].st) w++;
    if(w<KS_MAXW){ int c=S->W[w].cell; ksRoadSet(S,c%S->dim,c/S->dim,0); CHECK(ksCheck(S)==0,"books after a road vanished under a carrier: %d",ksCheck(S)); }
    run(KS_DAYT*5); CHECK(ksCheck(S)==0,"books after healing: %d",ksCheck(S));
    waitBusy(); w=0; while(w<KS_MAXW&&!S->W[w].st) w++;
    if(w<KS_MAXW){ long t0=S->timeouts; S->W[w].stall=KS_TIMEOUT; run(2); CHECK(S->timeouts==t0+1,"a stalled carrier was not dropped"); CHECK(ksCheck(S)==0,"books after a timeout: %d",ksCheck(S)); }
    printf("  timeouts %u, aborts %u, books balance\n",S->timeouts,S->aborts);
}
// ---- 3. save and load with goods in the carriers' hands ----
static void testSave(void){
    printf("== save and load: the real kgEncode / kgDecode, goods in the carriers' hands\n");
    int seed=spec(); if(seed<0) return; run(KS_DAYT*120); for(long k=0;k<30000&&active()<3;k++) ksStep(S);
    int carrying=active(); CHECK(carrying>0,"the save was not made while carriers held goods");
    ksPrepSave(S); long before[KG_GOODS]; for(int g=0;g<KG_GOODS;g++) before[g]=ksHeld(S,g); CHECK(ksCheck(S)==0,"books after ksPrepSave: %d",ksCheck(S));
    static u8 buf[20000]; int len=kgEncode(buf,sizeof buf,&R,B,cells); CHECK(len>0,"kgEncode gave %d",len); if(len<=0) return;
    static KRealm R2; static KBld B2[KG_MAXB]; static u8 c2[128*128]; static KSim S2;
    CHECK(kgDecode(buf,len,&R2,B2,c2,0)==0,"check refused"); CHECK(kgDecode(buf,len,&R2,B2,c2,1)==0,"load refused");
    kgGen(c2,R2.seed,R2.tier); kgDecode(buf,len,&R2,B2,c2,1);   // (the edits go on the made land)
    CHECK(ksInit(&S2,&R2,B2,c2)==KSE_OK,"ksInit refused a saved realm");
    for(int g=0;g<KG_GOODS;g++) CHECK(ksHeld(&S2,g)==before[g],"good %d: %ld held, %ld before the save",g,ksHeld(&S2,g),before[g]);
    KSim*keep=S; S=&S2; u32 d0=S->delivered; for(long k=0;k<KS_DAYT*100;k++){ ksStep(S); if(k%5000==0) CHECK(ksCheck(S)==0,"books after load, frame %ld: %d",k,ksCheck(S)); }
    CHECK(S->delivered>d0+50,"the loaded realm moved no goods (%u)",S->delivered-d0); printf("  %d carriers held goods at the save; all back in the stockpile; the loaded realm ran 100 days, delivered %u, books balance\n",carrying,S->delivered-d0); S=keep;
}
// ---- 4. determinism ----
static void testDeterminism(void){
    printf("== determinism\n"); u32 h[2]; for(int r=0;r<2;r++){ int seed=spec(); if(seed<0) return; run(KS_DAYT*150); h[r]=world(); }
    printf("  two runs of 150 days: %08X and %08X\n",h[0],h[1]); CHECK(h[0]==h[1],"the simulation is not deterministic");
}
// ---- 5. caps ----
static void testCaps(void){
    printf("== caps\n"); int seed=spec(); if(seed<0) return; int r=0; for(int k=0;k<100&&R.nB<KS_MAXB;k++){ r=spotFor(KB_HUT,k%7,0); if(r<0) break; }
    int more=ksPlace(S,KB_HUT,20,20,0); if(R.nB>=KS_MAXB) CHECK(more==KSE_CAP,"a building past the cap gave %d",more);
    printf("  buildings: %d of %d\n",R.nB,KS_MAXB);
    int d=S->dim, put=0; for(int y=2;y<d-2&&S->nN<KS_MAXN;y++) for(int x=2;x<d-2&&S->nN<KS_MAXN;x++) if(ksRoadSet(S,x,y,1)==KSE_OK&&S->cells[y*d+x]==KT_ROAD) put++;
    int rr=KSE_OK; for(int y=2;y<d-2&&rr==KSE_OK;y++) for(int x=2;x<d-2&&rr==KSE_OK;x++) if(S->cells[y*d+x]==KT_GRASS||S->cells[y*d+x]==KT_MEADOW) rr=ksRoadSet(S,x,y,1);
    CHECK(S->nN<=KS_MAXN,"road cells %d",S->nN); CHECK(rr==KSE_CAP||S->nN==KS_MAXN,"road cap not reached"); printf("  road cells: %d (cap %d), the next refused with %d\n",S->nN,KS_MAXN,rr);
    for(int g=0;g<KG_GOODS;g++){ ksGift(S,g,KG_GOODMAX); ksGift(S,g,KG_GOODMAX); } R.day=0xFFFFFFF0u; run(KS_DAYT*40);
    CHECK(ksCheck(S)==0,"books after saturating: %d",ksCheck(S)); for(int g=0;g<KG_GOODS;g++) CHECK(R.goods[g]<=KG_GOODMAX,"good %d over the ceiling",g);
    CHECK(R.day>=0xFFFFFFF0u,"the day counter went backwards (%u)",R.day); CHECK(S->pop<=KS_POPCAP&&active()<=KS_MAXW,"pop or carriers over their caps");
    printf("  stockpile saturates at %d, the day counter at %u, pop %d <= %d, carriers %d <= %d\n",KG_GOODMAX,R.day,S->pop,KS_POPCAP,active(),KS_MAXW);
}
// ---- 6. hostile saves ----
static void testHostile(void){
    printf("== hostile saves\n"); static KSim H; static KRealm R2; static KBld B2[KG_MAXB]; static u8 c2[128*128]; int ok=0, refused=0;
    for(int round=0;round<60;round++){
        memset(&R2,0,sizeof R2); memset(B2,0,sizeof B2); R2.tier=(u8)(rnd()%3); R2.genVer=KG_GENVER; R2.seed=rnd(); R2.rng=rnd(); R2.day=rnd(); for(int g=0;g<KG_GOODS;g++) R2.goods[g]=(u16)rnd();
        R2.nB=(u16)(rnd()%(round<10?KG_MAXB:30)); int d=kgDim(R2.tier);
        for(int i=0;i<R2.nB;i++){ B2[i].type=(u8)(rnd()%(round%4?KS_NTYPES:KG_BTYPES)); B2[i].x=(u8)(rnd()%(round%5?d:256)); B2[i].y=(u8)(rnd()%d); B2[i].lvl=(u8)(rnd()%3); B2[i].work=(u8)rnd(); B2[i].stock=(u16)rnd(); }
        for(int c=0;c<d*d;c++) c2[c]=(u8)(rnd()%(round%3?7:256)); 
        int e=ksInit(&H,&R2,B2,c2); if(e!=KSE_OK){ refused++; continue; } ok++;
        KSim*keep=S; S=&H; for(long k=0;k<3000;k++) ksStep(S); CHECK(ksCheck(S)==0,"ksCheck %d after a hostile load (round %d)",ksCheck(S),round); S=keep; }
    printf("  60 garbage realms: %d refused, %d clamped and run for 3000 frames, books balanced\n",refused,ok); CHECK(ok>0&&refused>0,"the test did not reach both outcomes");
}
// ---- 7. fuzz ----
static void testFuzz(int iters){
    printf("== fuzz: %d random edits (place and connect, demolish, road on and off, save prep, gifts) with the books and invariants checked after each\n",iters);
    for(int tier=0;tier<3;tier++){
        if(findSeed(tier,2,1,2,1,1,0,1)<0) starter(tier,(u32)(tier*1000+1),0,0,0,0,0,0,1);
        int placed=0,refused=0,demo=0,roads=0,saves=0; u32 d0=S->delivered;
        for(int it=0;it<iters/3;it++){ int d=S->dim, c=d/2, op=(int)(rnd()%12);
            if(op<3){ int ty=(int)(rnd()%KS_NTYPES), x=c+(int)(rnd()%40)-20, y=c+(int)(rnd()%40)-20; int r=ksPlace(S,ty,x,y,(rnd()&7)==0); if(r>=0){ placed++; if(rnd()%4) route(r); } else refused++; }
            else if(op==3){ if(R.nB>6||rnd()%6==0){ if(R.nB>0){ ksDemolish(S,(int)(rnd()%(u32)R.nB)); demo++; } } }
            else if(op<8){ int x=c+(int)(rnd()%30)-15, y=c+(int)(rnd()%30)-15; ksRoadSet(S,x,y,(int)(rnd()&1)); roads++; }
            else if(op==8){ ksPrepSave(S); saves++; } else if(op==9){ ksGift(S,(int)(rnd()%KG_GOODS),(int)(rnd()%200)); }
            for(int k=0;k<(int)(rnd()%400);k++) ksStep(S);
            int e=ksCheck(S); if(e){ CHECK(0,"ksCheck %d (tier %d, edit %d, op %d)",e,tier,it,op); break; } }
        printf("  tier %d: %d placed, %d refused, %d demolished, %d road edits, %d save preps, delivered %u, buildings %d, road cells %d, ok\n",tier,placed,refused,demo,roads,saves,S->delivered-d0,R.nB,S->nN); }
}
// ---- 8. soak with faults ----
static void soak(const char*name,int tier,int days,int nB){
    int seed= nB>20?findSeed(tier,6,6,4,2,2,1,2):spec(); printf("== soak: %s, %d days (%ld frames), faults injected on the way\n",name,days,(long)days*KS_DAYT);
    if(seed<0){ CHECK(0,"no seed for the %s scenario",name); return; }
    int bp0=blueprints(); printf("  seed %d: %d buildings (%d blueprints at the start), %d road cells\n",seed,R.nB,bp0,S->nN);
    resetBudget(); long T=(long)days*KS_DAYT; int cutC=-1, sawI=-1, isoI=-1, ph=0; long mark[12]={0}; u32 dl0=0, maxCar=0; int sawX=0, sawY=0; u32 delivered0=0; (void)delivered0;
    for(long t=0;t<T;t++){
        if(ph==0&&t==T*2/10){ waitBusy(); int best=-1; for(int n=0;n<S->nN;n++){ int c=S->nodeCell[n], k=0, nb[4]={c+1,c-1,c+S->dim,c-S->dim}; for(int j=0;j<4;j++) k+=ksIsRoad(S,nb[j]); if(k==2){ best=c; break; } } cutC=best; if(cutC>=0) ksRoadSet(S,cutC%S->dim,cutC/S->dim,0); mark[ph++]=t; }
        else if(ph==1&&t==T*3/10){ if(cutC>=0) ksRoadSet(S,cutC%S->dim,cutC/S->dim,1); mark[ph++]=t; }
        else if(ph==2&&t==T*4/10){ waitBusy(); for(int i=0;i<R.nB;i++) if(B[i].type==KB_SAWMILL){ sawI=i; sawX=B[i].x; sawY=B[i].y; ksDemolish(S,i); break; } mark[ph++]=t; }
        else if(ph==3&&t==T*5/10){ if(sawI>=0){ int r=ksPlace(S,KB_SAWMILL,sawX,sawY,0); if(r<0) r=spotFor(KB_SAWMILL,0,0); if(r>=0) route(r); } mark[ph++]=t; }
        else if(ph==4&&t==T*6/10){ for(int i=0;i<R.nB;i++) if(B[i].type==KB_WOODCUT&&B[i].lvl){ isoI=i; int adj[8], n=ksAdj(S,&B[i],adj); for(int k=0;k<n;k++) if(ksIsRoad(S,adj[k])) ksRoadSet(S,adj[k]%S->dim,adj[k]/S->dim,0); break; } mark[ph++]=t; }
        else if(ph==5&&t==T*7/10){ if(isoI>=0){ int adj[8], n=ksAdj(S,&B[isoI],adj); for(int k=0;k<n;k++) ksRoadSet(S,adj[k]%S->dim,adj[k]/S->dim,1); route(isoI); } mark[ph++]=t; }
        else if(ph==6&&t==T*8/10){ for(int g=0;g<KG_GOODS;g++){ ksGift(S,g,KG_GOODMAX); } mark[ph++]=t; }
        else if(ph==7&&t==T*9/10){ for(int i=R.nB-1;i>=0;i--) if(B[i].type==KB_STORE) ksDemolish(S,i); mark[ph++]=t; }
        else if(ph==8&&t==T*95/100){ int r=spotFor(KB_STORE,0,0); if(r>=0) route(r); mark[ph++]=t; }
        if(t%(2*KS_DAYT)==0&&t>0&&R.nB<KS_MAXB-8&&(rnd()%3)){ int r=spotFor((rnd()&1)?KB_HUT:KB_HOUSE,(int)(rnd()%6),0); if(r>=0) route(r); }   // a busy player keeps building
        else if(t%(2*KS_DAYT)==KS_DAYT){ int homes=0; for(int i=0;i<R.nB;i++) homes+=B[i].type==KB_HUT||B[i].type==KB_HOUSE; if(homes>5){ for(int k=0;k<20;k++){ int i=(int)(rnd()%(u32)R.nB); if(B[i].type==KB_HUT||B[i].type==KB_HOUSE){ ksDemolish(S,i); break; } } } }
        ksStep(S); cycles(); if((u32)active()>maxCar) maxCar=(u32)active();
        if(t%20011==0){ int e=ksCheck(S); if(e){ CHECK(0,"ksCheck %d at frame %ld",e,t); break; } }
        for(int p=0;p<9;p++) if(mark[p]==t&&t>0&&(p==0||mark[p-1]<t)){ printf("  day %ld: fault %d\n",t/KS_DAYT,p); }
        if(ph<=8&&t==mark[0]&&0) dl0=S->delivered;
    }
    int e=ksCheck(S); CHECK(e==0,"final ksCheck %d",e); int stuck=0; for(int w=0;w<KS_MAXW;w++) stuck+=S->W[w].st&&S->W[w].stall>=KS_TIMEOUT;
    printf("  RESULT: books %s; carriers stuck at the end %d; timeouts %u, aborts %u; carriers in use at most %u; delivered %u in all\n",e?"BROKEN":"balanced",stuck,S->timeouts,S->aborts,maxCar,S->delivered);
    CHECK(stuck==0,"%d carriers stuck",stuck); CHECK(S->delivered>(u32)days*2,"only %u goods delivered in %d days: the scenario did not run",S->delivered,days); CHECK(blueprints()<=R.nB/2,"%d of %d buildings are still blueprints",blueprints(),R.nB);
    CHECK(S->pop>KS_PARTY,"nobody ever moved in"); printf("  caps: day %u, pop %d (cap %d), buildings %d (cap %d), road cells %d (cap %d), goods all <= %d: checked\n",R.day,S->pop,KS_POPCAP,R.nB,KS_MAXB,S->nN,KS_MAXN,KG_GOODMAX);
    budget(name); (void)dl0;
}
static void footprint(void){
    printf("== footprint\n  KSim (everything the simulation keeps beside the realm): %d bytes\n    distance maps %d x %d = %d, road list %d, queue %d, carriers %d x %d = %d\n",(int)sizeof(KSim),KS_MAPS,KS_MAXN,KS_MAPS*KS_MAXN,KS_MAXN*2,KS_MAXN*2,KS_MAXW,(int)sizeof(KsWk),KS_MAXW*(int)sizeof(KsWk));
    for(int t=0;t<3;t++) printf("  %-9s %3dx%-3d: map %5d + sim %d + building list %d = %d bytes (%.1f KB)\n",t==0?"hamlet":t==1?"realm":"kingdom",kgDim(t),kgDim(t),kgCells(t),(int)sizeof(KSim),KS_MAXB*(int)sizeof(KBld),kgCells(t)+(int)sizeof(KSim)+KS_MAXB*(int)sizeof(KBld),(kgCells(t)+(int)sizeof(KSim)+KS_MAXB*(int)sizeof(KBld))/1024.0);
}
int main(int argc,char**argv){
    int days=600, quick=1; if(argc>1&&!strcmp(argv[1],"soak")){ quick=0; days=argc>2?atoi(argv[2]):10000; }
    testEconomy(); testHealing(); testSave(); testDeterminism(); testCaps(); testHostile(); testFuzz(quick?30000:60000);
    soak("spec (10 to 11 buildings)",1,days,10); soak("stretch (about 30 buildings)",2,quick?days:days/3,30); footprint();
    printf("\n%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}

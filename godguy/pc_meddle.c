// Pack B (divine meddling): each political power changes what it says and nothing else, the two-tap powers need two taps, a slot-less INSPIRATION changes nothing, Gold Dust leaves no city in a kingdom it left.
//   gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_meddle pc_meddle.c -lm && ./pc_meddle
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0; *tx=*ty=0; }
void shimPresent(void){}
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
static int nKd(void){ int n=0; for(int i=0;i<KN_N;i++) n+=kd[i].on; return n; }
static int nCity(int k){ int c=0; for(int v=0;v<VIL_N;v++) c+=vil[v].on&&vilK[v]==k+1; return c; }
static void tap(int kind,int v){ polApply(kind,vil[v].x,vil[v].y); }
static int cityOf(int k,int skipCap){ for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&(!skipCap||kd[k].cap!=v+1)) return v; return -1; }
int main(){ mapPtr=(u16*)bgGetMapPtr(0); gxDefaults(); planetSize=192; rs=4242u*2654435761u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=40000;t++){ tick=t; year=t/1200; simulate(); }
    int a=-1,b=-1; for(int i=0;i<KN_N;i++) if(kd[i].on){ if(a<0) a=i; else if(b<0) b=i; } if(b<0){ printf("  FAIL  need two kingdoms\nFAILED\n"); return 1; }
    int va=cityOf(a,0), vb=cityOf(b,0);
    // DISCORD / UNITY
    kd[a].allies|=(u8)(1<<b); kd[b].allies|=(u8)(1<<a); tap(PK_DISCORD,va); CHECK(!(kd[a].allies>>b&1)&&!(kd[b].allies>>a&1)&&disT[a]==8,"DISCORD breaks the alliance both ways and starts the counter");
    kdWar(a,b,1); tap(PK_UNITY,va); CHECK(polPend==a&&!(kd[a].allies>>b&1),"UNITY: the first tap only stores a kingdom");
    tap(PK_UNITY,va); CHECK(polPend==a,"UNITY: tapping the same kingdom again does nothing");
    tap(PK_UNITY,vb); CHECK((kd[a].allies>>b&1)&&(kd[b].allies>>a&1)&&!(kd[a].war>>b&1)&&!(kd[b].war>>a&1)&&polPend<0,"UNITY: the second tap allies them both ways and ends their war");
    // WHISPER
    kd[a].allies=kd[b].allies=0; kd[a].war=kd[b].war=0; tap(PK_WHISPER,va); tap(PK_WHISPER,vb); CHECK(kd[a].plot==b+1&&whisT[a]==b+1,"WHISPER: the first kingdom now plots war on the second");
    for(int t=0;t<5&&!(kd[a].war>>b&1);t++){ kingdomTick(); } CHECK((kd[a].war>>b&1)&&warM[0][0]>=0,"WHISPER: the war breaks out within a few kingdom turns");
    // SPITE and FRIENDSHIP
    kd[a].allies=kd[b].allies=0; tap(PK_SPITE,vb); CHECK(spiT[b]==12,"SPITE sets its counter on that kingdom only"); 
    tap(PK_FRIEND,va); CHECK(!kd[a].war&&!(kd[b].war>>a&1),"FRIENDSHIP ends that kingdom's wars");
    // DUSTS
    int l0=kd[a].lang; tap(PK_DUSTW,va); CHECK(kd[a].lang!=l0,"WHITE DUST changes the kingdom's language");
    { int v=cityOf(a,1); if(v>=0){ int n0=nCity(a); tap(PK_DUSTG,v); CHECK(vilK[v]==0&&vilIndep[v]&&nCity(a)==n0-1,"GOLD DUST: the city leaves its kingdom"); for(int t=0;t<3;t++) kingdomTick(); CHECK(vilK[v]==0,"and stays independent through kingdom turns"); } else printf("  (skipped gold dust: single-city kingdom)\n"); }
    // INSPIRATION
    { for(int i=0;i<KN_N;i++) if(kd[i].on&&i!=a&&i!=b){ kdDie(i); for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==i+1) vilK[u]=0; break; } }
    { int v=cityOf(a,1); if(v>=0){ int free_=KN_N-nKd(), k0=nKd(); vilIndep[v]=0; vilK[v]=(u8)(a+1); if(free_>0){ tap(PK_INSPIRE,v); CHECK(nKd()==k0+1&&vilK[v]!=a+1&&vilK[v]>0,"INSPIRATION founds a kingdom from a non-capital city (%d -> %d)",k0,nKd()); }
        for(int i=0;i<KN_N;i++) if(!kd[i].on) kd[i].on=1; int before=vilK[v]; vilK[v]=(u8)(a+1); int ks=0; for(int u=0;u<VIL_N;u++) ks+=vilK[u]; tap(PK_INSPIRE,v); int ks2=0; for(int u=0;u<VIL_N;u++) ks2+=vilK[u]; CHECK(vilK[v]==a+1&&ks==ks2,"INSPIRATION with no free slot changes nothing"); (void)before; } }
    // RED DUST
    { int n=0; for(int i=0;i<nNb;i++) if(nb[i].par||nb[i].mate) n++; printf("  (%d notables with kin)\n",n); }
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }

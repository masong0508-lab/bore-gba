// kingadv.h - EP6 THE LAND BOREFORE TIME: THE ADVISOR. PURE C (needs kingdom.h, kingsim.h, kinglog.h and kinghand.h before it; no hardware).
// A realm that stalls does not say why: carriers wait for a storehouse that was never built, a quarry has dug its rock away, the homes are full and nobody new arrives. kaAdvise looks at the running
// simulation (it only reads) and lists what is wrong or what comes next, most important first, as short sentences the GOALS screen (and now and then the text strip) can say.
// Each tip is a kind and two numbers (the building or the good): kaText turns one into at most 40 characters.
#ifndef KINGADV_H
#define KINGADV_H
enum { KAT_STORE, KAT_HOME, KAT_ROAD, KAT_EMPTY, KAT_WAIT, KAT_JOBS, KAT_FULL, KAT_CASTLE, KAT_NOCASTLE, KAT_STOCK, KAT_N };
typedef struct { u8 kind; u8 a; u16 b; } KAdv;   // kind; a: a building's number or a good; b: a count
#define KA_MAX 5
// does a road cell touch this building's footprint? (the carriers need one: ksDoor in kinglog.h)
static inline int kaRoaded(const KSim*S,const KBld*b){ int adj[8], n=ksAdj(S,b,adj); for(int i=0;i<n;i++) if(ksIsRoad(S,adj[i])) return 1; return 0; }
// has a gatherer anything left to gather in its reach? (the same 13 x 13 cells as ksGather: KS_GR each way from the middle of its 2 x 2)
static inline int kaHasResource(const KSim*S,const KBld*b,int res){
    int d=S->dim, cx=b->x+1, cy=b->y+1;
    for(int y=cy-KS_GR;y<=cy+KS_GR;y++) for(int x=cx-KS_GR;x<=cx+KS_GR;x++){ if(x<2||y<2||x>d-3||y>d-3) continue; if(S->cells[y*d+x]==res) return 1; }
    return 0;
}
// the tips, most important first; returns how many were written (at most max, never more than KA_MAX)
static inline int kaAdvise(const KSim*S,KAdv*out,int max){
    const KRealm*R=S->R; int n=0; if(max>KA_MAX) max=KA_MAX;
    int homes=0, cap=0, live=0, jobsOpen=0, producers=0, sites=0, castle=-1, built=0;
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type);
        if(!b->lvl){ sites++; if(T->kind==KK_CASTLE) castle=i; continue; }
        built++; if(T->kind==KK_CASTLE) castle=i;
        if(T->kind==KK_HOME){ homes++; live+=b->work; cap+=T->house; }
        if(T->kind==KK_GATHER||T->kind==KK_PROC){ producers++; if(!b->work) jobsOpen++; } }
    #define TIP(k,aa,bb) do{ if(n<max){ out[n].kind=(u8)(k); out[n].a=(u8)(aa); out[n].b=(u16)(bb); n++; } }while(0)
    if(!S->nStore&&(producers||sites)) TIP(KAT_STORE,0,0);                    // goods are kept in a storehouse: without one the carriers have nowhere to take them
    if(!homes&&built>=2) TIP(KAT_HOME,0,0);                                   // nobody moves in without a home
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(T->kind==KK_HOME||T->kind==KK_COMFORT) continue; if(!kaRoaded(S,b)){ TIP(KAT_ROAD,b->type,i); break; } }   // a road must touch the door
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(!b->lvl||T->kind!=KK_GATHER||!T->res) continue; if(!kaHasResource(S,b,T->res)){ TIP(KAT_EMPTY,b->type,i); break; } }   // it has dug or felled its reach bare: move it
    for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); if(!b->lvl||T->kind!=KK_PROC||!b->work) continue;
        if(ksIn(b)<2&&ksHeld(S,T->inG)<2){ TIP(KAT_WAIT,b->type,T->inG); break; } }   // a processor that has nothing to make anything of
    if(jobsOpen&&S->pop-S->emp<=0) TIP(KAT_JOBS,0,jobsOpen);                  // more jobs than Nubbins to do them
    if(homes&&live>=cap&&S->pop<KS_POPCAP) TIP(KAT_FULL,0,live);              // every home is full: more homes, more Nubbins
    if(castle>=0&&!S->B[castle].lvl){ int g=0, c=0; ksCostType(S,KB_CASTLE,S->B[castle].work,&g,&c); TIP(KAT_CASTLE,g,c); }   // what the castle's lot is waiting for
    else if(castle<0&&built>=8&&S->pop>=12) TIP(KAT_NOCASTLE,0,0);            // time for a castle
    for(int g=0;g<8;g++) if(S->R->goods[g]>=KG_GOODMAX-200){ TIP(KAT_STOCK,g,0); break; }   // the stockpile is at its ceiling: more of it is lost
    #undef TIP
    return n;
}
// one tip as words (at most 40 characters including the end)
static inline char* kaCat(char*d,const char*s){ while(*s) *d++=*s++; *d=0; return d; }
static inline void kaText(const KSim*S,const KAdv*t,char*b){
    char*e=b;
    switch(t->kind){
    case KAT_STORE:  kaCat(b,"NO STOREHOUSE YET  GOODS NEED A HOME"); break;
    case KAT_HOME:   kaCat(b,"NOBODY LIVES HERE  BUILD A HUT"); break;
    case KAT_ROAD:   e=kaCat(b,"NO ROAD REACHES THE "); e=kaCat(e,khName[KHM_HUT+t->a]); break;
    case KAT_EMPTY:  e=kaCat(b,khName[KHM_HUT+t->a]); kaCat(e," HAS RUN DRY  MOVE IT"); break;
    case KAT_WAIT:   e=kaCat(b,khName[KHM_HUT+t->a]); e=kaCat(e," IS WAITING FOR "); kaCat(e,khGood[t->b<8?t->b:0]); break;
    case KAT_JOBS:   e=khNum(b,t->b); kaCat(e," JOBS GOING BEGGING  BUILD HOMES"); break;
    case KAT_FULL:   kaCat(b,"EVERY HOME IS FULL  BUILD MORE"); break;
    case KAT_CASTLE: e=kaCat(b,"CASTLE IS WAITING FOR "); e=khNum(e,t->b); e=kaCat(e," "); kaCat(e,khGood[t->a<8?t->a:0]); break;
    case KAT_NOCASTLE: kaCat(b,"TIME FOR A CASTLE  YOU EARNED IT"); break;
    case KAT_STOCK:  e=kaCat(b,khGood[t->a<8?t->a:0]); kaCat(e," IS OVERFLOWING  USE SOME"); break;
    default: b[0]=0; }
    (void)S;
}
#endif

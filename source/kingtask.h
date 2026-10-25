// kingtask.h - EP6 THE LAND BOREFORE TIME, Phase 5 step c: the realm's numbers for the missions. PURE C (needs kingdom.h, kingsim.h and kinglog.h before it; kingq.h is included here).
// ktFeed fills v[EXK_N] (kingq.h's layout) from the running simulation. It only reads.
#ifndef KINGTASK_H
#define KINGTASK_H
#include "kingq.h"
_Static_assert(KS_NTYPES<=EXK_NBT&&KG_GOODS==8,"kingq.h's layout holds 14 building types and 8 goods");
static inline int ktCap(long x){ return x<0?0:x>EXK_CAP?EXK_CAP:(int)x; }
static inline void ktFeed(const KSim*S,int*v){
    const KRealm*R=S->R; for(int i=0;i<EXK_N;i++) v[i]=0;
    int built=0; for(int i=0;i<R->nB;i++){ const KBld*b=&S->B[i]; if(!b->lvl) continue; built++; if(b->type<EXK_NBT) v[EXK_BT+b->type]++; }
    v[EXK_BUILT]=ktCap(built); v[EXK_POP]=ktCap(S->pop); v[EXK_EMP]=ktCap(S->emp); v[EXK_CASTLE]=R->castle;
    v[EXK_ROAD]=ktCap(S->nN); v[EXK_DELIV]=ktCap((long)S->delivered);
    { int d=S->dim*S->dim, nb=0; for(int c=0;c<d;c++) nb+=(S->cells[c]&7)==KT_BRIDGE; v[EXK_BRIDGE]=ktCap(nb); }
    for(int g=0;g<8;g++) v[EXK_GOOD+g]=ktCap(ksHeld(S,g));
    for(int t=0;t<EXK_NBT;t++) v[EXK_BT+t]=ktCap(v[EXK_BT+t]);
}
#endif

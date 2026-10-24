// kingmem.h - EP6 THE LAND BOREFORE TIME, Phase 3 step e (layout reordered in Phase 4 step a): where the realm lives in memory. Needs kingdom.h, kingsim.h (and kinglog.h for ksStep) before it.
// THE IDEA: the realm needs no EWRAM of its own. While a kingdom is played the household's baked sprites (hhPool, 30,720 bytes, house.h) are not drawn,
// so the realm borrows that pool: the realm, the building list, the map and the simulation (KSim) are laid out inside it. The base game ALREADY does this for the
// creator's stage copy: stgClaim() (house.h) lets go of every baked household sprite and clears their keys, so they are baked again when play starts.
// This file only lays the realm out in a buffer the caller hands over; it changes nothing in the base game.
//
// HOW THE GAME WIRES IT (kingplay.h does exactly this; each line is a rule, not a suggestion):
//   enter kingdom mode:  stgClaim(); stgOk=0;  then  kmLayout(hhPool,SP_POOL,tier,&m)  (m.R / m.B / m.cells / m.S are the pointers; kgsLoad fills them through m.lscr, the realm's bytes)
//   while it runs:       NOTHING may bake household sprites (hhBakeAll, hhSprBegin) and the creator's stage copy must not be armed: they write at the start of the pool
//   save (or leave):     ksPrepSave(m.S), then kgsSave through m.sscr; leaving needs nothing else: the keys were cleared by stgClaim, so the household bakes again by itself
// SCRATCH (step 4a). A save needs a buffer for the encoded realm, and the worst realm (40 buildings, 512 road cells, a checkerboard of felled trees: 6,694 bytes in the 128 x 128
// tier) is MORE than the 6,048 bytes the pool has spare there. So the layout puts the simulation LAST and hands out two scratch areas that run to the end of the pool:
//   m.lscr / m.lN  to LOAD: from where the simulation will be to the end of the pool (the simulation is not started yet, so all of it is free)
//   m.sscr / m.sN  to SAVE: from KSim.dist to the end of the pool. Valid ONLY after ksPrepSave: the distance maps and the queue are a cache that ksPrepSave let go of (every map
//                  not ready, no building waiting for one), so the realm goes on running after a save, rebuilding its maps. (KSim keeps dist and queue as its last members for this.)
// A realm that does not fit the pool is refused (KM_NOFIT), never overrun. The buffer need not be aligned: the layout aligns every part (to 4 bytes on the GBA, which reads a
// misaligned word wrongly) and spends a few bytes of the pool on it.
#define KM_POOL 30720      // the size of hhPool (SP_POOL in house.h); tools/test_kingmem.c and the patch check that it still is
#define KM_NOFIT (-1)
typedef struct { KSim*S; KRealm*R; KBld*B; u8*cells; int used; u8*lscr; int lN; u8*sscr; int sN; } KmMem;
#define KM_AL ((uintptr_t)(_Alignof(KSim)>4?_Alignof(KSim):4))   // 4 on the GBA (a misaligned word reads wrongly there), 8 on a 64 bit PC
#define KM_UP(p) (((p)+KM_AL-1u)&~(KM_AL-1u))
static inline int kmNeed(int tier){ int d=kgDim(tier); if(!d) return 1<<30; return (int)sizeof(KSim)+(int)sizeof(KRealm)+KS_MAXB*(int)sizeof(KBld)+d*d+32; }   // +32: the alignment of the four parts
static inline int kmLayout(void*pool,int bytes,int tier,KmMem*m){   // 0 or KM_NOFIT. The realm, the list and the map are zeroed; kgGen / kgsLoad then fill them.
    if(!pool||!m||bytes<kmNeed(tier)) return KM_NOFIT;
    uintptr_t p=KM_UP((uintptr_t)pool), end=(uintptr_t)pool+(uintptr_t)bytes; int d=kgDim(tier);
    m->R=(KRealm*)p; p=KM_UP(p+sizeof(KRealm));
    m->B=(KBld*)p; p=KM_UP(p+(uintptr_t)KS_MAXB*sizeof(KBld));
    m->cells=(u8*)p; p=KM_UP(p+(uintptr_t)(d*d));
    m->S=(KSim*)p; if(p+sizeof(KSim)>end) return KM_NOFIT;
    m->used=(int)(p+sizeof(KSim)-(uintptr_t)pool);
    m->lscr=(u8*)p; m->lN=(int)(end-p);
    m->sscr=(u8*)m->S->dist; m->sN=(int)(end-(uintptr_t)m->sscr);
    memset(m->S,0,sizeof(KSim)); memset(m->R,0,sizeof(KRealm)); memset(m->B,0,(size_t)KS_MAXB*sizeof(KBld)); memset(m->cells,0,(size_t)(d*d));
    return 0;
}

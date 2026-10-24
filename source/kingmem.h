// kingmem.h - EP6 THE LAND BOREFORE TIME, Phase 3 step e: where the realm lives in memory. Needs kingdom.h, kingsim.h (and kinglog.h for ksStep) before it.
// THE IDEA: the realm needs no EWRAM of its own. While a kingdom is played the household's baked sprites (hhPool, 30,720 bytes, house.h) are not drawn,
// so the realm borrows that pool: the simulation (KSim), the building list and the map are laid out inside it. The base game ALREADY does this for the
// creator's stage copy: stgClaim() (house.h) lets go of every baked household sprite and clears their keys, so they are baked again when play starts.
// This file only lays the realm out in a buffer the caller hands over; it changes nothing in the base game.
//
// HOW THE GAME WIRES IT (Phase 4; each line is a rule, not a suggestion):
//   enter kingdom mode:  stgClaim(); stgOk=0;  then  kmLayout(hhPool,SP_POOL,tier,&m)  (m.R / m.B / m.cells / m.S are the pointers; fill them with kgDecode, or kgGen for a new realm)
//   while it runs:       NOTHING may bake household sprites (hhBakeAll, hhSprBegin) and the creator's stage copy must not be armed: they write at the start of the pool
//   leave kingdom mode:  ksPrepSave(m.S), kgEncode the realm into its slot, THEN let the household bake again. Nothing else: the keys were cleared by stgClaim, so it bakes again by itself
// A realm that does not fit the pool is refused (KM_NOFIT), never overrun. The buffer need not be aligned: the layout aligns every part (to 4 bytes on the GBA, which reads a
// misaligned word wrongly) and spends a few bytes of the pool on it.
#define KM_POOL 30720      // the size of hhPool (SP_POOL in house.h); tools/test_kingmem.c and the patch check that it still is
#define KM_NOFIT (-1)
typedef struct { KSim*S; KRealm*R; KBld*B; u8*cells; int used; } KmMem;
#define KM_AL ((uintptr_t)(_Alignof(KSim)>4?_Alignof(KSim):4))   // 4 on the GBA (a misaligned word reads wrongly there), 8 on a 64 bit PC
#define KM_UP(p) (((p)+KM_AL-1u)&~(KM_AL-1u))
static inline int kmNeed(int tier){ int d=kgDim(tier); if(!d) return 1<<30; return (int)sizeof(KSim)+(int)sizeof(KRealm)+KS_MAXB*(int)sizeof(KBld)+d*d+32; }   // +32: the alignment of the four parts
static inline int kmLayout(void*pool,int bytes,int tier,KmMem*m){   // 0 or KM_NOFIT. The realm, the list and the map are zeroed; kgGen / kgDecode then fill them.
    if(!pool||!m||bytes<kmNeed(tier)) return KM_NOFIT;
    uintptr_t p=KM_UP((uintptr_t)pool), end=(uintptr_t)pool+(uintptr_t)bytes; int d=kgDim(tier);
    m->S=(KSim*)p; p=KM_UP(p+sizeof(KSim));
    m->R=(KRealm*)p; p=KM_UP(p+sizeof(KRealm));
    m->B=(KBld*)p; p=KM_UP(p+(uintptr_t)KS_MAXB*sizeof(KBld));
    m->cells=(u8*)p; p=KM_UP(p+(uintptr_t)(d*d));
    if(p>end) return KM_NOFIT;
    m->used=(int)(p-(uintptr_t)pool);
    memset(m->S,0,sizeof(KSim)); memset(m->R,0,sizeof(KRealm)); memset(m->B,0,(size_t)KS_MAXB*sizeof(KBld)); memset(m->cells,0,(size_t)(d*d));
    return 0;
}

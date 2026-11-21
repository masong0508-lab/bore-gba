// BORE skating NPC route-target reservoir, fixed ROM-size helper.
// On each skate object found, seen is its 1-based index in the map scan.
// Returns a replacement cache slot, or -1 if the current reservoir is retained.
// The 16-bit random choice makes a bounded, representative set on dense maps.
#ifndef BORE_NPCSPOTS_H
#define BORE_NPCSPOTS_H
#define NPC_SPOT_CAP 48
static inline int npcSpotSlot(unsigned seen,unsigned rnd16){
    if(seen<=NPC_SPOT_CAP) return (int)seen-1;
    unsigned i=rnd16%seen;
    return i<NPC_SPOT_CAP?(int)i:-1;
}
#endif

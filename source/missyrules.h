// TV SHOW & TELL mission rules with no GBA dependencies or allocations.
// The original one-byte story-memory checksum remains unchanged.
// bits 0..3: prior slips, bit 4: one-more-night used, bits 5..7: hospital pressure band.
#ifndef BORE_MISSYRULES_H
#define BORE_MISSYRULES_H
#define MS_ONE_MORE_NIGHT 16u
#define MS_PRESSURE_BITS 224u
#define MS_PENDING_SCENE 64u
static inline unsigned msPressureBand(unsigned p){
    if(p>100u) p=100u;
    unsigned band=(p+7u)/14u;
    return band>7u?7u:band;
}
static inline unsigned msPressurePack(unsigned mem,unsigned pressure){
    return (mem&~MS_PRESSURE_BITS)|(msPressureBand(pressure)<<5);
}
static inline unsigned msPressureResume(unsigned mem,unsigned first){
    unsigned band=(mem&MS_PRESSURE_BITS)>>5;
    return band?band*14u:first;
}
static inline int msNightUsed(unsigned mem){ return (mem&MS_ONE_MORE_NIGHT)!=0; }
#endif

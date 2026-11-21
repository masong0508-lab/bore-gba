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
// Each sustained support (friends, wellbeing, funded care) offsets pressure.
// Three supports slowly recover the appeal. Always clamp before assigning to u8.
static inline unsigned msPressureStep(unsigned pressure,unsigned supports){
    int next=(int)pressure+2-(int)(supports>3u?3u:supports);
    return next<0?0u:next>100?100u:(unsigned)next;
}
// One final night extends the deadline by precisely one day; it must not
// erase five days of existing bedside progress or silently become five more.
static inline unsigned msDaysProgress(unsigned days,unsigned extension){
    return extension&&days?days-1u:days;
}
static inline int msNightUsed(unsigned mem){ return (mem&MS_ONE_MORE_NIGHT)!=0; }
#endif

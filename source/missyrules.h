// TV SHOW & TELL mission rules with no GBA dependencies or allocations.
// The original one-byte story-memory checksum remains unchanged.
// bits 0..3: prior slips, bit 4: one-more-night used, bits 5..7: hospital pressure band.
#ifndef BORE_MISSYRULES_H
#define BORE_MISSYRULES_H
#define MS_ONE_MORE_NIGHT 16u
#define MS_PRESSURE_BITS 224u
#define MS_PENDING_SCENE 64u
// Story status SRAM byte: low three bits chapter, next three task flags,
// bit six pending film, bit seven guest seen. Keep this layout save-compatible.
static inline unsigned msStatusPack(unsigned chapter,unsigned tasks,unsigned pending,unsigned guest){
    return (chapter&7u)|((tasks&7u)<<3)|(pending?MS_PENDING_SCENE:0u)|(guest?128u:0u);
}
static inline unsigned msStatusChapter(unsigned byte){ return byte&7u; }
static inline unsigned msStatusTasks(unsigned byte){ return (byte>>3)&7u; }
static inline unsigned msStatusPending(unsigned byte){ return (byte&MS_PENDING_SCENE)!=0u; }
static inline unsigned msStatusGuest(unsigned byte){ return (byte>>7)&1u; }
static inline unsigned msPressureBand(unsigned p){
    if(p>100u) p=100u;
    unsigned band=(p+7u)/14u;
    return band>7u?7u:band;
}
static inline unsigned msPressurePack(unsigned mem,unsigned pressure){
    return (mem&~MS_PRESSURE_BITS)|(msPressureBand(pressure)<<5);
}
// Band zero is a real low-pressure state (0..6), not a request to reset
// pressure to the starting value. Chapter four initializes its starting
// pressure explicitly on entry, including when saved before the first tick.
static inline unsigned msPressureResume(unsigned mem){
    unsigned band=(mem&MS_PRESSURE_BITS)>>5;
    return band?band*14u:0u;
}
// Each sustained support (friends, wellbeing, funded care) offsets pressure.
// Three supports slowly recover the appeal. Always clamp before assigning to u8.
// A positive social event can arrive immediately after loading a save.
static inline unsigned msPressureEase(unsigned pressure,unsigned amount){
    return pressure>amount?pressure-amount:0u;
}
static inline unsigned msPressureStep(unsigned pressure,unsigned supports){
    int next=(int)pressure+2-(int)(supports>3u?3u:supports);
    return next<0?0u:next>100?100u:(unsigned)next;
}
// The hospital has exactly one reprieve. 0=continue, 1=grant final night,
// 2=irreversible loss. Keep this decision pure for exhaustive host checks.
static inline unsigned msPressureOutcome(unsigned pressure,unsigned nightUsed){
    if(pressure<100u) return 0u;
    return nightUsed?2u:1u;
}
// One final night extends the deadline by precisely one day; it must not
// erase five days of existing bedside progress or silently become five more.
static inline unsigned msDaysProgress(unsigned days,unsigned extension){
    return extension&&days?days-1u:days;
}
static inline int msNightUsed(unsigned mem){ return (mem&MS_ONE_MORE_NIGHT)!=0; }
#endif

// Portable, pure rules for BORE's optional skate-park contests.
// No allocations, world state, or GBA hardware dependencies.
// The timed-run save block's existing spare byte is split into:
// bit 7 = ever won a park jam medal; bits 0..6 = completed jam day + 1 modulo 127.
// Zero means no jam yet; a day is unique in a sliding 127-day window.
#ifndef BORE_SKATEJAMRULES_H
#define BORE_SKATEJAMRULES_H
static inline unsigned sjDayTag(unsigned day){ return 1u+(day%127u); }
static inline int sjAlreadyToday(unsigned meta,unsigned day){ return (meta&127u)==sjDayTag(day); }
static inline unsigned sjRecord(unsigned meta,unsigned day,unsigned medal){
    return (meta&128u)|(medal?128u:0u)|sjDayTag(day);
}
static inline int sjQualified(unsigned meta,int skill,int tricks,int combo){
    return (meta&128u)!=0&&skill>=1&&tricks>=5&&combo>=250;
}
static inline int sjPrize(int medal){ return medal>=3?600:medal==2?300:medal==1?120:0; }
#endif

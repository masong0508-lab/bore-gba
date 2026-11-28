#ifndef GG_SHARED_TUNING_H
#define GG_SHARED_TUNING_H
/* Emergency aid never empties the donor's reserve or fills a recipient above 100. */
static inline int ggReliefAmount(int donor,int recipient){
    if(donor<=180||recipient>=80||recipient<0)return 0;
    int amount=donor-180;
    if(amount>60)amount=60;
    if(amount>100-recipient)amount=100-recipient;
    return amount;
}
/* Reduce the rotation once per preview, not once per visible pixel. */
static inline int ggTitlePhase(int tick,int width){
    return (int)((((unsigned)tick>>5)*(unsigned)width/600u)%(unsigned)width);
}
static inline int ggTitleLongitude(int lon,int width,int phase){
    int x=(lon*width>>12)+phase;
    return x>=width?x-width:x;
}
/* Catalogue movement is bounded by the compiled catalogue, not an old
 * fixed-size release. Direction is a single row or a ten-row page. */
static inline int ggBestiaryMove(int current,int delta,int count){
    if(count<=0)return 0;
    int next=current+delta;
    return next<0?0:next>=count?count-1:next;
}
/* Exact horizontal half-widths of the radius-ten village census.
 * One lookup per row replaces a circle test per map cell. */
static inline int ggCircleHalfWidth10(int dy){
    static const unsigned char half[11]={10,9,9,9,9,8,8,7,6,4,0};
    return half[dy<0?-dy:dy];
}
#endif

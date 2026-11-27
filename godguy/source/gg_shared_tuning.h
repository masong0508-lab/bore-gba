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
#endif

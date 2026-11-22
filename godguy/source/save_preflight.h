// Validate a save's readable structure before replacing the live world.
// Uses bounded scratch records and rewinds the existing stream; no second
// world, heap allocation, save-format change, or extra cartridge read.
#ifndef GG_SAVE_PREFLIGHT_H
#define GG_SAVE_PREFLIGHT_H
static int saveHeaderValid(const SaveHdr*h){
    int legacy=0;
#ifdef GG_BIGMAP
    legacy=h->szCr==sizeof(CrOld)&&h->szVil==sizeof(VilOld);
#endif
    u32 count=(u32)h->nCr|((u32)h->pad_[1]<<16);
    return h->magic==SAVE_MAGIC&&h->ver>=11&&h->ver<=14&&
        (legacy||(h->szCr==sizeof(Cr)&&h->szVil==sizeof(Vil)))&&
        validSize(h->gw)&&count<=MAXC;
}
static int saveSkip(SV*f,u32 bytes){
    u8 scratch[512];
    while(bytes){
        u32 n=bytes<sizeof scratch?bytes:(u32)sizeof scratch;
        if(svRead(scratch,1,n,f)!=n)return 0;
        bytes-=n;
    }
    return 1;
}
static int saveWord(SV*f,u32 expected){
    u32 value;
    return svRead(&value,4,1,f)==1&&value==expected;
}
static int savePreflight(SV*f,const SaveHdr*h){
    u32 count=(u32)h->nCr|((u32)h->pad_[1]<<16);
    u32 villages=h->planet?h->planet:VIL_OLDN;
    // All versions require the map, inhabitants, villages and journal.
    if(!saveSkip(f,3u*h->gw*h->gw)||
       !saveSkip(f,count*h->szCr)||
       !saveSkip(f,villages*h->szVil)||
       !saveSkip(f,sizeof kd+2u*villages+4u+34u*24u+2u*24u))return 0;
    // Version 11 deliberately permits missing optional legacy trailers.
    // Versions 12+ require their final records, which can only be reached
    // through the complete preceding sequence.
    if(h->ver==11)return 1;
    if(!saveSkip(f,32u+(villages>48?villages:48)+8u+600u+40u))return 0;
    u32 size,n,next,previous=0;
    if(!saveWord(f,0x31584747u)||svRead(&size,4,1,f)!=1||
       size<GX_HEAD||size>GX_HEAD+2u*512u||!saveSkip(f,size+8u))return 0;
    if(!saveWord(f,0x31484747u)||svRead(&size,4,1,f)!=1||
       (size!=sizeof(Hz)&&size!=sizeof(HzOld))||!saveSkip(f,size*HZ_N))return 0;
    if(!saveWord(f,0x31424747u)||!saveSkip(f,(u32)h->gw*h->gw/4u+32u+24u))return 0;
    if(!saveWord(f,0x31544747u)||!saveWord(f,sizeof(Boat))||
       svRead(&n,4,1,f)!=1||n>8u||!saveSkip(f,n*sizeof(Boat)))return 0;
    if(!saveWord(f,0x314C4747u)||svRead(&n,4,1,f)!=1||
       n>sizeof lw||!saveSkip(f,n))return 0;
    if(!saveWord(f,0x314E4747u)||svRead(&next,4,1,f)!=1||
       !saveWord(f,count))return 0;
    for(u32 i=0;i<count;i++){
        u32 gap;
        if(!idGet(f,&gap)||!gap||gap>~0u-previous)return 0;
        previous+=gap;
    }
    if(next<=previous||svRead(&n,4,1,f)!=1||n>count)return 0;
    previous=0;
    for(u32 i=0;i<n;i++){
        u32 gap;
        if(!idGet(f,&gap)||previous>=count||gap>=count-previous||
           !saveSkip(f,2))return 0;
        previous+=gap;
    }
    if(!saveSkip(f,4u*CSTAR_N)||!saveWord(f,sizeof(Nb))||
       svRead(&n,4,1,f)!=1||n>NB_N||!saveSkip(f,n*sizeof(Nb)))return 0;
    if(!saveWord(f,0x314B4747u)||!saveWord(f,KN_N)||
       !saveWord(f,sizeof(KdX))||!saveSkip(f,sizeof kx))return 0;
    if(!saveWord(f,0x31524747u)||!saveWord(f,sizeof hall)||
       !saveSkip(f,sizeof hall))return 0;
    if(!saveWord(f,0x31554747u)||!saveSkip(f,4))return 0;
    if(h->ver>=13&&(!saveWord(f,0x31464747u)||!saveSkip(f,16)))return 0;
    if(h->ver>=14&&(!saveWord(f,0x32524747u)||svRead(&n,4,1,f)!=1||
       n>0x7FFFFFFFu||!saveSkip(f,sizeof rz)))return 0;
    return 1;
}
#endif

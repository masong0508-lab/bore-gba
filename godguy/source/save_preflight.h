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
// Tile codes and ownership are array indices in the ordinary DS renderer.
static int saveBytesBounded(SV*f,u32 bytes,u32 maximum){
    u8 scratch[512];
    while(bytes){
        u32 n=bytes<sizeof scratch?bytes:(u32)sizeof scratch;
        if(svRead(scratch,1,n,f)!=n)return 0;
        for(u32 i=0;i<n;i++)if(scratch[i]>maximum)return 0;
        bytes-=n;
    }
    return 1;
}
static int saveCreatureRecordsValid(SV*f,const SaveHdr*h,u32 count){
    for(u32 i=0;i<count;i++){
        u32 alive,x,y,species,weapon,armor,trait,trait2;
#ifdef GG_BIGMAP
        if(h->szCr==sizeof(CrOld)){
            CrOld c;if(svRead(&c,sizeof c,1,f)!=1)return 0;
            alive=c.alive;x=c.x;y=c.y;species=c.sp;weapon=c.wep;armor=c.arm;trait=c.tr;trait2=c.tr2;
        }else
#endif
        {
            Cr c;if(svRead(&c,sizeof c,1,f)!=1)return 0;
            alive=c.alive;x=c.x;y=c.y;species=c.sp;weapon=c.wep;armor=c.arm;trait=c.tr;trait2=c.tr2;
        }
        // compact() immediately rebuilds occ[y][x] after loading. Check
        // against the saved world, not the currently open world's size.
        if(alive&&(x>=h->gw||y>=h->gw||species>=(u32)nSp||
           weapon>(u32)nIt||armor>(u32)nIt||
           trait>=sizeof trNm/sizeof trNm[0]||trait2>=sizeof trNm/sizeof trNm[0]))return 0;
    }
    return 1;
}
static int saveVillageRecordsValid(SV*f,const SaveHdr*h,u32 villages,u32 limit){
    for(u32 i=0;i<villages;i++){
        u32 on,x,y,race,foe;
#ifdef GG_BIGMAP
        if(h->szVil==sizeof(VilOld)){
            VilOld v;if(svRead(&v,sizeof v,1,f)!=1)return 0;
            on=v.on;x=v.x;y=v.y;race=v.race;foe=v.foe;
        }else
#endif
        {
            Vil v;if(svRead(&v,sizeof v,1,f)!=1)return 0;
            on=v.on;x=v.x;y=v.y;race=v.race;foe=v.foe;
        }
        // Extra source slots are discarded by rdVec. Retained villages
        // must never index a missing enemy or land outside this world.
        if(i<VIL_N&&on&&(x>=h->gw||y>=h->gw||race>=8||foe>limit))return 0;
    }
    return 1;
}
static int saveKingdomRecordsValid(SV*f,u32 villages){
    for(u32 i=0;i<KN_N;i++){
        Kd k;if(svRead(&k,sizeof k,1,f)!=1)return 0;
        if(k.on&&(k.race>=8||k.cap>villages||k.lord>KN_N||k.plot>KN_N||
           k.tech>=sizeof techN/sizeof techN[0]||
           k.rtr>=4||k.cul>=5||k.rlg>=4||k.lang>=5||k.nlang>=5||k.clan>=8))return 0;
    }
    return 1;
}
static int savePreflight(SV*f,const SaveHdr*h){
    u32 count=(u32)h->nCr|((u32)h->pad_[1]<<16);
    u32 villages=h->planet?h->planet:VIL_OLDN;
    u32 villageLimit=villages<VIL_N?villages:VIL_N;
    // Validate values used as indices as well as the byte lengths. A
    // complete file can still contain damaged coordinates or ownership.
    for(u32 y=0;y<h->gw;y++){
        if(!saveBytesBounded(f,h->gw,T_N-1)||
           !saveBytesBounded(f,h->gw,B_N-1)||
           !saveBytesBounded(f,h->gw,villageLimit))return 0;
    }
    if(!saveCreatureRecordsValid(f,h,count)||
       !saveVillageRecordsValid(f,h,villages,villageLimit)||
       !saveKingdomRecordsValid(f,villageLimit)||
       !saveSkip(f,villages)||!saveBytesBounded(f,villages,KN_N)||
       !saveSkip(f,4u+34u*24u+2u*24u))return 0;
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

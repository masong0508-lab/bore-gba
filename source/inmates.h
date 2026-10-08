// inmates.h - INMATES: the prison has a population. The same kind of voxel Sims as everyone else (baked like a household member, a hardware sprite on screen), not sprite art like the cops and skaters.
//
// WHO      Up to INM_WANT inmates (5) live in the EMPTY member places of the household (place HH_MAX-1-j, the same places the neighbours use when they drop by), so they cost no extra RAM
//          and share the sprite pool with the household: a household of 7 leaves no room (the limit is HH_MAX - hhN, as with visitors). They exist only on the PRISON lot. The same people
//          every time (inR is a hash of the inmate number, nothing random): name from the Sim names, a last name that is a prison number ("#4821"), a cropped / bald / flat top cut, now and then a beard,
//          glasses, or an elder. Skin, shape, eyes and mouth differ.
// ATTIRE   The creator's colour rows have no orange, so the uniform colours are set right after setColors (inmColors) and the pattern is painted into the voxels right after buildLook (inmDress).
//          0 ORANGE JUMPSUIT (orange long sleeve and pants, white shoes)     1 OLD STRIPES (white, a black stripe every other layer on the shirt and the pants, black shoes)
//          2 WORK BLUES (a blue shirt, darker blue pants, black shoes)       3 ORANGE PANTS + WHITE TANK (bare shoulders, white shoes)
//          Inmate j wears outfit j % 4, so the first ones you meet are in orange.
// LIFE     They wander the compound (the household's own path code, hhPlan wander + hhStepAlong), stop for 3 to 7 seconds, go on. R next to one: TALK / JOKE / COMPLIMENT / HIGH FIVE (the neighbour menu,
//          titled INMATE). They are not in the relationship tables and they do not fight yet (no PUNCH in that menu).
// NOT SAVED  Nothing here is saved: 14 bytes of EWRAM (inmPl, inmWt per place) and inmN. A new visit to the prison starts them at random free tiles of the yard.
#define INM_WANT 5
static u8 inmPl[HH_MAX] EWRAM_BSS;     // per place: 1 = this inmate stands on the lot (placed on its first tick)
static u16 inmWt[HH_MAX] EWRAM_BSS;    // per place: steps to stand still before the next walk
static u8 inmWas EWRAM_BSS;            // the inmates were set up on this lot (leaving the prison hands the places back to the neighbours)

static u8 inR(int j,int k){ u32 x=(u32)(j*97+k*31+11)*2654435761u; x^=x>>15; x*=2246822519u; return (u8)(x>>24); }   // a stable pseudo-random byte for inmate j, trait k
static int inmOut(int j){ return j&3; }   // the outfit: 0 jumpsuit, 1 stripes, 2 blues, 3 orange pants and a white tank

static void inmLook(int j,u8*lk,u8*stg){
    static const u8 shp[12]={0,1,3,4,5,6,17,18,19,20,21,22}, cut[6]={0,3,4,6,0,3};   // (the shapes hhRandLook uses) / CROP BALD SPIKY FLAT TOP
    int o=inmOut(j);
    for(int i=0;i<LK_N;i++) lk[i]=0;
    lk[LK_SHAPE]=shp[inR(j,3)%12]; lk[LK_SKIN]=(u8)(inR(j,4)%15); lk[LK_EYES]=(u8)(inR(j,5)%NEYE); lk[LK_MOUTH]=(u8)(inR(j,6)%NMOUTH);
    lk[LK_EARS]=(u8)(1+(inR(j,7)&1)); lk[LK_HSTYLE]=cut[inR(j,8)%6]; lk[LK_HCOL]=(u8)(inR(j,9)%NSW);
    lk[LK_BROW]=(u8)(inR(j,10)%6); lk[LK_EYECOL]=(u8)(inR(j,11)%NSW);
    lk[LK_GLASS]=(inR(j,12)%5==0)?1:0; lk[LK_BEARD]=(inR(j,13)%3==0)?(u8)(1+(inR(j,14)&1)):0;
    lk[LK_TOP]=0; lk[LK_BOT]=0; lk[LK_TOPSTY]=(u8)(o==3?2:1); lk[LK_BOTSTY]=0; lk[LK_SHOE]=(u8)((o==0||o==3)?1:2); lk[LK_HAT]=0; lk[LK_PATTERN]=0;
    *stg=(inR(j,15)%4==0)?AG_ELDER:AG_ADULT;
}
static void inmDress(int j){   // after buildLook: the OLD STRIPES outfit paints every other layer of the shirt and pants black (colour slot 3). The feet (layer 0) stay as the shoes
    if(inmOut(j)!=1) return;
    for(int y=1;y<H;y+=2)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x], c=v&15; if(c==6||c==7) vox[y][z][x]=(u8)((v&0xF0)|3); }
}
static void inmColors(int j){   // after setColors: the uniform colours (slot 6 the shirt, slot 7 the pants), shaded like every other colour of the creature
    static const u16 tc[4]={RGB(31,14,2),RGB(30,30,30),RGB(9,14,26),RGB(30,30,30)}, bc[4]={RGB(31,14,2),RGB(30,30,30),RGB(6,8,17),RGB(31,14,2)};
    int o=inmOut(j); base[6]=tc[o]; base[7]=bc[o];
    for(int i=6;i<8;i++){ sT[i]=base[i]; sL[i]=shade(base[i],12); sR[i]=shade(base[i],9); }
}
static void inmPick(void){   // hhBakeAll, before the visitors: who is in the prison (nobody anywhere else)
    if(!prHere()){ if(inmWas){ inmWas=0; inmN=0; for(int v=0;v<HH_MAX;v++){ inmPl[v]=0; hhKey[v]=0; } twKeep=0; } return; }   // (leaving: the neighbours get their places back)
    int n=HH_MAX-hhN; if(n>INM_WANT) n=INM_WANT; if(n<0) n=0;
    twKeep=1; for(int k=0;k<TW_N;k++){ twHas[k]=0; twOn[k]=0; }   // no neighbours drop by a prison
    if(inmWas&&n==inmN) return;
    inmWas=1; inmN=(u8)n;
    for(int v=0;v<HH_MAX;v++){ inmPl[v]=0; inmWt[v]=0; }
    for(int j=0;j<n;j++){ int v=HH_MAX-1-j; HhSim*s=&hhM[v]; u8 st;
        inmLook(j,s->look,&st); s->stage=st;
        { const char*nm=hhNames[(j*5+inR(j,1)%5)%24]; int i=0; for(;nm[i]&&i<HH_NM-1;i++) s->name[i]=nm[i]; s->name[i]=0; }
        { int id=1000+((int)inR(j,2)*37+j*811)%9000, d=1000; s->last[0]='#'; for(int i=1;i<5;i++){ s->last[i]=(char)('0'+(id/d)%10); d/=10; } s->last[5]=0; }
        for(int q=0;q<TR_N;q++) s->tr[q]=(u8)(inR(j,16+q)%11);
        s->uid=255; s->bubT=0; s->hp=HP_MAX; s->ltw=0; s->act=HA_IDLE; s->think=0; s->hd=(u8)(inR(j,20)&15); s->pn=s->pi=0; s->gok=0; s->item=0; s->tgt=0;
        for(int q=0;q<HN_N;q++) s->need[q]=80; }
}
static int inmIs(int m){ return m>=hhN&&m<HH_MAX&&inmPl[m]&&!curFl&&prHere(); }   // this place holds an inmate who is on the lot
static int inmWalk(int m){ const HhSim*s=&hhM[m]; return (s->act==HA_WALK&&s->pi<s->pn)?((lfr+m*5)>>3)&1:0; }   // the walking frame, out of step with each other
static int inmTaken(int x,int y,int self){ for(int j=0;j<inmN;j++){ int v=HH_MAX-1-j; if(v==self||!inmPl[v]) continue; if((int)(hhM[v].fx>>8)==x&&(int)(hhM[v].fy>>8)==y) return 1; } return 0; }
static void inmTick(int*planned){   // hhTick, on the ground floor
    if(!inmN||!inmWas||!prHere()) return;
    int x0,y0,x1,y1; nbRect(&nbT.lot[nbT.cur],&x0,&y0,&x1,&y1);
    int ry0=y0+7; if(y1-ry0<3) ry0=y0+1;   // the yard: below the cell block
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int j=0;j<inmN;j++){ int v=HH_MAX-1-j; if(v<hhN) continue; HhSim*s=&hhM[v];
        if(!inmPl[v]){   // first tick on the lot: a free tile of the yard, not next to you
            for(int t=0;t<120;t++){
                int x=x0+1+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(x1-x0-1)), y=ry0+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(y1-ry0));
                if(!hhWalk(x,y)||fxAbs(x-px)+fxAbs(y-py)<5||inmTaken(x,y,v)) continue;
                s->fx=x*256+128; s->fy=y*256+128; s->act=HA_IDLE; s->pn=s->pi=0; s->gok=0; inmWt[v]=(u16)(30+rnd8()*2); inmPl[v]=1; break; }
            continue; }
        if(s->act==HA_WALK){
            if(s->pi<s->pn){ hhStepAlong(s); continue; }
            s->act=HA_IDLE; inmWt[v]=(u16)(180+(rnd8()<<2)); continue; }   // there: stands 3 to 7 seconds
        if(inmWt[v]){ inmWt[v]--; continue; }
        { s32 dx=s->fx-lfx, dy=s->fy-lfy; if(((dx*dx+dy*dy)>>8)<=(380*380>>8)){ inmWt[v]=30; continue; } }   // you are talking to them: they stay put
        if(*planned) continue;
        *planned=1;
        if(hhPlan(s,0)>1) s->act=HA_WALK; else inmWt[v]=60;
    }
}

// inmates.h - INMATES: the prison has a population of voxel Sims in prison clothes (the same kind of character work as everyone else, baked like a household member, a hardware sprite on screen).
//
// POPULATION  OPTIONS > PLAY > BORES > INMATES: LOW 4, MEDIUM 8 (the default), HIGH 12 (inmPop, XO_INMATES). Read when you walk into the prison. More inmates than the household has places for:
//             an inmate is NOT a household member. Each one is an instance in inmS[INM_MAX] (an HhSim that walks with the household's own path code) and has its own sprite id (HH_MAX + j).
// LOOKS       An inmate only needs the BAKED SPRITES of a look. Those live in the free member places (a place past the household, or a member who is OUT because of the sentence: prHeld), up to HH_MAX of
//             them, so the sprite pool holds the same as a full household. inmSets looks are baked (one per free place, at most the population); inmate j wears look j % inmSets, so with 12 inmates and
//             7 places some of them are look-alikes. The names, the numbers ("#4821") and where they walk are their own. A household of 7 that is all at the prison leaves no place: no inmates then.
//             Looks are a hash of the number, so it is the same people every visit.
// ATTIRE      The creator's colour rows have no orange, so the uniform colours are set right after setColors (inmColors) and the pattern is painted into the voxels right after buildLook (inmDress).
//             0 ORANGE JUMPSUIT (orange long sleeve and pants, white shoes)     1 OLD STRIPES (white, a black stripe every other layer on the shirt and the pants, black shoes)
//             2 WORK BLUES (a blue shirt, darker blue pants, black shoes)       3 ORANGE PANTS + WHITE TANK (bare shoulders, white shoes)
// LIFE        They wander the compound, stop for 3 to 7 seconds, go on, and stand still while you talk to one. R next to one: TALK / JOKE / COMPLIMENT / HIGH FIVE (the neighbour menu, titled INMATE).
//             They are not in the relationship tables and do not fight yet.
// MEMORY      inmS: INM_MAX x about 280 bytes of EWRAM, plus about 100 bytes of flags, timers and screen positions. Nothing here is saved. Hardware: at most OBJ_SLOTS (8) Sims are drawn at once, the nearest.
static const u8 inmPop[3]={4,8,12};
static u8 inmPl[INM_MAX] EWRAM_BSS;     // 1 = this inmate stands on the lot (placed on its first tick)
static u16 inmWt[INM_MAX] EWRAM_BSS;    // steps to stand still before the next walk
static u8 inmWas EWRAM_BSS;             // the inmates were set up on this lot (leaving the prison hands the places back)

static u8 inR(int j,int k){ u32 x=(u32)(j*97+k*31+11)*2654435761u; x^=x>>15; x*=2246822519u; return (u8)(x>>24); }   // a stable pseudo-random byte for look / inmate j, trait k
static int inmOut(int k){ return k&3; }   // the outfit of look k: 0 jumpsuit, 1 stripes, 2 blues, 3 orange pants and a white tank

static void inmLook(int k,u8*lk,u8*stg){
    static const u8 shp[12]={0,1,3,4,5,6,17,18,19,20,21,22}, cut[6]={0,3,4,6,0,3};   // (the shapes hhRandLook uses) / CROP BALD SPIKY FLAT TOP
    int o=inmOut(k);
    for(int i=0;i<LK_N;i++) lk[i]=0;
    lk[LK_SHAPE]=shp[inR(k,3)%12]; lk[LK_SKIN]=(u8)(inR(k,4)%15); lk[LK_EYES]=(u8)(inR(k,5)%NEYE); lk[LK_MOUTH]=(u8)(inR(k,6)%NMOUTH);
    lk[LK_EARS]=(u8)(1+(inR(k,7)&1)); lk[LK_HSTYLE]=cut[inR(k,8)%6]; lk[LK_HCOL]=(u8)(inR(k,9)%NSW);
    lk[LK_BROW]=(u8)(inR(k,10)%6); lk[LK_EYECOL]=(u8)(inR(k,11)%NSW);
    lk[LK_GLASS]=(inR(k,12)%5==0)?1:0; lk[LK_BEARD]=(inR(k,13)%3==0)?(u8)(1+(inR(k,14)&1)):0;
    lk[LK_TOP]=0; lk[LK_BOT]=0; lk[LK_TOPSTY]=(u8)(o==3?2:1); lk[LK_BOTSTY]=0; lk[LK_SHOE]=(u8)((o==0||o==3)?1:2); lk[LK_HAT]=0; lk[LK_PATTERN]=0;
    *stg=(inR(k,15)%4==0)?AG_ELDER:AG_ADULT;
}
static void inmDress(int k){   // after buildLook: the OLD STRIPES outfit paints every other layer of the shirt and pants black (colour slot 3). The feet (layer 0) stay as the shoes
    if(inmOut(k)!=1) return;
    for(int y=1;y<H;y+=2)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x], c=v&15; if(c==6||c==7) vox[y][z][x]=(u8)((v&0xF0)|3); }
}
static void inmColors(int k){   // after setColors: the uniform colours (slot 6 the shirt, slot 7 the pants), shaded like every other colour of the creature
    static const u16 tc[4]={RGB(31,14,2),RGB(30,30,30),RGB(9,14,26),RGB(30,30,30)}, bc[4]={RGB(31,14,2),RGB(30,30,30),RGB(6,8,17),RGB(31,14,2)};
    int o=inmOut(k); base[6]=tc[o]; base[7]=bc[o];
    for(int i=6;i<8;i++){ sT[i]=base[i]; sL[i]=shade(base[i],12); sR[i]=shade(base[i],9); }
}
static int inmSetOf(int id){ if(id<HH_MAX) return id; return inmSets?inmSetPl[(id-HH_MAX)%inmSets]:0; }   // the place whose baked sprites this sprite id shows
static int inmOn(int j){ return j>=0&&j<inmN&&inmPl[j]&&!curFl&&prHere(); }   // inmate j is on the lot
static int inmWalk(int j){ const HhSim*s=&inmS[j]; return (s->act==HA_WALK&&s->pi<s->pn)?((lfr+j*5)>>3)&1:0; }   // the walking frame, out of step with each other
static void inmPick(void){   // hhBakeAll, before the visitors: who is in the prison (nobody anywhere else)
    if(!prHere()){ if(inmWas){ inmWas=0; inmN=0; inmSets=0; for(int v=0;v<HH_MAX;v++) hhKey[v]=0; for(int j=0;j<INM_MAX;j++) inmPl[j]=0; twKeep=0; } return; }   // (leaving: the places get their own sprites back)
    twKeep=1; for(int k=0;k<TW_N;k++){ twHas[k]=0; twOn[k]=0; }   // no neighbours drop by a prison
    int sp[HH_MAX], ns=0; for(int v=HH_MAX-1;v>=0;v--) if(v>=hhN||prHeld(&hhM[v])) sp[ns++]=v;   // the places whose sprites are free to use
    int pop=inmPop[xo[XO_INMATES]%3]; if(!ns) pop=0;
    int sets=ns<pop?ns:pop;
    if(inmWas&&pop==inmN&&sets==inmSets){ int same=1; for(int k=0;k<sets;k++) if(inmSetPl[k]!=sp[k]) same=0; if(same) return; }
    inmWas=1; inmN=(u8)pop; inmSets=(u8)sets; for(int k=0;k<sets;k++) inmSetPl[k]=(u8)sp[k];
    for(int j=0;j<INM_MAX;j++){ inmPl[j]=0; inmWt[j]=0; }
    for(int j=0;j<pop;j++){ HhSim*s=&inmS[j]; u8 st; int k=j%sets;
        inmLook(k,s->look,&st); s->stage=st;
        { const char*nm=hhNames[(j*2+inR(j,1)%2)%24]; int i=0; for(;nm[i]&&i<HH_NM-1;i++) s->name[i]=nm[i]; s->name[i]=0; }
        { int id=1000+((int)inR(j,2)*37+j*811)%9000, d=1000; s->last[0]='#'; for(int i=1;i<5;i++){ s->last[i]=(char)('0'+(id/d)%10); d/=10; } s->last[5]=0; }
        for(int q=0;q<TR_N;q++) s->tr[q]=(u8)(inR(j,16+q)%11);
        s->uid=255; s->bubT=0; s->hp=HP_MAX; s->ltw=0; s->act=HA_IDLE; s->think=0; s->hd=(u8)(inR(j,20)&15); s->pn=s->pi=0; s->gok=0; s->item=0; s->tgt=0;
        for(int q=0;q<HN_N;q++) s->need[q]=80; }
}
static void inmCalc(void){   // once a picture (main.c, next to hhCalc): where each inmate is on the screen
    if(curFl||!inmN) return;
    for(int j=0;j<inmN;j++){ if(!inmOn(j)) continue; const HhSim*s=&inmS[j]; s32 rx,ry; rotPos(s->fx,s->fy,&rx,&ry);
        inX[j]=(short)(LOX+(int)((rx-ry)>>5)); inY[j]=(short)(LOY+(int)((rx+ry)>>6)); inB[j]=(short)((rx>>8)+(ry>>8)); inV[j]=(u8)faceView[(s->hd+4*cview)&15]; inH[j]=(short)surfH(s->fx,s->fy); }
}
static int inmNear(void){   // R: the nearest inmate within 1.5 tiles, or -1
    if(xo[XO_MULTIFL]?pkHome>=0:curFl) return -1;
    int best=-1, bd=1<<30;
    for(int j=0;j<inmN;j++){ if(!inmOn(j)) continue; s32 dx=inmS[j].fx-lfx, dy=inmS[j].fy-lfy; int d=(int)((dx*dx+dy*dy)>>8); if(d<bd){ bd=d; best=j; } }
    return bd<=(380*380>>8)?best:-1;
}
static int inmTaken(int x,int y,int self){ for(int j=0;j<inmN;j++){ if(j==self||!inmPl[j]) continue; if((int)(inmS[j].fx>>8)==x&&(int)(inmS[j].fy>>8)==y) return 1; } return 0; }
static void inmTick(int*planned){   // hhTick, on the ground floor
    if(!inmN||!inmWas||!prHere()) return;
    int x0,y0,x1,y1; nbRect(&nbT.lot[nbT.cur],&x0,&y0,&x1,&y1);
    int ry0=y0+7; if(y1-ry0<3) ry0=y0+1;   // the yard: below the cell block
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int j=0;j<inmN;j++){ HhSim*s=&inmS[j];
        if(!inmPl[j]){   // first tick on the lot: a free tile of the yard, not next to you
            for(int t=0;t<120;t++){
                int x=x0+1+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(x1-x0-1)), y=ry0+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(y1-ry0));
                if(!hhWalk(x,y)||fxAbs(x-px)+fxAbs(y-py)<5||inmTaken(x,y,j)) continue;
                s->fx=x*256+128; s->fy=y*256+128; s->act=HA_IDLE; s->pn=s->pi=0; s->gok=0; inmWt[j]=(u16)(30+rnd8()*2); inmPl[j]=1; break; }
            continue; }
        if(s->act==HA_WALK){
            if(s->pi<s->pn){ hhStepAlong(s); continue; }
            s->act=HA_IDLE; inmWt[j]=(u16)(180+(rnd8()<<2)); continue; }   // there: stands 3 to 7 seconds
        if(inmWt[j]){ inmWt[j]--; continue; }
        { s32 dx=s->fx-lfx, dy=s->fy-lfy; if(((dx*dx+dy*dy)>>8)<=(380*380>>8)){ inmWt[j]=30; continue; } }   // you are talking to them: they stay put
        if(*planned) continue;
        *planned=1;
        if(hhPlan(s,0)>1) s->act=HA_WALK; else inmWt[j]=60;
    }
}

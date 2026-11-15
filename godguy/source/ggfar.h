// ggfar.h -- GodGuy step 10d: tiers and distant battles.
//
// The rotation in simulate() gives every creature a visit once in about 94 ticks at 12,000 creatures, and a creature acts only when its speed counter fills, so a war off screen crawled:
// two armies of 6,000 did not touch in two game years. This step keeps the cost of a tick where it was and makes the wars follow the pace of the full per-unit run, by thinking in tiers:
//   Tier A, actors:  everyone in the camera window thinks at their own pace every tick (the window is walked once a tick, as the glide loop already does).
//   Tier B, people:  the rotation, as before, for everyone else.
//   Tier C, crowd:   a war group (ggwar.h) whose centre is away from the camera. Its soldiers stay ordinary creatures; at each rotation visit they take as many steps towards the enemy village as
//                    the time since their last visit allows (farStride). When two far armies touch, the battle is ONE roll a tick for the pair (farTick): the blows the front line lands in a tick
//                    (frontage x blow / health, from the averages wgBuild keeps) are turned into kills among the other army's soldiers, picked from the group's member list. A notable picked to die
//                    gets its own survival roll (a coin; the survivor is left at a third of its health and scarred). Whoever kills gets the experience, so levels, records and the journal go on.
// Only crowds of more than FAR_MIN creatures use the tiers, so small worlds play exactly as before. Nothing here is stored in a save: the groups are counted again from the creatures when a game
// is saved and when it is loaded (saveGameRaw and loadGame call wgBuild after compact), so a loaded game goes on exactly as the one that was saved. No floating point, one divide per army a tick.
// -DGG_NOTIER builds the game as it was before this step (the benchmark's "old" side).
#ifndef GGFAR_H
#define GGFAR_H
#ifndef FAR_FRONT
#define FAR_FRONT 1        // fighters in the battle line of an army: this many for every tile of the square root of its size
#endif
#ifndef FAR_CONTACT
#define FAR_CONTACT 2      // armies touch when their centres are closer than two thirds of (root of a size + root of the other) plus this
#endif
#define FAR_MIN 600        // tiers are for crowds larger than this
#ifndef FAR_MARGIN
#define FAR_MARGIN 0       // tiles round the camera window that still count as near
#endif
#define FAR_STRIDE 12      // the most steps a soldier takes in one visit

static u8 wgSt[VIL_N];     // per group, this tick: bit 0 far from the camera, bit 1 touching an enemy army
static u8 farPair[VIL_N][2]; static int farNP;
static int farOn(void){
#if defined(GG_NOTIER)||defined(GG_FULLTURNS)
    return 0;
#else
    return nCr>FAR_MIN&&wgNc;
#endif
}
static int wgRolling(void){      // 10f: the count every 256 ticks is the cheap one (wgCommit) when the tiers are in use and the lists survived compact(); otherwise it is the exact pass (wgBuild)
#if defined(GG_NOTIER)||defined(GG_FULLTURNS)||defined(GG_NOROLL)
    return 0;
#else
    return nCr>FAR_MIN&&wgKeep>0;
#endif
}
static int isq(int v){ int r=0, b=1<<14; while(b>v) b>>=2; while(b){ if(v>=r+b){ v-=r+b; r=(r>>1)+b; } else r>>=1; b>>=2; } return r; }
static int inWin(int x,int y){ int wx=(camX>>3)-1, wy=(camY>>3)-1; return x>=wx&&x<wx+35&&y>=wy&&y<wy+27; }
static int farTile(int x,int y){ int wx=(camX>>3)-1, wy=(camY>>3)-1; return x<wx-FAR_MARGIN||x>=wx+35+FAR_MARGIN||y<wy-FAR_MARGIN||y>=wy+27+FAR_MARGIN; }

static int farKills(int a,int d){      // how many of d the line of a kills this tick: front x (blow - armour) / (pace x health), rounded by one roll
    int cnt=wg[a].cnt; if(!cnt||!wg[d].cnt) return 0; int eng=FAR_FRONT*isq(cnt); if(eng>cnt) eng=cnt;
    int dm=(int)wga[a].atk-(int)wga[d].def; if(dm<1) dm=1; int spd=wga[a].spd<3?3:wga[a].spd, hp=wga[d].hp<1?1:wga[d].hp;
    unsigned e=(unsigned)eng*(unsigned)dm*256u/((unsigned)spd*(unsigned)hp); int k=(int)(e>>8); if((int)(rnd()&255)<(int)(e&255)) k++; return k; }
static int farPick(int g){      // a living member of group g, chosen at random; the dead found on the way are dropped from the list. -1 if none is left
    occ_t*m=&wgMem[wgOff[g]];
    while(wgLen[g]){ u32 r=rnd()%wgLen[g]; int ci=(int)m[r]; if(ci<nCr&&cr[ci].alive) return ci; m[r]=m[--wgLen[g]]; }
    return -1; }
static void farApply(int d,int a,int k){      // k of group d fall to group a
    WarGrp*G=&wg[d]; for(int q=0;q<k&&G->cnt;q++){
        int j=farPick(d); if(j<0){ G->cnt=0; return; }
        if((crFl[j]&FL_NOTE)&&(rnd()&1)){ int mh=maxHp(&cr[j]); cr[j].hp=(u8)(mh/3<1?1:mh/3); crFl[j]|=FL_FIGHT; if(!(crFl[j]&FL_SCAR)){ crFl[j]|=FL_SCAR; int nk=nbFind(crId[j]); if(nk>=0&&nb[nk].wounds<255) nb[nk].wounds++; } continue; }      // a notable's own survival roll
        killCr(j); G->cnt--; int ia=farPick(a); if(ia>=0) gainXp(&cr[ia]); } }
static void farTick(void){
    if(!farOn()) return; farNP=0;
    for(int v=0;v<VIL_N;v++) wgSt[v]=(u8)(wg[v].cnt&&farTile(wg[v].cx,wg[v].cy));
    for(int v=0;v<VIL_N;v++){ if(!(wgSt[v]&1)) continue; WarGrp*g=&wg[v]; int f=g->tgt-1;
        if(f>=0&&wg[f].cnt&&(wgSt[f]&1)){ int dx=(int)g->cx-(int)wg[f].cx, dy=(int)g->cy-(int)wg[f].cy; int gap=(dx<0?-dx:dx)+(dy<0?-dy:dy); int cg=(isq(g->cnt)+isq(wg[f].cnt))*2/3+FAR_CONTACT;
            if(gap<=cg){ wgSt[v]|=2; wgSt[f]|=2; if(!(f<v&&wg[f].tgt-1==v)&&farNP<VIL_N){ farPair[farNP][0]=(u8)v; farPair[farNP][1]=(u8)f; farNP++; } continue; } }
        int spd=wga[v].spd<3?3:wga[v].spd; if(tick%spd==0&&wgDist(g)>3){ int dx=(int)g->tx-(int)g->cx, dy=(int)g->ty-(int)g->cy; if((dx<0?-dx:dx)>=(dy<0?-dy:dy)) g->cx=(u16)((int)g->cx+(dx>0?1:-1)); else g->cy=(u16)((int)g->cy+(dy>0?1:-1)); } }      // an army on the march: its centre keeps pace with its soldiers' strides
    for(int p=0;p<farNP;p++){ int v=farPair[p][0], f=farPair[p][1]; int kv=farKills(v,f), kf=farKills(f,v); farApply(f,v,kv); farApply(v,f,kf); } }
static void farStride(int i,int period){      // a far soldier's visit: the steps it would have taken since the last one
    if(i>=wgNc) return; int h=wgOf[i]; if(!h) return; h--; if(wgSt[h]!=1) return; Cr*c=&cr[i]; if(!c->alive||c->hp<=8) return; int k=period/effSpd(c); if(k>FAR_STRIDE) k=FAR_STRIDE;
    const Vil*F=&vil[wg[h].tgt-1]; const Sp*s=&sp[c->sp]; for(int q=0;q<k;q++){ int dx=F->x-c->x, dy=F->y-c->y; if(dx*dx+dy*dy<=4) break; if(!stepToward(c,i,s,F->x,F->y)) break; } }
static int winIdx[1024];
static void windowThink(void){      // tier A: whoever is in the camera window thinks at their own pace, every tick
    int x0=(camX>>3)-1, y0=(camY>>3)-1, m=0; for(int y=y0<0?0:y0;y<y0+27&&y<WH;y++) for(int x=x0<0?0:x0;x<x0+35&&x<WW;x++){ int j=occ[y][x]-1; if(j>=0&&m<1024) winIdx[m++]=j; }
    for(int q=0;q<m;q++){ Cr*c=&cr[winIdx[q]]; if(c->alive&&++c->t>=effSpd(c)){ c->t=0; think(winIdx[q]); } } }
#endif

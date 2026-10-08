// PANIC: when something scary happens (a gun is fired, a rocket goes off, a Sim is shot) the Sims close by drop what they are doing and RUN away from it for a few
// seconds: the stride doubles (hhStepAlong twice a step), so their legs run (hhLegs, house.h). Needs FREE WILL on. A knocked out Sim stays down.
// hhScare(tx, ty, radius, steps): tile of the scare, how far it reaches (tiles), how long they run (steps).
static u8 hhPanT[HH_MAX], hhPanX[HH_MAX], hhPanY[HH_MAX];
static int hhFleePlan(HhSim*s,int tx,int ty){   // a path to the free tile the best way off from (tx, ty): far from it, not far to walk
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if(!hhWalk(sx,sy)) return 0;
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    hhTakenScan(s);
    int qh=0, qt=0, goal=-1, best=-100000, st=sy*MW+sx;
    hhDist[st]=0; hhQ[qt++]=(u16)st;
    while(qh<qt){ int p=hhQ[qh++], x=p%MW, y=p/MW;
        int dx=x-tx, dy=y-ty; int sc=((dx<0?-dx:dx)+(dy<0?-dy:dy))*4-(int)hhDist[p]; if(p!=st&&sc>best&&!hhTakenAt(x,y)){ best=sc; goal=p; }
        if(hhDist[p]>=24) continue;   // (a short dash: the next plan carries on)
        for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d], np=ny*MW+nx; if(hhWalk(nx,ny)&&hhDist[np]==0xFFFF){ hhDist[np]=(u16)(hhDist[p]+1); hhQ[qt++]=(u16)np; } } }
    if(goal<0) return 0;
    int n=hhDist[goal], p=goal; s->pn=(u8)n; s->pi=0; s->gok=0;
    for(int k=n-1;k>=0;k--){ int x=p%MW, y=p/MW;
        for(int d=0;d<4;d++){ int px=x-hhDx[d], py=y-hhDy[d], pp=py*MW+px; if(px>=0&&py>=0&&px<MW&&py<MH&&hhDist[pp]==hhDist[p]-1){ s->path[k]=(u8)d; p=pp; break; } } }
    return n+1;
}
static void hhScare(int tx,int ty,int rad,int steps){
    if(!xo[XO_FREEWILL]||curFl) return;
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; if(s->act==HA_AWAY||(s->act==HA_SOC&&s->bub==IC_SKULL)) continue;   // (away, or knocked out)
        int dx=(int)(s->fx>>8)-tx, dy=(int)(s->fy>>8)-ty; if((dx<0?-dx:dx)+(dy<0?-dy:dy)>rad) continue;
        if(!hhPanT[m]){ s->bub=IC_BAIL; s->bubT=50; }
        hhPanT[m]=(u8)(steps>255?255:steps); hhPanX[m]=(u8)tx; hhPanY[m]=(u8)ty; }
}
static void hhPanicStep(int m,HhSim*s,int*planned){   // one logic step of a Sim that is running away
    if(hhPanT[m]) hhPanT[m]--;
    int walking=(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK||s->act==HA_LEAVE||s->act==HA_STAIR)&&s->pi<s->pn;
    if(!walking&&!*planned){ *planned=1; if(hhFleePlan(s,hhPanX[m],hhPanY[m])>1){ s->act=HA_WANDER; s->use=HN_FUN; s->item=0; walking=1; } else s->act=HA_IDLE; }
    if(walking){ hhStepAlong(s); if(s->pi<s->pn) hhStepAlong(s); }   // twice the speed: running
}

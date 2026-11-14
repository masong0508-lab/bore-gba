// PC test of GodGuy step 10c, war groups (source/ggwar.h): one record for an army (target village, soldiers, power sum, formation centre), counted from the soldiers.
// An independent count from the definition must give the same groups at every check through years of play; the groups must be sane (a target, a centre inside the soldiers' bounding box,
// armies that oppose each other pointing at each other's villages); counting changes nothing in the game.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ fails++; printf("  FAIL: %s\n",msg); } }while(0)
static void setup(unsigned seed,int size){ planetSize=size; rs=seed; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; monOn=1; }
static u32 stateHash(void){ u32 h=2166136261u; const u8*p;
#define H(ptr,n) p=(const u8*)(ptr); for(size_t q=0;q<(size_t)(n);q++) h=(h^p[q])*16777619u;
    for(int y=0;y<gH;y++){ H(world[y],gW); } H(cr,nCr*sizeof(Cr)); H(crId,nCr*sizeof(u32)); H(crFl,nCr*sizeof(u16)); H(vil,sizeof vil); H(&nCr,sizeof nCr); H(&year,sizeof year); return h; }
typedef struct { int cnt, power, sx, sy, minx, maxx, miny, maxy; } Ref;
static int refHome(const Cr*c){ int best=-1,bd=14*14+1; int race=arch[sp[c->sp].a].diet>=0?sp[c->sp].a:0; for(int v=0;v<VIL_N;v++){ if(!vil[v].on||vil[v].race!=race) continue; int dx=vil[v].x-c->x, dy=vil[v].y-c->y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
static int check(int*soldiersTotal,int*maxCnt,int*maxPow,int*opp){      // returns the number of disagreements
    static Ref R[VIL_N]; memset(R,0,sizeof R); for(int v=0;v<VIL_N;v++){ R[v].minx=R[v].miny=1<<20; R[v].maxx=R[v].maxy=-1; } int soldiers=0;
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=4) continue; if(c->age<=6||c->hp<=8) continue; int h=refHome(c); if(h<0) continue; if(!vil[h].foe||!vil[vil[h].foe-1].on) continue;
        Ref*r=&R[h]; r->cnt++; r->power+=1+c->dir+(c->rk?3:0); r->sx+=c->x; r->sy+=c->y; if(c->x<r->minx) r->minx=c->x; if(c->x>r->maxx) r->maxx=c->x; if(c->y<r->miny) r->miny=c->y; if(c->y>r->maxy) r->maxy=c->y; soldiers++; }
    wgBuild(); int bad=0, groups=0, sum=0; *opp=0;
    for(int v=0;v<VIL_N;v++){ const WarGrp*g=&wg[v]; if(g->cnt!=R[v].cnt){ bad++; continue; } if(!g->cnt){ if(g->tgt||g->power) bad++; continue; } groups++; sum+=g->cnt; if(g->cnt>*maxCnt) *maxCnt=g->cnt; if(g->power>*maxPow) *maxPow=g->power;
        if(g->power!=R[v].power) bad++; if(g->tgt!=vil[v].foe) bad++; if(g->cx!=R[v].sx/R[v].cnt||g->cy!=R[v].sy/R[v].cnt) bad++;
        if((int)g->cx<R[v].minx||(int)g->cx>R[v].maxx||(int)g->cy<R[v].miny||(int)g->cy>R[v].maxy) bad++; if(g->tx!=vil[g->tgt-1].x||g->ty!=vil[g->tgt-1].y) bad++; if(wgDist(g)<0) bad++;
        int f=wgOpposing(v); if(f>=0){ (*opp)++; if(!wg[f].cnt||wgGap(v)<0) bad++; } else if(wgGap(v)!=-1) bad++; }
    if(groups!=wgN||sum!=soldiers) bad++; *soldiersTotal+=soldiers; return bad; }
int main(void){
    int checks=0, bad=0, withGroups=0, soldiers=0, maxCnt=0, maxPow=0, opposing=0, hashBad=0;
    for(int size=128;size<=192;size+=64){ setup(4242u+size,size);
        for(int t=1;t<=4*2048;t++){ tick=t; simulate(); if((t&255)==128){ checks++; u32 h0=stateHash(); int o=0; bad+=check(&soldiers,&maxCnt,&maxPow,&o); opposing+=o; if(stateHash()!=h0) hashBad++; if(wgN>0) withGroups++; } } }
    printf("war groups: %d checks, %d with a group, %d disagreements; biggest group %d soldiers, power %d; %d army-against-army pairs seen; %d soldiers counted\n",checks,withGroups,bad,maxCnt,maxPow,opposing,soldiers);
    CHK(bad==0,"the groups equal an independent count from the definition"); CHK(hashBad==0,"counting changes nothing in the game"); CHK(withGroups>=3,"wars produced armies to count");
    // a made-up war: every village at war with the next, soldiers placed round the first: the group records it
    setup(11u,128); for(int t=1;t<=2*2048;t++){ tick=t; simulate(); } int a=-1,b=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on){ if(a<0) a=v; else if(b<0) b=v; }
    if(a>=0&&b>=0){ vil[a].foe=(u8)(b+1); vil[b].foe=(u8)(a+1); int o=0,sd=0,mc=0,mp=0; int bad2=check(&sd,&mc,&mp,&o); printf("made-up war between two villages: %d soldiers, %d disagreements, opposing pairs %d\n",sd,bad2,o); CHK(bad2==0,"the groups of a made-up war equal the count"); }
    printf(fails?"WARGRP FAILED (%d)\n":"WARGRP OK\n",fails); return fails?1:0; }

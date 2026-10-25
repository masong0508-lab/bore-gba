// kingfx.h - EP6 THE LAND BOREFORE TIME, Phase 6: the LIFE on the map. PURE C (needs kingdom.h, kingsim.h, kingview.h, kinggiant.h before it; no hardware).
// It plans the sprites of one frame and returns how many it wrote (at most KF_MAX; three OAM words a sprite: attribute 0, 1, 2). The plan holds EVERYTHING that is drawn over the map, the giant too:
//   - the CARRIERS walking the roads with what they carry above their heads (they glide from cell to cell at normal speed), a WORKER busy at the door of each working building, SMOKE from chimneys,
//   - THE PEOPLE: every Nubbin the realm counts (S->pop, up to KS_POPCAP) is somewhere. The carriers and the workers are drawn one at a time; the rest (the idle ones) are shared out: the ones at the TAVERN
//     from dusk, the COURT at the crowned castle's gate by day, the rest in front of their HOMES (a few at night: the others are indoors) and the founders at their camp. A place with 1 to 3 Nubbins shows them one by one,
//     pacing; more are made up of THRONGS (26 faces, up to three), a CROWD (13), GROUPS (5) and singles (kfCrowd): one OAM entry for 26 faces, so a realm of hundreds costs a few dozen sprites and
//     every Nubbin of the count is still on the map, in its place, at any size of realm (the sprite budget does not grow with the people),
//   - the CURSOR on the cell under the giant's feet, what the GIANT holds above its head, and the giant itself (so a Nubbin in front of its feet is drawn over it).
// The list is sorted so the one lower on the screen (nearer) is drawn on top (a lower OAM entry wins); the cursor goes last (under all).
// OBJ tiles (1D mapping, the art is kingart.h): giants 0..287 (9 frames of 32), Nubbins 288.. (6 frames of 2), goods 300..307, smoke 308..311, cursor 312..315, crowds 316..371 (two frames of: group 4 tiles,
// crowd 8, throng 16). OBJ palette banks: 0 the giant, 1..4 the Nubbins' clothes, 5 goods, 6 smoke, 7 the cursor, 8 the crowds.
#ifndef KINGFX_H
#define KINGFX_H
#define KF_MAX 120           // sprites a plan may hold (the OAM has 128)
#define KF_NUB 288
#define KF_GOOD 300
#define KF_SMOKE 308
#define KF_CURSOR 312
#define KF_CROWD 316
#define KF_GROUP_AT(f)  (KF_CROWD+(f)*28)          // the tiles of frame f (0 or 1) of the group (16 x 16), the crowd (32 x 16) and the throng (32 x 32)
#define KF_CROWD_AT(f)  (KF_CROWD+(f)*28+4)
#define KF_THRONG_AT(f) (KF_CROWD+(f)*28+12)
#define KF_THRONG 26         // faces in a throng
typedef struct { u16 prev[KS_MAXW], cur[KS_MAXW]; u8 age[KS_MAXW]; } KFx;   // per carrier: the cell it came from, the cell it is on, frames since it arrived
static inline void kfInit(KFx*F){ for(int i=0;i<KS_MAXW;i++){ F->prev[i]=F->cur[i]=0xFFFF; F->age[i]=255; } }
// once a frame, after the simulation ran: who stands where
static inline void kfTrack(KFx*F,const KSim*S){
    for(int i=0;i<KS_MAXW;i++){ const KsWk*w=&S->wk[i];
        if(!w->st){ F->cur[i]=F->prev[i]=0xFFFF; F->age[i]=255; continue; }
        if(F->cur[i]==0xFFFF){ F->cur[i]=F->prev[i]=w->cell; F->age[i]=255; }
        else if(w->cell!=F->cur[i]){ F->prev[i]=F->cur[i]; F->cur[i]=w->cell; F->age[i]=0; }
        else if(F->age[i]<255) F->age[i]++; }
}
static inline unsigned kfPut(u16*o,int n,int x,int y,int shape,int size,int flip,int tile,int prio,int bank){   // one sprite: its three words (position wraps; the caller checked it is on the screen)
    o[n*3]=(u16)((y&255)|(shape<<14)); o[n*3+1]=(u16)((x&511)|(flip?0x1000:0)|(size<<14)); o[n*3+2]=(u16)(tile|(prio<<10)|(bank<<12)); return (unsigned)(n+1); }
static inline int kfOn(const KView*V,int wx,int wy,int w,int h){ int x=wx-V->x, y=wy-V->y; return x>-w&&x<240&&y>-h&&y<160; }
// one Nubbin standing with its feet at world pixel (px,py): view 0 side, 1 down, 2 up; frame 0 or 1; look 0..3 (the clothes)
static inline int kfNub(u16*o,int n,const KView*V,int px,int py,int view,int frame,int flip,int look){
    if(n>=KF_MAX||!kfOn(V,px-4,py-15,8,16)) return n;
    return (int)kfPut(o,n,px-4-V->x,py-15-V->y,2,0,flip,KF_NUB+(view*2+frame)*2,1,1+(look&3)); }
static inline int kfIcon(u16*o,int n,const KView*V,int px,int py,int tile,int bank){   // an 8 x 8 icon with its top left at world pixel (px,py)
    if(n>=KF_MAX||!kfOn(V,px,py,8,8)) return n;
    return (int)kfPut(o,n,px-V->x,py-V->y,0,0,0,tile,1,bank); }
// ---- the people ----
// a place holding `count` Nubbins, the middle of their feet at world pixel (px,py). seed: which place (so their clothes and their pacing differ from the next place's).
// The count is drawn EXACTLY (to within one face): as many THRONGS (26 faces, one OAM entry) as fit, then a CROWD (13), GROUPS (5) and single Nubbins for the rest, side by side, so 8 Nubbins show as a group of
// five and three singles, and 300 show as three throngs: the sprites a place needs never pass 7, and no place holds more than three throngs (78: what the tavern or the court's yard can hold).
static inline int kfCrowd(u16*o,int n,const KView*V,int px,int py,int count,unsigned tick,unsigned seed){
    if(count<=0) return n;
    int t=count/KF_THRONG, r; if(t>3){ t=3; r=0; } else r=count-t*KF_THRONG;
    int c=0, g=0, s=0; if(r>=13){ c=1; r-=13; } g=r/5; s=r-g*5; if(s==4){ g++; s=0; }          // (four singles are one group, a face over)
    int units=t+c+g+s, wide=t*30+c*30+g*15+s*8; int x=px-wide/2, k=0; unsigned fr=((tick>>4)+seed)&1;
    for(int u=0;u<units;u++,k++){ int y=py+((k&1)?1:0)-((u%3==2)?1:0);
        if(u<t){ if(n+1>KF_MAX) break; if(kfOn(V,x,y-28,32,32)) n=(int)kfPut(o,n,x-V->x,y-28-V->y,0,2,0,KF_THRONG_AT((fr+(unsigned)u)&1u),1,8); x+=30; }
        else if(u<t+c){ if(n+1>KF_MAX) break; if(kfOn(V,x,y-14,32,16)) n=(int)kfPut(o,n,x-V->x,y-14-V->y,1,2,0,KF_CROWD_AT(fr),1,8); x+=30; }
        else if(u<t+c+g){ if(n+1>KF_MAX) break; if(kfOn(V,x,y-14,16,16)) n=(int)kfPut(o,n,x-V->x,y-14-V->y,0,1,0,KF_GROUP_AT((fr+(unsigned)u)&1u),1,8); x+=15; }
        else{ unsigned ph=((tick>>2)+(seed+(unsigned)u)*13u)&31u; int tri=(int)(ph<16u?ph:31u-ph);   // a single one paces a few pixels to and fro (a triangle wave)
            n=kfNub(o,n,V,x+4+(tri-8)/3,y,0,(int)((tick>>3)&1u),ph>=16u,(int)(seed+(unsigned)u)); x+=8; } }
    return n;
}
// the heights of an OBJ by shape (0 square, 1 wide, 2 tall) and size
static inline int kfH(unsigned a0,unsigned a1){ static const unsigned char h[3][4]={ {8,16,32,64}, {8,8,16,32}, {16,32,32,64} }; unsigned s=a0>>14; return s>2?8:h[s][a1>>14]; }
static inline int kfFeet(const u16*o,int i){ int y=o[i*3]&255; if(y>=192) y-=256; return y+kfH(o[i*3],o[i*3+1]); }   // the screen row of the bottom of sprite i
static inline void kfSort(u16*o,int lo,int n){   // insertion sort, stable: the lowest on the screen first (a lower OAM entry is drawn on top)
    for(int i=lo+1;i<n;i++){ u16 a=o[i*3],b=o[i*3+1],c=o[i*3+2]; int key=kfFeet(o,i), j=i-1;
        while(j>=lo&&kfFeet(o,j)<key){ o[(j+1)*3]=o[j*3]; o[(j+1)*3+1]=o[j*3+1]; o[(j+1)*3+2]=o[j*3+2]; j--; }
        o[(j+1)*3]=a; o[(j+1)*3+1]=b; o[(j+1)*3+2]=c; }
}
// how the day stands: 0 dawn, 1 morning, 2 afternoon, 3 dusk, 4 night (a fifth of the day each, the same fifths as the light of the day, kvDayTint)
static inline int kfPart(const KSim*S){ return (int)((S->t%KS_DAYT)*5u/KS_DAYT); }
// ---- the whole plan. tick: a frame counter. smooth: 1 at normal speed (carriers glide), 0 when the realm runs faster (they jump from cell to cell). ----
static inline int kfPlan(u16*o,const KFx*F,const KSim*S,const KView*V,const KGiant*G,unsigned tick,int smooth){
    int n=0, d=S->dim, part=kfPart(S), night=part==4;
    // what the giant holds
    { int hk=S->hd.k;
      if(hk==KHD_GOODS&&S->hd.g<8) n=kfIcon(o,n,V,G->x-4,G->y-56,KF_GOOD+S->hd.g,5);
      else if(hk==KHD_BLD) n=kfIcon(o,n,V,G->x-4,G->y-56,KF_GOOD+5,5);
      else if(hk==KHD_NUB) n=kfNub(o,n,V,G->x,G->y-40,1,(int)((tick>>3)&1),0,S->hd.bt?1:0); }
    // the giant itself (it goes in the sorted list: a Nubbin in front of its feet is drawn over it)
    { unsigned a0,a1,a2; kgOam(G,V,&a0,&a1,&a2); if(a0!=0x200u){ o[n*3]=(u16)a0; o[n*3+1]=(u16)a1; o[n*3+2]=(u16)a2; n++; } }
    // the carriers, with their loads
    int active=0;
    for(int i=0;i<KS_MAXW;i++){ const KsWk*w=&S->wk[i]; if(!w->st||F->cur[i]==0xFFFF) continue; active++;
        if(n>=KF_MAX-3) continue;
        int cx=F->cur[i]%d, cy=F->cur[i]/d, px=cx*KV_CELL+8, py=cy*KV_CELL+13, view=0, flip=0;
        int fx=F->prev[i]%d, fy=F->prev[i]/d; int ddx=cx-fx, ddy=cy-fy;
        if(smooth&&F->age[i]<KS_WALK){ int t=F->age[i]+1; px=(fx*KV_CELL+8)*(KS_WALK-t)/KS_WALK+px*t/KS_WALK; py=(fy*KV_CELL+13)*(KS_WALK-t)/KS_WALK+py*t/KS_WALK; }
        if(ddx>0) { view=0; flip=0; } else if(ddx<0) { view=0; flip=1; } else if(ddy>0) view=1; else if(ddy<0) view=2;
        n=kfNub(o,n,V,px,py,view,(int)(((tick>>2)+(unsigned)i)&1),flip,i);
        if(w->good<8) n=kfIcon(o,n,V,px-4,py-23,KF_GOOD+w->good,5); }
    // how many are where: the workers are drawn at their doors; the idle ones (everyone the realm counts that is neither at work nor carrying) are shared out
    int homes=0, taverns=0, court=-1, camp=-1; unsigned workers=0;
    for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(!b->lvl) continue; const KsType*T=ksT(b->type);
        if(T->kind==KK_HOME) homes+=b->work; else if(b->type==KB_TAVERN) taverns++; else if(T->kind==KK_CASTLE) court=i; else if(T->kind==KK_STORE&&camp<0) camp=i;
        if(T->staff&&b->work) workers++; }
    if(camp<0) for(int i=0;i<S->R->nB;i++) if(S->B[i].lvl){ camp=i; break; }
    int idle=S->pop-(int)workers-active; if(idle<0) idle=0;
    int atTav=0, atCourt=0;
    if(taverns&&part>=3){ atTav=idle*2/5; if(atTav>KF_THRONG*3*taverns) atTav=KF_THRONG*3*taverns; }   // the taverns fill at dusk (and the nights are long)
    if(court>=0&&!night){ atCourt=idle/4; if(atCourt>KF_THRONG*3) atCourt=KF_THRONG*3; }                                         // the court gathers before the crowned castle by day
    int rest=idle-atTav-atCourt; if(rest<0) rest=0;
    int founders=homes<S->pop?S->pop-homes:0; if(founders>rest) founders=rest;                                     // the founding party has no home: they camp
    int homed=rest-founders;                                                                                         // the rest are the homes' people: shared out by the Nubbins that live in each
    int homeSeen=night?homed/4:homed;                                                                                // (at night the others are indoors)
    int hdone=0, tdone=0;
    for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(!b->lvl) continue; const KsType*T=ksT(b->type);
        int wx=b->x*KV_CELL, wy=b->y*KV_CELL, vis=kfOn(V,wx-32,wy-12,96,80);
        if(T->staff&&b->work){ if(vis&&n<KF_MAX-3) n=kfNub(o,n,V,wx+16,wy+36,1,(int)(((tick>>4)+(unsigned)i)&1),0,i+1); }   // a worker at the door, shifting his weight
        if(T->kind==KK_HOME&&b->work){ int cum=homes?homeSeen*(hdone+b->work)/homes:0; int share=cum-(homes?homeSeen*hdone/homes:0); hdone+=b->work;
            if(vis&&n<KF_MAX-8) n=kfCrowd(o,n,V,wx+16+(int)(kvHash(b->x,b->y)%9u)-4,wy+38,share,tick,(unsigned)i); }
        if(b->type==KB_TAVERN&&taverns){ int cum=atTav*(tdone+1)/taverns, share=cum-atTav*tdone/taverns; tdone++;
            if(vis&&n<KF_MAX-8) n=kfCrowd(o,n,V,wx+16,wy+40,share,tick,(unsigned)i+3u); }
        if(i==court&&vis&&n<KF_MAX-8) n=kfCrowd(o,n,V,wx+16,wy+42,atCourt,tick,(unsigned)i+7u);
        if(i==camp&&founders>0&&vis&&n<KF_MAX-8) n=kfCrowd(o,n,V,wx+16,wy+44,founders,tick,(unsigned)i+11u);
    }
    // smoke from the chimneys of homes with people in them, and the tavern
    int smoke=0;
    for(int i=0;i<S->R->nB&&n<KF_MAX-2;i++){ const KBld*b=&S->B[i]; if(!b->lvl) continue; const KsType*T=ksT(b->type);
        int wx=b->x*KV_CELL, wy=b->y*KV_CELL; if(!kfOn(V,wx,wy-12,32,48)) continue;
        if(T->kind==KK_HOME&&b->work&&smoke<10){ int sx=b->type==KB_HOUSE?wx+22:wx+16, sy=wy+(b->type==KB_HOUSE?1:4); unsigned ph=(tick/6+(unsigned)i*5)&15; if(ph<12) n=kfIcon(o,n,V,sx-4+(int)((ph>>2)&1),sy-(int)ph,KF_SMOKE+(int)(ph/3),6); smoke++; }
        if(b->type==KB_TAVERN&&smoke<10){ unsigned ph=(tick/6+(unsigned)i*5)&15; if(ph<12) n=kfIcon(o,n,V,wx+3,wy+1-(int)ph,KF_SMOKE+(int)(ph/3),6); smoke++; } }
    kfSort(o,0,n);
    // the cursor: brackets round the cell under the giant's feet, blinking (last: under everything)
    if(((tick>>4)&3)!=3&&n<KF_MAX){ int x=kgCx(G)*KV_CELL-V->x, y=kgCy(G)*KV_CELL-V->y; if(x>-16&&x<240&&y>-16&&y<160) n=(int)kfPut(o,n,x,y,0,1,0,KF_CURSOR,1,7); }
    return n;
}
#endif

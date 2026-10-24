// haul_proto.c - EP6 Phase 0, hauling prototype (PC). Run: sh tools/phase0/run_phase0.sh
// 40 Nubbins, 10 buildings, a road network on a 128 x 128 map. Goods move by a request/offer matcher and per-building DISTANCE MAPS over the
// road nodes (no path search per worker: a worker steps to the neighbour node with the lowest distance). It counts the work done each frame
// (matcher comparisons, worker steps, BFS node visits) so the cost can be turned into GBA cycles, soaks 10,000 realm days, cuts a road and
// destroys a building on the way, and checks that goods are conserved and no worker stays stuck. It is NOT a GBA measurement.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MAPW 128
#define MAXB 32
#define MAXW 128
#define MAXN 3072
#define DAYT 600          // frames per realm day (10 s at 60 fps) [ASSUMED]
#define STEPT 6           // a Nubbin steps one road node every 6 frames
#define TIMEOUT 900       // frames without progress before a job is dropped (heals stalls)
enum { WOOD,STONE,WOOL,CRYS,PLANK,CUT,CLOTH,POL,NG };
enum { K_GATHER,K_PROC,K_STORE,K_CASTLE };
typedef struct { uint8_t kind,alive,x,y,dx,dy,g,gout; uint16_t node; uint16_t stock[NG],resv[NG],infl[NG],want[NG]; uint16_t phase; } Bld;
typedef struct { uint8_t st; uint8_t good; int8_t src,dst; uint16_t node; uint32_t last; } Wk;   // st 0 idle, 1 to source, 2 to destination
static uint8_t road[MAPW][MAPW]; static uint16_t nodeAt[MAPW][MAPW]; static uint16_t nodeX[MAXN],nodeY[MAXN]; static int NN;
static Bld B[MAXB]; static int NB; static Wk W[MAXW]; static int NW;
static uint8_t dist[MAXB][MAXN]; static uint8_t dDirty[MAXB];
static long made[NG],used[NG],lost[NG],delivered; static long castleProgress;
static uint32_t rs=12345; static uint32_t rnd(void){ rs=rs*1664525u+1013904223u; return rs>>8; }
// ---- work counters (per frame) ----
static long opMatch,opWalk,opBfs; static long sumMatch,sumWalk,sumBfs,maxMatch,maxWalk,maxBfs,maxTot; static long frames;
static long maxStall; static long timeouts,aborts;
static void buildNodes(void){ NN=0; memset(nodeAt,0xFF,sizeof nodeAt);
    for(int y=0;y<MAPW;y++) for(int x=0;x<MAPW;x++) if(road[y][x]&&NN<MAXN){ nodeAt[y][x]=(uint16_t)NN; nodeX[NN]=(uint16_t)x; nodeY[NN]=(uint16_t)y; NN++; } }
static void bfs(int b){ static uint16_t q[MAXN]; memset(dist[b],255,MAXN); if(!B[b].alive){ return; } int n0=nodeAt[B[b].dy][B[b].dx]; if(n0==0xFFFF) return;
    int h=0,t=0; q[t++]=(uint16_t)n0; dist[b][n0]=0;
    while(h<t){ int n=q[h++]; opBfs++; int x=nodeX[n],y=nodeY[n]; int nx[4]={x+1,x-1,x,x},ny[4]={y,y,y+1,y-1};
        for(int k=0;k<4;k++){ if(nx[k]<0||ny[k]<0||nx[k]>=MAPW||ny[k]>=MAPW) continue; int m=nodeAt[ny[k]][nx[k]]; if(m==0xFFFF||dist[b][m]!=255) continue; if(dist[b][n]>=254) continue; dist[b][m]=(uint8_t)(dist[b][n]+1); q[t++]=(uint16_t)m; } } }
static void markAllDirty(void){ for(int b=0;b<NB;b++) dDirty[b]=1; }
static void carve(int x0,int y0,int x1,int y1){ int x=x0,y=y0; road[y][x]=1; while(x!=x1){ x+=x<x1?1:-1; road[y][x]=1; } while(y!=y1){ y+=y<y1?1:-1; road[y][x]=1; } }
static int addB(int kind,int g,int gout,int x,int y){ Bld*b=&B[NB]; memset(b,0,sizeof *b); b->kind=(uint8_t)kind; b->alive=1; b->x=(uint8_t)x; b->y=(uint8_t)y; b->dx=(uint8_t)x; b->dy=(uint8_t)(y+1); b->g=(uint8_t)g; b->gout=(uint8_t)gout; b->phase=(uint16_t)(rnd()%40);
    carve(b->dx,b->dy,64,64); return NB++; }
static void layout(int nb,int nw){
    memset(road,0,sizeof road); NB=0; NW=nw; road[64][64]=1;
    // ring of buildings round a hub at (64,64): gatherers, processors, storehouses, the castle
    int plan[MAXB][3]; int n=0;
    for(int g=0;g<4;g++){ plan[n][0]=K_GATHER; plan[n][1]=g; plan[n][2]=g; n++; }           // wood, stone, wool, crystal
    for(int g=0;g<4;g++){ plan[n][0]=K_PROC; plan[n][1]=g; plan[n][2]=g+4; n++; }          // sawmill, stonemason, weaver, cutter
    plan[n][0]=K_STORE; n++; plan[n][0]=K_CASTLE; n++;
    while(n<nb){ int g=n%4; plan[n][0]=(n%3==0)?K_STORE:(n%3==1)?K_GATHER:K_PROC; plan[n][1]=g; plan[n][2]=plan[n][0]==K_PROC?g+4:g; n++; }
    if(nb<n) n=nb;
    for(int i=0;i<n;i++){ int ang=i*360/n; double r=12+(i%3)*8; int x=64+(int)(r*(i%2?1.0:1.0)*((ang<90)?1:(ang<180)?-1:(ang<270)?-1:1)*((i*7)%5+4)/6.0)+(int)(rnd()%7)-3; int y=64+(int)(r*((ang<90)?1:(ang<180)?1:(ang<270)?-1:-1)*((i*3)%5+4)/6.0)+(int)(rnd()%7)-3;
        if(x<4)x=4; if(y<4)y=4; if(x>MAPW-6)x=MAPW-6; if(y>MAPW-8)y=MAPW-8; addB(plan[i][0],plan[i][1],plan[i][2],x,y); }
    // a few cross roads so the network has loops (a cut road can be routed round)
    for(int i=0;i+1<NB;i+=2) carve(B[i].dx,B[i].dy,B[i+1].dx,B[i+1].dy);
    buildNodes(); for(int b=0;b<NB;b++){ B[b].node=nodeAt[B[b].dy][B[b].dx]; }
    for(int b=0;b<NB;b++){ Bld*p=&B[b];
        if(p->kind==K_PROC) p->want[p->g]=6; if(p->kind==K_CASTLE) for(int g=4;g<8;g++) p->want[g]=4; if(p->kind==K_STORE) for(int g=0;g<NG;g++) p->want[g]=3; }
    for(int w=0;w<NW;w++){ int b=w%NB; W[w].st=0; W[w].node=B[b].node; W[w].src=W[w].dst=-1; W[w].last=0; }
    markAllDirty();
}
static int available(int b,int g){ return (int)B[b].stock[g]-(int)B[b].resv[g]; }
static void release(Wk*w,int returnGood){   // end a job: free the reservations; goods in hand go back to the source (or the nearest store)
    if(w->st==0) return;
    if(w->src>=0&&w->dst>=0){ if(w->st==1){ if(B[w->src].resv[w->good]) B[w->src].resv[w->good]--; } if(B[w->dst].infl[w->good]) B[w->dst].infl[w->good]--; }
    if(w->st==2&&returnGood){ int back=-1; if(B[w->src].alive) back=w->src; else for(int b=0;b<NB;b++) if(B[b].alive&&B[b].kind==K_STORE){ back=b; break; }
        if(back>=0) B[back].stock[w->good]++; else lost[w->good]++; }
    w->st=0; w->src=w->dst=-1; }
static void matcher(long t){   // two requesters a frame, round robin
    static int rr=0;
    for(int k=0;k<2;k++){ int r=rr; rr=(rr+1)%NB; Bld*q=&B[r]; if(!q->alive) continue;
        for(int g=0;g<NG;g++){ opMatch++; int need=(int)q->want[g]-(int)q->stock[g]-(int)q->infl[g]; if(need<=0) continue;
            int best=-1,bd=255; for(int s=0;s<NB;s++){ opMatch++; if(s==r||!B[s].alive) continue; if(q->kind==K_STORE&&B[s].kind==K_STORE) continue;
                if(available(s,g)<=0) continue; if(B[s].kind==K_STORE&&q->kind==K_STORE) continue; int d=B[s].node==0xFFFF||q->node==0xFFFF?255:dist[r][B[s].node]; if(d<bd){ bd=d; best=s; } }
            if(best<0||bd>=255) continue;
            int bw=-1,bwd=255; for(int w=0;w<NW;w++){ opMatch++; if(W[w].st!=0) continue; int d=dist[best][W[w].node]; if(d<bwd){ bwd=d; bw=w; } }
            if(bw<0) continue;
            W[bw].st=1; W[bw].good=(uint8_t)g; W[bw].src=(int8_t)best; W[bw].dst=(int8_t)r; W[bw].last=(uint32_t)t; B[best].resv[g]++; q->infl[g]++; } } }
static void walkers(long t){
    for(int w=0;w<NW;w++){ Wk*k=&W[w]; if(k->st==0) continue;
        long stall=(long)(t-k->last); if(stall>maxStall) maxStall=stall;
        int tgt=k->st==1?k->src:k->dst;
        if(!B[tgt].alive||(stall>TIMEOUT)){ if(stall>TIMEOUT) timeouts++; else aborts++; release(k,1); continue; }
        if((t+w)%STEPT) continue;
        if(k->node==B[tgt].node){   // arrived
            if(k->st==1){ Bld*s=&B[k->src]; if(s->stock[k->good]>0&&s->resv[k->good]>0){ s->stock[k->good]--; s->resv[k->good]--; k->st=2; k->last=(uint32_t)t; } else { release(k,0); }
            } else { B[k->dst].stock[k->good]++; B[k->dst].infl[k->good]--; delivered++; k->st=0; k->src=k->dst=-1; }
            continue; }
        int n=k->node,x=nodeX[n],y=nodeY[n]; int nx[4]={x+1,x-1,x,x},ny[4]={y,y,y+1,y-1}; int bestn=-1,bd=dist[tgt][n]; opWalk++;
        for(int d=0;d<4;d++){ opWalk++; if(nx[d]<0||ny[d]<0||nx[d]>=MAPW||ny[d]>=MAPW) continue; int m=nodeAt[ny[d]][nx[d]]; if(m==0xFFFF) continue; if(dist[tgt][m]<bd){ bd=dist[tgt][m]; bestn=m; } }
        if(bestn>=0){ k->node=(uint16_t)bestn; k->last=(uint32_t)t; } } }
static void produce(long t){
    for(int b=0;b<NB;b++){ Bld*p=&B[b]; if(!p->alive) continue; int per=(t+p->phase)%(p->kind==K_GATHER?20:p->kind==K_PROC?30:p->kind==K_CASTLE?40:99999);
        if(per) continue;
        if(p->kind==K_GATHER&&p->stock[p->gout]<8){ p->stock[p->gout]++; made[p->gout]++; }
        else if(p->kind==K_PROC&&p->stock[p->g]>=2&&p->stock[p->gout]<8){ p->stock[p->g]-=2; used[p->g]+=2; p->stock[p->gout]++; made[p->gout]++; }
        else if(p->kind==K_CASTLE){ int all=1; for(int g=4;g<8;g++) if(!p->stock[g]) all=0; if(all){ for(int g=4;g<8;g++){ p->stock[g]--; used[g]++; } castleProgress++; } } } }
static int conserved(void){ long carried[NG]={0},inst[NG]={0}; for(int w=0;w<NW;w++) if(W[w].st==2) carried[W[w].good]++;
    for(int b=0;b<NB;b++) for(int g=0;g<NG;g++) inst[g]+=B[b].stock[g]; int ok=1; for(int g=0;g<NG;g++){ long l=made[g]-used[g]-lost[g]-inst[g]-carried[g]; if(l){ ok=0; printf("  LEDGER OFF good %d by %ld\n",g,l); } } return ok; }
static void rebuildRoads(void){   // roads changed: new node numbers, workers keep their cell (a worker on a removed cell steps to a road next to it)
    int ox[MAXW],oy[MAXW]; for(int w=0;w<NW;w++){ ox[w]=nodeX[W[w].node]; oy[w]=nodeY[W[w].node]; }
    buildNodes(); for(int b=0;b<NB;b++) B[b].node=nodeAt[B[b].dy][B[b].dx];
    for(int w=0;w<NW;w++){ int x=ox[w],y=oy[w]; int m=nodeAt[y][x];
        if(m==0xFFFF){ int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1}; for(int k=0;k<4&&m==0xFFFF;k++) if(x+dx[k]>=0&&y+dy[k]>=0&&x+dx[k]<MAPW&&y+dy[k]<MAPW) m=nodeAt[y+dy[k]][x+dx[k]]; }
        if(m==0xFFFF) m=nodeAt[64][64]; W[w].node=(uint16_t)m; }
    markAllDirty(); }
static long delivRate(long a,long b){ return b-a; }
int main(int argc,char**argv){
    int nb=argc>1?atoi(argv[1]):10, nw=argc>2?atoi(argv[2]):40, days=argc>3?atoi(argv[3]):10000;
    layout(nb,nw); long T=(long)days*DAYT; int cutDone=0,killDone=0,fixDone=0,rebuildDone=0,isoDone=0,unisoDone=0; int killV=-1; uint16_t saveWant[NG]; uint8_t isoSave[4]; long mark[8]={0}; int cutX=0,cutY=0; long afterCut=0,deliveredAtCut=0,deliveredAtFix=0;
    int roadCells=0; for(int y=0;y<MAPW;y++) for(int x=0;x<MAPW;x++) roadCells+=road[y][x];
    printf("map %dx%d  buildings %d  workers %d  road nodes %d  days %d (%ld frames)\n",MAPW,MAPW,NB,NW,NN,days,T);
    for(long t=1;t<=T;t++){
        opMatch=opWalk=opBfs=0;
        if(!cutDone&&t==T*2/10){ // cut a road cell on a spine near the hub (the cross roads let traffic go round)
            int best=-1; for(int n=0;n<NN;n++){ int c=0; int x=nodeX[n],y=nodeY[n]; if(nodeAt[y][x+1]!=0xFFFF)c++; if(nodeAt[y][x-1]!=0xFFFF)c++; if(nodeAt[y+1][x]!=0xFFFF)c++; if(nodeAt[y-1][x]!=0xFFFF)c++; if(c==2&&abs(x-64)+abs(y-64)>3&&abs(x-64)+abs(y-64)<14&&(rnd()%5==0)) best=n; }
            if(best>=0){ cutX=nodeX[best]; cutY=nodeY[best]; road[cutY][cutX]=0; rebuildRoads(); printf("  day %ld: road cell (%d,%d) cut\n",t/DAYT,cutX,cutY); }
            cutDone=1; mark[0]=delivered; }
        if(!fixDone&&t==T*3/10){ if(road[cutY][cutX]==0){ road[cutY][cutX]=1; rebuildRoads(); printf("  day %ld: road mended\n",t/DAYT); } fixDone=1; mark[1]=delivered; }
        if(!killDone&&t==T*4/10){ int v=-1; for(int b=NB-1;b>=0;b--) if(B[b].kind==K_PROC){ v=b; break; } killV=v; if(v>=0){ for(int g=0;g<NG;g++){ lost[g]+=B[v].stock[g]; B[v].stock[g]=0; saveWant[g]=B[v].want[g]; B[v].want[g]=0; B[v].infl[g]=0; } B[v].alive=0; markAllDirty(); printf("  day %ld: building %d destroyed (its stock is counted as lost)\n",t/DAYT,v); } killDone=1; mark[2]=delivered; }
        if(!rebuildDone&&t==T*5/10){ if(killV>=0){ Bld*p=&B[killV]; p->alive=1; for(int g=0;g<NG;g++){ p->want[g]=saveWant[g]; p->resv[g]=0; } markAllDirty(); printf("  day %ld: building %d rebuilt\n",t/DAYT,killV); } rebuildDone=1; mark[3]=delivered; }
        if(!isoDone&&t==T*6/10){ // wall in building 0: the roads round its door are removed, so it cannot be reached at all
            int x=B[0].dx,y=B[0].dy; int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1}; for(int k=0;k<4;k++){ int nx=x+dx[k],ny=y+dy[k]; isoSave[k]=road[ny][nx]; road[ny][nx]=0; }
            rebuildRoads(); printf("  day %ld: building 0 walled in (no road reaches it)\n",t/DAYT); isoDone=1; mark[4]=delivered; }
        if(!unisoDone&&t==T*7/10){ int x=B[0].dx,y=B[0].dy; int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1}; for(int k=0;k<4;k++) road[y+dy[k]][x+dx[k]]=isoSave[k]; rebuildRoads(); printf("  day %ld: building 0 reachable again\n",t/DAYT); unisoDone=1; mark[5]=delivered; }
        // time slice: one distance map rebuilt a frame
        for(int b=0;b<NB;b++) if(dDirty[b]){ bfs(b); dDirty[b]=0; break; }
        matcher(t); walkers(t); produce(t);
        frames++; sumMatch+=opMatch; sumWalk+=opWalk; sumBfs+=opBfs; if(opMatch>maxMatch)maxMatch=opMatch; if(opWalk>maxWalk)maxWalk=opWalk; if(opBfs>maxBfs)maxBfs=opBfs; if(opMatch+opWalk+opBfs>maxTot)maxTot=opMatch+opWalk+opBfs;
        if(t%(DAYT*1000)==0) printf("  day %ld: delivered %ld, castle progress %ld, conserved %s\n",t/DAYT,delivered,castleProgress,conserved()?"yes":"NO");
    }
    int stuck=0; for(int w=0;w<NW;w++) if(W[w].st&&(T-(long)W[w].last)>TIMEOUT) stuck++;
    printf("RESULT: goods ledger %s; workers stuck at the end %d; longest stall seen %ld frames (timeout %d); timeouts %ld, aborts %ld\n",conserved()?"balanced":"BROKEN",stuck,maxStall,TIMEOUT,timeouts,aborts);
    printf("RESULT: delivered per 1000 days: before the cut %.0f | road cut %.0f | road mended %.0f | a processor destroyed %.0f | rebuilt %.0f | a building walled in %.0f | reachable again %.0f\n",
        mark[0]*1000.0/(T*2/10/DAYT),(mark[1]-mark[0])*1000.0/(T/10/DAYT),(mark[2]-mark[1])*1000.0/(T/10/DAYT),(mark[3]-mark[2])*1000.0/(T/10/DAYT),(mark[4]-mark[3])*1000.0/(T/10/DAYT),(mark[5]-mark[4])*1000.0/(T/10/DAYT),(delivered-mark[5])*1000.0/(T*3/10/DAYT));
    double am=(double)sumMatch/frames,aw=(double)sumWalk/frames,ab=(double)sumBfs/frames;
    printf("OPS per frame  average: matcher %.1f  walker %.1f  BFS %.1f   worst frame: matcher %ld  walker %ld  BFS %ld  all three %ld\n",am,aw,ab,maxMatch,maxWalk,maxBfs,maxTot);
    // cycle estimates: ARM7TDMI, code in ROM or IWRAM, data in EWRAM. LOW = 30 / 25 / 20 cycles an op, HIGH = 80 / 60 / 50 [ASSUMED, to be replaced by a measurement on a cart]
    double lo=am*25+aw*30+ab*20, hi=am*60+aw*80+ab*50; double wlo=maxMatch*25+maxWalk*30+maxBfs*20, whi=maxMatch*60+maxWalk*80+maxBfs*50;
    printf("CYCLES per frame (budget 280,896): average %.0f to %.0f (%.1f%% to %.1f%%)   worst frame %.0f to %.0f (%.1f%% to %.1f%%)\n",lo,hi,lo/2809.0,hi/2809.0,wlo,whi,wlo/2809.0,whi/2809.0);
    printf("MEMORY: distance maps %d x %d road nodes = %d bytes (u8 each); workers %d x 10 B; buildings %d x ~90 B\n",NB,NN,NB*NN,NW,NB);
    printf("MEMORY: a cell-to-node table as a plain u16 grid would be %d bytes: too big for the EWRAM left; the real build needs a hash or node ids kept in the map\n",MAPW*MAPW*2);
    return 0; }

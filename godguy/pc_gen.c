// PC test of the planet generator (port step 7): main's six land shapes (CONTINENTS included) still make exactly the planets they always made when COASTS and MOUNTAINS are NORMAL,
// and the two new controls really change the coast and the mountains. The table below is main's output before the controls existed: {edge, size, shape, seed, hash}.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static unsigned gh(void){ unsigned h=2166136261u; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) h=(h^world[y][x])*16777619u; return h; }
static const unsigned base[][5]={
{0,128,0,1,902764837u},
{0,128,0,2,986369049u},
{0,128,0,3,3335674374u},
{0,128,1,1,3286489776u},
{0,128,1,2,1435771533u},
{0,128,1,3,3045048381u},
{0,128,2,1,674464127u},
{0,128,2,2,680632919u},
{0,128,2,3,3516581566u},
{0,128,3,1,1766987193u},
{0,128,3,2,634809644u},
{0,128,3,3,85339307u},
{0,128,4,1,3309236196u},
{0,128,4,2,2157998452u},
{0,128,4,3,3698737367u},
{0,128,5,1,2832115477u},
{0,128,5,2,3703277852u},
{0,128,5,3,473766466u},
{0,256,0,1,1097758074u},
{0,256,0,2,1285833054u},
{0,256,0,3,2721204583u},
{0,256,1,1,3692088514u},
{0,256,1,2,3404534660u},
{0,256,1,3,1259087504u},
{0,256,2,1,2202721150u},
{0,256,2,2,646968132u},
{0,256,2,3,3266107112u},
{0,256,3,1,1369954872u},
{0,256,3,2,2437218477u},
{0,256,3,3,3661360759u},
{0,256,4,1,4010891633u},
{0,256,4,2,2062267613u},
{0,256,4,3,3880133907u},
{0,256,5,1,2168232054u},
{0,256,5,2,3200626002u},
{0,256,5,3,4005898726u},
{2,128,0,1,4069121283u},
{2,128,0,2,2369255479u},
{2,128,0,3,4284921899u},
{2,128,1,1,3677000697u},
{2,128,1,2,3279463061u},
{2,128,1,3,3072563745u},
{2,128,2,1,4227092168u},
{2,128,2,2,2062345060u},
{2,128,2,3,712708427u},
{2,128,3,1,1361970474u},
{2,128,3,2,2640507785u},
{2,128,3,3,3934284023u},
{2,128,4,1,4052102349u},
{2,128,4,2,4180483014u},
{2,128,4,3,3180631950u},
{2,128,5,1,1887512540u},
{2,128,5,2,4000818286u},
{2,128,5,3,3715896277u},
{2,256,0,1,3152590262u},
{2,256,0,2,245515156u},
{2,256,0,3,483756973u},
{2,256,1,1,2259257849u},
{2,256,1,2,1896943536u},
{2,256,1,3,2551390928u},
{2,256,2,1,2255842183u},
{2,256,2,2,2547922242u},
{2,256,2,3,94494646u},
{2,256,3,1,1367389507u},
{2,256,3,2,279375277u},
{2,256,3,3,3605103670u},
{2,256,4,1,2828317913u},
{2,256,4,2,556765541u},
{2,256,4,3,4055611546u},
{2,256,5,1,541708792u},
{2,256,5,2,2261986571u},
{2,256,5,3,2179788224u},
};
static int count(int t){ int n=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) n+=world[y][x]==t; return n; }
static int coast(void){ int n=0; for(int y=1;y<WH-1;y++) for(int x=1;x<WW-1;x++){ int l=world[y][x]>T_WATER; if(l&&(world[y][x+1]<=T_WATER||world[y+1][x]<=T_WATER)) n++; } return n; }
static int masses(int minsz){ static u8 seen[MAXD][MAXD]; memset(seen,0,sizeof seen); static int stk[MAXD*MAXD]; int n=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){ if(seen[y][x]||world[y][x]<=T_WATER) continue; int sp=0,sz=0; stk[sp++]=y*MAXD+x; seen[y][x]=1; while(sp){ int c=stk[--sp]; int cx=c%MAXD, cy=c/MAXD; sz++; for(int d=0;d<4;d++){ int nx=cx+dxs[d], ny=cy+dys[d]; if(nx<0||ny<0||nx>=WW||ny>=WH||seen[ny][nx]||world[ny][nx]<=T_WATER) continue; seen[ny][nx]=1; stk[sp++]=ny*MAXD+nx; } } if(sz>=minsz) n++; } return n; }

static int marchIn(int x0,int y0,int x1,int y1,int mask){
    int hits=0, area=0;
    for(int y=y0*WH/1024;y<y1*WH/1024;y++) for(int x=x0*WW/1024;x<x1*WW/1024;x++){
        hits+=(mask>>world[y][x])&1;
        area++;
    }
    return hits*100/area;
}

// Walk with the human terrain rules, optionally inside a narrow band across a ford.
static int marchReach(int x,int y,int x1,int x2,int y1,int y2,int *sites){
    static u8 seen[MAXD][MAXD];
    static int queue[MAXD*MAXD];
    memset(seen,0,sizeof seen);
    if(!walkable(&sp[0],x,y)) return 0;
    seen[y][x]=1;
    int read=0, written=0;
    queue[written++]=y*MAXD+x;
    while(read<written){
        int i=queue[read++], cx=i%MAXD, cy=i/MAXD;
        for(int d=0;d<4;d++){
            int nx=cx+dxs[d], ny=cy+dys[d];
            if(nx<x1||nx>=x2||ny<y1||ny>=y2||seen[ny][nx]||!walkable(&sp[0],nx,ny)) continue;
            seen[ny][nx]=1;
            queue[written++]=ny*MAXD+nx;
        }
    }
    if(sites){
        *sites=0;
        for(int i=0;i<(int)(sizeof marchSites/sizeof marchSites[0]);i++){
            int sx,sy;
            marchSite(i,&sx,&sy);
            *sites+=seen[sy][sx];
        }
    }
    if(x2==WW) return written*100/(WW*WH);
    for(int iy=y1;iy<y2;iy++) if(seen[iy][x2-1]) return 1;
    return 0;
}

static int marchTest(void){
    cShape=GW_MARCH;
    cEdge=cSea=cClim=cWet=cCoast=cMtn=0;
    int fails=0;
    for(int i=0;i<SZ_N;i++){
        gW=gH=szList[i];
        genWorld(4242);
        unsigned baseHash=gh();
        int clearStart=buildable(WW/2,WH/2);
        int sites=0;
        int connected=marchReach(WW/2,WH/2,0,WW,0,WH,&sites);
        int north=marchIn(255,140,765,320,(1<<T_MOUNT)|(1<<T_HILL)|(1<<T_SNOW));
        int west=marchIn(146,320,376,590,1<<T_FOREST);
        int east=marchIn(685,392,810,622,(1<<T_DESERT)|(1<<T_HILL)|(1<<T_MOUNT));
        int south=marchIn(465,680,695,798,1<<T_SWAMP);
        int river=marchIn(490,425,600,575,1<<T_WATER);
        int upper=marchReach(472*WW/1024,362*WH/1024,472*WW/1024,581*WW/1024,346*WH/1024,380*WH/1024,NULL);
        int lower=marchReach(462*WW/1024,666*WH/1024,462*WW/1024,621*WW/1024,650*WH/1024,682*WH/1024,NULL);
        rs=5; genWorld(4242);
        int repeat=gh()==baseHash;
        genWorld(4243);
        int varies=gh()!=baseHash;
        int ok=clearStart&&connected>=35&&sites==8&&north>=20&&west>=35&&east>=35&&south>=8&&river>=3&&upper&&lower&&repeat&&varies;
        printf("Riven March %d: %s (start %d, walk %d%%, sites %d, north %d%%, west %d%%, east %d%%, marsh %d%%, river %d%%, fords %d/%d, seeds %d/%d)\n",WW,ok?"OK":"FAIL",clearStart,connected,sites,north,west,east,south,river,upper,lower,repeat,varies);
        fails+=!ok;
    }
    return fails;
}

static int marchVariety(void){
    static u8 land[128][128];
    static const u32 seeds[]={4243,69778,42,77,0xdeadbeef,0xffffffff};
    int fails=0;
    gW=gH=128;
    genWorld(4242);
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) land[y][x]=world[y][x]>T_WATER;
    for(unsigned i=0;i<sizeof seeds/sizeof seeds[0];i++){
        genWorld(seeds[i]);
        int changed=0,sites=0;
        for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) changed+=land[y][x]!=(world[y][x]>T_WATER);
        int coastChange=changed*100/(WW*WH);
        int islands=masses(WW*WH/2600)-1;
        int tributary=marchIn(275,445,465,602,1<<T_WATER);
        int walk=marchReach(WW/2,WH/2,0,WW,0,WH,&sites);
        int ok=coastChange>=8&&islands>=3&&tributary>=4&&sites==8&&walk>=35;
        printf("Riven March seed %u: %s (land/water change %d%%, islands %d, west tributary %d%%, sites %d)\n",seeds[i],ok?"OK":"FAIL",coastChange,islands,tributary,sites);
        fails+=!ok;
    }
    return fails;
}

static int marchRoutesAtExtremes(void){
    static const struct {
        u32 seed;
        int sea,climate,wet,coast,mountains;
    } cases[]={
        {1251615044u,2,0,-1,1,0}, // an inlet can otherwise cut off the high pass
        {235539461u,2,0,1,1,-1},
        {1094649946u,2,-1,-1,1,0},
        {2412102358u,-2,2,1,0,1}, // the east escarpment needs a way through
    };
    int fails=0;
    gW=gH=128;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
        cSea=cases[i].sea;
        cClim=cases[i].climate;
        cWet=cases[i].wet;
        cCoast=cases[i].coast;
        cMtn=cases[i].mountains;
        genWorld(cases[i].seed);
        int sites=0;
        marchReach(WW/2,WH/2,0,WW,0,WH,&sites);
        int upper=marchReach(472*WW/1024,362*WH/1024,472*WW/1024,581*WW/1024,346*WH/1024,380*WH/1024,NULL);
        int lower=marchReach(462*WW/1024,666*WH/1024,462*WW/1024,621*WW/1024,650*WH/1024,682*WH/1024,NULL);
        int ok=sites==8&&upper&&lower;
        printf("Riven March extreme controls seed %u: %s (sites %d, fords %d/%d)\n",cases[i].seed,ok?"OK":"FAIL",sites,upper,lower);
        fails+=!ok;
    }
    cSea=cClim=cWet=cCoast=cMtn=0;
    return fails;
}

static int marchSettlements(void){
    planetSize=MAXD;
    seedFix=4242;
    newWorld();
    seedFix=0;
    int home=-1;
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].race==0){ home=v; break; }
    int nearCenter=home>=0&&abs(vil[home].x-WW/2)<=WW/32&&abs(vil[home].y-WH/2)<=WH/32;
    int villages=nVilOn;
    int keeps=0,gates=0,walls=0,estates=0,families=0;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        int kind=bld[y][x];
        keeps+=kind==B_KEEP;
        gates+=ggBuildingIsGate(kind);
        walls+=kind==B_WALL;
        int mask=ggResidenceMask(x,y,kind);
        estates+=kind==B_ESTATE&&mask==7;
        families+=(kind==B_HOUSE_EW&&mask==1)||(kind==B_HOUSE_NS&&mask==2);
    }
    cLife=0;
    seedFix=4242;
    newWorld();
    cLife=2;
    seedFix=0;
    int emptyHomes=0;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)emptyHomes+=ggBuildingIsHome(bld[y][x]);
    int ok=nearCenter&&villages>=5&&nVilOn==0&&!emptyHomes&&
           keeps>=1&&gates==2*keeps&&walls==14*keeps&&estates>=3&&families>=3;
    printf("Riven March settlements: %s (%d villages, %d keeps, %d gates, %d walls, %d estates, %d family houses)\n",
           ok?"OK":"FAIL",villages,keeps,gates,walls,estates,families);
    return !ok;
}

static int constructionTool(void){
    const int x=16,y=16;
    for(int dy=-4;dy<=4;dy++)for(int dx=-4;dx<=4;dx++){
        world[y+dy][x+dx]=T_GRASS;
        bld[y+dy][x+dx]=bldv[y+dy][x+dx]=0;
        occ[y+dy][x+dx]=0;
    }
    for(int i=0;i<PW_N;i++)if(pw[i].kind==PK_BUILD)power=i;
    constructionMode=GG_CONSTRUCTION_N-1;
    world[y+2][x+2]=T_WATER;
    usePower(x,y,1);
    int partial=0;
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)partial+=bld[y+dy][x+dx]!=B_NONE;
    world[y+2][x+2]=T_GRASS;
    usePower(x,y,0);
    int dragged=!!bld[y][x];
    usePower(x,y,1);
    int built=bld[y][x]==B_KEEP&&bld[y][x+2]==B_GATE_EW&&bld[y][x-2]==B_GATE_EW;
    const char *path="/tmp/gg-settlement-fixture.sav";
    favPow[power]=1;
    int saved=saveGameRaw(path);
    bld[y][x]=B_NONE;
    favPow[power]=0;
    int restored=saved&&loadGameRaw(path)&&bld[y][x]==B_KEEP&&
                 bld[y][x+2]==B_GATE_EW&&favPow[power];
    remove(path);
    int protected=!ggOpenBuildingPlot(x+3,y)&&!ggOpenBuildingPlot(x+1,y);
    constructionMode=0;
    usePower(x+3,y,1);
    usePower(x+1,y,1);
    int motion=!walkable(&sp[0],x,y+2)&&walkable(&sp[0],x+2,y);
    int ok=!partial&&!dragged&&built&&restored&&motion&&protected&&
           !bld[y][x+3]&&!bld[y][x+1];
    printf("Construction tool: %s (atomic castle, save round trip, pawn traversal)\n",
           ok?"OK":"FAIL");
    return !ok;
}

static int residentialConstruction(void){
    const int x=40,y=30,mx=48,my=30;
    for(int row=y;row<=y+2;row++)for(int col=x;col<=mx+2;col++){
        world[row][col]=T_GRASS;
        bld[row][col]=bldv[row][col]=0;
        occ[row][col]=0;
    }
    constructionMode=0;
    world[y][x+1]=T_WATER;
    usePower(x,y,1);
    int house=bld[y][x]==B_HOUSE_NS&&bld[y+1][x]==B_WING_S&&
              ggResidenceMask(x,y,bld[y][x])==GG_RES_SOUTH;
    constructionMode=1;
    world[my+1][mx+1]=T_WATER;
    usePower(mx,my,1);
    int atomic=!bld[my][mx]&&!bld[my][mx+1]&&!bld[my+1][mx];
    world[my+1][mx+1]=T_GRASS;
    usePower(mx,my,1);
    int estate=bld[my][mx]==B_ESTATE&&ggResidenceMask(mx,my,B_ESTATE)==7;
    const char *path="/tmp/gg-residential-lots-test.sav";
    int saved=saveGameRaw(path);
    bld[my+1][mx+1]=B_NONE;
    int restored=saved&&loadGameRaw(path)&&ggResidenceMask(mx,my,B_ESTATE)==7;
    remove(path);
    u8 owner=bldv[my][mx];
    bldv[my][mx+1]=(u8)(owner^1u);
    int rootX,rootY;
    int separate=ggResidenceMask(mx,my,B_ESTATE)==(GG_RES_SOUTH|GG_RES_CORNER)&&
                 !ggResidenceResolveWing(mx+1,my,B_WING_E,&rootX,&rootY);
    bldv[my][mx+1]=owner;
    for(int i=0;i<PW_N;i++)if(pw[i].kind==PK_INSPECT)power=i;
    usePower(mx+1,my,1);
    int inspectedEstate=owner||!strcmp(info,"MANOR ESTATE");
    razeAt(mx+1,my,256);
    int damage=bld[my][mx+1]==B_RUIN&&
               ggResidenceMask(mx,my,B_ESTATE)==(GG_RES_SOUTH|GG_RES_CORNER);
    int ok=house&&atomic&&estate&&restored&&separate&&inspectedEstate&&damage;
    printf("Residential lots: %s (directional house, estate, owner, inspection, save and damage)\n",
           ok?"OK":"FAIL");
    return !ok;
}

static int marchDistrictGrades(void){
    const int x=64,y=32;
    for(int dy=-4;dy<=4;dy++)for(int dx=-4;dx<=4;dx++){
        world[y+dy][x+dx]=T_GRASS;
        bld[y+dy][x+dx]=occ[y+dy][x+dx]=0;
    }
    int level=marchDistrictSite(x,y);
    world[y-3][x+1]=T_SNOW;
    int steep=!marchDistrictSite(x,y);
    printf("District approaches: %s (reject an unclimbable ring)\n",level&&steep?"OK":"FAIL");
    return !(level&&steep);
}

int main(){ int fails=0; makeSpecies(); cSea=cClim=cWet=0; cCoast=cMtn=0;
    int bad=0, n=(int)(sizeof base/sizeof base[0]); for(int i=0;i<n;i++){ cEdge=(int)base[i][0]; gW=gH=planetSize=(int)base[i][1]; cShape=(int)base[i][2]; genWorld(base[i][3]*977u); if(gh()!=base[i][4]) bad++; }
    printf("planets with COASTS and MOUNTAINS normal identical to before: %d of %d\n",n-bad,n); if(bad){ fails++; printf("  FAIL: the generator changed\n"); }
    cEdge=0; gW=gH=planetSize=256; int cont=0; for(int sd=1;sd<=5;sd++){ cShape=0; genWorld(sd*977u); int m=masses(256*256/400); cont+=m; } printf("CONTINENTS at 256: %d land masses over 5 seeds\n",cont); if(cont<10){ fails++; printf("  FAIL: continents gone\n"); }
    int ms[6]; for(int sh=0;sh<6;sh++){ int sum=0; for(int sd=1;sd<=5;sd++){ cShape=sh; genWorld(sd*977u); sum+=masses(256*256/600); } ms[sh]=sum; } printf("land masses by shape (CONTINENTS .. INLAND SEAS): %d %d %d %d %d %d\n",ms[0],ms[1],ms[2],ms[3],ms[4],ms[5]); if(!(ms[1]<ms[2])){ fails++; printf("  FAIL: shapes\n"); }
    cShape=0; unsigned a,b; int cs[3], mt[3];
    for(int c=-1;c<=1;c++){ cCoast=c; genWorld(4242u); cs[c+1]=coast(); } cCoast=0; printf("coast tiles SMOOTH / NORMAL / RUGGED: %d %d %d\n",cs[0],cs[1],cs[2]); if(!(cs[0]<cs[1]&&cs[1]<cs[2])){ fails++; printf("  FAIL: coast\n"); }
    for(int m=-1;m<=1;m++){ cMtn=m; genWorld(4242u); mt[m+1]=count(T_MOUNT)+count(T_HILL); } cMtn=0; printf("hills+mountains FEW / NORMAL / MANY: %d %d %d\n",mt[0],mt[1],mt[2]); if(!(mt[0]<mt[1]&&mt[1]<mt[2])){ fails++; printf("  FAIL: mountains\n"); }
    cCoast=1; cMtn=1; genWorld(77u); a=gh(); genWorld(77u); b=gh(); genWorld(78u); unsigned c2=gh(); printf("same seed same planet: %s, other seed other planet: %s\n",a==b?"YES":"NO",a!=c2?"YES":"NO"); if(a!=b||a==c2){ fails++; printf("  FAIL: seeds\n"); }
    // the land share still follows SEA
    cCoast=cMtn=0; int lp[5]; for(int s=-2;s<=2;s++){ cSea=s; cShape=0; genWorld(4242u); int l=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l+=world[y][x]>T_WATER; lp[s+2]=l*100/(WW*WH); } cSea=0; printf("land share DRY .. FLOODED: %d%% %d%% %d%% %d%% %d%%\n",lp[0],lp[1],lp[2],lp[3],lp[4]); if(!(lp[0]>lp[2]&&lp[2]>lp[4])){ fails++; printf("  FAIL: sea\n"); }
    fails+=marchTest();
    fails+=marchVariety();
    fails+=marchRoutesAtExtremes();
    fails+=marchSettlements();
    fails+=constructionTool();
    fails+=residentialConstruction();
    fails+=marchDistrictGrades();
    printf("%s\n",fails?"GEN FAIL":"GEN OK"); return fails?1:0; }

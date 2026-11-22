// Exercise outdoor actions through the 3DS game's physical input path.
#define GG_TEST_NATIVE_ORIGIN
#include "3ds/main3ds.c"

static int physicalKeys,frame;
void plat_setCinema(int on){ (void)on; }
int plat_keys(int *x,int *y){ *x=*y=0; return physicalKeys; }

static void require(int ok,const char *message){
    if(!ok){ fprintf(stderr,"OUTDOOR FAIL: %s\n",message); exit(1); }
}
static int inputFrame(int keys){
    int held,x,y;
    physicalKeys=keys;
    shimInput(frame++,&held,&x,&y);
    return held;
}
static void fieldAction(void){
    inputFrame(KEY_L|KEY_A);
    inputFrame(0);
}
static void aimEast(void){ ggExplore.angle=0; }

static void checkMarch(int size,unsigned seed){
    cShape=GW_MARCH;planetSize=size;cLife=0;seedFix=seed;
    newWorld();
    int landmarks=0,docks=0,routes=0;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        landmarks+=bld[y][x]==B_RUIN||bld[y][x]==B_STATUE;
        if(bld[y][x]==B_DOCK){
            int dx,dy;
            docks++;
            int reachable=ggOutdoorFindFerry(x,y,NULL,&dx,&dy);
            routes+=reachable;
            if(!reachable)fprintf(stderr,"Unrouted dock at %d,%d\n",x,y);
        }
    }
    if(landmarks<5||docks<2||routes!=docks)
        fprintf(stderr,"March size %d seed %u: %d sites, %d docks, %d working landings\n",
                size,seed,landmarks,docks,routes);
    require(landmarks>=5&&docks>=2&&routes==docks,
            "a new March has discoverable sites and connected public docks");
}

int main(void){
    gW=gH=128;
    for(int y=0;y<WH;y++) memset(world[y],T_GRASS,WW);
    makeSpecies();makeItems();idReset();
    titleOn=0; storyOn=0; camX=32;camY=64;

    int quickSave=KEY_SELECT|KEY_L|KEY_R;
    require(inputFrame(quickSave)==quickSave&&!ggExplore.active,
            "the existing three-button quick save reaches the map unchanged");
    inputFrame(0);
    inputFrame(KEY_SELECT|KEY_L);
    require(ggExplore.active&&!ggExplore.actorId,"SELECT+L opens walking from the map without possessing anyone");
    inputFrame(0);
    aimEast();
    bld[20][22]=B_RUIN;
    fieldAction();
    require(jrN==1&&mapPinMask,"surveying a ruin records a discovery and marks the map");
    fieldAction();
    require(jrN==1&&mapPinMask==1,"revisiting a ruin does not duplicate the journal or pins");
    bld[20][22]=0;
    ggExplore.material=3;
    int edits=ggExplore.edits;
    inputFrame(KEY_L|KEY_A);
    inputFrame(KEY_A);
    require(ggExplore.edits==edits,"releasing L first cannot turn an interaction into painting");
    require(jrN==1&&mapPinMask==1,"viewing ordinary terrain leaves discovery slots free");
    inputFrame(0);
    inputFrame(KEY_START);
    inputFrame(0);
    require(!ggExplore.active,"Start returns to the world map");

    int person=addCr(0,20,20);
    require(person>=0&&ggHybridBegin(person,0),"enter a real citizen");
    Cr *citizen=&cr[person];
    citizen->food=40;
    aimEast();
    int town=vilFound(0,26,21);
    require(town>=0,"found an outdoor test village");
    bld[20][22]=B_FARM;bldv[20][22]=(u8)(town+1);
    int borderTown=vilFound(3,64,28);
    require(borderTown>=0,"found a border village");
    Cr traveler=*citizen;
    traveler.x=62;traveler.y=27;
    warM[0][3]=1;
    require(ggOutdoorAtWar(&traveler,borderTown),"a traveler remains recognized as an enemy away from home");
    warM[0][3]=0;
    u16 initialStock=vil[town].stock;
    fieldAction();
    require(citizen->food>40&&vil[town].stock<initialStock&&bld[20][22]==B_FARM,
            "farm provisions use real stores and leave the farm standing");

    bld[20][22]=B_DOCK;bldv[20][22]=0;
    bld[20][31]=B_DOCK;bldv[20][31]=0;
    for(int y=19;y<=21;y++) for(int x=23;x<=30;x++) world[y][x]=T_WATER;
    u32 id=crId[person];
    for(int y=19;y<=21;y++) world[y][26]=T_GRASS;
    u8 food=citizen->food;
    fieldAction();
    require(citizen->x==20&&citizen->food==food,"a disconnected dock leaves the player and rations in place");
    for(int y=19;y<=21;y++) world[y][26]=T_WATER;
    int swimmer=-1;
    for(int a=8;a<ARCH_A&&swimmer<0;a++)
        if(arch[a].mask==M_WATER)swimmer=addCr(8+(a-8)*16,28,19);
    require(swimmer>=0,"spawn a water-dwelling test creature");
    Cr *waterDweller=&cr[swimmer];
    int lx,ly;
    require(!ggOutdoorFindFerry(22,20,waterDweller,&lx,&ly),
            "a water-dweller needs a landing their habitat allows");
    giveTrait(waterDweller,TR_WING,0);
    require(ggOutdoorFindFerry(22,20,waterDweller,&lx,&ly)&&lx==32&&ly==20,
            "a winged water-dweller can land ashore");
    fieldAction();
    require(citizen->x>=31&&(ggExplore.px>>8)==citizen->x&&
            (ggExplore.py>>8)==citizen->y,
            "an actual ferry crossing lands the citizen and camera together");
    require(crId[person]==id&&occ[citizen->y][citizen->x]==person+1&&!occ[20][20],
            "ferrying moves one citizen with no stale or duplicate occupancy");
    int bank=20+GG_FERRY_RADIUS;
    bld[80][20]=bld[80][bank]=B_DOCK;
    for(int x=21;x<bank;x++)world[80][x]=T_WATER;
    require(ggOutdoorFindFerry(20,80,NULL,&lx,&ly)&&lx==bank+1&&ly==80,
            "a ferry can reach the far dock at the route limit");
    mapPtr=(u16*)bgGetMapPtr(0);
    checkMarch(128,4242);
    checkMarch(256,77);
    checkMarch(512,69778);
    checkMarch(1024,4243);
    puts("OUTDOOR OK: free walking, bounded discovery and live stores, ferry with stable citizen identity");
    return 0;
}

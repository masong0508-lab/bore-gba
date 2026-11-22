// Riven March uses a 1024-unit survey at every planet size. The valley,
// settlements and crossings give the maps a common geography. The seed
// determines the shore, river courses, islands and smaller terrain features.
// Only the finished terrain tiles persist.
enum { MARCH_SPAN=1024, MARCH_UPPER_FORD=362, MARCH_LOWER_FORD=666 };

static const u16 marchSites[][2]={
    {511,520}, // central meadows
    {332,407}, // western woods
    {632,304}, // high pass
    {745,572}, // eastern foothills
    {500,710}, // south bank
    {215,563}, // western headland
    {649,628}, // east bank
    {750,726}, // above the bay
};

static void marchSite(int i,int *x,int *y){
    *x=marchSites[i][0]*WW/MARCH_SPAN;
    *y=marchSites[i][1]*WH/MARCH_SPAN;
}

static int marchDist(int dx,int dy){
    dx=abs(dx); dy=abs(dy);
    return dx>dy?dx+dy/2:dy+dx/2;
}

static int marchJitter(int s,int feature,int part,int range){
    return (int)(hsh(feature,part,s)%(u32)(2*range+1))-range;
}

enum { MARCH_FINE_NOISE=4,MARCH_NOISE_POINTS=(MARCH_SPAN>>MARCH_FINE_NOISE)+1 };
typedef struct { u8 top[MARCH_NOISE_POINTS],bottom[MARCH_NOISE_POINTS]; int gy,weight; } MarchNoise;

static int marchEase(int coordinate,int bits){
    int cell=1<<bits,t=coordinate&(cell-1);
    return t*t*(3*cell-2*t)>>(2*bits);
}

// Cache the two lattice rows shared by a scanline to keep the live preview cheap.
static void marchNoiseRow(MarchNoise *noise,int y,int bits,int s){
    int gy=y>>bits;
    if(gy!=noise->gy){
        for(int x=0;x<=(MARCH_SPAN>>bits);x++){
            noise->top[x]=(u8)hsh(x,gy,s);
            noise->bottom[x]=(u8)hsh(x,gy+1,s);
        }
        noise->gy=gy;
    }
    noise->weight=marchEase(y,bits);
}

static int marchNoiseAt(const MarchNoise *noise,int x,int bits){
    int gx=x>>bits,weight=marchEase(x,bits);
    int a=noise->top[gx],b=noise->top[gx+1],c=noise->bottom[gx],d=noise->bottom[gx+1];
    int top=a+((b-a)*weight>>bits),bottom=c+((d-c)*weight>>bits);
    return top+((bottom-top)*noise->weight>>bits)-128;
}

// Cubic interpolation makes the direction continuous at each survey point.
static int marchCurve(const s16 *points,int last,int at){
    int i=at/128,t=at%128;
    if(i>=last) return points[last];
    int a=points[i?i-1:0],b=points[i],c=points[i+1],d=points[i+1<last?i+2:last];
    int bend=2*a-5*b+4*c-d+(-a+3*b-3*c+d)*t/128;
    return b+((c-a+bend*t/128)*t/128)/2;
}

typedef struct { int x,y,rx,lean,level,xSlope,ySlope; } MarchOval;
typedef struct { int x,limit,level,slope; } MarchOvalRow;

// One cross section of a tilted ellipse. The inputs and result are in survey
// units. A positive level means inside. Precompute the divisions during setup.
static MarchOval marchOval(int x,int y,int rx,int ry,int lean){
    int level=(rx<ry?rx:ry)/2;
    return (MarchOval){x,y,rx,lean,level,(level<<14)/(rx*rx),(level<<14)/(ry*ry)};
}

static MarchOvalRow marchOvalRow(const MarchOval *oval,int y,int margin){
    int dy=y-oval->y;
    int level=oval->level-(dy*dy*oval->ySlope>>14);
    if(level<-margin) return (MarchOvalRow){0};
    return (MarchOvalRow){oval->x+dy*oval->lean/256,oval->rx*2+margin,level,oval->xSlope};
}

static int marchOvalLevel(const MarchOvalRow *row,int x){
    int dx=x-row->x;
    if(abs(dx)>=row->limit) return -1024;
    return row->level-(dx*dx*row->slope>>14);
}

typedef struct { s16 x,y,rx,ry,lean; u8 shift,spread; } MarchShoreForm;
static const MarchShoreForm marchCoastLand[]={
    {486,480,395,398,-31,26,16}, // mainland
    {176,505,138,145,-66,26,17}, // west headlands
    {776,665,148,172, 71,32,17}, // east headlands
    {481,203,265,116,132,36,23}, // northern uplands
    {356,742,153,142,-75,36,19}, // south peninsula
};
static const MarchShoreForm marchCoastIslands[]={
    {107,177, 53, 36,-54,25,10},
    {129,245, 51, 28,105,21, 8},
    {232, 91, 61, 32,-42,29,10},
    {909,292, 48, 68,-33,27,11},
    {951,381, 36, 43, 45,19,10},
    {914,841, 52, 38,125,27,10},
    {840,908, 62, 46, 43,34,12},
    {949,913, 42, 35,-97,18, 9},
    {519,951, 67, 46,145,33,12},
    { 92,749, 51, 64,-67,22,12},
    {156,817, 40, 38,154,20,12},
};
static const MarchShoreForm marchCoastBays[]={
    {156,167,145,158,114,40,33},
    { 93,344,125, 74,-126,35,21},
    {850,440,147, 67,225,34,19},
    {935,686, 98,135, 74,30,23},
    {622,911,184,139,-89,30,23},
    {711,104, 68,106,-98,30,20},
    {134,655,123, 55,138,33,18},
};
enum {
    MARCH_LAND_FORMS=sizeof marchCoastLand/sizeof marchCoastLand[0],
    MARCH_ISLAND_FORMS=sizeof marchCoastIslands/sizeof marchCoastIslands[0],
    MARCH_BAY_FORMS=sizeof marchCoastBays/sizeof marchCoastBays[0],
};

static MarchOval marchShoreOval(const MarchShoreForm *form,int s,int feature){
    return marchOval(form->x+marchJitter(s,feature,0,form->shift),
        form->y+marchJitter(s,feature,1,form->shift),
        form->rx+marchJitter(s,feature,2,form->spread),
        form->ry+marchJitter(s,feature,3,form->spread),
        form->lean+marchJitter(s,feature,4,112));
}

// Streams are sampled down the main valley or across the two sloping banks.
// A cross-valley profile uses five points from its spring to its confluence.
typedef struct { s16 start,end,ford,points[5]; } MarchTributary;
typedef struct { s16 x,ridge,west,east; u8 westFord,eastFord; } MarchColumn;
static MarchColumn marchColumns[MAXD];
enum { MARCH_NO_STREAM=-1024 };

static int marchRiver(const s16 *points,int y,int s){
    return marchCurve(points,8,y)+(vnoise(0,y,64,s+3)-128)/11;
}

static MarchTributary marchTributary(int western,int joinY,int joinX,int s){
    MarchTributary t;
    int part=western?1:2;
    t.start=(s16)((western?238:786)+marchJitter(s,part,19,western?22:28));
    t.end=(s16)joinX;
    int ford=western?368:joinX+(t.start-joinX)*3/8;
    t.ford=(s16)(ford+marchJitter(s,part,20,13));
    t.points[0]=(s16)((western?445:340)+marchJitter(s,part,21,23));
    t.points[1]=(s16)((western?480:352)+marchJitter(s,part,22,24));
    t.points[2]=(s16)((western?503:395)+marchJitter(s,part,23,21));
    t.points[3]=(s16)((western?480:397)+marchJitter(s,part,24,15));
    t.points[4]=(s16)joinY;
    return t;
}

static int marchTributaryAt(const MarchTributary *t,int x){
    int at=(x-t->start)*512/(t->end-t->start);
    if(at<0||at>512) return MARCH_NO_STREAM;
    return marchCurve(t->points,4,at);
}

static void genMarch(u32 seed){
    static const s16 riverPoints[]={365,395,472,523,559,533,598,704,811};
    static const u8 riverSpread[]={36,43,40,23,12,15,36,44,46};
    static const s16 ridgePoints[]={236,227,246,177,190,224,169,213,224};
    // Fold the full 32-bit seed into independently salted feature samples.
    int s=(int)(hsh((int)(seed>>16),(int)(seed&65535),91)&0x3fffffffu);
    s16 river[9],ridge[9];
    for(int i=0;i<9;i++){
        river[i]=(s16)(riverPoints[i]+marchJitter(s,42,i,riverSpread[i]));
        ridge[i]=(s16)(ridgePoints[i]+marchJitter(s,43,i,33));
    }
    int upperX=366+marchJitter(s,43,11,29),eastPassX=632+marchJitter(s,43,12,28);
    int springY=207+marchJitter(s,42,11,23),springX=marchRiver(river,springY,s);
    int westJoin=476+marchJitter(s,42,12,16),eastJoin=429+marchJitter(s,42,13,17);
    MarchTributary west=marchTributary(1,westJoin,marchRiver(river,westJoin,s),s);
    MarchTributary east=marchTributary(0,eastJoin,marchRiver(river,eastJoin,s),s);
    int split=720+marchJitter(s,42,14,12),deltaSpread=52+marchJitter(s,42,15,19);
    int deltaBend=66+marchJitter(s,42,16,22);

    MarchOval land[MARCH_LAND_FORMS],islands[MARCH_ISLAND_FORMS],bays[MARCH_BAY_FORMS];
    for(int i=0;i<MARCH_LAND_FORMS;i++) land[i]=marchShoreOval(&marchCoastLand[i],s,i+50);
    for(int i=0;i<MARCH_ISLAND_FORMS;i++) islands[i]=marchShoreOval(&marchCoastIslands[i],s,i+55);
    for(int i=0;i<MARCH_BAY_FORMS;i++) bays[i]=marchShoreOval(&marchCoastBays[i],s,i+70);
    MarchOval westShoulder=marchOval(272,531,138,181,-16);
    MarchOval eastShoulder=marchOval(716,642,125,151,31);
    MarchOval valley=marchOval(545,500,178,315,-14);
    MarchOval sources[]={
        marchOval(springX,springY,30+marchJitter(s,80,0,9),24,-75),
        marchOval(west.start,west.points[0],48+marchJitter(s,81,0,13),32,121),
        marchOval(east.start,east.points[0],31,25+marchJitter(s,82,0,7),-117),
    };
    int oxbowY=587+marchJitter(s,83,0,10),oxbowX=marchRiver(river,oxbowY,s)-62;
    MarchOval oxbow=marchOval(oxbowX,oxbowY,32,35+marchJitter(s,83,1,7),108);
    MarchOval oxbowBar=marchOval(oxbowX+12,oxbowY-5,25,29,108);
    int tarnX=306+marchJitter(s,84,0,35),tarnY=680+marchJitter(s,84,1,39);
    MarchOval tarn=marchOval(tarnX,tarnY,39,26+marchJitter(s,84,2,8),-128);

    int xScale=((MARCH_SPAN<<16)+WW-1)/WW,yScale=((MARCH_SPAN<<16)+WH-1)/WH;
    for(int tx=0;tx<WW;tx++){
        int x=tx*xScale>>16;
        marchColumns[tx]=(MarchColumn){(s16)x,(s16)marchCurve(ridge,8,x),
            (s16)marchTributaryAt(&west,x),(s16)marchTributaryAt(&east,x),
            abs(x-west.ford)<=6,abs(x-east.ford)<=6};
    }
    MarchNoise broadField={.gy=-1},detailField={.gy=-1},roughField={.gy=-1};
    for(int ty=0;ty<WH;ty++){
        int y=ty*yScale>>16;
        if(y<14||y>=MARCH_SPAN-14){ memset(world[ty],T_DEEP,WW); continue; }
        marchNoiseRow(&broadField,y,7,s);
        marchNoiseRow(&detailField,y,5,s+1);
        marchNoiseRow(&roughField,y,MARCH_FINE_NOISE,s+5);
        int mainX=marchRiver(river,y,s);
        int channel=9+y/105+(vnoise(0,y,64,s+11)-128)/35;
        int ford=abs(y-MARCH_UPPER_FORD)<=7||abs(y-MARCH_LOWER_FORD)<=7;
        int delta=y>=split?y-split:0;
        int westDelta=mainX-delta*deltaBend/100;
        int eastDelta=mainX+delta*deltaSpread/100;
        int deltaWidth=8+delta/45;
        int temperature=76+y/9+cClim*24;

        MarchOvalRow shoreLand[MARCH_LAND_FORMS],shoreIslands[MARCH_ISLAND_FORMS],shoreBays[MARCH_BAY_FORMS];
        int nLand=0,nIslands=0,nBays=0;
        for(int i=0;i<MARCH_LAND_FORMS;i++){
            MarchOvalRow row=marchOvalRow(&land[i],y,90);
            if(row.limit) shoreLand[nLand++]=row;
        }
        for(int i=0;i<MARCH_BAY_FORMS;i++){
            MarchOvalRow row=marchOvalRow(&bays[i],y,90);
            if(row.limit) shoreBays[nBays++]=row;
        }
        for(int i=0;i<MARCH_ISLAND_FORMS;i++){
            MarchOvalRow row=marchOvalRow(&islands[i],y,60);
            if(row.limit) shoreIslands[nIslands++]=row;
        }
        MarchOvalRow westShelf=marchOvalRow(&westShoulder,y,24);
        MarchOvalRow eastShelf=marchOvalRow(&eastShoulder,y,24);
        MarchOvalRow valleyFloor=marchOvalRow(&valley,y,24);
        MarchOvalRow pools[4];
        int nPools=0;
        for(int i=0;i<3;i++){
            MarchOvalRow row=marchOvalRow(&sources[i],y,90);
            if(row.limit) pools[nPools++]=row;
        }
        MarchOvalRow tarnRow=marchOvalRow(&tarn,y,90);
        if(tarnRow.limit) pools[nPools++]=tarnRow;
        MarchOvalRow oxbowRow=marchOvalRow(&oxbow,y,90);
        MarchOvalRow barRow=marchOvalRow(&oxbowBar,y,7);
        for(int tx=0;tx<WW;tx++){
            const MarchColumn *col=&marchColumns[tx];
            int x=col->x;
            if(x<14||x>=MARCH_SPAN-14){ world[ty][tx]=T_DEEP; continue; }
            int broad=marchNoiseAt(&broadField,x,7),detail=marchNoiseAt(&detailField,x,5);
            int warpedX=x+broad*(cCoast+5)/11+detail*(cCoast+2)/15;
            int shore=-1024;
            for(int i=0;i<nLand;i++){
                int level=marchOvalLevel(&shoreLand[i],warpedX);
                if(level>shore) shore=level;
            }
            for(int i=0;i<nBays;i++){
                int level=-marchOvalLevel(&shoreBays[i],warpedX);
                if(level<shore) shore=level;
            }
            int surf=broad*(cCoast+3)/20+detail*(cCoast+2)/15;
            shore+=surf;
            // The interior shoulders keep low land behind the inlets.
            int shelf=marchOvalLevel(&westShelf,x),eastLevel=marchOvalLevel(&eastShelf,x);
            if(eastLevel>shelf) shelf=eastLevel;
            int valleyLevel=marchOvalLevel(&valleyFloor,x);
            if(valleyLevel>shelf) shelf=valleyLevel;
            shelf+=surf/4;
            if(shelf>shore) shore=shelf;
            // Scattered skerries rise out of the water beyond the mainland.
            int mainland=shore,islandX=x+broad/5+detail/10;
            for(int i=0;i<nIslands;i++){
                int level=marchOvalLevel(&shoreIslands[i],islandX)+surf/2;
                if(level>shore) shore=level;
            }
            int edge=x<MARCH_SPAN-x?x:MARCH_SPAN-x;
            int edgeY=y<MARCH_SPAN-y?y:MARCH_SPAN-y;
            if(edgeY<edge) edge=edgeY;
            int sea=cSea*9;
            if(edge<112) sea+=(112-edge)*(112-edge)/145;
            shore-=sea;
            if(shore<-17){ world[ty][tx]=T_DEEP; continue; }
            if(shore<0){ world[ty][tx]=T_WATER; continue; }

            // Distances from the banks share one scale so smaller streams and
            // oxbows grade into the same sand, meadows and floodplain.
            int onMainland=mainland>=sea;
            int mainBank=onMainland&&y>=springY?abs(x-mainX)-channel:1024;
            if(onMainland&&delta){
                int branch=abs(x-westDelta)-deltaWidth;
                if(branch<mainBank) mainBank=branch;
                branch=abs(x-eastDelta)-deltaWidth;
                if(branch<mainBank) mainBank=branch;
            }
            int westBank=abs(y-col->west)-11,eastBank=abs(y-col->east)-10;
            int water=mainBank<=0||westBank<=0||eastBank<=0;
            if(water){
                int crossing=mainBank<=0?ford:westBank<=0?col->westFord:col->eastFord;
                world[ty][tx]=crossing?T_SAND:T_WATER;
                continue;
            }
            int bank=mainBank;
            if(westBank<bank) bank=westBank;
            if(eastBank<bank) bank=eastBank;
            int pool=-1024;
            for(int i=0;i<nPools;i++){
                int level=marchOvalLevel(&pools[i],x);
                if(level>pool) pool=level;
            }
            if(oxbowRow.limit){
                int outer=marchOvalLevel(&oxbowRow,x),inner=-marchOvalLevel(&barRow,x);
                if(inner<outer) outer=inner;
                if(outer>pool) pool=outer;
            }
            pool+=detail/26;
            if(pool>0){ world[ty][tx]=T_WATER; continue; }
            if(-pool<bank) bank=-pool;
            if(shore<5+detail/29||bank<=2+detail/56){ world[ty][tx]=T_SAND; continue; }

            int ridgeY=col->ridge+broad/4;
            int alpine=100-abs(y-ridgeY)+detail/2+broad/7;
            int end=x<510?x-148:873-x;
            // Saddles through the north ridge and a lower valley along each stream.
            int pass=abs(x-upperX-(y-210)/5-detail/10);
            int eastPass=abs(x-eastPassX+(y-210)/6+detail/12);
            if(eastPass<pass) pass=eastPass;
            if(alpine>end) alpine=end;
            if(alpine>15+pass) alpine=15+pass;
            int escarpmentX=744+(y-420)/6+broad/2;
            int escarpment=73-abs(x-escarpmentX)-abs(y-498)/4+detail/3;
            int gorge=23+abs(y-571+detail/16);
            if(escarpment>gorge) escarpment=gorge;
            int foothills=61-marchDist(x-333,((y-629)*3)/4)+broad/3+detail/2;
            int rise=alpine>escarpment?alpine:escarpment;
            if(foothills>rise) rise=foothills;
            if(bank<54&&rise>bank/2) rise=bank/2;

            int rough=marchNoiseAt(&roughField,x,MARCH_FINE_NOISE);
            int wet=129+broad/3+detail/2+rough/5+cWet*26;
            wet+=x<471?(471-x)/4:x>559?-(x-559)/4:0;
            int bankside=33-marchDist(x-490,y-523);
            if(bankside>0) wet+=bankside*2;
            int floodplain=y+broad/4+detail/3-645;
            if(floodplain>0&&bank<74){
                int floodWet=(74-bank)/2+19;
                wet+=floodplain<floodWet?floodplain:floodWet;
            }

            u8 terrain;
            if(rise>67-cMtn*12) terrain=temperature+detail/8<114?T_SNOW:T_MOUNT;
            else if(rise>36-cMtn*8) terrain=T_HILL;
            else if(temperature+detail/3<64) terrain=T_SNOW;
            else if(floodplain>16&&bank<74+detail/6&&wet>125&&rise<27) terrain=T_SWAMP;
            else if(wet<100&&temperature+detail/4>117) terrain=T_DESERT;
            else if(wet>165&&temperature+broad/5+detail/5+rough/9>153) terrain=T_JUNGLE;
            else if(wet>145&&!(bank<18&&wet<177)) terrain=T_FOREST;
            else if(rise>18&&wet<116&&rough>22) terrain=T_DIRT;
            else terrain=T_GRASS;
            world[ty][tx]=terrain;
        }
    }
    // Leave clearings on the dry ground at each settlement. Keep the opening
    // centered on buildable land in every climate.
    int radius=2+WW/192;
    for(int i=0;i<(int)(sizeof marchSites/sizeof marchSites[0]);i++){
        int x,y; marchSite(i,&x,&y);
        for(int dy=-radius;dy<=radius;dy++) for(int dx=-radius;dx<=radius;dx++){
            if(dx*dx+dy*dy>radius*radius) continue;
            if(world[y+dy][x+dx]>T_WATER) world[y+dy][x+dx]=T_GRASS;
        }
    }
    world[WH/2][WW/2]=T_GRASS;
}

// Places to visit in a newly created Riven March. These are ordinary map
// buildings, so saving, rendering and overworld edits use the existing data.
#ifndef GG_MARCH_LANDMARKS_H
#define GG_MARCH_LANDMARKS_H

enum { GG_FERRY_RADIUS=40 };

static int marchLandingReady(int x,int y){
    if(x<2||y<2||x>=WW-2||y>=WH-2||bld[y][x]||occ[y][x])return 0;
    int t=world[y][x];
    if(t<=T_WATER||t==T_LAVA||t==T_MOUNT)return 0;
    for(int d=0;d<4;d++){
        int nx=x+dxs[d],ny=y+dys[d];
        t=world[ny][nx];
        if(t>T_WATER&&t!=T_LAVA&&t!=T_MOUNT&&!bld[ny][nx]&&!occ[ny][nx])return 1;
    }
    return 0;
}

static void marchFerryLandings(int surveyY){
    int center=surveyY*WH/1024;
    int radius=5+WH/88;
    for(int r=0;r<=radius;r++){
        for(int side=0;side<(r?2:1);side++){
            int y=center+(side?-r:r);
            int stop=WW*7/8;
            if(y<3||y>=WH-3)continue;
            for(int x=WW*3/8;x<stop;){
                if(!isWater(x,y)){ x++;continue; }
                int left=x-1;
                while(x<stop&&isWater(x,y))x++;
                int width=x-left-1;
                // The other dock sits one tile past the end of the water.
                if(width<2||width>=GG_FERRY_RADIUS||
                   !marchLandingReady(left,y)||!marchLandingReady(x,y))continue;
                bld[y][left]=B_DOCK;
                bld[y][x]=B_DOCK;
                return;
            }
        }
    }
}

static void marchFort(int surveyX,int surveyY,int gate,int civic){
    int x0=surveyX*WW/1024,y0=surveyY*WH/1024;
    int radius=WW/40+8,best=radius*radius+1,x=-1,y=-1;
    for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
        int d=dx*dx+dy*dy,nx=x0+dx,ny=y0+dy;
        if(d>=best||!ggFortSite(nx,ny,gate)||vilAny(nx,ny,6)>=0)continue;
        best=d;x=nx;y=ny;
    }
    if(x<0||!ggPlaceFort(-1,x,y,gate))return;
    // Keep the external approaches clear. A service building sits
    // inside one courtyard corner and the other side stays open.
    putBld(-1,x-1,y-1,civic);
}

// Survey centers place services on both banks and on the approaches to the
// woods and bay. Extra stops on larger maps keep the distances between visits
// manageable while leaving each settlement its own patch of country.
static const struct { u16 x,y,minSize; u8 civic; } marchDistrictSites[]={
    {402,491,128,B_WORKSHOP}, {638,492,128,B_MARKET},
    {574,671,128,B_INN},     {221,442,128,B_INN},
    {433,351,384,B_MARKET},  {679,666,384,B_WORKSHOP},
    {283,698,768,B_MARKET},  {706,354,768,B_INN},
};

static int marchDistrictWalkway(int x,int y){
    return x>1&&y>1&&x<WW-2&&y<WH-2&&!bld[y][x]&&
           ((M_LAND>>world[y][x])&1);
}

static int marchDistrictGrade(int x,int y,int nx,int ny){
    return abs(ggExpGround(world[y][x])-ggExpGround(world[ny][nx]))<=GG_WORLD_WALK_RISE;
}

static int marchDistrictSite(int x,int y){
    // A lane divides the houses and the shop. Its approaches and the whole
    // outside ring stay open, including a path around either end of the lane.
    if(!ggOpenBuildingPlot(x+2,y-1)||!ggResidencePlan(x-2,y-2,B_MANOR)||
       !ggResidencePlan(x+1,y+1,B_HOUSE)||vilAny(x,y,5)>=0)return 0;
    for(int i=-3;i<=3;i++){
        if(!ggOpenBuildingPlot(x+i,y)||!marchDistrictWalkway(x+i,y-3)||
           !marchDistrictWalkway(x+i,y+3)||!marchDistrictWalkway(x-3,y+i)||
           !marchDistrictWalkway(x+3,y+i))return 0;
        if(i>-3&&(!marchDistrictGrade(x+i,y-3,x+i-1,y-3)||
                  !marchDistrictGrade(x+i,y+3,x+i-1,y+3)||
                  !marchDistrictGrade(x-3,y+i,x-3,y+i-1)||
                  !marchDistrictGrade(x+3,y+i,x+3,y+i-1)))return 0;
    }
    return (marchDistrictWalkway(x-4,y)&&marchDistrictGrade(x-3,y,x-4,y))||
           (marchDistrictWalkway(x+4,y)&&marchDistrictGrade(x+3,y,x+4,y));
}

static void marchDistricts(void){
    enum { N_DISTRICTS=sizeof marchDistrictSites/sizeof *marchDistrictSites };
    coord_t placed[N_DISTRICTS][2];
    int n=0, radius=WW/38+11, spacing=8+WW/32;
    for(int i=0;i<N_DISTRICTS;i++){
        if(WW<marchDistrictSites[i].minSize)continue;
        int sx=marchDistrictSites[i].x*WW/MARCH_SPAN;
        int sy=marchDistrictSites[i].y*WH/MARCH_SPAN;
        int x=-1,y=-1,best=radius*radius+1;
        for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
            int d=dx*dx+dy*dy,nx=sx+dx,ny=sy+dy;
            if(d>=best)continue;
            int k=0;
            for(;k<n;k++)if(marchDist(nx-placed[k][0],ny-placed[k][1])<spacing)break;
            if(k!=n||!marchDistrictSite(nx,ny))continue;
            best=d;x=nx;y=ny;
        }
        if(x<0)continue;
        // Both residences were checked together and have disjoint plots.
        for(int dx=-3;dx<=3;dx++){
            if(abs(dx)<=2)setCell(x+dx,y,T_DIRT);
            putBld(-1,x+dx,y,B_PAVING);
        }
        ggPlaceResidence(-1,x-2,y-2,B_MANOR);
        ggPlaceResidence(-1,x+1,y+1,B_HOUSE);
        putBld(-1,x+2,y-1,marchDistrictSites[i].civic);
        placed[n][0]=(coord_t)x;placed[n][1]=(coord_t)y;n++;
    }
}

static void marchCivicCenters(void){
    static const struct { u16 x,y,minSize; u8 kind,turn; } centers[]={
        {442,418,128,GG_MARKET_SQUARE,1},
        {690,533,384,GG_GUILD_YARD,3},
        {340,635,768,GG_GARDEN_COURT,2}
    };
    for(unsigned i=0;i<sizeof centers/sizeof *centers;i++){
        if(WW<centers[i].minSize)continue;
        int sx=centers[i].x*WW/MARCH_SPAN,sy=centers[i].y*WH/MARCH_SPAN;
        int radius=WW/60+8,x=-1,y=-1,best=radius*radius+1;
        GgBuildPlan plan;
        ggCompoundPlan(&plan,centers[i].kind,centers[i].turn);
        for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
            int d=dx*dx+dy*dy,nx=sx+dx,ny=sy+dy;
            if(d>=best||!ggOpenBuildingPlot(nx,ny)||vilAny(nx,ny,5)>=0)continue;
            if(ggPlanCheck(&plan,nx,ny)!=GG_SITE_CLEAR)continue;
            best=d;x=nx;y=ny;
        }
        if(x>=0)ggPlanCommit(&plan,-1,x,y);
    }
}

static void marchLandmarks(void){
    // Each landmark has its own district. Search the nearby dry ground
    // because river bends, coastlines and vegetation vary by seed.
    static const u16 sites[][3]={
        {345,286,B_RUIN}, {218,380,B_STATUE}, {408,638,B_RUIN},
        {622,425,B_STATUE}, {780,438,B_RUIN}, {696,293,B_RUIN},
        {251,645,B_RUIN}, {655,795,B_STATUE}, {782,700,B_RUIN},
    };
    marchFerryLandings(474);
    marchFerryLandings(714);
    if(cLife>0){
        marchFort(652,345,B_GATE_EW,B_BARRACKS);
        marchFort(380,704,B_GATE_NS,B_MARKET);
    }
    for(unsigned i=0;i<sizeof sites/sizeof *sites;i++){
        int x0=sites[i][0]*WW/1024,y0=sites[i][1]*WH/1024;
        int distance=WW/40+5,best=distance*distance+1,x=-1,y=-1;
        for(int dy=-distance;dy<=distance;dy++)
            for(int dx=-distance;dx<=distance;dx++){
                int d=dx*dx+dy*dy,nx=x0+dx,ny=y0+dy;
                if(d>=best||!buildable(nx,ny)||occ[ny][nx])continue;
                if(vilAny(nx,ny,5)>=0)continue;
                best=d;x=nx;y=ny;
            }
        if(x>=0)bld[y][x]=(u8)sites[i][2];
    }
    if(cLife>0){ marchDistricts();marchCivicCenters(); }
}
#endif

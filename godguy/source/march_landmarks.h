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
}
#endif

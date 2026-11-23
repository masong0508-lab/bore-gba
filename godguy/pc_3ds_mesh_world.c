// Source-integrated true 3D world regression, fully SDK free.
#define main ggMeshLegacyGameMain
#include "pc_explore3d.c"
#undef main
static void meshRequire(int good,const char*why){
    if(!good){fprintf(stderr,"3DS WORLD MESH FAIL: %s\n",why);exit(1);}
}
static void meshHumanPerspective(unsigned char *pixels){
    GgMeshContext*m=&ggMeshContext;
    static const unsigned char coat[3][3]={{14,12,9},{5,5,6},{21,16,12}};
    for(int axis=0;axis<2;axis++)for(int forward=384;forward<=1536;forward*=2){
        int cos=axis?0:4096,sin=axis?4096:0;
        ggMeshBegin(m,pixels,2,0,0,13,cos,sin,42,65,89);
        m->fogMax=0;
        if(axis)ggMeshWindow(m,-100,100,forward,1,0,25);
        else ggMeshSideFacade(m,forward,1,-100,100,0,25);
        int door=0;
        for(int y=0;y<240;y++){
            const unsigned char *ink=pixels+((y*400+200)*3);
            if(ink[0]==(axis?42:46)&&ink[1]==(axis?45:51)&&ink[2]==(axis?53:58))door++;
        }
        ggMeshBegin(m,pixels,2,0,0,13,cos,sin,42,65,89);
        m->fogMax=0;
        ggMeshPersonDraw(m,axis?0:forward,axis?forward:0,0,forward,0,
                         1001,4,128+axis*64,60,0,coat);
        int first=240,last=-1;
        for(int y=0;y<240;y++)for(int x=120;x<280;x++)if(m->depth[y*400+x]){
            if(y<first)first=y;
            if(y>last)last=y;
        }
        int person=last-first+1;
        if(door<12||person*100<door*70||person*100>door*96)
            fprintf(stderr,"Facade axis %d at %d: adult %d px, door %d px\n",
                    axis,forward,person,door);
        meshRequire(door>=12&&person*100>=door*70&&person*100<=door*96,
                    "resident and doorway do not share realistic proportions at equal depth");
        GgMeshProjected foot=ggMeshProject(m,(GgMeshCameraVertex){forward,0,0,0,0});
        meshRequire(last>=foot.y-2&&last<foot.y,
                    "perspective scaling lifted the resident's shoes off their ground point");
    }
    int near=ggMeshPersonHeight(m,192,0),far=ggMeshPersonHeight(m,384,0);
    meshRequire(abs(near-2*far)<=3,
                "a hard close-up size cap changes the resident's perspective");
    int frontRed=0;
    for(int turn=0;turn<2;turn++){
        int facing=turn?4096:-4096;
        ggExpScenicQuality=2;
        ggExplore.angle=turn?0:128;
        ggMeshBeginDaylight(m,pixels,2,0,0,13,facing,0,64,98,141,1);
        ggMeshPersonDraw(m,turn?600:-600,0,0,600,0,
                         1001,4,turn?128:0,60,0,coat);
        int red=ggMeshPersonCel.palette[GG_P_COAT_M][0];
        if(turn)meshRequire(red>frontRed,"resident colors ignore the world's sunlight direction");
        else frontRed=red;
    }
}
static void meshResidenceFootprint(unsigned char *pixels){
    const int x=22,y=21;
    GgExplore3D previous=ggExplore;
    bld[y][x]=B_ESTATE;
    bld[y][x+1]=B_WING_E;
    bld[y+1][x]=B_WING_S;
    bld[y+1][x+1]=B_WING_SE;
    ggExplore.px=(x-3)*256+128;
    ggExplore.py=y*256+128;
    ggExplore.angle=0;
    ggMeshWorldDraw(pixels,1);
    meshRequire(ggMeshBuildingsDrawn==1,
                "the four plots draw the same estate more than once");
    int eastX=(x+1)*256+128,eastY=y*256+128;
    int southX=x*256+128,southY=(y+1)*256+128;
    meshRequire(ggChronBuildingAt(eastX,eastY,12,1)&&
                ggChronBuildingAt(southX,southY,12,1)&&
                ggChronBuildingAt(eastX,southY,12,1),
                "the estate's wings do not provide physical cover");
    bld[y][x+1]=B_NONE;
    meshRequire(!ggChronBuildingAt(eastX,eastY,12,1)&&
                !ggMeshLotSolid(x,y,B_ESTATE,eastX,eastY,
                                ggExpGround(T_GRASS)+12,ggExpGround(T_GRASS))&&
                ggChronBuildingAt(southX,southY,12,1),
                "damage left invisible cover or removed an intact wing");
    bld[y][x]=bld[y+1][x]=bld[y+1][x+1]=B_NONE;
    ggExplore=previous;
}
static unsigned long long meshSurfaceHash(const void *data,size_t size){
    const unsigned char *p=data;
    unsigned long long hash=0;
    for(size_t i=0;i<size;i++)hash=hash*131+p[i];
    return hash;
}
static void meshPavedWalk(unsigned char *pixels){
    const int tx=28,ty=26,cx=tx*256+128,cy=ty*256+128;
    GgExplore3D previous=ggExplore;
    int quality=ggExpScenicQuality,scenery=ggExpSceneryEnabled;
    world[ty][tx]=T_DIRT;
    world[ty-1][tx]=T_HILL;
    ggExplore.px=cx-2*256;ggExplore.py=cy;ggExplore.angle=0;
    ggExpSceneryEnabled=0;
    for(int q=1;q<=2;q++){
        ggExpScenicQuality=q;
        ggMeshWorldDraw(pixels,0);
        GgMeshContext *m=&ggMeshContext;
        size_t depthBytes=(size_t)m->w*m->h*sizeof *m->depth;
        unsigned long long depth=meshSurfaceHash(m->depth,depthBytes);
        unsigned long long color=meshSurfaceHash(pixels,400*240*3);
        int count=ggMeshBuildingsDrawn;
        bld[ty][tx]=B_PAVING;
        ggMeshWorldDraw(pixels,0);
        meshRequire(depth==meshSurfaceHash(m->depth,depthBytes)&&
                    count==ggMeshBuildingsDrawn,
                    "paving changed the depth of the underlying terrain");
        meshRequire(color!=meshSurfaceHash(pixels,400*240*3),
                    "paving blends into dirt at a supported detail level");
        meshRequire(q!=1||m->textureSamples==0,
                    "paving added perspective texture work to reduced detail");
        bld[ty][tx]=B_NONE;
    }
    bld[ty][tx]=B_PAVING;
    meshRequire(ggExpWalkable(tx,ty,ggExpGround(T_GRASS),0)&&
                ggChronLand(tx,ty),"paving blocks player or Chronicle travel");
    meshRequire(ggExpLineClear(tx-2,ty,tx+1,ty)&&
                !ggChronBuildingAt(cx,cy,1,1),
                "paving obstructs line of sight or combat at ground level");
    int targetX,targetY;
    ggExpTarget(&targetX,&targetY);
    meshRequire(targetX==tx+1&&targetY==ty,
                "paving blocks the aim ray before its reachable target");
    ggChron.routeOx=tx-5;ggChron.routeOy=ty-5;
    ggChron.px[0]=(short)(tx+1);ggChron.py[0]=(short)ty;
    ggChronNavBuild(0);
    meshRequire(ggChronNavDistance[5*GG_CHRON_GRID+5]==1,
                "the Chronicle navigation cache skips a paved cell");
    bld[ty][tx]=B_NONE;
    world[ty][tx]=world[ty-1][tx]=T_GRASS;
    ggExpScenicQuality=quality;ggExpSceneryEnabled=scenery;
    ggExplore=previous;
}
static void meshWellCover(unsigned char *pixels){
    const int tx=28,ty=26,cx=tx*256+128,cy=ty*256+128;
    int ground=ggExpGround(T_GRASS);
    bld[ty][tx]=B_WELL;
    meshRequire(!ggExpWalkable(tx,ty,ground,0)&&!ggChronLand(tx,ty),
                "the well does not reserve its tile against walking");
    meshRequire(ggChronBuildingAt(cx-49,cy,6,1)&&
                !ggChronBuildingAt(cx,cy,6,1)&&
                !ggChronBuildingAt(cx-60,cy-60,6,1)&&
                !ggChronBuildingAt(cx-49,cy,GG_WORLD_EYE_HEIGHT,1),
                "well cover does not follow a low, hollow, chamfered ring");
    meshRequire(ggChronBuildingAt(cx-58,cy,9,1)&&
                !ggChronBuildingAt(cx-58,cy,5,1),
                "the projecting coping gives cover below its actual height");
    meshRequire(!ggChronCombatLine(cx-256,cy,ground+6,cx+256,cy,ground+6)&&
                ggChronCombatLine(cx-256,cy,ground+GG_WORLD_EYE_HEIGHT,
                                  cx+256,cy,ground+GG_WORLD_EYE_HEIGHT),
                "shots do not distinguish the well rim from the open air above it");
    for(int q=1;q<=2;q++){
        GgMeshContext *m=&ggMeshContext;
        ggMeshBegin(m,pixels,q,cx-384,cy,ground+6,4096,0,42,65,89);
        ggMeshBuilding(m,tx,ty,B_WELL,ground,q==2);
        meshRequire(m->depth[m->horizon*m->w+m->w/2]>180&&
                    m->triangles<=76,"the well lost its rim or exceeded its mesh budget");
        ggMeshBegin(m,pixels,q,cx-384,cy,ground+GG_WORLD_EYE_HEIGHT,
                    4096,0,42,65,89);
        ggMeshBuilding(m,tx,ty,B_WELL,ground,q==2);
        meshRequire(!m->depth[m->horizon*m->w+m->w/2],
                    "the low rim visually covers a standing eye-height ray");
    }
    world[ty][tx-1]=T_SAND;
    int verge=ggMeshGroundAtQ8(cx-53,cy);
    meshRequire(verge<ground&&ggChronBuildingAt(cx-53,cy,verge,0),
                "well cover leaves a gap along its downhill foundation");
    world[ty][tx-1]=T_GRASS;
    bld[ty][tx]=B_NONE;
}
static void meshEntrances(unsigned char *pixels){
    GgMeshContext*m=&ggMeshContext;
    int x=65*256,y=64*256;
    for(int ty=63;ty<=65;ty++)world[ty][64]=T_HILL;
    int floor=ggMeshGroundAtQ8(x+19,y+128);
    int nominal=ggExpGround(T_GRASS);
    meshRequire(floor>nominal+3,"door test needs an uphill threshold");
    ggMeshBegin(m,pixels,2,x+19-384,y+128,floor+13,4096,0,42,65,89);
    m->fogMax=0;
    ggMeshBuilding(m,65,64,B_HOUSE,nominal,1);
    int last=-1;
    for(int line=0;line<240;line++){
        const unsigned char *p=pixels+((line*400+200)*3);
        if(p[0]==46&&p[1]==51&&p[2]==58)last=line;
    }
    GgMeshProjected step=ggMeshProject(m,(GgMeshCameraVertex){384,0,floor,0,0});
    meshRequire(last>=step.y-2&&last<=step.y+2,
                "sloped ground buries part of the intended doorway");
    for(int ty=63;ty<=65;ty++)world[ty][64]=T_GRASS;
    ggMeshBegin(m,pixels,2,x+128,y+19-384,nominal+13,0,4096,42,65,89);
    m->fogMax=0;
    ggMeshBuilding(m,65,64,B_TEMPLE,nominal,1);
    int open=0;
    for(int line=0;line<240;line++){
        const unsigned char *p=pixels+((line*400+200)*3);
        if(p[0]==42&&p[1]==45&&p[2]==53)open++;
    }
    meshRequire(open>65,"the central portico post blocks the temple entrance");
}
static void meshBuildingSilhouettes(unsigned char *pixels){
    GgMeshContext *m=&ggMeshContext;
    int cx=64*256+128,cy=64*256+128;
    int ground=ggExpGround(T_GRASS);
    // Each sample is a camera ray aimed at a visible architectural
    // feature. Its depth distinguishes real projection from a decal.
    static const struct {
        int build,dx,dy,dz,minDepth,maxDepth;
        const char *failure;
    } checks[]={
        {B_HOUSE,-110,0,41,176,65535,"cottage roof lacks a raised gable"},
        {B_TOWER,-82,0,56,166,65535,"tower gallery does not project beyond its shaft"},
        {B_TEMPLE,-142,62,13,195,65535,"temple has no load-bearing porch on the visible gable"},
        {B_RUIN,-100,0,6,0,145,"ruin breach is blocked by an intact exterior wall"},
        {B_DOCK,-88,180,5,172,65535,"pier shoreward approach and landing have the same narrow width"},
        {B_BORDER,-108,56,3,0,0,"a border flag blocks the view like a solid roof"}
    };
    int passed=1;
    world[65][64]=T_WATER;
    for(unsigned i=0;i<sizeof checks/sizeof checks[0];i++){
        int eye=ground+(checks[i].build==B_TOWER?42:22);
        ggMeshBegin(m,pixels,2,cx-450,cy,eye,4096,0,42,65,89);
        ggMeshBuilding(m,64,64,checks[i].build,ground,1);
        GgMeshVertex point=ggMeshV(cx+checks[i].dx,cy+checks[i].dy,
                                  ground+checks[i].dz);
        GgMeshProjected sample=ggMeshProject(m,ggMeshCameraPoint(m,point));
        meshRequire((unsigned)sample.x<(unsigned)m->w&&
                    (unsigned)sample.y<(unsigned)m->h,
                    "building silhouette sample fell outside the camera");
        int depth=m->depth[sample.y*m->w+sample.x];
        if(depth<checks[i].minDepth||depth>checks[i].maxDepth){
            fprintf(stderr,"3DS ARCHITECTURE FAIL: %s (depth %d)\n",
                    checks[i].failure,depth);
            passed=0;
        }
    }
    world[65][64]=T_GRASS;
    meshRequire(passed,"building silhouettes lost their architectural volumes");
}
static void meshPorticoRotation(unsigned char *pixels){
    GgMeshContext *m=&ggMeshContext;
    int x=64*256+128,y=64*256+128,ground=ggExpGround(T_GRASS);
    GgMeshVertex column=ggMeshV(x-143,y-62,ground+12);
    for(int offset=-3;offset<=3;offset+=6){
        ggMeshBegin(m,pixels,2,x-100+offset,y-500,ground+22,0,4096,42,65,89);
        ggMeshBuilding(m,64,64,B_TEMPLE,ground,1);
        GgMeshProjected sample=ggMeshProject(m,ggMeshCameraPoint(m,column));
        meshRequire(m->depth[sample.y*m->w+sample.x]>=145,
                    "west temple column disappears when the camera crosses its facade plane");
    }
}
static void meshFoundationOverShelf(void){
    const int x=64*256,y=64*256;
    world[64][64]=T_MOUNT;world[64][63]=T_WATER;
    world[63][63]=T_SAND;world[65][63]=T_FOREST;
    for(int row=63;row<=65;row++)world[row][64]=row==64?T_MOUNT:T_HILL;
    int base=ggMeshFoundationGround(x+15,y+19,x+47,y+238,48);
    int shelf=ggMeshGroundAtQ8(x+15,y+37);
    meshRequire(base<shelf,"foundation skirt leaves a visible gap over a recessed shore shelf");
    world[64][64]=world[64][63]=world[63][63]=world[65][63]=T_GRASS;
    world[63][64]=world[65][64]=T_GRASS;
}
static void meshPlantedBoots(unsigned char *pixels){
    const int wx=65*256+128,wy=65*256+72,forward=384;
    GgMeshContext*m=&ggMeshContext;
    world[65][65]=T_HILL;
    int ground=ggMeshGroundAtQ8(wx,wy);
    ggMeshBegin(m,pixels,2,wx-forward,wy,ground+13,4096,0,55,77,99);
    ggMeshPersonDraw(m,wx,wy,ground,forward,0,1001,0,128,0,0,famCol[0]);
    int width=ggMeshPersonWidth(ggMeshPersonHeight(m,forward,ground));
    int left=m->w/2-width/2,support[2]={0,0};
    for(int x=left;x<left+width;x++){
        int column=(x-left)*GG_PERSON_W/width;
        if(!ggMeshPersonCel.pixels[(GG_PERSON_H-1)*GG_PERSON_W+column])continue;
        int y=m->h-1;
        while(y>=0&&!m->depth[y*m->w+x])y--;
        int side=(x-m->w/2)*forward/m->focal;
        int z=ggMeshGroundAtQ8(wx,wy+side);
        int floor=ggMeshProject(m,(GgMeshCameraVertex){forward,side,z,0,0}).y;
        meshRequire(abs(floor-y-1)<=3,"resident boot lifts or sinks on a cross slope");
        support[x>m->w/2]++;
    }
    meshRequire(support[0]>=3&&support[1]>=3,"cross-slope fixture missed both boots");
    world[65][65]=T_GRASS;
}
static void meshNearArchitecture(unsigned char *pixels){
    GgMeshContext *m=&ggMeshContext;
    ggMeshBegin(m,pixels,2,0,0,10,4096,0,42,65,89);
    // The near plane removes the front of this column. Its far face
    // must still occlude the view down the middle as the camera passes.
    ggMeshArchitectureBlock(m,64,-80,180,80,0,22,147,126,105);
    int depth=m->depth[m->horizon*m->w+m->w/2];
    meshRequire(abs(depth-65535/180)<=2,
                "column disappeared while crossing the near plane");
}
static void meshArchitectureCover(unsigned char *pixels){
    GgMeshContext *m=&ggMeshContext;
    int tile=64,edge=tile*256,center=edge+128,ground=ggExpGround(T_GRASS);
    int eye=ground+GG_WORLD_EYE_HEIGHT,passed=1;
    // The two authored porches project west (temple) and north (hall).
    // Aim along their center aisles and through the visible columns in
    // the same world-space directions that the rasterizer sees.
    for(int axis=0;axis<2;axis++){
        int build=axis?B_HALL:B_TEMPLE;
        int across=center+(axis?51:62);
        int sx=edge-380,end=edge+(axis?16:-5);
        bld[tile][tile]=build;
        ggMeshBegin(m,pixels,2,axis?across:sx,axis?sx:across,eye,
                    axis?0:4096,axis?4096:0,42,65,89);
        ggMeshBuilding(m,tile,tile,build,ground,1);
        int depth=m->depth[m->horizon*m->w+m->w/2];
        meshRequire(depth>=170,"porch cover fixture missed the rendered shaft");
        int column=ggChronCombatCover(axis?across:edge-15,
                                       axis?edge-1:across,eye);
        int blocked=!ggChronCombatLine(axis?across:sx,axis?sx:across,eye,
                                       axis?across:end,axis?end:across,eye);
        int aisle=ggChronCombatLine(axis?center:sx,axis?sx:center,eye,
                                   axis?center:end,axis?end:center,eye);
        if(!column||!blocked||!aisle){
            fprintf(stderr,"3DS COVER FAIL: %s porch column %d, blocked ray %d, open aisle %d\n",
                    axis?"hall":"temple",column,blocked,aisle);
            passed=0;
        }
    }
    bld[tile][tile]=B_STATUE;
    static const struct {int dy,dz,solid;} statue[]={{48,13,0},{0,13,1},{0,36,1}};
    for(unsigned i=0;i<sizeof statue/sizeof statue[0];i++){
        int y=center+statue[i].dy,z=ground+statue[i].dz;
        ggMeshBegin(m,pixels,2,center-400,y,z,4096,0,42,65,89);
        ggMeshBuilding(m,tile,tile,B_STATUE,ground,1);
        int solid=m->depth[m->horizon*m->w+m->w/2]!=0;
        meshRequire(solid==statue[i].solid,"statue cover fixture missed its expected silhouette");
        if(ggChronCombatCover(center,y,z)!=solid){
            fprintf(stderr,"3DS COVER FAIL: statue at lateral %d, height %d (rendered %d)\n",
                    statue[i].dy,statue[i].dz,solid);
            passed=0;
        }
    }
    bld[tile][tile]=B_RUIN;
    int x=edge+50,y=edge+206,z=ground+11;
    ggMeshBegin(m,pixels,2,edge-380,y,z,4096,0,42,65,89);
    ggMeshBuilding(m,tile,tile,B_RUIN,ground,1);
    meshRequire(m->depth[m->horizon*m->w+m->w/2],
                "ruin cover fixture missed the taller intersecting wall");
    if(!ggChronCombatCover(x,y,z)){
        fputs("3DS COVER FAIL: shorter ruin wall masks intersecting cover\n",stderr);
        passed=0;
    }
    bld[tile][tile]=B_NONE;
    meshRequire(passed,"combat cover disagrees with visible architectural volumes");
}
static void meshGateTraversal(void){
    GgExplore3D saved=ggExplore;
    const int tile=63,mid=tile*256+128,front=tile*256-24;
    const int eye=ggExpGround(T_GRASS)+GG_WORLD_EYE_HEIGHT;
    for(int north=0;north<=1;north++){
        int gate=north?B_GATE_NS:B_GATE_EW;
        bld[tile][tile]=gate;
        int ax=north?mid:front,ay=north?front:mid;
        int bx=north?mid:front+320,by=north?front+320:mid;
        meshRequire(ggChronGateRoute(tile,tile,tile+!north,tile+north)&&
                   !ggChronGateRoute(tile,tile,tile+north,tile+!north),
                    "campaign path crosses a gate sideways through its masonry");
        meshRequire(ggChronCombatLine(ax,ay,eye,bx,by,eye),
                    "visible gate passage blocks a shot through its center");
        ggExplore.px=ax;ggExplore.py=ay;ggExplore.angle=north?64:0;
        for(int step=0;step<16;step++)ggExpAdvance(KEY_UP,0,0,0);
        meshRequire(north?ggExplore.py>=by:ggExplore.px>=bx,
                    "first-person movement cannot walk through a gate");
        int dx=north?78:0,dy=north?0:78;
        meshRequire(!ggChronCombatLine(ax+dx,ay+dy,eye,bx+dx,by+dy,eye),
                    "gate pier does not stop a shot");
        ggExplore.px=ax+dx;ggExplore.py=ay+dy;
        for(int step=0;step<16;step++)ggExpAdvance(KEY_UP,0,0,0);
        meshRequire(north?ggExplore.py<mid:ggExplore.px<mid,
                    "first-person movement passes through a gate pier");
    }
    bld[tile][tile]=B_NONE;
    ggExplore=saved;
}
static void meshRoundGeometryRegression(unsigned char *pixels){
    // Exercise the real final facet of every circular world primitive.
    // In particular, the six/eight-sided closing vertex used to read
    // sin/cos[256]; the bounds-sanitized integration build must catch that
    // even if an unrelated neighbor in static memory looks plausible.
    int x=64*256+128,y=64*256+128;
    int ground=ggMeshGroundAtQ8(x,y);
    for(int quality=1;quality<=2;quality++)
      for(int angle=0;angle<256;angle+=64)
        for(int shape=0;shape<5;shape++){
            int cosine=ggExpCos[angle],sine=ggExpSin[angle];
            ggMeshBegin(&ggMeshContext,pixels,quality,
                        x-cosine*512/4096,y-sine*512/4096,
                        ground+13,cosine,sine,42,65,89);
            switch(shape){
            case 0:
                // The legacy 2x path deliberately omits soft shadows.
                if(ggMeshContext.scale>1)continue;
                ggMeshContactShadow(&ggMeshContext,x,y,70,13);break;
            case 1:
                ggMeshFoliage(&ggMeshContext,x,y,ground,1,733,quality);break;
            case 2:
                ggMeshShrub(&ggMeshContext,x,y,ground,733,2);break;
            case 3:
                ggMeshCloudPuff(&ggMeshContext,x,y,ground+15,90,7,
                                130,140,160);break;
            case 4:
                ggMeshActorSpecies(&ggMeshContext,x,y,ground,
                    0,0,0,20,famCol[0],quality);break;
            }
            meshRequire(ggMeshContext.triangles>=4&&
                        ggMeshContext.fragments>8,
                        "closed world primitive disappeared at a camera heading");
        }
}
int main(void){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    ggExpBegin(64,64,0);
    ggExpMeshEnabled=1;ggExpScenicQuality=2;
    ggExplore.angle=0;ggExplore.px=64*256+128;
    ggExplore.py=64*256+128;
    bld[64][67]=B_HALL;bld[66][69]=B_HOUSE;
    bld[63][70]=B_TEMPLE;
    static unsigned char pixels[400*240*3];
    meshHumanPerspective(pixels);
    meshResidenceFootprint(pixels);
    meshPavedWalk(pixels);
    meshWellCover(pixels);
    meshEntrances(pixels);
    meshBuildingSilhouettes(pixels);
    meshPorticoRotation(pixels);
    meshFoundationOverShelf();
    meshPlantedBoots(pixels);
    meshNearArchitecture(pixels);
    meshArchitectureCover(pixels);
    meshGateTraversal();
    ggExpRenderMode(pixels,1);
    meshRequire(ggExpMeshActive&&ggMeshContext.w==400,
                "cinema is still a raycast image, not 3D mesh geometry");
    meshRequire(ggMeshContext.blendedFragments>50,
                "buildings, people and vegetation have no actual world shadows");
    meshRequire(ggMeshContext.textureSamples>600,
                "new mesh still consists of flat-color surfaces");
    meshRequire(ggMeshGroundMaterial(T_GRASS)==GG_MAT_MEADOW&&
                ggMeshGroundMaterial(T_SNOW)==GG_MAT_SNOW&&
                ggMeshGroundMaterial(T_MOUNT)==GG_MAT_ROCK,
                "material sampler ignores physical terrain biomes");
    meshRequire(ggMeshContext.triangles>100&&
                ggMeshContext.fragments>1500&&
                ggMeshTilesDrawn>20&&ggMeshBuildingsDrawn>=2,
                "live world polygons or building volumes are missing");
    meshRequire(ggMeshPropsDrawn>0&&
                ggMeshPropsDrawn<=ggFilmScenicBudget(ggExpScenicQuality,0),
                "visible scenery budget is empty or allocates beyond the fixed queue");
    // Dynamic polygonal people and animals use real 3D bones and
    // floor joints. The image itself must change between poses, not
    // just a simulated animation counter.
    static const u8 colors[3][3]={
       {20,17,12},{10,8,7},{26,21,16}
    };
    int beforeActor=ggMeshContext.triangles;
    ggMeshActorCharacter(&ggMeshContext,65*256+128,64*256+128,
                         ggMeshGroundAtQ8(65*256+128,64*256+128),
                         0,30,2,1,colors,2);
    meshRequire(ggMeshContext.triangles>beforeActor+45,
                "humanoid inhabitants remain flat 8x8 bitmap sprites");
    beforeActor=ggMeshContext.triangles;
    // Test a visible quadruped next to the center ray, not a subject
    // beyond the camera's side frustum. Count real rasterized faces.
    ggMeshActorCharacter(&ggMeshContext,65*256+128,64*256+160,
                         ggMeshGroundAtQ8(65*256+128,64*256+160),
                         0,80,4,0,colors,2);
    meshRequire(ggMeshContext.triangles>beforeActor+20,
                "four-legged species did not receive actual 3D bodies");
    ggExpRenderMode(pixels,1);
    unsigned long long checksum=0;
    for(int i=0;i<400*240*3;i+=13)
        checksum=checksum*131+pixels[i];
    ggExplore.angle=16;
    ggExpRenderMode(pixels,1);
    unsigned long long moved=0;
    for(int i=0;i<400*240*3;i+=13)moved=moved*131+pixels[i];
    meshRequire(checksum!=moved,
                "3D buildings did not exhibit moving-camera perspective");
    meshRequire(ggMeshContext.w==400&&
                ggMeshContext.fragments>500,
                "perspective camera rotation lost all physical mesh faces");
    ggExpScenicQuality=2;
    ggExpRenderMode(pixels,0);
    meshRequire(ggMeshContext.w==400&&ggMeshContext.scale==1,
                "New 3DS high-detail exploration is still a chunky 2x viewport");
    ggExpScenicQuality=1;
    ggExpRenderMode(pixels,0);
#ifdef GG_NEW3DS_ONLY
    meshRequire(ggMeshContext.w==400&&ggMeshContext.scale==1&&
                ggMeshContext.textureSamples==0,
                "New 3DS frame-budget fallback must shed textures, not resolution");
#else
    meshRequire(ggMeshContext.w==200&&ggMeshContext.scale==2&&
                ggMeshContext.textureSamples==0,
                "Old 3DS must keep its inexpensive untextured fallback");
#endif
    // Every actor still uses the same physically projected world foot.
    for(int wx=64*256;wx<=65*256;wx+=64)
        meshRequire(ggMeshGroundAtQ8(wx,64*256+128)>=0,
                    "sloped triangle ground failed point sampling");
    // Verify live occupant integration, not only manually drawn rigs.
    nCr=1;sp[0].a=0;arch[0].diet=4;arch[0].shape=1;
    cr[0]=(Cr){0,66,64,1,0};crId[0]=733;
    occ[64][66]=1;ggExpMeshEnabled=1;
    ggExpScenicQuality=2;ggExplore.angle=0;
    ggExpRenderMode(pixels,0);
    meshRequire(ggExpNpcsDrawn==1,
                "playable citizen was not passed to the 3D actor renderer");
    meshRequire(ggMeshPersonCount==1&&ggMeshPersonPixels>40,
                "playable person is not a visibly rendered procedural sprite");
    meshRequire(ggMeshPersonFacing(128,0)==0&&
                ggMeshPersonFacing(0,0)==4&&
                ggMeshPersonFacing(192,0)==2,
                "world-anchored procedural people have incorrect turn directions");
    meshRequire(ggMeshPersonWidth(48)==32&&
                ggMeshPersonWidth(96)==64&&
                ggMeshPersonWidth(192)==128,
                "3DS citizens have horizontally squashed faces and body sprites");
    occ[64][66]=0;nCr=0;
    // Replace one cell with a steep mountain while its neighbors remain
    // grass. The four mesh slopes must plant architectural foundations and
    // the moving eye at their REAL Q8 positions, not at cell-average Z.
    world[64][64]=T_MOUNT;
    int sx=64*256+128,sy=64*256+128;
    int nominal=ggMeshGroundAtQ8(sx,sy);
    int slope=ggMeshGroundAtQ8(sx-64,sy);
    meshRequire(nominal==ggExpGround(T_MOUNT)&&slope<nominal,
                "triangular hillside sampler stopped interpolating");
    int foundation=ggMeshFoundationGround(64*256+28,64*256+28,
                                           64*256+228,64*256+228,nominal);
    meshRequire(foundation<nominal&&
                foundation<=ggMeshGroundAtQ8(64*256+28,64*256+28),
                "building base still floats over its downhill terrain corner");
    ggExplore.stepPhase=0;
    ggExplore.px=sx-64;ggExplore.py=sy;
    ggExpRenderMode(pixels,0);
    meshRequire(ggMeshContext.eye==slope+13,
                "3DS eye still jumps to the nominal height on a hillside");
    ggExplore.px=sx;ggExplore.py=sy;
    ggExpRenderMode(pixels,0);
    meshRequire(ggMeshContext.eye==nominal+13&&
                ggMeshContext.eye>slope+13,
                "camera climb did not follow physical terrain continuously");
    ggExpScenicQuality=1;
    ggExpRenderMode(pixels,0);
#ifdef GG_NEW3DS_ONLY
    meshRequire(ggMeshContext.eye==nominal+13&&
                ggMeshContext.scale==1&&ggMeshContext.w==400,
                "New 3DS native quality path lost slope-following camera height");
#else
    meshRequire(ggMeshContext.eye==nominal+13&&
                ggMeshContext.scale==2&&ggMeshContext.w==200,
                "Old 3DS quality path lost terrain-following camera height");
#endif
    ggExpScenicQuality=2;
    world[64][64]=T_GRASS;
    ggExplore.px=sx;ggExplore.py=sy;
    // Moving an actual simulation citizen changes its world-space feet
    // and heading, not merely the animation tile or a screen-space wobble.
    ggPersonTrackReset();
    GgPersonMotion planted=ggPersonMotionSample(733,66*256+128,
                                                64*256+128,0,0);
    GgPersonMotion striding=ggPersonMotionSample(733,67*256+128,
                                                 64*256+128,10,0);
    GgPersonMotion mid=ggPersonMotionSample(733,67*256+128,
                                            64*256+128,15,0);
    meshRequire(!planted.walking&&striding.walking&&
                mid.x>striding.x&&mid.x<67*256+128&&
                mid.heading==0,
                "3DS citizens still teleport cell-to-cell instead of walking");
    ggPersonTrackReset();
    // Wall/world occlusion cannot be bypassed by the new generated
    // sprite painter. Front/behind use the SAME camera and geometry.
    ggMeshBegin(&ggMeshContext,pixels,2,0,0,10,4096,0,42,65,89);
    ggMeshContext.fogMax=0;
    ggMeshQuad(&ggMeshContext,
               ggMeshV(512,-400,0),ggMeshV(512,400,0),
               ggMeshV(512,400,40),ggMeshV(512,-400,40),
               111,101,93);
    ggMeshPersonCount=ggMeshPersonPixels=0;
    ggMeshPersonDraw(&ggMeshContext,1024,0,0,1024,0,
                     1001,4,128,60,0,colors);
    meshRequire(ggMeshPersonPixels==0&&ggMeshPersonCount==0,
                "procedural person visible through nearer stone building");
    ggMeshPersonDraw(&ggMeshContext,256,0,0,256,0,
                     1001,4,128,60,0,colors);
    meshRequire(ggMeshPersonCount==1&&ggMeshPersonPixels>50,
                "nearer procedural person clipped behind farther wall");
    // Test a stable city resident's eight orientations without an
    // artist-drawn atlas or any camera-animated face randomization.
    GgPersonCel still,blink;
    ggPersonGenerate(&still,1001,4,0,60,0,colors);
    ggPersonGenerate(&blink,1001,4,0,135,0,colors);
    meshRequire(!memcmp(still.palette,blink.palette,
                        sizeof still.palette),
                "procedural citizen rerolled colors between frames");
    // The actual jittered location must meet the camera's horizontal
    // frustum before it can spend one of the 34/56/84 decor slots.
    ggMeshBegin(&ggMeshContext,pixels,2,0,0,10,4096,0,42,65,89);
    int sceneryForward=0;
    meshRequire(ggMeshSceneryVisible(&ggMeshContext,1024,0,96,
                                    &sceneryForward)&&
                sceneryForward==1024,
                "centerline village decoration is rejected by culling");
    meshRequire(ggMeshSceneryVisible(&ggMeshContext,1024,900,96,0)&&
                ggMeshSceneryVisible(&ggMeshContext,1024,-900,96,0),
                "near-screen-edge prop crowns pop away prematurely");
    meshRequire(!ggMeshSceneryVisible(&ggMeshContext,1024,1900,96,0)&&
                !ggMeshSceneryVisible(&ggMeshContext,1024,-1900,96,0)&&
                !ggMeshSceneryVisible(&ggMeshContext,-1024,0,96,0),
                "offscreen or behind-camera scenery wastes the polygon budget");
    ggMeshBegin(&ggMeshContext,pixels,1,0,0,10,0,4096,42,65,89);
    meshRequire(ggMeshSceneryVisible(&ggMeshContext,0,1024,96,0)&&
                !ggMeshSceneryVisible(&ggMeshContext,1600,1024,96,0),
                "Old 3DS scene-culling ignores camera rotation/low-res viewport");
    // The Old 3DS world uses 200x120 depth and a 2x2 output grid.
    // An actor drawn on single 400x240 pixels creates flickering limbs,
    // inconsistent silhouettes, and four times as many depth probes.
    ggMeshBegin(&ggMeshContext,pixels,1,0,0,10,4096,0,42,65,89);
    ggMeshPersonCount=ggMeshPersonPixels=0;
    ggMeshPersonDraw(&ggMeshContext,512,0,0,512,0,
                     1001,4,128,60,0,colors);
    meshRequire(ggMeshPersonCount==1&&ggMeshPersonPixels>100,
                "citizen disappears after native/low quality mode change");
#ifndef GG_NEW3DS_ONLY
    for(int y=0;y<240;y+=2)for(int x=0;x<400;x+=2){
        const unsigned char *p=pixels+(y*400+x)*3;
        meshRequire(!memcmp(p,p+3,3)&&
                    !memcmp(p,p+400*3,3)&&
                    !memcmp(p,p+400*3+3,3),
                    "coarse citizen writes broken 2x2 pixels over 3D geometry");
    }
#endif
    meshRoundGeometryRegression(pixels);
    // Keep legacy renderer selectable while the new 3D port is tuned.
    ggExpMeshEnabled=0;
    ggExpRenderMode(pixels,0);
    meshRequire(!ggExpMeshActive,
                "legacy raster fallback was accidentally removed");
    puts("3DS WORLD MESH OK: live terrain/buildings, geometry parallax, physical z depth, scalable framebuffer and fallback");
    return 0;
}

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
                ggMeshActorHorizontalBody(&ggMeshContext,
                    x-29,y,x+29,y,ground+9,17,5,
                    150,110,75,quality>=2?8:6);break;
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
    meshEntrances(pixels);
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

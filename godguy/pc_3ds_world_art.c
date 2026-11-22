// Actual-source environment art regression. The flat-ground check fails
// on the previous pinwheel-lit terrain, even with all textures disabled.
#ifndef GG_NEW3DS_ONLY
#define GG_NEW3DS_ONLY
#endif
#define main ggWorldArtLegacyMain
#include "pc_explore3d.c"
#undef main
static void artRequire(int condition,const char*why){
    if(!condition){fprintf(stderr,"WORLD ART FAIL: %s\n",why);exit(1);}
}
static void artFlatGround(unsigned char *pixels,int terrain){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=(u8)terrain;bld[y][x]=0;occ[y][x]=0;
    }
    GgMeshContext*m=&ggMeshContext;
    ggMeshBegin(m,pixels,1,64*256+128,64*256+128,
                ggExpGround(terrain)+13,4096,0,80,120,150);
    m->fogMax=0;
    for(int y=60;y<69;y++)for(int x=63;x<70;x++)
        ggMeshGroundTile(m,x,y,terrain,ggFilmScenicHash(x,y,17));
    int minimum[3]={255,255,255},maximum[3]={0,0,0},samples=0;
    for(int y=160;y<235;y+=3)for(int x=10;x<390;x+=3){
        if(m->depth[y*400+x]==0)continue;
        for(int c=0;c<3;c++){
            int value=pixels[(y*400+x)*3+c];
            if(value<minimum[c])minimum[c]=value;
            if(value>maximum[c])maximum[c]=value;
        }
        samples++;
    }
    artRequire(samples>2000,"flat-ground comparison missed the physical surface");
    for(int c=0;c<3;c++)
        artRequire(maximum[c]-minimum[c]<=1,
          "flat biome contains tile-edge or four-sector lighting seams");
    artRequire(m->w==400&&m->h==240&&m->textureSamples==0,
                "budget mode changed native resolution or re-enabled per-pixel materials");
}
static void artFacadeDepth(unsigned char *pixels){
    // Deliberately choose a distance at which wall, frame and pane
    // have IDENTICAL stored inverse depths. A typical village wall
    // used to hide all its windows at exactly these common distances.
    GgMeshContext*m=&ggMeshContext;
    int cx=64*256,cy=64*256+128,wall=cx+1100;
    for(int occluder=0;occluder<2;occluder++){
        ggMeshBegin(m,pixels,2,cx,cy,23,4096,0,80,120,150);
        m->fogMax=0;
        ggMeshQuad(m,ggMeshV(wall,cy-100,10),ggMeshV(wall,cy+100,10),
                     ggMeshV(wall,cy+100,47),ggMeshV(wall,cy-100,47),
                     185,175,149);
        static unsigned char covered[400*240*3];
        if(occluder){
            int front=cx+900;
            ggMeshQuad(m,ggMeshV(front,cy-120,8),ggMeshV(front,cy+120,8),
                       ggMeshV(front,cy+120,50),ggMeshV(front,cy-120,50),
                       140,55,48);
            memcpy(covered,pixels,sizeof covered);
        }
        ggMeshSideFacade(m,wall,1,cy-100,cy+100,10,37);
        artRequire(m->depthBias==0,"facade depth tolerance leaked into later actors");
        if(occluder){
            artRequire(memcmp(covered,pixels,sizeof covered)==0,
                        "distant facade painted over a nearer physical occluder");
        }else{
            int blue=0;
            for(int y=0;y<240;y++)for(int x=0;x<400;x++){
                int k=(y*400+x)*3;
                if(pixels[k+1]>pixels[k]+25&&pixels[k+2]>pixels[k]+35)
                    blue++;
                if(m->depth[y*400+x])
                    artRequire(m->depth[y*400+x]<=65535/(1100-9),
                                "facade altered physical stored inverse depth");
            }
            // Count only rendered glazing, not the blue background.
            int glazing=0;
            for(int y=0;y<240;y++)for(int x=0;x<400;x++){
                int k=(y*400+x)*3;
                if(m->depth[y*400+x]&&pixels[k+1]>pixels[k]+25&&
                   pixels[k+2]>pixels[k]+35)glazing++;
            }
            artRequire(glazing>20&&blue>glazing,
                        "quantized wall depth still hides real window glazing");
        }
    }
}
static void artPropAnchors(unsigned char *pixels){
    // The reserved body lane is 20 Q8 units either side of each
    // centre-to-centre cardinal segment. Broad solid bases reach at
    // most 60 units from their anchor (fences); trunk/pole bases reach
    // at most 13. Tree canopies and flags may overhang above these lanes.
    for(int kind=GG_SCENE_SHRUB;kind<=GG_SCENE_STALL;kind++){
        if(kind==GG_SCENE_CROPS||kind==GG_SCENE_REEDS)continue;
        int narrow=kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
                   kind==GG_SCENE_LAMP||kind==GG_SCENE_BRAZIER||
                   kind==GG_SCENE_BANNER;
        for(int sample=0;sample<256;sample++){
            uint32_t key=ggFilmScenicHash(sample,kind,7);
            int x,y;ggMeshPropAnchor(64,64,kind,key,&x,&y);
            int dx=abs(x-(64*256+128)),dy=abs(y-(64*256+128));
            int base=narrow?13:60;
            artRequire(dx-base>20&&dy-base>20,
                        "solid scenery base blocks a central cardinal body lane");
            if(narrow){
                int apart=dx-dy;
                artRequire(apart*apart>2*(20+base)*(20+base),
                            "narrow trunk/pole lies on a diagonal walking lane");
            }
            int againX,againY;
            occ[64][64]=1;
            ggMeshPropAnchor(64,64,kind,key,&againX,&againY);
            occ[64][64]=0;
            artRequire(x==againX&&y==againY,
                        "scenery jumps when a citizen enters its cell");
            for(int corner=0;corner<4;corner++){
                int tx=(corner&1)?WW-1:0,ty=(corner&2)?WH-1:0;
                ggMeshPropAnchor(tx,ty,kind,key,&x,&y);
                artRequire(x>=tx*256&&x<(tx+1)*256&&
                           y>=ty*256&&y<(ty+1)*256,
                            "scenery anchor escapes its boundary world cell");
            }
        }
    }
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    // Confirm the real draw uses the shared anchor, not just the queue.
    // At trunk height, the old cell-centre position must remain clear.
    GgMeshContext*m=&ggMeshContext;
    int wx,wy;uint32_t key=ggFilmScenicHash(66,64,7);
    ggMeshPropAnchor(66,64,GG_SCENE_OAK,key,&wx,&wy);
    ggMeshBegin(m,pixels,2,64*256+128,64*256+128,23,4096,0,80,120,150);
    m->fogMax=0;
    ggMeshProp(m,66,64,GG_SCENE_OAK,10,key,1);
    GgMeshProjected trunk=ggMeshProject(m,
        ggMeshCameraPoint(m,ggMeshV(wx,wy,15)));
    GgMeshProjected lane=ggMeshProject(m,
        ggMeshCameraPoint(m,ggMeshV(66*256+128,64*256+128,15)));
    artRequire(trunk.x>=0&&trunk.x<400&&trunk.y>=0&&trunk.y<240&&
               m->depth[trunk.y*400+trunk.x]!=0,
                "actual tree trunk does not render at its shared scenery anchor");
    artRequire(m->depth[lane.y*400+lane.x]==0,
                "actual trunk still occupies the citizen's cell-centre position");
    // Culling and distance/LOD ordering must use the same anchor. At
    // eight headings the old copied central jitter produces wrong depths.
    nCr=0;ggExpBegin(64,64,0);ggExpScenicQuality=2;
    for(int heading=0;heading<256;heading+=32){
        ggExplore.angle=heading;ggMeshWorldDraw(pixels,1);
        artRequire(ggMeshPropsDrawn>10,"anchor culling comparison selected no useful scene");
        for(int i=0;i<ggMeshPropsDrawn;i++){
            const GgMeshVisibleProp*p=&ggMeshVisibleProps[i];
            ggMeshPropAnchor(p->tx,p->ty,p->kind,p->key,&wx,&wy);
            int forward=(int)(((int64_t)(wx-m->cx)*m->cosine+
                                (int64_t)(wy-m->cy)*m->sine)/4096);
            artRequire(p->forward==forward&&
                       ggMeshSceneryVisible(m,wx,wy,136,NULL),
                        "visible prop queue culls a different position from the actual draw");
        }
    }
}
static void artTropicalSilhouette(unsigned char *pixels){
    memset(world,T_JUNGLE,sizeof world);
    int x=65*256+128,y=64*256+128,z=ggMeshGroundAtQ8(x,y);
    GgMeshContext *m=&ggMeshContext;
    int frame=ggExplore.frame;
    ggExplore.frame=40;
    ggMeshBegin(m,pixels,2,x-448,y,z+20,4096,0,36,59,89);
    ggMeshFoliage(m,x,y,z,2,0x2571bb32u,2);
    int crown=0,trunk=0;
    for(int y0=25;y0<46;y0++)for(int x0=150;x0<250;x0++)
        crown+=m->depth[y0*m->w+x0]!=0;
    for(int y0=100;y0<111;y0++)for(int x0=174;x0<227;x0++)
        trunk+=m->depth[y0*m->w+x0]!=0;
    artRequire(crown>75&&trunk>20&&trunk<160&&m->triangles<92,
               "tropical canopy does not stand clear above a narrow trunk");
    ggExplore.frame=frame;
}
static void artSceneryToggle(unsigned char *pixels){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    bld[64][67]=B_HALL;nCr=0;ggExpBegin(64,64,0);
    ggExplore.angle=0;ggExpScenicQuality=2;
    int previous=ggExpSceneryEnabled;ggExpSceneryEnabled=0;
    ggMeshWorldDraw(pixels,1);
    int tiles=ggMeshTilesDrawn,buildings=ggMeshBuildingsDrawn;
    artRequire(ggMeshPropsDrawn==0&&tiles>20&&buildings==1,
                "scenery toggle hides terrain/buildings or leaves decorative props enabled");
    GgMeshContext*m=&ggMeshContext;
    GgMeshCameraVertex point=ggMeshCameraPoint(m,
        ggMeshV(67*256+28,64*256+128,23));
    GgMeshProjected wall=ggMeshProject(m,point);
    artRequire(ggMeshHidden(m,wall.x,wall.y,point.f+256),
                "disabled scenery removes the actual building's foreground depth");
    static unsigned char clean[400*240*3];
    memcpy(clean,pixels,sizeof clean);
    ggExpSceneryEnabled=1;ggMeshWorldDraw(pixels,1);
    artRequire(ggMeshPropsDrawn>10&&ggMeshTilesDrawn==tiles&&
               ggMeshBuildingsDrawn==buildings,
                "restoring scenery changed authoritative terrain/buildings");
    int changed=0;
    for(int i=0;i<400*240;i++)changed+=memcmp(clean+i*3,pixels+i*3,3)!=0;
    artRequire(changed>500,"scenery flag does not change the actual rendered picture");
    ggExpSceneryEnabled=0;ggMeshWorldDraw(pixels,1);
    artRequire(!memcmp(clean,pixels,sizeof clean),
                "scenery toggle fails to restore the same underlying world frame");
    ggExpSceneryEnabled=previous;
}
static void artWaterline(unsigned char *pixels){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=x<66?T_DEEP:x==66?T_WATER:T_GRASS;
        bld[y][x]=0;occ[y][x]=0;
    }
    int level=ggExpGround(T_WATER);
    artRequire(ggMeshGroundAtQ8(65*256+128,64*256+128)==level&&
               ggMeshGroundAtQ8(66*256+128,64*256+128)==level&&
               ggMeshGroundAtQ8(67*256,64*256+128)==level&&
               ggMeshGroundAtQ8(67*256+128,64*256+128)>level,
               "deep sea, shallows, and dry bank do not share a level shoreline");
    artRequire(ggMeshGroundAtQ8(67*256+18,64*256+128)==level&&
               ggMeshGroundAtQ8(67*256+80,64*256+128)>level,
               "shore has no level shelf below its dry bank");
    uint32_t key=0;
    artRequire(ggOutdoorPropKind(65,64,T_DEEP,key)==GG_SCENE_NONE&&
               ggOutdoorPropKind(66,64,T_WATER,key)==GG_SCENE_REEDS,
               "open water gets dry scenery or a nearby bank loses its reeds");
    int x,y;
    ggMeshPropAnchor(66,64,GG_SCENE_REEDS,key,&x,&y);
    artRequire((x&255)>164&&x>>8==66&&y>>8==64,
               "shore reeds use a tile-center anchor instead of the bank");
    memset(world,T_DEEP,sizeof world);
    nCr=0;ggExplore.px=64*256+128;ggExplore.py=64*256+128;ggExplore.angle=0;
    ggExpScenicQuality=2;ggExplore.frame=0;
    ggMeshWorldDraw(pixels,1);
    static unsigned char first[400*240*3];
    static uint16_t waterDepth[400*240];
    memcpy(first,pixels,sizeof first);
    memcpy(waterDepth,ggMeshDepthStorage,sizeof waterDepth);
    ggExplore.frame=54;
    ggMeshWorldDraw(pixels,1);
    int moving=0;
    for(int i=0;i<400*240;i++)
        if(waterDepth[i])moving+=memcmp(first+i*3,pixels+i*3,3)!=0;
    artRequire(ggMeshPropsDrawn==0&&moving>2000&&
               !memcmp(waterDepth,ggMeshDepthStorage,sizeof waterDepth),
               "open water has scenery or animated highlights disrupt physical depth");
}
static void artNearBank(unsigned char *pixels){
    for(int y=0;y<WH;y++){
        memset(world[y],y<=64?T_HILL:T_MOUNT,WW);
        memset(bld[y],0,WW);
    }
    ggExplore.px=64*256+128;ggExplore.py=64*256+176;
    ggExplore.angle=64;
    for(int detail=1;detail<=2;detail++){
        ggExpScenicQuality=detail;
        ggMeshWorldDraw(pixels,1);
        for(int y=180;y<240;y++)for(int x=0;x<400;x++)
            artRequire(ggMeshDepthStorage[y*400+x]!=0,
                       "near terrain clipping opens a hole under a steep bank");
        artRequire(ggMeshContext.near==GG_MESH_NEAR,
                   "ground near clip changes later actor and prop projection");
    }
}
static void artSteepShoulder(void){
    memset(world,T_GRASS,sizeof world);
    world[64][65]=T_MOUNT;
    int x=65*256+128,y=64*256+128;
    artRequire(ggMeshGroundAtQ8(x,y)==48,"steep relief moves a summit anchor");
    for(int side=0;side<4;side++){
        int dx=ggOutdoorSideX[side],dy=ggOutdoorSideY[side];
        artRequire(ggMeshGroundAtQ8(x+dx*128,y+dy*128)==19,
                   "steep relief moves a shared tile-edge anchor");
        int shoulder=ggMeshGroundAtQ8(x+dx*64,y+dy*64);
        artRequire(shoulder>=37&&shoulder<48,
                   "steep relief is still a straight line from toe to summit");
    }
}
static void artSolidContacts(unsigned char *pixels){
    memset(world,T_GRASS,sizeof world);
    GgMeshContext*m=&ggMeshContext;
    static const int props[]={GG_SCENE_SHRUB,GG_SCENE_CRATES,GG_SCENE_COLUMN};
    int ground=ggExpGround(T_GRASS);
    for(unsigned i=0;i<sizeof props/sizeof *props;i++){
        int wx,wy;
        uint32_t key=29763u;
        ggMeshPropAnchor(65,64,props[i],key,&wx,&wy);
        for(int heading=0;heading<256;heading+=64){
            int cosine=ggExpCos[heading],sine=ggExpSin[heading];
            ggMeshBegin(m,pixels,1,wx-cosine/12,wy-sine/12,
                        ground+13,cosine,sine,80,120,150);
            m->fogMax=0;
            GgMeshProjected contact=ggMeshProject(m,
                ggMeshCameraPoint(m,ggMeshV(wx,wy,ground)));
            ggMeshProp(m,65,64,props[i],ground,key,1);
            int nearFoot=0;
            for(int y=contact.y-1;y<=contact.y+8;y++)
                for(int x=contact.x-16;x<=contact.x+16;x++)
                    nearFoot+=m->depth[y*m->w+x]!=0;
            artRequire(nearFoot>5,"solid scenery leaves an air gap beneath its footprint");
        }
    }
}

static void artContactLighting(unsigned char *pixels){
    memset(world,T_GRASS,sizeof world);
    GgMeshContext*m=&ggMeshContext;
    int wx=65*256+128,wy=64*256+128;
    int z=ggExpGround(T_GRASS);
    ggMeshBegin(m,pixels,1,wx-360,wy,z+13,4096,0,80,120,150);
    m->fogMax=0;
    for(int y=63;y<=65;y++)for(int x=64;x<=66;x++)
        ggMeshGroundTile(m,x,y,T_GRASS,0);
    static uint16_t physical[400*240];
    memcpy(physical,m->depth,sizeof physical);
    GgMeshProjected foot=ggMeshProject(m,ggMeshCameraPoint(m,ggMeshV(wx,wy,z)));
    int before=pixels[(foot.y*m->w+foot.x)*3+1];
    ggMeshContactShadow(m,wx,wy,30,13);
    artRequire(pixels[(foot.y*m->w+foot.x)*3+1]<before,
               "contact shadow is detached from the foot on level ground");
    artRequire(!memcmp(physical,m->depth,sizeof physical),
               "the contact patch overwrites the ground's physical depth");
    world[65][66]=T_SWAMP;
    artRequire(!ggMeshShadowFlat(65,65,64,64,z),
               "a neighbouring low tile moves the corner inside a seemingly level shadow");
}

static void artDockShore(unsigned char *pixels){
    GgMeshContext*m=&ggMeshContext;
    int wx=65*256+128,wy=64*256+128;
    int ground=ggExpGround(T_GRASS);
    for(int side=0;side<4;side++){
        int dx=ggOutdoorSideX[side],dy=ggOutdoorSideY[side];
        memset(world,T_GRASS,sizeof world);
        world[64+dy][65+dx]=T_WATER;
        artRequire(ggMeshGroundAtQ8(wx+dx*110,wy+dy*110)==1&&
                   ggMeshGroundAtQ8(wx+dx*48,wy+dy*48)>1,
                   "a shore orientation loses its level shelf or floods the dock approach");
        ggMeshBegin(m,pixels,2,wx-dx*380,wy-dy*380,ground+13,
                    dx*4096,dy*4096,80,120,150);
        m->material=GG_MAT_WOOD;
        ggMeshBuilding(m,65,64,B_DOCK,ground,1);
        GgMeshProjected end=ggMeshProject(m,
            ggMeshCameraPoint(m,ggMeshV(wx+dx*170,wy+dy*170,ground+3)));
        int deck=0;
        for(int y=end.y-2;y<=end.y+2;y++)
            for(int x=end.x-4;x<=end.x+4;x++)deck+=m->depth[y*m->w+x]!=0;
        artRequire(deck>10,"pier does not extend toward its actual water tile");
    }
}

int main(void){
    static unsigned char pixels[400*240*3];
    ggExpTrigInit();
    artFlatGround(pixels,T_GRASS);
    artFlatGround(pixels,T_SNOW);
    artFlatGround(pixels,T_SAND);
    artFacadeDepth(pixels);
    artPropAnchors(pixels);
    artTropicalSilhouette(pixels);
    artSceneryToggle(pixels);
    // On the same camera, actual hills still carry directional light
    // and height. Removing the fake pinwheel must not flatten real land.
    world[64][65]=T_MOUNT;
    GgMeshContext*m=&ggMeshContext;
    ggMeshBegin(m,pixels,1,64*256+128,64*256+128,21,4096,0,80,120,150);
    m->fogMax=0;
    ggMeshGroundTile(m,65,64,T_MOUNT,733);
    int min=255,max=0,covered=0;
    for(int y=0;y<240;y++)for(int x=0;x<400;x++){
        if(!m->depth[y*400+x])continue;
        int g=pixels[(y*400+x)*3+1];
        if(g<min)min=g;if(g>max)max=g;covered++;
    }
    artRequire(covered>2000&&max-min>=10,
                "real slope normals lost visible directional light");
    artWaterline(pixels);
    artSteepShoulder();
    artNearBank(pixels);
    artSolidContacts(pixels);
    artContactLighting(pixels);
    artDockShore(pixels);
    // A coast and a crowded settlement exercise the new shore, crown
    // and architecture paths together, with the fixed native resources.
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=x>67?T_WATER:T_FOREST;
        bld[y][x]=x<68&&((x+y)%7==0)?B_HOUSE:0;
    }
    nCr=0;ggExpBegin(64,64,0);ggExpMeshEnabled=1;
    ggExpScenicQuality=2;
    ggExpRenderMode(pixels,1);
    artRequire(m->triangles<5000&&ggMeshPropsDrawn<=GG_SCENE_LIMIT,
                "environment detail exceeded the bounded scene budget");
    artRequire(m->blendedFragments>50&&m->textureSamples>500,
                "coast/settlement scenery disappeared from the real renderer");
    artRequire(sizeof ggMeshDepthStorage==192000,
                "environment art increased the fixed depth-buffer allocation");
    printf("WORLD ART OK: flat biome seams removed; physical slopes retained; "
           "coast/settlement %d triangles, %d props, %zu-byte native depth\n",
           m->triangles,ggMeshPropsDrawn,sizeof ggMeshDepthStorage);
    return 0;
}

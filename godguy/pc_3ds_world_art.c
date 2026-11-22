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
int main(void){
    static unsigned char pixels[400*240*3];
    ggExpTrigInit();
    artFlatGround(pixels,T_GRASS);
    artFlatGround(pixels,T_SNOW);
    artFlatGround(pixels,T_SAND);
    artFacadeDepth(pixels);
    artPropAnchors(pixels);
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

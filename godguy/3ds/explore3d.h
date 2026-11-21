// GodGuy's first playable, first-person 3DS world.
// Software voxel-column terrain at 200x120, scaled exactly 2x to the
// existing 400x240 top RGB buffer; no new framebuffer, GPU allocations,
// 3D model assets, or duplicate world. Bottom LCD retains the 2D map.
// Uses the SAME world[][], bld[][], occ[][] and setCell() as god-game play.
//
// Real-world voxel terrain, collision, first-person walking, live tile
// editing, crosshair, world location and safe return to the 2D map.
// It is a light sandbox foundation, not a claimed Minecraft block engine.
#ifndef GG_EXPLORE3D_H
#define GG_EXPLORE3D_H
#include "film_architecture3d.h"
#include "film_facade3d.h"
#include "film_floor3d.h"
#include "film_occlusion3d.h"
#include "film_scenery3d.h"
enum { GG_EXP_W=200, GG_EXP_H=120, GG_EXP_MAX_DEPTH=42,
       GG_EXP_EXIT=1, GG_EXP_SAVE=2, GG_EXP_CHANGE=4 };
enum { GG_EXP_MATERIALS=8 };
static const u8 ggExpMaterials[GG_EXP_MATERIALS]={
    T_GRASS,T_DIRT,T_SAND,T_FOREST,T_HILL,T_MOUNT,T_WATER,T_SNOW
};
static const char *const ggExpMaterialNames[GG_EXP_MATERIALS]={
    "GRASS","DIRT","SAND","FOREST","HILL","MOUNTAIN","WATER","SNOW"
};
typedef struct {
    int active,wasPaused;
    // Persistent selected-creature identity. The population list can
    // compact on save; an array slot is NOT a durable identity.
    uint32_t actorId;
    int actorIndex;
    char actorName[64]; // crName can generate up to 46 characters
    int px,py;                   // tile coordinates in Q8 (1 tile = 256)
    int angle;                   // 0..255; one revolution is 256
    int material;
    int frame,rendered,stepPhase;
    int lastHeld,lastTargetX,lastTargetY;
    int edits,feedback,feedbackT;
} GgExplore3D;
static GgExplore3D ggExplore;
static short ggExpSin[256],ggExpCos[256];
static int ggExpTrigReady;

static int ggExpClamp(int n,int low,int high){
    return n<low?low:n>high?high:n;
}
static void ggExpTrigInit(void){
    if(ggExpTrigReady)return;
    for(int i=0;i<256;i++){
        double a=(6.2831853071795864769*i)/256.0;
        ggExpSin[i]=(short)(sin(a)*4096.0);
        ggExpCos[i]=(short)(cos(a)*4096.0);
    }
    ggExpTrigReady=1;
}
static int ggExpGround(int t){
    static const u8 heights[T_N]={
        0,1,5,10,16,25,48,31,6,8,7,18,8
    };
    return (unsigned)t<T_N?heights[t]:8;
}
static int ggExpBuilding(int b){
    switch(b){
    case B_HUT:return 18;
    case B_HOUSE:return 25;
    case B_HALL:return 37;
    case B_TOWER:return 55;
    case B_FARM:return 6;
    case B_RUIN:return 12;
    case B_DOCK:return 7;
    case B_TEMPLE:return 34;
    case B_STATUE:return 28;
    case B_LIBRARY:return 31;
    default:return 0;
    }
}
static int ggExpWalkable(int x,int y,int fromHeight,int climb){
    if(x<1||y<1||x>=WW-1||y>=WH-1)return 0;
    int t=world[y][x];
    if((unsigned)t>=T_N)return 0; // reject damaged terrain indices
    if(ggExplore.actorId){
        // The actor can begin inside their own home or water habitat.
        // Allow fractional travel there; ggHybridMove applies the true
        // species-specific stepOk() rule at every tile boundary.
        if(x==(ggExplore.px>>8)&&y==(ggExplore.py>>8))return 1;
        return !bld[y][x]&&!occ[y][x];
    }
    if(t<=T_WATER||t==T_LAVA||bld[y][x])return 0;
    return ggExpGround(t)-fromHeight<=(climb?30:19);
}
static int ggExpBegin(int x,int y,int priorPause){
    ggExpTrigInit();
    x=ggExpClamp(x,1,WW-2);
    y=ggExpClamp(y,1,WH-2);
    int safe=ggExpWalkable(x,y,60,1);
    if(!safe){
        // Search bounded, nearby live-world terrain only. In an ocean
        // planet, don't instantiate a stuck camera on blocked water.
        // The expedition caller returns to the original 2D world;
        // an intentionally selected resident can still use their actual
        // creature-specific habitat through ggHybridBegin.
        for(int r=1;r<=14&&!safe;r++)
            for(int dy=-r;dy<=r&&!safe;dy++)
                for(int dx=-r;dx<=r&&!safe;dx++){
                    int nx=x+dx,ny=y+dy;
                    if(ggExpWalkable(nx,ny,60,1)){
                        x=nx;y=ny;safe=1;
                    }
                }
    }
    ggExplore.active=safe;
    ggExplore.wasPaused=priorPause;
    ggExplore.actorId=0;
    ggExplore.actorIndex=-1;
    ggExplore.actorName[0]=0;
    ggExplore.px=x*256+128;
    ggExplore.py=y*256+128;
    ggExplore.angle=64; // begin looking into the map, not a random shot
    ggExplore.frame=0;
    ggExplore.rendered=0;
    ggExplore.stepPhase=0;
    ggExplore.lastHeld=0;
    ggExplore.feedback=0;
    ggExplore.feedbackT=0;
    ggExplore.lastTargetX=x;
    ggExplore.lastTargetY=y;
    return safe;
}
static void ggExpTarget(int *tx,int *ty){
    int dir=ggExplore.angle&255;
    // Aim a short three-tile construction ray from the central crosshair.
    *tx=(ggExplore.px+ggExpCos[dir]*768/4096)>>8;
    *ty=(ggExplore.py+ggExpSin[dir]*768/4096)>>8;
}
static int ggExpEdit(int tx,int ty,int material,int removing){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    if(tx<1||ty<1||tx>=WW-1||ty>=WH-1||
       (tx==px&&ty==py)||bld[ty][tx]||occ[ty][tx])return 0;
    int t=removing?T_GRASS:ggExpMaterials[material%GG_EXP_MATERIALS];
    if(world[ty][tx]==t)return 0;
    // Never secretly raze a civilization or crush a living creature.
    // setCell keeps the 2D map, biome data and terrain transitions in sync.
    setCell(tx,ty,(u8)t);
    ggExplore.edits++;
    return 1;
}
static int ggExpStep(int held,int touchX,int touchY){
    if(!ggExplore.active)return 0;
    int down=held&~ggExplore.lastHeld;
    ggExplore.lastHeld=held;
    ggExplore.frame++;
    if(ggExplore.feedbackT>0)ggExplore.feedbackT--;
    if(down&KEY_START)return GG_EXP_EXIT;
    if(down&KEY_SELECT)return GG_EXP_SAVE;
    if(down&KEY_X){
        ggExplore.material=(ggExplore.material+1)%GG_EXP_MATERIALS;
        ggExplore.feedback=1;ggExplore.feedbackT=90;
    }
    if(held&KEY_L)ggExplore.angle=(ggExplore.angle-2)&255;
    if(held&KEY_R)ggExplore.angle=(ggExplore.angle+2)&255;
    int forward=!!(held&KEY_UP)-!!(held&KEY_DOWN);
    int strafe=!!(held&KEY_RIGHT)-!!(held&KEY_LEFT);
    if(forward||strafe){
        int a=ggExplore.angle;
        int speed=held&KEY_B?35:20;
        if(forward&&strafe)speed=speed*3/4;
        int dx=(ggExpCos[a]*forward-ggExpSin[a]*strafe)*speed/4096;
        int dy=(ggExpSin[a]*forward+ggExpCos[a]*strafe)*speed/4096;
        int from=ggExpGround(world[ggExplore.py>>8][ggExplore.px>>8]);
        int nx=ggExplore.px+dx,ny=ggExplore.py+dy;
        if(ggExpWalkable(nx>>8,ggExplore.py>>8,from,held&KEY_B))
            ggExplore.px=nx;
        if(ggExpWalkable(ggExplore.px>>8,ny>>8,from,held&KEY_B))
            ggExplore.py=ny;
        ggExplore.stepPhase++;
    }else ggExplore.stepPhase=0;
    // The full 256x192 lower-screen map is interactive. A previous
    // 160px cutoff made the bottom 32px silently dead on real 3DS
    // touchscreens (including DraStic-equivalent coordinate mapping).
    // Teleport only to unoccupied, genuinely walkable world tiles.
    if((down&KEY_TOUCH)&&touchX>=0&&touchX<256&&
       touchY>=0&&touchY<192){
        int nx=(camX+touchX)>>3,ny=(camY+touchY)>>3;
        if(!ggExplore.actorId&&ggExpWalkable(nx,ny,60,1)){
            ggExplore.px=nx*256+128;
            ggExplore.py=ny*256+128;
        }
    }
    ggExpTarget(&ggExplore.lastTargetX,&ggExplore.lastTargetY);
    if(down&(KEY_A|KEY_Y)){
        int changed=ggExpEdit(ggExplore.lastTargetX,
                              ggExplore.lastTargetY,
                              ggExplore.material,!!(down&KEY_Y));
        ggExplore.feedback=changed?2:3;
        ggExplore.feedbackT=90;
        if(changed)return GG_EXP_CHANGE;
    }
    return 0;
}
// Write a 2x2 low-res pixel into the existing top LCD's 400x240 RGB.
// A single function limits memory traffic on Old 3DS; no temp surface.
static void ggExpDot(u8 *out,int x,int y,int r,int g,int b){
    if((unsigned)x>=GG_EXP_W||(unsigned)y>=GG_EXP_H)return;
    u8 *a=out+((y*2)*400+x*2)*3;
    u8 *z=a+400*3;
    for(int k=0;k<2;k++){
        a[k*3+0]=z[k*3+0]=(u8)r;
        a[k*3+1]=z[k*3+1]=(u8)g;
        a[k*3+2]=z[k*3+2]=(u8)b;
    }
}
// The terrain loop already clips x and y to 200x120. Avoid thousands of
// bounds checks and per-dot call overhead in its hottest drawing path.
static inline void ggExpDotInView(u8 *out,int x,int y,int r,int g,int b){
    u8 *p=out+((y*2)*400+x*2)*3;
    p[0]=p[3]=p[1200]=p[1203]=(u8)r;
    p[1]=p[4]=p[1201]=p[1204]=(u8)g;
    p[2]=p[5]=p[1202]=p[1205]=(u8)b;
}
// Foreground spans are populated during the existing 400-ray scene pass.
// Actor overlay depth is zero for the world, HUD, exploration and 2D screens.
static GgFilmOcclusionColumn ggExpFilmOcclusion[GG_FILM_OCCLUSION_COLS];
static int ggExpFilmDrawDepth;
// Declared before ggExpRenderMode; populated later by population3d.h.
// The first-person facade and actor compositors share this depth
// without an include-order dependency or a second creature search.
static int ggExpNpcClosestDepth;
// The real-time POV is rendered in 200x120, expanded 2x. Scenery must
// align to that pixel grid rather than introducing mismatched 1px props.
static int ggExpScenicCoarse;
static inline int ggExpFilmPixelHidden(int x,int y){
    return ggExpFilmDrawDepth>0&&
           ggFilmOcclusionHidden(ggExpFilmOcclusion,x,y,ggExpFilmDrawDepth);
}
static void ggExpRect(u8*out,int x,int y,int w,int h,int r,int g,int b){
    if(ggExpScenicCoarse){
        int right=x+w,bottom=y+h;
        x-=x&1;y-=y&1;
        w=((right+1)&~1)-x;
        h=((bottom+1)&~1)-y;
    }
    int x1=ggExpClamp(x,0,400),y1=ggExpClamp(y,0,240);
    int x2=ggExpClamp(x+w,0,400),y2=ggExpClamp(y+h,0,240);
    for(int iy=y1;iy<y2;iy++)
        for(int ix=x1;ix<x2;ix++){
            if(ggExpFilmPixelHidden(ix,iy))continue;
            u8*p=out+(iy*400+ix)*3;
            p[0]=(u8)r;p[1]=(u8)g;p[2]=(u8)b;
        }
}
static void ggExpText(u8*out,int x,int y,const char*str,int r,int g,int b){
    for(;*str;str++,x+=6){
        int ch=(unsigned char)*str;
        if(ch>='a'&&ch<='z')ch-=32;
        if(ch<32||ch>95)continue;
        for(int ry=0;ry<7;ry++){
            int bits=font5x7[ch-32][ry];
            for(int rx=0;rx<5;rx++)if((bits>>(4-rx))&1)
                ggExpRect(out,x+rx,y+ry,1,1,r,g,b);
        }
    }
}
// The story uses full native 400x240 ray columns; real-time exploration
// retains the original 200x120 fast renderer. Both access the SAME world.
// Actor-safe film scenery renderer: physical world locations, stable
// deterministic set dressing, existing RGB drawing and foreground depth.
// The fixed near-region scan is at most 441 cells, with at most 84 props.
// No extra per-pixel world search, meshes, textures, heap or framebuffer.
static int ggExpSceneryEnabled=1;
static int ggExpScenicVisible;
static int ggExpScenicForegroundPass;
static void ggExpScenicShadow(u8*out,int x,int foot,int half,int depth){
    if(ggExpScenicCoarse||ggExpScenicForegroundPass||
       foot<1||foot>=240)return;
    int y=foot-1;
    int start=ggExpClamp(x-half,0,400);
    int end=ggExpClamp(x+half,0,400);
    for(int px=start;px<end;px++){
        if(ggFilmOcclusionHidden(ggExpFilmOcclusion,px,y,depth))continue;
        u8*p=out+(y*400+px)*3;
        p[0]=(u8)(p[0]*210/256);
        p[1]=(u8)(p[1]*210/256);
        p[2]=(u8)(p[2]*210/256);
    }
}
static void ggExpScenicProp(u8*out,int kind,int x,int foot,int h,
                            int depth,uint32_t key){
    if(h<3)return;
    int widthRatio=kind==GG_SCENE_OAK||kind==GG_SCENE_STALL?7:
                   kind==GG_SCENE_PINE?5:
                   kind==GG_SCENE_SHRUB?12:
                   kind==GG_SCENE_GRASS?6:4;
    int w=ggExpClamp(h*widthRatio/11,3,52);
    int trunk=ggExpClamp(h/9,1,5);
    int y=foot-h;
    int warmth=(int)((key>>9)&7u);
    int breeze=ggFilmScenicWind(key,ggExplore.frame);
    ggExpFilmDrawDepth=ggExpScenicCoarse?0:depth;
    ggExpScenicShadow(out,x,foot,w/2,depth);
    // LOD is deliberately minimal, but never turns a market stall
    // into the same stick as a spruce or lantern. Tiny readable icons
    // preserve the environment's identity out to the mist horizon.
    if(h<=8){
        int cls=ggFilmScenicFarClass(kind);
        int hh=h>3?h:3;
        if(cls==1){
            ggExpRect(out,x,foot-hh/2,1,hh/2,92,76,59);
            ggExpRect(out,x-3,foot-hh,6,hh/2+1,55,101,72);
            ggExpRect(out,x-2,foot-hh,3,1,102,140,89);
        }else if(cls==2){
            ggExpRect(out,x-2,foot-hh/2,1,hh/2,57,111,73);
            ggExpRect(out,x,foot-hh,1,hh,72,122,76);
            ggExpRect(out,x+2,foot-hh/2-1,1,hh/2+1,
                      kind==GG_SCENE_CROPS?181:92,
                      kind==GG_SCENE_CROPS?153:129,74);
        }else if(cls==3){
            ggExpRect(out,x-2,foot-hh,4,hh,101,106,106);
            ggExpRect(out,x-2,foot-hh,2,hh/2+1,152,149,134);
        }else if(cls==4){
            ggExpRect(out,x,foot-hh+2,1,hh-2,89,81,68);
            ggExpRect(out,x-1,foot-hh,3,3,239,177,97);
            ggExpRect(out,x,foot-hh,1,1,255,233,177);
        }else if(cls==5){
            ggExpRect(out,x-2,foot-hh,1,hh,122,115,103);
            ggExpRect(out,x-1,foot-hh,4,3,144,70,88);
            ggExpRect(out,x,foot-hh+1,1,1,210,172,108);
        }else if(cls==6){
            ggExpRect(out,x-3,foot-hh/2,6,hh/2,134,96,62);
            ggExpRect(out,x-3,foot-hh/2,6,1,184,138,84);
            ggExpRect(out,x-2,foot-1,1,1,44,46,51);
            ggExpRect(out,x+2,foot-1,1,1,44,46,51);
        }else if(cls==7){
            ggExpRect(out,x-3,foot-hh,1,hh,129,93,62);
            ggExpRect(out,x+3,foot-hh,1,hh,129,93,62);
            ggExpRect(out,x-3,foot-hh/2,7,1,175,132,89);
        }else if(cls==8){
            ggExpRect(out,x-3,foot-hh+1,7,2,153,81,70);
            ggExpRect(out,x-3,foot-hh+1,2,1,215,165,111);
            ggExpRect(out,x-2,foot-hh+3,1,hh-3,106,79,57);
            ggExpRect(out,x+3,foot-hh+3,1,hh-3,106,79,57);
        }
        ggExpFilmDrawDepth=0;
        return;
    }
    if(kind==GG_SCENE_OAK||kind==GG_SCENE_PINE){
        int pine=kind==GG_SCENE_PINE;
        int foliageR=pine?39:43,foliageG=pine?79:90,
            foliageB=pine?64:68;
        ggExpRect(out,x-trunk/2,foot-h/2,trunk,h/2,65,56,48);
        ggExpRect(out,x,foot-h/2,1,h/3,122,90,65);
        if(pine){
            // Three overlapping triangular boughs with individually
            // lit slopes, not a stack of rectangular green billboards.
            for(int tier=0;tier<3;tier++){
                int top=y+tier*h/5;
                int levelHeight=h*(5-tier)/12;
                for(int band=0;band<6;band++){
                    int width=w*(band+2)*(4-tier)/30;
                    if(width<2)width=2;
                    int yy=top+band*levelHeight/6;
                    int next=top+(band+1)*levelHeight/6;
                    int rowH=next-yy;if(rowH<1)rowH=1;
                    int offset=(int)((key>>(tier*5+band))&3u)-1;
                    int crownX=x+breeze+offset;
                    ggExpRect(out,crownX-width/2,yy,width,rowH,
                        34+tier*3,71+band*3+tier*3,59+band*2);
                    ggExpRect(out,crownX-width/3,yy,
                        ggExpClamp(width/3,1,30),rowH,
                        59+tier*3,104+band*3,81+band*2);
                }
            }
        }else{
            // Real broadleaf contour: nine irregular rounded facets,
            // light striking the upper-left while a dark under-canopy
            // wraps over the trunk. Deterministic in world-space.
            int crownY=y+h/10,crownH=h*3/5;
            for(int band=0;band<9;band++){
                int half=ggFilmScenicCrownHalf(w/2,band,key);
                int yy=crownY+band*crownH/9;
                int next=crownY+(band+1)*crownH/9;
                int rowH=next-yy;if(rowH<1)rowH=1;
                int center=x+breeze+((band>5)?1:0);
                int shadow=band>=6;
                ggExpRect(out,center-half,yy,half*2,rowH,
                          shadow?35:54-band/2,
                          shadow?71:104+band*2,
                          shadow?57:75+band);
                if(half>=3){
                    ggExpRect(out,center-half+1,yy,half-1,rowH,
                              79-band/2,132-band/2,93-band/2);
                }
            }
            // Two lopsided foliage clusters keep even the near crown
            // from being a uniform oval. The trunk stays motionless.
            ggExpRect(out,x+breeze-w/3,y+h/4,
                      ggExpClamp(w/4,1,16),h/7+1,80,137,92);
            ggExpRect(out,x+breeze+w/5,y+h/3,
                      ggExpClamp(w/5,1,13),h/7+1,43,83,66);
        }
    }else if(kind==GG_SCENE_SHRUB||kind==GG_SCENE_GRASS||
             kind==GG_SCENE_CROPS||kind==GG_SCENE_REEDS){
        int stalks=kind==GG_SCENE_CROPS?5:kind==GG_SCENE_GRASS?3:4;
        for(int i=0;i<stalks;i++){
            int xx=x+(i*2-stalks+1)*w/(stalks+1);
            int ht=h*(3+(int)((key>>(i*3))&3u))/6;
            int thick=ggExpClamp(w/13,1,3);
            ggExpRect(out,xx,foot-ht/2,thick,ht/2,
                      44,91,57);
            ggExpRect(out,xx+breeze,foot-ht,thick,ht/2+1,
                      51,101,62);
            if(kind==GG_SCENE_CROPS||kind==GG_SCENE_REEDS){
                ggExpRect(out,xx+1,foot-ht,h/15+1,h/6+1,
                          kind==GG_SCENE_REEDS?118:154,
                          kind==GG_SCENE_REEDS?132:157,78);
            }else{
                ggExpRect(out,xx-1,foot-ht/2,ggExpClamp(w/4,1,6),
                          h/5+1,68,112,69);
            }
        }
        if(kind==GG_SCENE_SHRUB){
            ggExpRect(out,x-w/2,foot-h/2,w,h/3,42,85,59);
            ggExpRect(out,x+breeze-w*2/5,foot-h*2/3,
                      w*4/5,h/3,54,105,67);
            ggExpRect(out,x+breeze-w/3,foot-h*3/4,
                      w/2,h/5,79,123,80);
        }else if(kind==GG_SCENE_GRASS&&((key>>5)&7u)<2u){
            // Occasional tiny flower/wild seed head, world-anchored.
            ggExpRect(out,x+breeze,foot-h,2,2,184,177,121);
        }
    }else if(kind==GG_SCENE_STONE||kind==GG_SCENE_COLUMN){
        int stone=kind==GG_SCENE_COLUMN;
        int bw=stone?w/2:w;
        ggExpRect(out,x-bw/2,foot-h,bw,h,
                  96,102,104);
        ggExpRect(out,x-bw/2,foot-h,bw/3+1,h,
                  148,145,130);
        ggExpRect(out,x-bw/2,foot-h/2,bw,h/9+1,
                  74,80,88);
        if(stone){
            ggExpRect(out,x-w/2,foot-h,w,h/8+1,
                      153,150,136);
            ggExpRect(out,x-w/2,foot-h/8,w,h/8+1,
                      91,92,94);
            ggExpRect(out,x+w/4,foot-h,w/5,h/5,72,78,83);
        }else{
            ggExpRect(out,x-w/3,foot-h,w*2/3,h/5,
                      129,132,132);
            ggExpRect(out,x+w/5,foot-h/3,w/6,h/5,65,73,79);
        }
    }else if(kind==GG_SCENE_LAMP||kind==GG_SCENE_BRAZIER){
        int brazier=kind==GG_SCENE_BRAZIER;
        ggExpRect(out,x-trunk/2,y+(brazier?h/2:0),
                  trunk,h-(brazier?h/2:0),71,68,61);
        ggExpRect(out,x-w/4,y,w/2,ggExpClamp(h/5,2,7),
                  78,72,66);
        ggExpRect(out,x-w/6,y+h/20,w/3,ggExpClamp(h/7,2,6),
                  225+warmth,167+warmth,80);
        ggExpRect(out,x-1,y+h/20,2,ggExpClamp(h/10,1,5),
                  255,227,145);
        ggExpRect(out,x-w/3,foot-2,w*2/3,2,87,76,66);
    }else if(kind==GG_SCENE_BANNER){
        ggExpRect(out,x,foot-h,2,h,122,117,106);
        ggExpRect(out,x+2,y+h/12,w,h/2,
                  121+warmth,64,78);
        ggExpRect(out,x+w/3,y+h/5,w/3,h/7,
                  213,174,94);
        ggExpRect(out,x+2,foot-2,w/2,2,67,69,68);
    }else if(kind==GG_SCENE_FENCE){
        int railY=foot-h*2/3;
        ggExpRect(out,x-w/2,foot-h,2,h,123,87,59);
        ggExpRect(out,x+w/2-2,foot-h,2,h,100,75,56);
        ggExpRect(out,x-w/2,railY,w,2,157,116,77);
        ggExpRect(out,x-w/2,railY+h/4,w,2,105,80,61);
    }else if(kind==GG_SCENE_CRATES||kind==GG_SCENE_CART){
        int cart=kind==GG_SCENE_CART;
        ggExpRect(out,x-w/2,foot-h*2/3,w,h/2,
                  120,81,54);
        ggExpRect(out,x-w/2,foot-h*2/3,w,h/9+1,
                  177,132,88);
        ggExpRect(out,x-w/4,foot-h*2/3,w/5,h/2,
                  87,64,47);
        if(cart){
            ggExpRect(out,x-w/3,foot-h/5,w/6,h/5,
                      42,45,50);
            ggExpRect(out,x+w/5,foot-h/5,w/6,h/5,
                      42,45,50);
            ggExpRect(out,x-w/4,foot-h,w/2,h/3,
                      138,102,68);
        }else{
            ggExpRect(out,x+w/6,foot-h,w/3,h/3,
                      159,111,72);
        }
    }else if(kind==GG_SCENE_STALL){
        ggExpRect(out,x-w/2,foot-h/2,2,h/2,103,76,56);
        ggExpRect(out,x+w/2-2,foot-h/2,2,h/2,103,76,56);
        ggExpRect(out,x-w/2,y+h/8,w,h/6+2,137,77,67);
        ggExpRect(out,x-w/3,y+h/8,w/3,h/6+2,211,160,108);
        ggExpRect(out,x+w/6,y+h/8,w/4,h/6+2,211,160,108);
        ggExpRect(out,x-w/3,foot-h/3,w*2/3,h/6,
                  135,95,62);
    }
    ggExpFilmDrawDepth=0;
}
// Base layer is rendered after the world (before cast); the optional
// foreground-only repeat is drawn after the cast to occlude the closest
// bodies with genuine near-field scenery. The same projected geometry
// is used in both passes and no scenery object is ever cached.
static GgFilmScenicItem ggExpScenicItems[GG_SCENE_LIMIT];
static int ggExpScenicCount;
static void ggExpScenicDraw(u8*out,int foreground,int nearestActorDepth){
    if(!ggExpSceneryEnabled){
        if(!foreground){ggExpScenicCount=0;ggExpScenicVisible=0;}
        return;
    }
    if(!foreground){
        int cx=ggExplore.px>>8,cy=ggExplore.py>>8;
        int angle=ggExplore.angle&255;
        int cosine=ggExpCos[angle],sine=ggExpSin[angle];
        int cameraGround=ggExpGround(world[cy][cx]);
        int radius=ggExpScenicCoarse?7:GG_SCENE_RADIUS;
        int budget=ggExpScenicCoarse?34:GG_SCENE_LIMIT;
        ggExpScenicCount=0;
        for(int ty=cy-radius;ty<=cy+radius;ty++){
            if((unsigned)ty>=WH)continue;
            for(int tx=cx-radius;tx<=cx+radius;tx++){
                if((unsigned)tx>=WW)continue;
                if(bld[ty][tx]||occ[ty][tx])continue;
                uint32_t key=ggFilmScenicHash(tx,ty,7);
                // Fixed world-space thinning preserves bilateral density,
                // and never depends on where the camera sweep begins.
                if(((key>>24)&7u)>=5u)continue;
                int near=0;
                if(tx>0&&bld[ty][tx-1])near=bld[ty][tx-1];
                else if(tx+1<WW&&bld[ty][tx+1])near=bld[ty][tx+1];
                else if(ty>0&&bld[ty-1][tx])near=bld[ty-1][tx];
                else if(ty+1<WH&&bld[ty+1][tx])near=bld[ty+1][tx];
                int kind=ggFilmScenicKind(world[ty][tx],near,key);
                if(!kind)continue;
                int wx=tx*256+128+(int)((key>>12)&95u)-48;
                int wy=ty*256+128+(int)((key>>20)&95u)-48;
                int dx=wx-ggExplore.px,dy=wy-ggExplore.py;
                int forward=(dx*cosine+dy*sine)/4096;
                if(forward<=128||forward>radius*256)continue;
                int side=(-dx*sine+dy*cosine)/4096;
                if(side<-forward*6/5||side>forward*6/5)continue;
                int x=ggFilmProjectScreenX(side,forward);
                if(x<-60||x>=460)continue;
                int feet=ggFilmProjectFootY(forward,cameraGround,
                                            ggExpGround(world[ty][tx]));
                // Nearby trees and stalls must not vanish because their bases
                // enter the lower viewport. The HUD clips them naturally.
                if(feet<70||feet>=238)continue;
                int height=ggFilmScenicHeight(kind)*12*256/(forward+256);
                if(height<3)continue;
                int maxHeight=ggExpScenicCoarse?64:96;
                if(height>maxHeight)height=maxHeight;
                ggExpScenicItems[ggExpScenicCount++]=(GgFilmScenicItem){
                    x,feet,height,forward,kind,key
                };
                if(ggExpScenicCount>=budget)goto compiled;
            }
        }
compiled:
        ggFilmScenicSort(ggExpScenicItems,ggExpScenicCount);
        ggExpScenicVisible=ggExpScenicCount;
    }
    // Painter order is physical depth, not map scanning order.
    // The short foreground repeat reads this SAME fixed queue; it never
    // rescans, rehashes or reallocates the live game world.
    ggExpScenicForegroundPass=foreground;
    for(int i=0;i<ggExpScenicCount;i++){
        const GgFilmScenicItem *p=&ggExpScenicItems[i];
        if(foreground&&
           (nearestActorDepth<=0||p->depth+96>=nearestActorDepth))
            continue;
        ggExpScenicProp(out,p->kind,p->x,p->foot,p->height,
                        p->depth,p->key);
    }
    ggExpScenicForegroundPass=0;
    ggExpFilmDrawDepth=0;
}
static void ggExpRenderMode(u8 *top,int cinematic){
    ggExpFilmDrawDepth=0;
    if(cinematic)ggFilmOcclusionClear(ggExpFilmOcclusion);
    int columns=cinematic?400:GG_EXP_W;
    int screenHeight=cinematic?240:GG_EXP_H;
    int a=ggExplore.angle&255;
    int terrain=world[ggExplore.py>>8][ggExplore.px>>8];
    if((unsigned)terrain>=T_N)terrain=T_DIRT;
    GgFilmScenicAtmosphere mood=ggFilmScenicAtmosphere(terrain);
    int eye=ggExpGround(terrain)+13;
    int walkBob=ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0;
    int horizon=cinematic?116+walkBob*2:58+walkBob;
    int dirX=ggExpCos[a],dirY=ggExpSin[a];
    int rightX=-dirY,rightY=dirX;
    int fogTick=ggExplore.frame/8;
    // Native-resolution cinema: more nuanced sky without adding a
    // second texture/framebuffer. Exploration still copies the fast 2x row.
    for(int y=0;y<screenHeight;y++){
        int sky=y<horizon;
        int r,g,b;
        if(cinematic){
            r=sky?30+y/3:72+y/18;
            g=sky?45+y/2:92+y/15;
            b=sky?77+y/2:94+y/20;
        }else{
            // Exploration uses the same restrained cool sky family as
            // the high-quality film rather than fluorescent cyan.
            r=sky?32+y/2:67;g=sky?57+y:99;b=sky?89+y:113;
        }
        r=ggExpClamp(r+mood.skyR,0,255);
        g=ggExpClamp(g+mood.skyG,0,255);
        b=ggExpClamp(b+mood.skyB,0,255);
        u8 *line=top+(cinematic?y:y*2)*400*3;
        for(int x=0;x<400;x++){
            line[x*3]=(u8)r;
            line[x*3+1]=(u8)g;
            line[x*3+2]=(u8)b;
        }
        if(!cinematic)memcpy(line+400*3,line,400*3);
    }
    // Three-layer, parallax-stable distant cloud banks. Unlike the
    // former seven equal-width horizontal bars, these break into
    // thinner irregular silhouettes anchored to camera yaw/location.
    // The fast 2x gameplay renderer retains its low-cost cloud pass.
    for(int k=0;k<(cinematic?12:7);k++){
        int drift=fogTick+(ggExplore.angle&255)*3+
                  (ggExplore.px>>7);
        int x=(k*79+drift)%(cinematic?570:290)-40;
        int y=cinematic?14+(k*19)%64:8+(k*13)%29;
        int w=cinematic?27+(k*17)%48:18+(k*7)%24;
        if(cinematic){
            // Broken cloud banks have mass and overlapping shoulders,
            // rather than the previous computer-like cyan scan lines.
            // Eight rects at most per bank, no translucency buffer.
            int cr=ggExpClamp(109+mood.skyR/2,0,255);
            int cg=ggExpClamp(127+mood.skyG/2,0,255);
            int cb=ggExpClamp(149+mood.skyB/2,0,255);
            ggExpRect(top,x+w/5,y+2,w*3/5,5,cr-31,cg-32,cb-30);
            ggExpRect(top,x+w/12,y+3,w/3,3,cr-18,cg-17,cb-14);
            ggExpRect(top,x+w/3,y-1,w/3,5,cr,cg,cb);
            ggExpRect(top,x+w/4,y+1,w*2/5,2,cr+8,cg+6,cb+4);
            ggExpRect(top,x+w*2/3,y+2,w/4,3,
                      cr-14,cg-12,cb-8);
            ggExpRect(top,x+w/3,y+6,w*2/5,1,cr-41,cg-37,cb-30);
        }else{
            for(int cx=x;cx<x+w;cx++)for(int cy=y;cy<y+3;cy++)
                ggExpDot(top,cx,cy,
                         122+mood.skyR/2,160+mood.skyG/2,
                         184+mood.skyB/2);
        }
    }
    // Film-only perspective floor: the same projected ground that cast
    // boots touch, unlike the old succession of solid green voxel steps.
    // The row distance is computed only 123 times. Rays share the 400px
    // top buffer, exactly like the existing renderer: no new framebuffer.
    if(cinematic){
        int rowDistance[240],rowMist[240];
        int cameraGround=ggExpGround(terrain);
        for(int py=horizon+1;py<240;py++){
            rowDistance[py]=ggFilmFloorForward(py,horizon,
                eye-cameraGround,GG_EXP_MAX_DEPTH*256);
            rowMist[py]=ggFilmFloorMist(rowDistance[py]);
        }
        for(int x=0;x<400;x++){
            int skew=x-200;
            int floorRayX=dirX+rightX*skew/GG_FILM_FOCAL;
            int floorRayY=dirY+rightY*skew/GG_FILM_FOCAL;
            for(int y=horizon+1;y<240;y++){
                // Half-sample the distant ground (already misted),
                // retain 1:1 native pixels near actors' feet. Saves
                // thousands of terrain/color lookups each film frame.
                if((x&1)&&y<170){
                    u8*p=top+(y*400+x)*3;
                    p[0]=p[-3];p[1]=p[-2];p[2]=p[-1];
                    continue;
                }
                int d=rowDistance[y];
                int wx=ggExplore.px+floorRayX*d/4096;
                int wy=ggExplore.py+floorRayY*d/4096;
                int tx=wx>>8,ty=wy>>8;
                int type=(unsigned)tx<WW&&(unsigned)ty<WH?
                    world[ty][tx]:T_DIRT;
                if((unsigned)type>=T_N)type=T_DIRT;
                int r=(terrCol[type][0]*8*4+72)/5;
                int g=(terrCol[type][1]*8*3+71)/4;
                int b=(terrCol[type][2]*8*4+91)/5;
                if(type==T_GRASS||type==T_FOREST){
                    g=(g*4+r)/5;
                    b=(b*3+g)/4;
                }
                int grain=ggFilmFloorGrain(wx,wy,ggExplore.frame);
                int detail=ggFilmScenicFloorDetail(type,wx,wy);
                r+=grain+detail;g+=grain+detail/2;b+=grain+detail/3;
                int mist=rowMist[y];
                r=(r*(256-mist)+mood.hazeR*mist)/256;
                g=(g*(256-mist)+mood.hazeG*mist)/256;
                b=(b*(256-mist)+mood.hazeB*mist)/256;
                u8*p=top+(y*400+x)*3;
                p[0]=(u8)ggExpClamp(r,0,255);
                p[1]=(u8)ggExpClamp(g,0,255);
                p[2]=(u8)ggExpClamp(b,0,255);
            }
        }
    }
    // Front-to-back silhouette-buffer voxel ray caster. 200 rays, max
    // 42 terrain samples each, with every cell read directly from the
    // live 3DS game world. No second mesh, z-buffer or GPU allocation.
    for(int x=0;x<columns;x++){
        int skew=(x-columns/2);
        int rayX=dirX+rightX*skew/(cinematic?250:125);
        int rayY=dirY+rightY*skew/(cinematic?250:125);
        int highest=screenHeight;
        int previousX=-1,previousY=-1;
        // Advance fixed-point ray numerators instead of two multiplies
        // at every sample. Division retains the EXACT original rounding
        // (including negative directions), so cell/roof pixels do not shift.
        const int stepX=rayX*256,stepY=rayY*256;
        int sampleX=0,sampleY=0;
        for(int d=1;d<=GG_EXP_MAX_DEPTH;d++){
            sampleX+=stepX;sampleY+=stepY;
            int wx=ggExplore.px+sampleX/4096;
            int wy=ggExplore.py+sampleY/4096;
            int tx=wx>>8,ty=wy>>8;
            if((unsigned)tx>=WW||(unsigned)ty>=WH)break;
            if(tx==previousX&&ty==previousY)continue;
            previousX=tx;previousY=ty;
            int type=world[ty][tx],build=bld[ty][tx];
            if((unsigned)type>=T_N)type=T_DIRT;
            int ground=ggExpGround(type),rise=ggExpBuilding(build);
            // The ground plane has already drawn level or lower land.
            // Only actual raised cliffs and building VOLUMES must be
            // added by the silhouette caster; never redraw a green
            // vertical wall all the way to the screen's bottom.
            if(cinematic&&!build&&ground<=ggExpGround(terrain))continue;
            int h=ground+rise;
            GgFilmRoofVolume roof={h,h,0,256};
            if(cinematic&&build){
                // Each visible ray hits the same world-space gabled roof
                // that a rotated camera would see; no pyramid cutouts.
                roof=ggFilmRoofVolume(build,wx,wy,ground,rise);
                h=roof.roofTop;
            }else if(build==B_HUT||build==B_HOUSE||
                     build==B_HALL||build==B_TEMPLE){
                // Do not change the lower-cost 200x120 exploration path.
                int ex=abs((wx&255)-128),ey=abs((wy&255)-128);
                int edge=ex>ey?ex:ey;
                h-=edge/(build==B_HALL?16:12);
            }
            // Height is measured in coarse world-cell units, not raw
            // screen pixels. The former scale (68) pushed nearby ground
            // entirely below the viewport; six gives legible walking
            // terrain and roofs while still exaggerating cliffs.
            int projected=horizon-(h-eye)*(cinematic?12:6)/(d+1);
            if(projected>=highest)continue;
            int from=ggExpClamp(projected,0,screenHeight);
            int to=ggExpClamp(highest,0,screenHeight);
            if(cinematic&&(build||ground>ggExpGround(terrain))){
                // Building walls end at their physical foundation. Raised
                // grass/forest/hill faces end at the nearby level ground,
                // not at the BOTTOM OF THE SCREEN. Two-height steps
                // become a few-pixel embankment, not 80px solid cubes.
                int baseHeight=build?ground:ggExpGround(terrain);
                int bottom=ggFilmTerrainSurfaceY(baseHeight,eye,
                                                  horizon,d,12);
                if(bottom<to)to=ggExpClamp(bottom,from,to);
            }
            if(cinematic&&from<to&&
               (build||ground>ggExpGround(terrain)))
                ggFilmOcclusionAdd(ggExpFilmOcclusion,x,from,to,d*256);
            int baseR,baseG,baseB;
            if(build){
                // Stone, timber, roofs and towers are recognizable solid
                // blocks, positioned exactly at their simulation tiles.
                int wood=build==B_HOUSE||build==B_HUT||build==B_DOCK;
                baseR=wood?160:139;baseG=wood?112:135;baseB=wood?83:144;
                if(build==B_RUIN){baseR=102;baseG=102;baseB=108;}
            }else{
                baseR=terrCol[type][0]*8;
                baseG=terrCol[type][1]*8;
                baseB=terrCol[type][2]*8;
            }
            if(cinematic){
                // Restrained painterly atmosphere: natural colors,
                // cool distant stone and less neon-green grass.
                baseR=(baseR*4+72)/5;
                baseG=(baseG*3+71)/4;
                baseB=(baseB*4+91)/5;
            }
            if(!build&&(type==T_GRASS||type==T_FOREST||
                         type==T_JUNGLE)){
                // Shared naturalistic color grading across both render
                // modes. The playable POV no longer turns this world
                // into a neon green lawn when leaving a cutscene.
                baseG=(baseG*4+baseR)/5;
                baseB=(baseB*3+baseG)/4;
            }
            int shade=256-d*3;
            if(shade<110)shade=110;
            baseR=baseR*shade/256;
            baseG=baseG*shade/256;
            baseB=baseB*shade/256;
            // Per-visible-cell atmospheric depth, not a pixel pass.
            // Exploration has a smaller fog budget so long sightlines
            // stay responsive while film shots get richer distant air.
            int mist=cinematic?ggExpClamp((d-6)*6,0,144):
                               ggExpClamp((d-8)*3,0,84);
            baseR=(baseR*(256-mist)+mood.hazeR*mist)/256;
            baseG=(baseG*(256-mist)+mood.hazeG*mist)/256;
            baseB=(baseB*(256-mist)+mood.hazeB*mist)/256;
            int bright=build?13:6;
            // One material coordinate per ray/visible tile rather than
            // calling RNG or scanning any texture map per pixel.
            int surfaceU=(abs(dirX)>abs(dirY)?wy:wx)&255;
            int wallShade=cinematic&&build?
                shade*ggFilmWallFaceLight(wx,wy)/256:shade;
            int roofBottom=cinematic&&roof.roofPitch?
                ggFilmRoofBottomScreen(roof.wallTop,eye,
                                        horizon,d,12):-1;
            int roofR=0,roofG=0,roofB=0;
            if(roofBottom>=0)
                ggFilmRoofMaterial(build,wx,wy,roof.faceLight,shade,
                                   &roofR,&roofG,&roofB);
            // Terrain strata, pitched roofs, shadowed stone and tree
            // crowns. Keep these material cues stable in world space so
            // moving the camera doesn't cause procedural visual noise.
            for(int y=from;y<to;y++){
                int edge=y-from;
                int materialY=cinematic?y/2:y;
                int grain=((tx*17+ty*11+materialY/6)&3)==0?7:0;
                int r=baseR+(edge<2?bright:0)-grain;
                int g=baseG+(edge<2?bright:0)-grain;
                int b=baseB+(edge<2?bright:0)-grain;
                if(build&&edge<5){
                    // Architecturally distinct roofs in the real cities.
                    if(build==B_HOUSE||build==B_HUT){
                        r=125*shade/256;g=70*shade/256;b=51*shade/256;
                    }else if(build==B_HALL){
                        r=86*shade/256;g=81*shade/256;b=100*shade/256;
                    }else if(build==B_TEMPLE){
                        r=194*shade/256;g=174*shade/256;b=137*shade/256;
                    }
                }else if(build==B_TOWER&&(y-from)%(cinematic?22:11)<(cinematic?3:2)){
                    r+=15;g+=16;b+=19; // bands between stone courses
                }else if(!build&&type==T_FOREST&&edge<(cinematic?14:7)){
                    g+=17;r-=6; // canopy over a darker forest wall
                }else if(!build&&type==T_MOUNT&&((materialY+ty*3)/9)%4==0){
                    r+=8;g+=9;b+=11; // strata follow world coordinates
                }
                if(!cinematic&&build&&d<=14&&edge>4){
                    // The same authored facade height contract at half
                    // resolution, evaluated ONLY for near buildings.
                    // This keeps the fast 200-ray POV legible without
                    // doing 400-ray film materials every gameplay frame.
                    int wallHeight=ggFilmFacadeHeightAt(y*2,horizon*2,
                                                        eye,d,ground);
                    int feature=ggFilmFacadeFeature(build,surfaceU,
                                                    wallHeight);
                    if(feature==GG_FACADE_GLASS){
                        r=122*shade/256;g=106*shade/256;
                        b=90*shade/256;
                    }else if(feature==GG_FACADE_DOOR||
                             feature==GG_FACADE_PORTAL){
                        r=43*shade/256;g=52*shade/256;
                        b=59*shade/256;
                    }else if(feature==GG_FACADE_FRAME||
                             feature==GG_FACADE_COURSE||
                             feature==GG_FACADE_BRACE){
                        r=r*3/5;g=g*3/5;b=b*3/5;
                    }else if(feature==GG_FACADE_PILASTER||
                             feature==GG_FACADE_RELIEF){
                        r=r*7/6;g=g*7/6;b=b*6/5;
                    }else if(feature==GG_FACADE_SHUTTER){
                        r=73*shade/256;g=59*shade/256;
                        b=53*shade/256;
                    }else if(feature==GG_FACADE_SILL||
                             feature==GG_FACADE_CORNICE){
                        r=r*5/4;g=g*5/4;b=b*5/4;
                    }else if(feature==GG_FACADE_MOSS){
                        r=52;g=83;b=64;
                    }
                }
                if(cinematic&&build){
                    // Compute a building's vertical detail position from
                    // REAL WORLD HEIGHT, not y-from-roof pixels. Doors,
                    // windows and masonry courses no longer crawl or
                    // stretch independently when the camera dollies.
                    int wallHeight=ggFilmFacadeHeightAt(y,horizon,
                                                        eye,d,ground);
                    int feature=ggFilmFacadeFeature(build,surfaceU,
                                                    wallHeight);
                    int stone=build==B_TOWER||build==B_HALL||
                              build==B_TEMPLE||build==B_LIBRARY;
                    if(stone){
                        r=(151+(build==B_TEMPLE?28:0))*wallShade/256;
                        g=(146+(build==B_TEMPLE?24:0))*wallShade/256;
                        b=(142+(build==B_TEMPLE?15:0))*wallShade/256;
                    }else{
                        r=baseR*241/256;
                        g=baseG*266/256;
                        b=baseB*277/256;
                    }
                    if(roofBottom>=0&&y<roofBottom){
                        // Genuine world-space gable geometry added in
                        // the previous milestone; retain it and its
                        // face-dependent highlights unmodified.
                        r=roofR;g=roofG;b=roofB;
                        if(y+1==roofBottom){r+=13;g+=10;b+=8;}
                    }else if(feature==GG_FACADE_COURSE){
                        r=r*4/5;g=g*4/5;b=b*4/5;
                    }else if(feature==GG_FACADE_BEAM){
                        r=80*wallShade/256;g=62*wallShade/256;
                        b=54*wallShade/256;
                    }else if(feature==GG_FACADE_FRAME){
                        r=74*wallShade/256;g=68*wallShade/256;
                        b=71*wallShade/256;
                    }else if(feature==GG_FACADE_GLASS){
                        // Dim amber glazing set INTO a dark mullion,
                        // not a flat luminous yellow billboard.
                        int glow=((ggExplore.frame/17+tx*5+ty*11)&7)<2?
                                 8:0;
                        r=(149+glow)*wallShade/256;
                        g=(126+glow/2)*wallShade/256;
                        b=(100+glow/3)*wallShade/256;
                    }else if(feature==GG_FACADE_DOOR){
                        r=54*wallShade/256;g=52*wallShade/256;
                        b=58*wallShade/256;
                    }else if(feature==GG_FACADE_PORTAL){
                        r=48*wallShade/256;g=61*wallShade/256;
                        b=77*wallShade/256;
                    }else if(feature==GG_FACADE_TRIM){
                        r=113*wallShade/256;g=103*wallShade/256;
                        b=94*wallShade/256;
                    }else if(feature==GG_FACADE_SILL){
                        r=163*wallShade/256;g=157*wallShade/256;
                        b=138*wallShade/256;
                    }else if(feature==GG_FACADE_CORNICE){
                        r=130*wallShade/256;g=122*wallShade/256;
                        b=115*wallShade/256;
                    }else if(feature==GG_FACADE_BRACE){
                        r=105*wallShade/256;g=76*wallShade/256;
                        b=53*wallShade/256;
                    }else if(feature==GG_FACADE_MOSS){
                        r=62*wallShade/256;g=94*wallShade/256;
                        b=76*wallShade/256;
                    }else if(feature==GG_FACADE_SCULPT){
                        r=171*wallShade/256;g=159*wallShade/256;
                        b=140*wallShade/256;
                    }else if(feature==GG_FACADE_PILASTER){
                        int edge=(surfaceU&7)<2;
                        r=(194+edge*14)*wallShade/256;
                        g=(187+edge*9)*wallShade/256;
                        b=(172+edge*7)*wallShade/256;
                    }else if(feature==GG_FACADE_RELIEF){
                        int frieze=((surfaceU/7+wallHeight/2)&3)==0;
                        r=(132+frieze*20)*wallShade/256;
                        g=(121+frieze*15)*wallShade/256;
                        b=(109+frieze*10)*wallShade/256;
                    }else if(feature==GG_FACADE_SHUTTER){
                        int slat=(wallHeight&1)==0;
                        r=(82+slat*17)*wallShade/256;
                        g=(58+slat*13)*wallShade/256;
                        b=(51+slat*10)*wallShade/256;
                    }
                }else if(cinematic&&!build){
                    // Restrained grass and stone microtexture in genuine
                    // world cells: no static sprites or screen-space noise.
                    int pattern=(tx*19+ty*31+materialY/3)&15;
                    if(type==T_GRASS||type==T_FOREST){
                        if(pattern<2){r+=8;g+=9;b+=4;}
                        else if(pattern>13){r-=7;g-=9;b-=4;}
                        // A coherent moving breeze across actual terrain
                        // tiles; feet stay fixed and distant grass fades.
                        int gust=(ggExplore.frame/7+tx*9+ty*5)&31;
                        if(edge<16&&gust<8){
                            g+=5-gust/2;
                            r+=2;
                        }
                    }else if(type==T_MOUNT||type==T_HILL){
                        if(pattern<3){r+=11;g+=9;b+=6;}
                    }
                }
                int cr=ggExpClamp(r,0,255),
                    cg=ggExpClamp(g,0,255),
                    cb=ggExpClamp(b,0,255);
                if(cinematic){
                    u8 *p=top+(y*400+x)*3;
                    p[0]=(u8)cr;p[1]=(u8)cg;p[2]=(u8)cb;
                }else ggExpDotInView(top,x,y,cr,cg,cb);
            }
            highest=from;
            if(!highest)break;
        }
    }
    ggExpScenicCoarse=!cinematic;
    ggExpScenicDraw(top,0,0);
    ggExpScenicCoarse=0;
    // Draw nearby live inhabitants as indexed 2D cels projected onto
    // the same raycast world. A bounded on-map search avoids scanning the
    // whole creature list or allocating skeletal 3D models.
    if(cinematic)return; // no first-person crosshair, build HUD or 2x
                        // creature cels pasted over filmed cast
    ggExpRenderCreatures(top);
    // Nearby foliage can pass in FRONT of actual world-depth citizens.
    // Reuse the existing POV scenery selection, no second tile scan.
    if(ggExpNpcClosestDepth){
        ggExpScenicCoarse=1;
        ggExpScenicDraw(top,1,ggExpNpcClosestDepth);
        ggExpScenicCoarse=0;
    }
    // Smaller brass-bracketed aim marker; it communicates the true
    // editable world target without a huge retro-computer cross.
    ggExpRect(top,195,115,4,1,34,41,47);
    ggExpRect(top,201,115,4,1,34,41,47);
    ggExpRect(top,195,123,4,1,34,41,47);
    ggExpRect(top,201,123,4,1,34,41,47);
    ggExpRect(top,196,116,3,1,240,216,164);
    ggExpRect(top,201,116,3,1,240,216,164);
    ggExpRect(top,196,122,3,1,240,216,164);
    ggExpRect(top,201,122,3,1,240,216,164);
    ggExpRect(top,199,118,2,3,233,213,161);
    // One coherent, readable scene UI shared with the film captions:
    // warm brass keylines, cool slate panels and short grouped controls.
    ggExpFilmDrawDepth=0;
    ggExpRect(top,0,0,400,28,13,24,35);
    ggExpRect(top,0,26,400,2,174,137,85);
    ggExpRect(top,0,0,3,28,192,148,93);
    ggExpText(top,9,5,ggExplore.actorId?"CITIZEN'S EYES":
                                       "GODGUY  /  EXPLORING",
              242,220,164);
    char buf[96];
    snprintf(buf,sizeof buf,"X:%d  Y:%d",
             ggExplore.px>>8,ggExplore.py>>8);
    ggExpText(top,293,5,buf,220,225,221);
    int localTerrain=world[ggExplore.py>>8][ggExplore.px>>8];
    const char *region=localTerrain==T_FOREST?"WOODLAND":
                 localTerrain==T_JUNGLE?"JUNGLE":
                 localTerrain==T_HILL?"HIGHLANDS":
                 localTerrain==T_MOUNT?"MOUNTAINS":
                 localTerrain==T_SNOW?"SNOWFIELD":
                 localTerrain==T_SAND||localTerrain==T_DESERT?"DRYLANDS":
                 localTerrain==T_SWAMP?"MARSHLAND":
                 localTerrain<=T_WATER?"COAST":
                 localTerrain==T_DIRT?"ROADLAND":"GREEN COUNTRY";
    snprintf(buf,sizeof buf,"REGION: %s",region);
    ggExpText(top,9,17,buf,181,203,204);
    if(ggExplore.actorId)
        snprintf(buf,sizeof buf,"YOU ARE: %.18s",ggExplore.actorName);
    else snprintf(buf,sizeof buf,"BUILD: %s",
                  ggExpMaterialNames[ggExplore.material]);
    ggExpText(top,223,17,buf,225,204,153);
    ggExpRect(top,376,15,15,9,
              ggExpClamp(terrCol[ggExpMaterials[ggExplore.material]][0]*8,0,255),
              ggExpClamp(terrCol[ggExpMaterials[ggExplore.material]][1]*8,0,255),
              ggExpClamp(terrCol[ggExpMaterials[ggExplore.material]][2]*8,0,255));
    ggExpRect(top,0,196,400,44,12,22,33);
    ggExpRect(top,0,195,400,1,178,143,91);
    ggExpRect(top,0,239,400,1,178,143,91);
    // Controls are grouped by actual action rather than one long
    // unexplained NES-style string of letters.
    ggExpRect(top,8,202,92,14,29,42,53);
    ggExpRect(top,105,202,92,14,29,42,53);
    ggExpRect(top,202,202,92,14,29,42,53);
    ggExpRect(top,299,202,92,14,29,42,53);
    ggExpText(top,14,206,ggExplore.actorId?"A  INTERACT":"A  PLACE",
              239,218,163);
    ggExpText(top,111,206,"Y  CLEAR",239,218,163);
    ggExpText(top,208,206,"X  MATERIAL",239,218,163);
    ggExpText(top,305,206,"B  SPRINT",239,218,163);
    ggExpText(top,9,225,"MOVE PAD   LOOK L/R   TAP MAP TO WARP",
              202,217,218);
    ggExpText(top,250,225,"SELECT SAVE  START EXIT",171,197,206);
    if(ggExplore.feedbackT){
        const char *msg=ggExplore.feedback==2?"TERRAIN CHANGED":
                        ggExplore.feedback==3?"BLOCKED: PROTECTED/TOO FAR":
                        ggExplore.feedback==4?"WORLD SAVED":
                        ggExplore.feedback==5?"SAVE FAILED: RETURN TO MAP":
                        "BUILDING MATERIAL SELECTED";
        int warning=ggExplore.feedback==3||ggExplore.feedback==5;
        ggExpRect(top,8,179,330,13,warning?69:44,
                  warning?39:62,warning?38:58);
        ggExpRect(top,8,179,3,13,warning?219:169,
                  warning?115:185,warning?103:139);
        ggExpText(top,16,182,msg,250,224,174);
    }
}
static void ggExpRender(u8 *top){
    ggExpRenderMode(top,0);
}
#endif

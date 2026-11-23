#ifndef GG_MESH_RESIDENTIAL_LOTS3D_H
#define GG_MESH_RESIDENTIAL_LOTS3D_H

// Masses share the settlement roof and solid primitives. Plan coordinates
// are relative to the parent, with a west-facing entrance before rotation.
typedef struct { int x0,y0,x1,y1; } GgLotRect;
typedef struct { GgLotRect wall; int floor,upper; } GgLotElevation;
typedef struct {
    GgSettlementPlan mass;
    GgLotElevation elevation[4];
    int elevations,rotate,doorPlane,doorCenter,doorFloor;
} GgLotPlan;

static inline int ggMeshLotHandles(int build){
    return build==B_HOUSE_EW||build==B_HOUSE_NS||build==B_ESTATE||
           build==B_WING_E||build==B_WING_S||build==B_WING_SE;
}
static inline GgLotRect ggLotRotate(const GgLotPlan *p,GgLotRect b){
    return p->rotate?(GgLotRect){b.y0,b.x0,b.y1,b.x1}:b;
}
static void ggLotBox(GgLotPlan*p,int x0,int y0,int x1,int y1,int z0,int z1,int color){
    GgLotRect b=ggLotRotate(p,(GgLotRect){x0,y0,x1,y1});
    ggSettlementBox(&p->mass,b.x0,b.y0,b.x1,b.y1,z0,z1,color);
}
static void ggLotFoot(GgLotPlan*p,int x0,int y0,int x1,int y1,int top,int color){
    GgLotRect b=ggLotRotate(p,(GgLotRect){x0,y0,x1,y1});
    ggSettlementFoot(&p->mass,b.x0,b.y0,b.x1,b.y1,top,color);
}
static void ggLotRoof(GgLotPlan*p,int x0,int y0,int x1,int y1,int eave,int peak,
                       int axis,int ridge,int hip,int color,int open){
    GgLotRect b=ggLotRotate(p,(GgLotRect){x0,y0,x1,y1});
    ggSettlementRoof(&p->mass,b.x0,b.y0,b.x1,b.y1,eave,peak,axis^p->rotate,
                     ridge,hip,color,open);
}
static void ggLotWall(GgLotPlan*p,int x0,int y0,int x1,int y1,int floor,int eave,
                       int wall,int upper){
    GgLotRect b=ggLotRotate(p,(GgLotRect){x0,y0,x1,y1});
    ggSettlementFoot(&p->mass,b.x0,b.y0,b.x1,b.y1,floor+2,GG_SETTLE_FOUNDATION);
    ggSettlementBox(&p->mass,b.x0,b.y0,b.x1,b.y1,floor+2,eave,wall);
    p->elevation[p->elevations++]=(GgLotElevation){b,floor,upper};
}
static int ggLotEntranceLevel(GgLotPlan*p,int x,int y){
    int plane=(p->rotate?p->mass.oy:p->mass.ox)+x;
    int across=(p->rotate?p->mass.ox:p->mass.oy)+y;
    int floor=ggMeshDoorFloor(plane,p->rotate,1,across-100,across+100);
    int panel=plane-GG_MESH_DOOR_OFFSET;
    int center=ggMeshGroundAtQ8(p->rotate?across:panel,p->rotate?panel:across);
    p->doorPlane=plane;p->doorCenter=across;p->doorFloor=center>floor?center:floor;
    return p->doorFloor;
}
static void ggLotEntrance(GgLotPlan*p,int x,int y,int floor,int wall){
    int width=ggMeshDoorHalfWidth(100);
    int top=floor+GG_WORLD_DOOR_HEIGHT;
    ggLotFoot(p,x-17,y-width-10,x,y-width,floor+2,GG_SETTLE_FOUNDATION);
    ggLotFoot(p,x-17,y+width,x,y+width+10,floor+2,GG_SETTLE_FOUNDATION);
    ggLotBox(p,x-17,y-width-10,x,y-width,floor+2,top+3,wall);
    ggLotBox(p,x-17,y+width,x,y+width+10,floor+2,top+3,wall);
    ggLotBox(p,x-17,y-width,x,y+width,top,top+3,wall);
    ggLotBox(p,x-9,y-width,x-8,y+width,floor,top,GG_SETTLE_DOOR);
    ggLotFoot(p,x-29,y-width-8,x-8,y+width+8,floor,GG_SETTLE_FOUNDATION);
}
static void ggLotPorch(GgLotPlan*p,int x0,int y0,int x1,int y1,int eave,
                       int wall,int roof,int gabled){
    ggLotFoot(p,x0+8,y0+7,x0+27,y0+25,eave-1,wall);
    ggLotFoot(p,x0+8,y1-25,x0+27,y1-7,eave-1,wall);
    ggLotBox(p,x0+3,y0+3,x0+30,y1-3,eave-3,eave,wall);
    if(gabled)ggLotRoof(p,x0,y0,x1,y1,eave,eave+9,0,128,0,roof,0);
    else ggLotRoof(p,x0,y0,x1,y1,eave,eave+7,1,256,0,roof,1);
}
static void ggLotChimney(GgLotPlan*p,int x,int y,int base,int top,int color){
    ggLotBox(p,x-11,y-13,x+11,y+13,base,top-3,color);
    ggLotBox(p,x-15,y-16,x+15,y+16,top-3,top,GG_SETTLE_MASONRY);
}
static void ggLotHouse(GgLotPlan*p,int wing,int variant){
    int g=p->mass.ground,front=58,end=wing?480:222;
    int floor=ggLotEntranceLevel(p,front,122);
    int eave=g+33,lintel=floor+GG_WORLD_DOOR_HEIGHT+6;
    if(eave<lintel)eave=lintel;
    static const unsigned char wall[]={GG_SETTLE_PLASTER,GG_SETTLE_GREEN,GG_SETTLE_CREAM};
    static const unsigned char roof[]={GG_SETTLE_TERRA,GG_SETTLE_SLATE,GG_SETTLE_SHINGLE};
    int ink=wall[variant],shingle=roof[variant];
    ggLotWall(p,front,36,207,end,g,eave,ink,eave-12);
    ggLotFoot(p,52,32,214,end+5,g+3,GG_SETTLE_FOUNDATION);
    ggLotBox(p,54,32,211,end+4,eave-13,eave-11,GG_SETTLE_DARK_OAK);
    ggLotRoof(p,40,24,227,end+15,eave-1,eave+19,1,
               variant==2?164:128,variant==0?0:38,shingle,0);
    ggLotEntrance(p,front,122,floor,ink);
    ggLotPorch(p,16,65,72,179,floor+GG_WORLD_DOOR_HEIGHT+3,
                 GG_SETTLE_OAK,shingle,variant==0);
    ggLotChimney(p,175,178,eave,eave+25,GG_SETTLE_LIMESTONE);
    if(wing){
        // The second room has an attached shed, dormer and chimney.
        ggLotBox(p,66,345,116,412,eave+3,eave+15,ink);
        ggLotRoof(p,59,335,124,421,eave+14,eave+23,0,128,0,shingle,0);
        ggLotWall(p,188,315,236,454,g,g+22,GG_SETTLE_CEDAR,0);
        ggLotRoof(p,177,304,246,465,g+21,g+32,1,0,0,shingle,0);
        ggLotChimney(p,166,419,eave,eave+25,GG_SETTLE_LIMESTONE);
    }
}
static void ggLotEstate(GgLotPlan*p,unsigned mask,int variant){
    int g=p->mass.ground,front=56,entry=121;
    int floor=ggLotEntranceLevel(p,front,entry);
    int eave=g+47,lintel=floor+GG_WORLD_DOOR_HEIGHT+12;
    if(eave<lintel)eave=lintel;
    int accent=variant==1?GG_SETTLE_TERRA:GG_SETTLE_SLATE;
    ggLotWall(p,front,34,232,228,g,eave,GG_SETTLE_LIMESTONE,g+27);
    ggLotBox(p,51,30,238,233,eave-4,eave-1,GG_SETTLE_MASONRY);
    ggLotRoof(p,42,20,247,244,eave-1,eave+19,1,128,42,accent,0);
    ggLotEntrance(p,front,entry,floor,GG_SETTLE_LIMESTONE);
    ggLotFoot(p,17,72,43,172,floor-2,GG_SETTLE_FOUNDATION);
    ggLotPorch(p,16,63,69,179,floor+GG_WORLD_DOOR_HEIGHT+5,
                 GG_SETTLE_LIMESTONE,GG_SETTLE_TERRA,1);
    ggLotChimney(p,192,160,eave,eave+26,GG_SETTLE_MASONRY);
    if(mask&GG_RES_EAST){
        ggLotWall(p,214,48,476,219,g,g+34,GG_SETTLE_PLASTER,g+20);
        ggLotRoof(p,202,34,491,233,g+33,g+47,0,128,35,accent,0);
    }
    if(mask&GG_RES_SOUTH){
        ggLotWall(p,54,211,221,474,g,g+37,GG_SETTLE_CREAM,g+22);
        ggLotRoof(p,40,199,237,491,g+36,g+53,1,128,37,accent,0);
    }
    if(mask&GG_RES_CORNER){
        // Extend the garden wing toward the surviving neighbors.
        int x=(mask&GG_RES_SOUTH)?207:278,y=(mask&GG_RES_EAST)?204:278;
        ggLotWall(p,x,y,474,471,g,g+30,GG_SETTLE_GREEN,0);
        ggLotRoof(p,x-11,y-12,488,486,g+29,g+42,(mask&GG_RES_SOUTH)?0:1,
                   128,29,GG_SETTLE_SHINGLE,0);
    }
}
static void ggLotOutbuilding(GgLotPlan*p,int variant){
    int front=63,g=p->mass.ground,floor=ggLotEntranceLevel(p,front,127);
    int eave=floor+GG_WORLD_DOOR_HEIGHT+4;
    int color=variant==1?GG_SETTLE_CEDAR:GG_SETTLE_MASONRY;
    ggLotWall(p,front,56,199,198,g,eave,color,0);
    ggLotRoof(p,50,44,212,211,eave-1,eave+13,0,128,0,GG_SETTLE_SHINGLE,0);
    ggLotEntrance(p,front,127,floor,color);
}
static void ggMeshLotPlan(GgLotPlan*p,int tx,int ty,int build,int ground,unsigned mask){
    p->mass.count=0;p->mass.ox=tx*256;p->mass.oy=ty*256;p->mass.ground=ground;
    p->elevations=0;p->rotate=build==B_HOUSE_EW;
    int variant=ggMeshSettlementVariant(tx,ty,build);
    if(build==B_ESTATE)ggLotEstate(p,mask,variant);
    else if(build==B_HOUSE_EW||build==B_HOUSE_NS)
        ggLotHouse(p,(mask&(build==B_HOUSE_EW?GG_RES_EAST:GG_RES_SOUTH))!=0,variant);
    else ggLotOutbuilding(p,variant);
}

static void ggLotPane(GgMeshContext*m,int axis,int plane,int center,int half,
                      int low,int high,int r,int g,int b){
    ggMeshQuad(m,ggMeshFacadePoint(axis,plane,center-half,low),
                 ggMeshFacadePoint(axis,plane,center+half,low),
                 ggMeshFacadePoint(axis,plane,center+half,high),
                 ggMeshFacadePoint(axis,plane,center-half,high),r,g,b);
}
static void ggLotWindowRow(GgMeshContext*m,const GgLotPlan*p,int axis,int wall,
                            int facing,int start,int end,int low,int upper,
                            int detail,int trim){
    int length=end-start,count=length>350?6:length>230?4:2;
    int plane=wall-facing*2,half=length>230?17:13;
    int stride=detail||count<=2?1:2;
    for(int i=0;i<count;i+=stride){
        int u=start+(2*i+1)*length/(2*count);
        // Leave the human-scale entrance readable even as a wall
        // stretches along an additional plot.
        if(axis!=p->rotate||wall!=p->doorPlane||
           abs(u-p->doorCenter)>ggMeshDoorHalfWidth(100)+half+8){
            if(detail)ggSettlementGlazing(m,axis,plane,facing,u,half,low,low+7,trim);
            else ggLotPane(m,axis,plane,u,half,low,low+7,76,105,115);
        }
        if(upper){
            if(detail)ggSettlementGlazing(m,axis,plane,facing,u,half-3,upper,upper+6,0);
            else ggLotPane(m,axis,plane,u,half-3,upper,upper+6,76,105,115);
        }
    }
}
static void ggLotOpenings(GgMeshContext*m,const GgLotPlan*p,int build,int detail){
    int prior=m->material,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    int view=p->rotate?m->cy:m->cx;
    if(view<p->doorPlane){
        if(detail)ggMeshDoorway(m,p->doorPlane,p->rotate,1,p->doorCenter,100,p->doorFloor);
        else ggLotPane(m,p->rotate,p->doorPlane-GG_MESH_DOOR_OFFSET,
                       p->doorCenter,ggMeshDoorHalfWidth(100),p->doorFloor,
                       p->doorFloor+GG_WORLD_DOOR_HEIGHT,46,51,58);
    }
    for(int i=0;i<p->elevations;i++){
        const GgLotElevation *face=&p->elevation[i];
        int x0=p->mass.ox+face->wall.x0,x1=p->mass.ox+face->wall.x1;
        int y0=p->mass.oy+face->wall.y0,y1=p->mass.oy+face->wall.y1;
        int low=face->floor+9,upper=face->upper;
        int trim=detail&&(i==0||build!=B_ESTATE);
        // Each wall is at most one visible facade per axis. Its trim
        // never replaces a body, foundation or collision surface.
        if(m->cx<x0)
            ggLotWindowRow(m,p,0,x0,1,y0,y1,low,upper,detail,trim);
        else if(m->cx>x1)
            ggLotWindowRow(m,p,0,x1,-1,y0,y1,low,upper,detail,trim);
        if(m->cy<y0)
            ggLotWindowRow(m,p,1,y0,1,x0,x1,low,upper,detail,trim);
        else if(m->cy>y1)
            ggLotWindowRow(m,p,1,y1,-1,x0,x1,low,upper,detail,trim);
    }
    m->material=prior;m->depthBias=bias;
}
static int ggLotFooting(const GgLotPlan*p,const GgSettlementPart*b){
    return ggMeshFoundationGround(p->mass.ox+b->x0,p->mass.oy+b->y0,
                                  p->mass.ox+b->x1,p->mass.oy+b->y1,p->mass.ground);
}
static int ggMeshLotDraw(GgMeshContext*m,int tx,int ty,int build,int ground,int nearDetail){
    if(!ggMeshLotHandles(build))return 0;
    GgLotPlan p;
    ggMeshLotPlan(&p,tx,ty,build,ground,ggResidenceMask(tx,ty,build));
    int prior=m->material,footings[16];
    unsigned sampled=0;
    int family=build==B_HOUSE_EW||build==B_HOUSE_NS;
    // The wider family plinth encloses the narrower wall footing.
    for(int i=family;i<p.mass.count;i++){
        const GgSettlementPart *b=&p.mass.part[i];
        const unsigned char *ink=ggSettlementColors[b->color];
        m->material=ink[3];
        if(b->type>=GG_SETTLE_ROOF)ggSettlementDrawRoof(m,&p.mass,b,ink);
        else{
            int bottom=b->z0;
            if(b->type==GG_SETTLE_FOOT){
                unsigned key=(b->x0>>8)+(b->x1>>8)*2+(b->y0>>8)*4+(b->y1>>8)*8;
                unsigned bit=1u<<key;
                if(!(sampled&bit)){
                    footings[key]=ggLotFooting(&p,b);
                    sampled|=bit;
                }
                bottom=footings[key];
            }
            ggMeshArchitectureBlock(m,p.mass.ox+b->x0,p.mass.oy+b->y0,
                                    p.mass.ox+b->x1,p.mass.oy+b->y1,
                                    bottom,b->z1,ink[0],ink[1],ink[2]);
        }
    }
    int detail=nearDetail&&m->detailQuality>=2&&
               abs(m->cx-p.mass.ox-128)<1024&&abs(m->cy-p.mass.oy-128)<1024;
    ggLotOpenings(m,&p,build,detail);
    m->material=prior;
    return 1;
}
static int ggMeshLotSolid(int tx,int ty,int build,int x,int y,int z,int ground){
    if(!ggMeshLotHandles(build))return -1;
    int u=x-tx*256,v=y-ty*256;
    if((unsigned)u>=512||(unsigned)v>=512||z>ground+122)return 0;
    unsigned mask=(unsigned)ggResidenceMask(tx,ty,build);
    int quadrant=(u>>8)+2*(v>>8);
    if(quadrant&&!(mask&(1u<<(quadrant-1))))return 0;
    GgLotPlan p;
    ggMeshLotPlan(&p,tx,ty,build,ground,mask);
    for(int i=0;i<p.mass.count;i++){
        const GgSettlementPart *b=&p.mass.part[i];
        if(u<b->x0||u>b->x1||v<b->y0||v>b->y1||z>b->z1)continue;
        if(b->type==GG_SETTLE_FOOT){
            if(z>=b->z0||z>=ggLotFooting(&p,b))return 1;
        }else if(z>=b->z0){
            if(b->type==GG_SETTLE_BLOCK)return 1;
            int cap=ggSettlementRoofZ(b,u,v);
            if(z*256<=cap&&(b->type==GG_SETTLE_ROOF||z*256>=cap-512))return 1;
        }
    }
    return 0;
}

#endif

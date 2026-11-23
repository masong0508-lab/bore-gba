#ifndef GG_MESH_SETTLEMENT3D_H
#define GG_MESH_SETTLEMENT3D_H

enum {
    GG_SETTLE_BLOCK, GG_SETTLE_FOOT, GG_SETTLE_ROOF, GG_SETTLE_AWNING,
    GG_SETTLE_PART_LIMIT=32
};
enum {
    GG_SETTLE_FOUNDATION, GG_SETTLE_LIMESTONE, GG_SETTLE_MASONRY,
    GG_SETTLE_PLASTER, GG_SETTLE_CREAM, GG_SETTLE_CLAY, GG_SETTLE_GREEN,
    GG_SETTLE_OAK, GG_SETTLE_DARK_OAK, GG_SETTLE_CEDAR, GG_SETTLE_DOOR,
    GG_SETTLE_TERRA, GG_SETTLE_SLATE, GG_SETTLE_THATCH, GG_SETTLE_SHINGLE,
    GG_SETTLE_INDIGO, GG_SETTLE_LINEN, GG_SETTLE_MADDER, GG_SETTLE_BRASS
};
static const unsigned char ggSettlementColors[][4]={
    {117,108,92,GG_MAT_STONE}, {198,182,153,GG_MAT_STONE},
    {150,147,134,GG_MAT_STONE}, {221,203,172,GG_MAT_STONE},
    {222,202,156,GG_MAT_STONE}, {183,126,100,GG_MAT_STONE},
    {161,174,145,GG_MAT_STONE}, {143,109,72,GG_MAT_WOOD},
    {91,66,49,GG_MAT_WOOD}, {168,119,84,GG_MAT_WOOD},
    {46,51,58,GG_MAT_NONE}, {155,78,57,GG_MAT_ROOF},
    {80,101,120,GG_MAT_ROOF}, {162,131,76,GG_MAT_ROOF},
    {94,119,100,GG_MAT_ROOF}, {82,117,150,GG_MAT_FABRIC},
    {221,197,145,GG_MAT_FABRIC}, {163,76,62,GG_MAT_FABRIC},
    {200,162,85,GG_MAT_NONE}
};
typedef struct {
    short x0,y0,x1,y1,z0,z1,ridge;
    unsigned char type,color,axis,hip;
} GgSettlementPart;
typedef struct {
    GgSettlementPart part[GG_SETTLE_PART_LIMIT];
    int count,ox,oy,ground,variant;
    int x0,y0,x1,y1,eave,floor,hasDoor,upper;
} GgSettlementPlan;

static inline int ggMeshSettlementHandles(int build){
    return build==B_HUT||build==B_HOUSE||build==B_MANOR||
           build==B_WORKSHOP||build==B_INN||build==B_BARRACKS||build==B_MARKET;
}
static inline int ggMeshSettlementVariant(int tx,int ty,int build){
    return (int)(ggFilmScenicHash(tx,ty,71+build*9)%3u);
}
static void ggSettlementPart(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                             int z0,int z1,int color,int type,
                             int axis,int ridge,int hip){
    p->part[p->count++]=(GgSettlementPart){x0,y0,x1,y1,z0,z1,ridge,type,color,axis,hip};
}
static void ggSettlementBox(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                            int z0,int z1,int color){
    ggSettlementPart(p,x0,y0,x1,y1,z0,z1,color,GG_SETTLE_BLOCK,0,0,0);
}
static void ggSettlementFoot(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                             int top,int color){
    ggSettlementPart(p,x0,y0,x1,y1,p->ground-1,top,color,GG_SETTLE_FOOT,0,0,0);
}
static void ggSettlementRoof(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                             int eave,int peak,int axis,int ridge,int hip,
                             int color,int open){
    ggSettlementPart(p,x0,y0,x1,y1,eave-2,peak,color,
                     open?GG_SETTLE_AWNING:GG_SETTLE_ROOF,axis,ridge,hip);
}
static void ggSettlementWall(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                             int top,int color){
    ggSettlementFoot(p,x0,y0,x1,y1,p->ground+2,GG_SETTLE_FOUNDATION);
    ggSettlementBox(p,x0,y0,x1,y1,p->ground+2,top,color);
}
static void ggSettlementHouseShell(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                                   int top,int color){
    int half=(y1-y0)/2,center=(y0+y1)/2,width=ggMeshDoorHalfWidth(half);
    int low=center-width,high=center+width;
    p->floor=ggMeshDoorFloor(p->ox+x0,0,1,p->oy+y0,p->oy+y1);
    int lintel=p->floor+GG_WORLD_DOOR_HEIGHT;
    int eave=top>lintel+3?top:lintel+3;
    p->x0=x0;p->x1=x1;p->y0=y0;p->y1=y1;p->eave=eave;p->hasDoor=1;
    ggSettlementWall(p,x0,y0,x1,y1,eave,color);
    // Two structural returns and an overhead lintel give the doorway
    // an actual recess. The closing door sits behind the wall plane.
    ggSettlementFoot(p,x0-12,y0,x0,low,p->ground+2,GG_SETTLE_FOUNDATION);
    ggSettlementFoot(p,x0-12,high,x0,y1,p->ground+2,GG_SETTLE_FOUNDATION);
    ggSettlementBox(p,x0-12,y0,x0,low,p->ground+2,eave,color);
    ggSettlementBox(p,x0-12,high,x0,y1,p->ground+2,eave,color);
    ggSettlementBox(p,x0-12,low,x0,high,lintel,eave,color);
    ggSettlementBox(p,x0-9,low,x0-8,high,p->floor,lintel,GG_SETTLE_DOOR);
    ggSettlementFoot(p,x0-18,low-5,x0-6,high+5,p->floor,GG_SETTLE_FOUNDATION);
}
static void ggSettlementChimney(GgSettlementPlan*p,int x,int y,int base,
                                int top,int half,int color){
    ggSettlementBox(p,x-half,y-half,x+half,y+half,base,top-3,color);
    ggSettlementBox(p,x-half-3,y-half-3,x+half+3,y+half+3,top-3,top,GG_SETTLE_MASONRY);
    ggSettlementBox(p,x-half+3,y-half+3,x+half-3,y+half-3,top,top+1,GG_SETTLE_DARK_OAK);
}
static void ggSettlementCanopy(GgSettlementPlan*p,int x0,int y0,int x1,int y1,
                               int roofLow,int roofHigh,int color){
    // The sloped roof and widely spaced posts leave a useful aisle.
    ggSettlementFoot(p,x0+5,y0+7,x0+24,y0+24,roofLow-1,GG_SETTLE_DARK_OAK);
    ggSettlementFoot(p,x0+5,y1-24,x0+24,y1-7,roofLow-1,GG_SETTLE_DARK_OAK);
    ggSettlementBox(p,x0+3,y0+5,x0+25,y1-5,roofLow-2,roofLow+1,GG_SETTLE_OAK);
    ggSettlementRoof(p,x0,y0,x1,y1,roofLow,roofHigh,1,256,0,color,1);
}
static void ggSettlementDwelling(GgSettlementPlan*p,int build){
    int g=p->ground,v=p->variant;
    if(build==B_HUT){
        static const unsigned char wall[]={GG_SETTLE_OAK,GG_SETTLE_LIMESTONE,GG_SETTLE_GREEN};
        static const unsigned char roof[]={GG_SETTLE_THATCH,GG_SETTLE_SHINGLE,GG_SETTLE_TERRA};
        ggSettlementHouseShell(p,58,58,194-v*5,198,g+22,wall[v]);
        int t=p->eave;
        ggSettlementRoof(p,40,43,207-v*5,213,t-1,t+15+2*v,v==1,
                          v==2?165:128,v==1?24:0,roof[v],0);
        if(v==0){
            ggSettlementCanopy(p,23,85,60,171,g+19,g+23,GG_SETTLE_THATCH);
        }else if(v==1){
            ggSettlementChimney(p,170,143,t,t+23,11,GG_SETTLE_LIMESTONE);
        }else{
            ggSettlementWall(p,168,139,222,207,g+17,GG_SETTLE_CEDAR);
            ggSettlementRoof(p,159,129,232,218,g+16,g+24,1,0,0,GG_SETTLE_SHINGLE,0);
        }
        return;
    }
    static const unsigned char wall[]={GG_SETTLE_PLASTER,GG_SETTLE_CREAM,GG_SETTLE_CLAY};
    static const unsigned char roof[]={GG_SETTLE_TERRA,GG_SETTLE_SLATE,GG_SETTLE_SHINGLE};
    int x1=v==0?182:v==1?203:190;
    ggSettlementHouseShell(p,28,44-v*3,x1,212+v*3,g+26,wall[v]);
    int t=p->eave;
    if(v==1){
        // A jettied upper storey overhangs the shaded lower walls.
        p->upper=t+2;
        ggSettlementBox(p,15,35,x1+9,221,t-1,t+2,GG_SETTLE_DARK_OAK);
        ggSettlementBox(p,16,36,x1+8,220,t+2,t+12,GG_SETTLE_LIMESTONE);
        ggSettlementBox(p,14,34,x1+10,222,t+10,t+12,GG_SETTLE_DARK_OAK);
        ggSettlementRoof(p,8,29,x1+15,227,t+11,t+25,0,128,0,roof[v],0);
        ggSettlementChimney(p,173,169,t+12,t+30,12,GG_SETTLE_MASONRY);
    }else{
        ggSettlementRoof(p,8,28,x1+14,228,t-1,t+23,0,
                          v==2?166:128,0,roof[v],0);
        ggSettlementChimney(p,144,173,t,t+30,13,GG_SETTLE_LIMESTONE);
        int y=v==0?143:34;
        ggSettlementWall(p,x1-19,y,230,y+80,g+18,GG_SETTLE_CEDAR);
        ggSettlementRoof(p,x1-26,y-9,240,y+89,g+17,g+26,1,0,0,
                          v==0?GG_SETTLE_SHINGLE:GG_SETTLE_TERRA,0);
    }
}
static void ggSettlementMarket(GgSettlementPlan*p){
    int g=p->ground;
    ggSettlementFoot(p,22,26,234,230,g+1,GG_SETTLE_FOUNDATION);
    for(int side=0;side<2;side++)for(int end=0;end<2;end++){
        int x=end?207:31,y=side?207:34;
        ggSettlementFoot(p,x,y,x+18,y+17,g+22,GG_SETTLE_DARK_OAK);
    }
    for(int side=0;side<2;side++){
        int y=side?193:48;
        ggSettlementBox(p,29,y,227,y+12,g+20,g+22,GG_SETTLE_OAK);
        ggSettlementFoot(p,82,y+4,203,y+30,g+7,GG_SETTLE_CEDAR);
        ggSettlementBox(p,76,y,210,y+35,g+7,g+9,GG_SETTLE_OAK);
    }
    // A physically thin canopy leaves the ridge and the center aisle
    // clear underneath. Each colored panel is part of the same roof.
    int dye=p->variant==1?GG_SETTLE_MADDER:GG_SETTLE_INDIGO;
    for(int i=0;i<5;i++)
        ggSettlementRoof(p,18+i*44,24,18+(i+1)*44,232,g+21,g+29,0,128,0,
                          (i&1)?GG_SETTLE_LINEN:dye,1);
    ggSettlementBox(p,169,61,197,86,g+9,g+13,GG_SETTLE_CEDAR);
    ggSettlementBox(p,176,191,207,215,g+10,g+13,GG_SETTLE_LINEN);
}
static void ggSettlementPublic(GgSettlementPlan*p,int build){
    int g=p->ground;
    if(build==B_MARKET){ggSettlementMarket(p);return;}
    if(build==B_MANOR){
        ggSettlementHouseShell(p,69,42,222,212,g+46,GG_SETTLE_LIMESTONE);
        int t=p->eave;
        p->upper=g+25;
        ggSettlementRoof(p,52,27,239,226,t-1,t+17,0,128,44,GG_SETTLE_SLATE,0);
        ggSettlementFoot(p,23,82,65,174,g+2,GG_SETTLE_FOUNDATION);
        ggSettlementFoot(p,34,78,66,178,g+3,GG_SETTLE_LIMESTONE);
        ggSettlementFoot(p,43,86,67,170,g+4,GG_SETTLE_LIMESTONE);
        ggSettlementFoot(p,42,87,62,105,g+29,GG_SETTLE_MASONRY);
        ggSettlementFoot(p,42,151,62,169,g+29,GG_SETTLE_MASONRY);
        ggSettlementBox(p,37,81,74,174,g+26,g+30,GG_SETTLE_LIMESTONE);
        ggSettlementRoof(p,31,76,79,180,g+29,g+42,0,128,0,GG_SETTLE_TERRA,0);
        ggSettlementChimney(p,183,166,t,t+27,14,GG_SETTLE_LIMESTONE);
    }else if(build==B_WORKSHOP){
        ggSettlementHouseShell(p,91,47,228,210,g+25,GG_SETTLE_MASONRY);
        int t=p->eave;
        ggSettlementRoof(p,79,33,242,224,t-1,t+12,0,145,0,GG_SETTLE_SHINGLE,0);
        ggSettlementCanopy(p,21,65,98,203,g+21,t+4,GG_SETTLE_SLATE);
        ggSettlementFoot(p,29,71,96,196,g+1,GG_SETTLE_FOUNDATION);
        ggSettlementFoot(p,33,166,85,193,g+6,GG_SETTLE_OAK);
        ggSettlementFoot(p,177,151,217,191,t+11,GG_SETTLE_MASONRY);
        ggSettlementChimney(p,197,170,t+11,t+30,15,GG_SETTLE_CLAY);
    }else if(build==B_INN){
        ggSettlementHouseShell(p,67,43,209,215,g+36,GG_SETTLE_CREAM);
        int t=p->eave;
        p->upper=t-11;
        ggSettlementBox(p,49,37,219,221,t-13,t-10,GG_SETTLE_DARK_OAK);
        ggSettlementBox(p,50,37,218,221,t-10,t+1,GG_SETTLE_PLASTER);
        ggSettlementRoof(p,36,25,233,232,t,t+17,0,128,0,GG_SETTLE_TERRA,0);
        ggSettlementCanopy(p,18,58,71,201,g+20,g+26,GG_SETTLE_SHINGLE);
        ggSettlementFoot(p,25,62,66,194,g+1,GG_SETTLE_FOUNDATION);
        ggSettlementBox(p,23,70,59,76,t-5,t-3,GG_SETTLE_DARK_OAK);
        ggSettlementBox(p,27,72,31,76,t-11,t-3,GG_SETTLE_OAK);
        ggSettlementBox(p,20,70,43,76,t-15,t-9,GG_SETTLE_BRASS);
        ggSettlementChimney(p,178,174,t,t+24,13,GG_SETTLE_MASONRY);
    }else{
        ggSettlementHouseShell(p,55,40,222,216,g+34,GG_SETTLE_MASONRY);
        int t=p->eave;
        ggSettlementRoof(p,44,27,236,229,t-1,t+16,1,128,39,GG_SETTLE_SLATE,0);
        for(int side=0;side<2;side++){
            int y=side?161:39;
            ggSettlementFoot(p,22,y-3,78,y+57,g+3,GG_SETTLE_FOUNDATION);
            ggSettlementWall(p,26,y,74,y+54,t+3,GG_SETTLE_MASONRY);
            ggSettlementBox(p,23,y-3,77,y+57,t+1,t+4,GG_SETTLE_LIMESTONE);
            ggSettlementRoof(p,19,y-8,81,y+62,t+3,t+11,0,128,14,GG_SETTLE_SLATE,0);
        }
        ggSettlementFoot(p,29,97,55,159,g+1,GG_SETTLE_FOUNDATION);
        ggSettlementBox(p,36,100,53,156,g+20,g+24,GG_SETTLE_DARK_OAK);
    }
}
static void ggMeshSettlementPlan(GgSettlementPlan*p,int tx,int ty,int build,int ground){
    p->count=0;p->ox=tx*256;p->oy=ty*256;p->ground=ground;
    p->variant=ggMeshSettlementVariant(tx,ty,build);
    p->hasDoor=0;p->upper=0;
    if(build==B_HUT||build==B_HOUSE)ggSettlementDwelling(p,build);
    else ggSettlementPublic(p,build);
}
static inline int ggSettlementFootingBottom(const GgSettlementPlan *p){
    // The plans fit one cell. Its bound anchors every wall skirt, post
    // and step to the same terrain without resampling for each piece.
    return ggMeshFoundationGround(p->ox,p->oy,p->ox+255,p->oy+255,p->ground);
}

typedef struct { int u0,u1,v0,v1,ridge,eave; } GgSettlementRoofFrame;
static inline GgSettlementRoofFrame ggSettlementRoofFrame(const GgSettlementPart*b){
    int u0=b->axis?b->y0:b->x0,u1=b->axis?b->y1:b->x1;
    int v0=b->axis?b->x0:b->y0,v1=b->axis?b->x1:b->y1;
    return (GgSettlementRoofFrame){u0,u1,v0,v1,v0+(v1-v0)*b->ridge/256,b->z0+2};
}
static inline GgMeshVertex ggSettlementVertex(const GgSettlementPlan*p,int axis,
                                              int u,int v,int z){
    return ggMeshV(p->ox+(axis?v:u),p->oy+(axis?u:v),z);
}
static inline GgMeshVertex ggSettlementLower(GgMeshVertex v,int depth){
    v.z-=depth;return v;
}
static void ggSettlementRoofQuad(GgMeshContext*m,int axis,GgMeshVertex a,
                                 GgMeshVertex b,GgMeshVertex c,GgMeshVertex d,
                                 int r,int g,int bl){
    if(axis)ggMeshQuad(m,a,d,c,b,r,g,bl);
    else ggMeshQuad(m,a,b,c,d,r,g,bl);
}
static void ggSettlementRoofTriangle(GgMeshContext*m,int axis,GgMeshVertex a,
                                     GgMeshVertex b,GgMeshVertex c,int r,int g,int bl){
    if(axis)ggMeshTriangle(m,a,c,b,r,g,bl);
    else ggMeshTriangle(m,a,b,c,r,g,bl);
}
static void ggSettlementGable(GgMeshContext*m,int axis,GgMeshVertex a,
                              GgMeshVertex peak,GgMeshVertex b,int r,int g,int bl){
    GgMeshVertex crown=ggSettlementLower(peak,3);
    if(crown.z<a.z)crown.z=a.z;
    ggSettlementRoofTriangle(m,axis,a,crown,b,137+r/5,104+g/5,73+bl/5);
    ggSettlementRoofTriangle(m,axis,a,peak,crown,r,g,bl);
    ggSettlementRoofTriangle(m,axis,crown,peak,b,r,g,bl);
}
static void ggSettlementDrawRoof(GgMeshContext*m,const GgSettlementPlan*p,
                                 const GgSettlementPart*b,const unsigned char *ink){
    GgSettlementRoofFrame f=ggSettlementRoofFrame(b);
    int axis=b->axis,open=b->type==GG_SETTLE_AWNING;
    int r=ink[0],g=ink[1],bl=ink[2],edgeR=r*3/4,edgeG=g*3/4,edgeB=bl*3/4;
    int z0=f.v0==f.ridge?b->z1:f.eave,z1=f.v1==f.ridge?b->z1:f.eave;
    GgMeshVertex a=ggSettlementVertex(p,axis,f.u0,f.v0,z0);
    GgMeshVertex c=ggSettlementVertex(p,axis,f.u1,f.v0,z0);
    GgMeshVertex d=ggSettlementVertex(p,axis,f.u1,f.v1,z1);
    GgMeshVertex e=ggSettlementVertex(p,axis,f.u0,f.v1,z1);
    GgMeshVertex l=ggSettlementVertex(p,axis,f.u0+b->hip,f.ridge,b->z1);
    GgMeshVertex h=ggSettlementVertex(p,axis,f.u1-b->hip,f.ridge,b->z1);
    if(f.ridge>f.v0)ggSettlementRoofQuad(m,axis,a,c,h,l,r,g,bl);
    if(f.ridge<f.v1)ggSettlementRoofQuad(m,axis,l,h,d,e,r,g,bl);
    if(b->hip){
        ggSettlementRoofTriangle(m,axis,a,l,e,r,g,bl);
        ggSettlementRoofTriangle(m,axis,c,d,h,r,g,bl);
    }
    GgMeshVertex A=ggSettlementLower(a,open?2:a.z-b->z0);
    GgMeshVertex C=ggSettlementLower(c,open?2:c.z-b->z0);
    GgMeshVertex D=ggSettlementLower(d,open?2:d.z-b->z0);
    GgMeshVertex E=ggSettlementLower(e,open?2:e.z-b->z0);
    ggSettlementRoofQuad(m,axis,A,C,c,a,edgeR,edgeG,edgeB);
    ggSettlementRoofQuad(m,axis,D,E,e,d,edgeR,edgeG,edgeB);
    int u=axis?m->cy-p->oy:m->cx-p->ox;
    GgMeshVertex closest=ggMeshV(m->cosine>=0?p->ox+b->x0:p->ox+b->x1,
                                 m->sine>=0?p->oy+b->y0:p->oy+b->y1,b->z0);
    int clipped=ggMeshCameraPoint(m,closest).f<=m->near;
    if(open){
        GgMeshVertex L=ggSettlementLower(l,2),H=ggSettlementLower(h,2);
        if(m->eye<=b->z1||clipped){
            if(f.ridge>f.v0)ggSettlementRoofQuad(m,axis,L,H,C,A,edgeR,edgeG,edgeB);
            if(f.ridge<f.v1)ggSettlementRoofQuad(m,axis,E,D,H,L,edgeR,edgeG,edgeB);
        }
        if(u<=f.u0||clipped){
            if(f.ridge>f.v0)ggSettlementRoofQuad(m,axis,A,a,l,L,edgeR,edgeG,edgeB);
            if(f.ridge<f.v1)ggSettlementRoofQuad(m,axis,L,l,e,E,edgeR,edgeG,edgeB);
        }
        if(u>=f.u1||clipped){
            if(f.ridge>f.v0)ggSettlementRoofQuad(m,axis,H,h,c,C,edgeR,edgeG,edgeB);
            if(f.ridge<f.v1)ggSettlementRoofQuad(m,axis,D,d,h,H,edgeR,edgeG,edgeB);
        }
        return;
    }
    // Fascia and gables close the loft. The same footprint and slope
    // describe the roof in the combat cover query.
    m->material=GG_MAT_WOOD;
    a.z=c.z=d.z=e.z=f.eave;
    if(u<=f.u0||clipped){
        ggSettlementRoofQuad(m,axis,E,A,a,e,edgeR,edgeG,edgeB);
        if(!b->hip)ggSettlementGable(m,axis,a,l,e,edgeR,edgeG,edgeB);
    }
    if(u>=f.u1||clipped){
        ggSettlementRoofQuad(m,axis,C,D,d,c,edgeR,edgeG,edgeB);
        if(!b->hip)ggSettlementGable(m,axis,d,h,c,edgeR,edgeG,edgeB);
    }
    if(m->eye<=b->z0||clipped)ggSettlementRoofQuad(m,axis,E,D,C,A,edgeR,edgeG,edgeB);
}
static int ggSettlementRoofZ(const GgSettlementPart*b,int x,int y){
    GgSettlementRoofFrame f=ggSettlementRoofFrame(b);
    int u=b->axis?y:x,v=b->axis?x:y,span,offset;
    if(v==f.ridge){span=1;offset=1;}
    else if(v<f.ridge){span=f.ridge-f.v0;offset=v-f.v0;}
    else{span=f.v1-f.ridge;offset=f.v1-v;}
    int rise=(b->z1-f.eave)*256;
    int top=rise*offset/span;
    if(b->hip){
        int end=u-f.u0<f.u1-u?u-f.u0:f.u1-u;
        if(end<b->hip){
            int hip=rise*end/b->hip;
            if(hip<top)top=hip;
        }
    }
    return f.eave*256+top;
}
static void ggSettlementGlazing(GgMeshContext*m,int axis,int plane,int facing,
                                int center,int half,int low,int high,int nearDetail){
    int a=center-half,b=center+half,glass=plane-facing*2;
    ggMeshQuad(m,ggMeshFacadePoint(axis,plane,a-4,low-1),
                 ggMeshFacadePoint(axis,plane,b+4,low-1),
                 ggMeshFacadePoint(axis,plane,b+4,high+1),
                 ggMeshFacadePoint(axis,plane,a-4,high+1),91,76,61);
    ggMeshQuad(m,ggMeshFacadePoint(axis,glass,a,low),
                 ggMeshFacadePoint(axis,glass,b,low),
                 ggMeshFacadePoint(axis,glass,b,high),
                 ggMeshFacadePoint(axis,glass,a,high),83,122,133);
    if(nearDetail){
        int mullion=glass-facing;
        ggMeshQuad(m,ggMeshFacadePoint(axis,mullion,center-2,low),
                     ggMeshFacadePoint(axis,mullion,center+2,low),
                     ggMeshFacadePoint(axis,mullion,center+2,high),
                     ggMeshFacadePoint(axis,mullion,center-2,high),180,146,103);
        ggMeshQuad(m,ggMeshFacadePoint(axis,mullion,a-6,low-1),
                     ggMeshFacadePoint(axis,mullion,b+6,low-1),
                     ggMeshFacadePoint(axis,mullion,b+6,low),
                     ggMeshFacadePoint(axis,mullion,a-6,low),191,170,137);
    }
}
static void ggSettlementFacade(GgMeshContext*m,const GgSettlementPlan*p,int build,
                               int nearDetail){
    if(!p->hasDoor)return;
    int prior=m->material,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    int x0=p->ox+p->x0,x1=p->ox+p->x1,y0=p->oy+p->y0,y1=p->oy+p->y1;
    int cx=(x0+x1)/2,cy=(y0+y1)/2;
    int g=p->ground,lower=g+9,upper=lower+7;
    int slit=build==B_BARRACKS?6:build==B_HUT?10:14;
    if(m->cx<x0){
        ggMeshDoorway(m,x0,0,1,cy,(y1-y0)/2,p->floor);
        for(int side=-1;side<=1;side+=2){
            int y=cy+side*(y1-y0)/3;
            ggSettlementGlazing(m,0,x0-14,1,y,slit,lower,upper,nearDetail);
        }
        if(p->upper){
            int plane=x0-(build==B_INN?19:14);
            for(int i=-1;i<=1;i++)
                ggSettlementGlazing(m,0,plane,1,cy+i*46,slit-3,
                                    p->upper+2,p->upper+8,nearDetail);
        }
    }
    if(m->cx>x1)
        for(int side=-1;side<=1;side+=2)
            ggSettlementGlazing(m,0,x1+2,-1,cy+side*(y1-y0)/4,
                                slit,lower,upper,nearDetail);
    int facing=m->cy<y0?1:m->cy>y1?-1:0;
    if(facing){
        int plane=facing>0?y0-2:y1+2;
        for(int side=-1;side<=1;side+=2)
            ggSettlementGlazing(m,1,plane,facing,cx+side*(x1-x0)/4,
                                slit,lower,upper,nearDetail);
        if(p->upper){
            if(build==B_INN)plane=facing>0?p->oy+35:p->oy+223;
            if(build==B_HOUSE)plane=facing>0?p->oy+34:p->oy+222;
            for(int side=-1;side<=1;side+=2)
                ggSettlementGlazing(m,1,plane,facing,cx+side*(x1-x0)/4,
                                    slit-3,p->upper+2,p->upper+8,nearDetail);
        }
    }
    m->material=prior;m->depthBias=bias;
}
static int ggMeshSettlementDraw(GgMeshContext*m,int tx,int ty,int build,int ground,
                                int nearDetail){
    if(!ggMeshSettlementHandles(build))return 0;
    GgSettlementPlan p;
    ggMeshSettlementPlan(&p,tx,ty,build,ground);
    int prior=m->material,footing=ggSettlementFootingBottom(&p);
    for(int i=0;i<p.count;i++){
        const GgSettlementPart *b=&p.part[i];
        const unsigned char *ink=ggSettlementColors[b->color];
        m->material=ink[3];
        if(b->type>=GG_SETTLE_ROOF)ggSettlementDrawRoof(m,&p,b,ink);
        else{
            int x0=p.ox+b->x0,y0=p.oy+b->y0,x1=p.ox+b->x1,y1=p.oy+b->y1;
            int bottom=b->type==GG_SETTLE_FOOT?footing:b->z0;
            ggMeshArchitectureBlock(m,x0,y0,x1,y1,bottom,b->z1,ink[0],ink[1],ink[2]);
        }
    }
    ggSettlementFacade(m,&p,build,nearDetail);
    m->material=prior;
    return 1;
}
static int ggMeshSettlementSolid(int tx,int ty,int build,int x,int y,int z,int ground){
    if(!ggMeshSettlementHandles(build))return -1;
    int u=x-tx*256,v=y-ty*256;
    if((unsigned)u>=256||(unsigned)v>=256||z>ground+112)return 0;
    GgSettlementPlan p;
    ggMeshSettlementPlan(&p,tx,ty,build,ground);
    for(int i=0;i<p.count;i++){
        const GgSettlementPart *b=&p.part[i];
        if(u<b->x0||u>b->x1||v<b->y0||v>b->y1||z>b->z1)continue;
        if(b->type==GG_SETTLE_FOOT){
            if(z>=b->z0||z>=ggSettlementFootingBottom(&p))return 1;
        }else if(z>=b->z0){
            if(b->type==GG_SETTLE_BLOCK)return 1;
            int top=ggSettlementRoofZ(b,u,v);
            if(z*256<=top&&(b->type==GG_SETTLE_ROOF||z*256>=top-512))return 1;
        }
    }
    return 0;
}

#endif

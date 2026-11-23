// Real mesh-based GodGuy world. Each visible tile contributes physical
// 3D ground triangles, architecture and low-poly vegetation to the same
// z-tested perspective camera. No second simulation, texture atlas,
// allocation, full RGB buffer or arbitrary floating 2D scenery stickers.
// Old 3DS draws 200x120 polygons at 2x; New 3DS cinematic draws 400x240.
// Requires explore3d.h's world, ground, trig, palette and scene definitions.
#ifndef GG_MESH_WORLD3D_H
#define GG_MESH_WORLD3D_H
static int ggMeshTilesDrawn,ggMeshBuildingsDrawn,ggMeshPropsDrawn;
static inline int ggMeshGroundCenter(int type){
    return ggExpGround(type<=T_WATER?T_WATER:type);
}
static inline int ggMeshCornerHeight(int x,int y){
    int sum=0,n=0;
    for(int oy=-1;oy<=0;oy++)
        for(int ox=-1;ox<=0;ox++){
            if((unsigned)(x+ox)<WW&&(unsigned)(y+oy)<WH){
                int type=ggMeshTerrainAt(x+ox,y+oy);
                // All water shares a level. Adjacent dry cells meet it
                // at the same corner, so the bank rises out of the water.
                if(type<=T_WATER)return ggMeshGroundCenter(type);
                sum+=ggMeshGroundCenter(type);n++;
            }
        }
    return n?sum/n:8;
}
static inline int ggMeshGroundCross(GgMeshVertex a,GgMeshVertex b,GgMeshVertex p){
    return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);
}

enum { GG_GROUND_PLANE, GG_GROUND_SHOULDER, GG_GROUND_SHELF };
static const unsigned char ggMeshGroundFaceCount[]={1,3,5};
static const unsigned char ggMeshGroundFaces[][5][3]={
    {{0,1,2}},
    {{0,1,3},{1,2,3},{2,0,3}},
    {{0,1,3},{0,3,4},{0,4,2},{1,2,4},{1,4,3}}
};
static const unsigned char ggMeshGroundCornerX[4]={0,1,1,0};
static const unsigned char ggMeshGroundCornerY[4]={0,0,1,1};
static const unsigned char ggMeshGroundSide[4]={2,1,3,0};

static inline GgMeshVertex ggMeshGroundCorner(int tx,int ty,int corner){
    int x=tx+ggMeshGroundCornerX[corner],y=ty+ggMeshGroundCornerY[corner];
    return ggMeshV(x*256,y*256,ggMeshCornerHeight(x,y));
}
// Detail stays strictly inside the old centre-to-edge triangle, so any
// flat, cut-bank or mountain sector can meet its neighbours without seams.
// Point zero is the cell centre and points one and two are shared corners.
static inline int ggMeshGroundDetail(int tx,int ty,int sector,int type,
                                      GgMeshVertex point[5]){
    if(type<=T_WATER)return GG_GROUND_PLANE;
    int z=point[0].z,a=point[1].z,b=point[2].z;
    int delta=z-(a+b)/2;
    int side=ggMeshGroundSide[sector];
    int shelf=a==ggExpGround(T_WATER)&&b==a&&type!=T_LAVA&&
        ggMeshTerrainAt(tx+ggOutdoorSideX[side],ty+ggOutdoorSideY[side])<=T_WATER;
    if(!shelf&&delta<12)return GG_GROUND_PLANE;
    int dx=(point[2].x-point[1].x)/256,dy=(point[2].y-point[1].y)/256;
    uint32_t key=ggFilmScenicHash(tx,ty,113+sector);
    if(shelf){
        for(int i=0;i<2;i++){
            int inset=20+(int)((key>>(i*9))&15u);
            int along=48+(int)((key>>(i*9+4))&31u);
            if(i)along=256-along;
            point[3+i]=ggMeshV(point[1].x+dx*along-dy*inset,
                               point[1].y+dy*along+dx*inset,a);
        }
        return GG_GROUND_SHELF;
    }
    // Break the grade near the toe so the upper facets stay visible
    // when viewed from the foot of a mountain or raised bank.
    int inset=40,along=112+(int)(key&31u);
    int shoulder=((a+b)*(128-inset)+2*z*inset+128)/256+
                 ggMeshClamp(delta/4,0,6);
    point[3]=ggMeshV(point[1].x+dx*along-dy*inset,
                     point[1].y+dy*along+dx*inset,shoulder);
    return GG_GROUND_SHOULDER;
}

// Contacts and camera sample the actual rendered planes, including the
// bank's submerged shelf and the gentler transition onto a steep summit.
static inline int ggMeshGroundAtQ8(int wx,int wy){
    int tx=wx>>8,ty=wy>>8;
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 8;
    int type=ggMeshTerrainAt(tx,ty);
    int z=ggMeshGroundCenter(type);
    if(type<=T_WATER)return z;
    int u=(wx&255)-128,v=(wy&255)-128;
    int au=u<0?-u:u,av=v<0?-v:v;
    int sector=au>=av?(u>=0?1:3):(v>=0?2:0);
    GgMeshVertex point[5]={ggMeshV(tx*256+128,ty*256+128,z),
        ggMeshGroundCorner(tx,ty,sector),ggMeshGroundCorner(tx,ty,(sector+1)&3)};
    if(point[1].z==z&&point[2].z==z)return z;
    int detail=ggMeshGroundDetail(tx,ty,sector,type,point);
    if(detail==GG_GROUND_PLANE){
        int dist=au>=av?au:av;
        int along=sector==0?u:sector==1?v:sector==2?-u:-v;
        return (z*(128-dist)*2+point[1].z*(dist-along)+
                point[2].z*(dist+along)+128)/256;
    }
    GgMeshVertex p=ggMeshV(wx,wy,0);
    for(int i=0;i<ggMeshGroundFaceCount[detail];i++){
        const unsigned char *face=ggMeshGroundFaces[detail][i];
        GgMeshVertex a=point[face[0]],b=point[face[1]],c=point[face[2]];
        int wa=ggMeshGroundCross(b,c,p),wb=ggMeshGroundCross(c,a,p);
        int wc=ggMeshGroundCross(a,b,p);
        if(wa>=0&&wb>=0&&wc>=0){
            int area=wa+wb+wc;
            return (a.z*wa+b.z*wb+c.z*wc+area/2)/area;
        }
    }
    return z;
}
// Every rendered terrain triangle stays at or above the lowest tile
// height contributing to its shared corners. Use that bound for buried
// skirts so even a recessed shelf between footing corners seals the wall.
static inline int ggMeshFoundationGround(int x0,int y0,int x1,int y1,
                                          int nominal){
    int base=nominal;
    int left=x0>>8,right=x1>>8,top=y0>>8,bottom=y1>>8;
    if((left<0||top<0||right>=WW||bottom>=WH)&&base>8)base=8;
    left=ggMeshClamp(left-1,0,WW-1);right=ggMeshClamp(right+1,0,WW-1);
    top=ggMeshClamp(top-1,0,WH-1);bottom=ggMeshClamp(bottom+1,0,WH-1);
    for(int y=top;y<=bottom;y++)for(int x=left;x<=right;x++){
        int z=ggMeshGroundCenter(ggMeshTerrainAt(x,y));
        if(z<base)base=z;
    }
    return base-1;
}
static void ggMeshBox(GgMeshContext*m,int x0,int y0,int x1,int y1,
                       int bottom,int top,int r,int g,int b){
    if(x1<=x0||y1<=y0||top<=bottom)return;
    GgMeshVertex a=ggMeshV(x0,y0,bottom),c=ggMeshV(x1,y0,bottom),
        d=ggMeshV(x1,y1,bottom),e=ggMeshV(x0,y1,bottom);
    GgMeshVertex A=ggMeshV(x0,y0,top),C=ggMeshV(x1,y0,top),
        D=ggMeshV(x1,y1,top),E=ggMeshV(x0,y1,top);
    ggMeshQuad(m,a,c,C,A,r*9/10,g*9/10,b*9/10);
    ggMeshQuad(m,c,d,D,C,r*7/10,g*7/10,b*7/10);
    ggMeshQuad(m,d,e,E,D,r*8/10,g*8/10,b*8/10);
    ggMeshQuad(m,e,a,A,E,r,g,b);
    ggMeshQuad(m,A,C,D,E,r*11/10,g*11/10,b*11/10);
}
static void ggMeshGroundBox(GgMeshContext*m,int x0,int y0,int x1,int y1,
                             int top,int r,int g,int b){
    int bottom=ggMeshFoundationGround(x0,y0,x1,y1,top);
    ggMeshBox(m,x0,y0,x1,y1,bottom,top,r,g,b);
}
static inline int ggMeshShade(int c,int light){
    return ggMeshClamp(c*light/256,0,255);
}
// Small world-space plants, cobbles and weathering instead of a
// monolithic triangle carpet. Only nearby native-detail tiles get these
// facets; distant / Old 3DS tiles retain the original four triangles.
static void ggMeshGroundLife(GgMeshContext*m,int tx,int ty,int type,
                             uint32_t key){
    if(type!=T_GRASS&&type!=T_HILL&&type!=T_FOREST&&
       type!=T_JUNGLE&&type!=T_SWAMP)return;
    if((key&3u)==0u)return;
    int x=tx*256+74+(int)((key>>4)&95u);
    int y=ty*256+74+(int)((key>>13)&95u);
    int tall=type==T_JUNGLE||type==T_SWAMP;
    int flower=(type==T_GRASS||type==T_HILL)&&((key>>19)&3u)==0u;
    int sway=ggFilmScenicWind(key,ggExplore.frame)*3;
    for(int i=0;i<5;i++){
        int a=(int)((key>>24)&255u)+i*51;
        int dx=ggExpCos[a&255]/170,dy=ggExpSin[a&255]/170;
        int sx=x+dx,sy=y+dy;
        int z=ggMeshGroundAtQ8(sx,sy);
        int height=1+tall+(int)((key>>(i*3+3))&1u);
        GgMeshVertex tip=ggMeshV(sx+sway+dx/3,sy+dy/3,z+height);
        ggMeshTriangle(m,ggMeshV(sx-4,sy-4,ggMeshGroundAtQ8(sx-4,sy-4)-1),
            ggMeshV(sx+4,sy+3,ggMeshGroundAtQ8(sx+4,sy+3)-1),
            tip,
            109-18*tall+i*4,145-10*tall+i*2,66+i*3);
        if(flower&&i<3){
            ggMeshTriangle(m,tip,
                ggMeshV(tip.x-3,tip.y-3,tip.z+1),
                ggMeshV(tip.x+4,tip.y+2,tip.z+1),
                (key&4u)?225:199,(key&4u)?182:165,(key&4u)?100:179);
        }
    }
}
static GgMeshVertex ggMeshShorePoint(int tx,int ty,int side,
                                     int along,int inset){
    int x=tx*256+(side<2?(side==0?inset:255-inset):along);
    int y=ty*256+(side>=2?(side==2?inset:255-inset):along);
    return ggMeshV(x,y,ggMeshGroundCenter(T_WATER));
}
// Shallower water and broken crests both sit on the level water plane.
static void ggMeshShore(GgMeshContext*m,int tx,int ty,int side,uint32_t key){
    int material=m->material,opacity=m->opacity,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    int surge=1+ggMatWave(ggExplore.frame/5,15);
    m->opacity=62;
    ggMeshQuad(m,ggMeshShorePoint(tx,ty,side,0,0),
        ggMeshShorePoint(tx,ty,side,255,0),
        ggMeshShorePoint(tx,ty,side,255,36),
        ggMeshShorePoint(tx,ty,side,0,36),93,169,153);
    m->opacity=91;
    for(int piece=0;piece<2;piece++){
        int a=16+piece*116+(int)((key>>(piece*5))&15u);
        int b=a+52+(int)((key>>(piece*6+10))&47u);
        int inner=5+surge+piece*7;
        ggMeshQuad(m,ggMeshShorePoint(tx,ty,side,a,inner),
            ggMeshShorePoint(tx,ty,side,b,inner+4),
            ggMeshShorePoint(tx,ty,side,b,inner+9),
            ggMeshShorePoint(tx,ty,side,a,inner+5),190,224,213);
    }
    m->material=material;m->opacity=opacity;m->depthBias=bias;
}
static void ggMeshGroundTile(GgMeshContext*m,int tx,int ty,
                              int type,uint32_t key){
    int prior=m->material,priorSeed=m->materialSeed,priorUV=m->groundUV;
    int priorNear=m->near;
    // A steep walkable bank can rise almost to eye height before the
    // normal geometry near plane. Ground still has to cover the view
    // below it; keep the original clip distance for props and actors.
    m->near=24;
    int paved=ggOutdoorPaved(type,bld[ty][tx]);
    m->material=paved?GG_MAT_PAVING:ggMeshGroundMaterial(type);
    // One material field per biome; using the cell hash here made
    // neighbouring meadow tiles visibly reset their colour and grain.
    m->materialSeed=(int)ggOutdoorGroundSeed(paved?T_DIRT:type,ggExplore.frame);
    m->groundUV=1;
    GgMeshVertex center=ggMeshV(tx*256+128,ty*256+128,ggMeshGroundCenter(type));
    GgMeshVertex corners[4];
    for(int i=0;i<4;i++)corners[i]=ggMeshGroundCorner(tx,ty,i);
    int r,g,bl;
    if(paved)ggOutdoorPavingColor(&r,&g,&bl);
    else ggOutdoorGroundColor(type,&r,&g,&bl);
    for(int sector=0;sector<4;sector++){
        GgMeshVertex point[5]={center,corners[sector],corners[(sector+1)&3]};
        int detail=ggMeshGroundDetail(tx,ty,sector,type,point);
        int dry=detail==GG_GROUND_SHELF?3:ggMeshGroundFaceCount[detail];
        for(int i=0;i<dry;i++){
            const unsigned char *face=ggMeshGroundFaces[detail][i];
            ggMeshTriangle(m,point[face[0]],point[face[1]],point[face[2]],r,g,bl);
        }
        if(detail==GG_GROUND_SHELF){
            int land=m->material,seed=m->materialSeed;
            m->material=GG_MAT_WATER;
            m->materialSeed=(int)ggOutdoorGroundSeed(T_WATER,ggExplore.frame);
            for(int i=dry;i<ggMeshGroundFaceCount[detail];i++){
                const unsigned char *face=ggMeshGroundFaces[detail][i];
                ggMeshTriangle(m,point[face[0]],point[face[1]],point[face[2]],58,124,124);
            }
            m->material=land;m->materialSeed=seed;
        }
    }
    if(type<=T_WATER){
        unsigned shore=ggOutdoorShoreSides(tx,ty,1);
        for(int side=0;shore;side++,shore>>=1)
            if(shore&1u)ggMeshShore(m,tx,ty,side,key);
    }
    m->material=prior;m->materialSeed=priorSeed;m->groundUV=priorUV;
    m->near=priorNear;
    ggMeshTilesDrawn++;
}
typedef struct { int left,right,hip,count,y[5],z[5]; } GgMeshRoofProfile;
static inline GgMeshRoofProfile ggMeshRoofProfile(int build,int x0,int y0,
                                                 int x1,int y1,int wallTop){
    int overhang=build==B_TEMPLE?17:14;
    int hip=build==B_HALL?50:build==B_LIBRARY?32:0;
    int rise=build==B_HOUSE?19:build==B_HUT?16:build==B_FARM?21:
              build==B_HALL?12:build==B_LIBRARY?13:9;
    GgMeshRoofProfile p={x0-overhang,x1+overhang,hip,3,
        {y0-overhang,(y0+y1)/2,y1+overhang},
        {wallTop-2,wallTop+rise,wallTop-2}};
    if(build==B_FARM){
        // The steep lower pitch gives the barn a full-height loft.
        p.count=5;
        p.y[2]=p.y[1];p.y[1]=y0+24;
        p.y[3]=y1-24;p.y[4]=y1+overhang;
        p.z[2]=p.z[1];p.z[1]=p.z[3]=wallTop+16;p.z[4]=p.z[0];
    }
    return p;
}
static inline int ggMeshHasRoof(int build){
    return build==B_HOUSE||build==B_HUT||build==B_FARM||
           build==B_HALL||build==B_TEMPLE||build==B_LIBRARY;
}
static void ggMeshRoof(GgMeshContext*m,int build,
                       int x0,int y0,int x1,int y1,int wallTop){
    int prior=m->material;
    GgMeshRoofProfile p=ggMeshRoofProfile(build,x0,y0,x1,y1,wallTop);
    int hip=p.hip,count=p.count,left=p.left,right=p.right;
    const int *y=p.y,*z=p.z;
    int rise=z[count/2]-wallTop;
    int r=164,g=85,b=57;
    if(build==B_FARM){r=120;g=67;b=54;}
    if(build==B_TEMPLE){r=109;g=130;b=107;}
    if(build==B_HALL){r=110;g=119;b=91;}
    if(build==B_LIBRARY){r=78;g=105;b=123;}
    m->material=GG_MAT_ROOF;
    for(int i=0;i<count-1;i++){
        int a=hip&&i==1?hip:0,bias=hip&&i==0?hip:0;
        GgMeshVertex a0=ggMeshV(left+a,y[i],z[i]);
        GgMeshVertex a1=ggMeshV(right-a,y[i],z[i]);
        GgMeshVertex b0=ggMeshV(left+bias,y[i+1],z[i+1]);
        GgMeshVertex b1=ggMeshV(right-bias,y[i+1],z[i+1]);
        ggMeshQuad(m,a0,a1,b1,b0,r,g,b);
        if(!hip){
            // The timber fascia closes the visible thickness along
            // each verge. The pale gable sits inside that overhang.
            GgMeshVertex aBottom=a0,bBottom=b0;
            aBottom.z-=3;bBottom.z-=3;
            GgMeshVertex aInner=aBottom,bInner=bBottom;
            aInner.x=x0;bInner.x=x0;
            if(m->cx<=x0){
                ggMeshQuad(m,aBottom,a0,b0,bBottom,93,61,45);
                ggMeshQuad(m,aBottom,bBottom,bInner,aInner,85,64,49);
            }
            aBottom.x=a1.x;bBottom.x=b1.x;
            aInner.x=x1;bInner.x=x1;
            if(m->cx>=x1){
                ggMeshQuad(m,aBottom,bBottom,b1,a1,93,61,45);
                ggMeshQuad(m,aInner,bInner,bBottom,aBottom,85,64,49);
            }
            a0.x=b0.x=x0;
            aBottom.x=bBottom.x=x0;
            aBottom.z=bBottom.z=wallTop-3;
            if(m->cx<=x0)ggMeshQuad(m,aBottom,a0,b0,bBottom,172,148,113);
            a0.x=b0.x=x1;
            aBottom.x=bBottom.x=x1;
            if(m->cx>=x1)ggMeshQuad(m,aBottom,bBottom,b0,a0,172,148,113);
        }
    }
    if(hip){
        ggMeshTriangle(m,ggMeshV(left,y[0],z[0]),
                       ggMeshV(left+hip,y[1],z[1]),
                       ggMeshV(left,y[2],z[2]),r,g,b);
        ggMeshTriangle(m,ggMeshV(right,y[0],z[0]),
                       ggMeshV(right,y[2],z[2]),
                       ggMeshV(right-hip,y[1],z[1]),r,g,b);
        ggMeshQuad(m,ggMeshV(left,y[0],z[0]-3),ggMeshV(left,y[0],z[0]),
                     ggMeshV(left,y[2],z[0]),ggMeshV(left,y[2],z[0]-3),96,66,48);
        ggMeshQuad(m,ggMeshV(right,y[2],z[0]-3),ggMeshV(right,y[2],z[0]),
                     ggMeshV(right,y[0],z[0]),ggMeshV(right,y[0],z[0]-3),96,66,48);
    }
    m->material=GG_MAT_WOOD;
    ggMeshQuad(m,ggMeshV(left,y[0],z[0]-3),ggMeshV(right,y[0],z[0]-3),
                 ggMeshV(right,y[0],z[0]),ggMeshV(left,y[0],z[0]),96,66,48);
    ggMeshQuad(m,ggMeshV(right,y[count-1],z[0]-3),ggMeshV(left,y[count-1],z[0]-3),
                 ggMeshV(left,y[count-1],z[0]),ggMeshV(right,y[count-1],z[0]),96,66,48);
    ggMeshQuad(m,ggMeshV(right,y[0],z[0]-3),ggMeshV(left,y[0],z[0]-3),
                 ggMeshV(left,y[count-1],z[0]-3),ggMeshV(right,y[count-1],z[0]-3),79,59,48);
    int ridge=y[count/2];
    ggMeshBox(m,left+hip,ridge-4,right-hip,ridge+4,wallTop+rise-1,
              wallTop+rise+1,r*3/4,g*3/4,b*3/4);
    m->material=prior;
}
enum { GG_MESH_DOOR_OFFSET=9 };
static inline int ggMeshDoorHalfWidth(int half){ return half>=80?27:22; }
static int ggMeshDoorFloor(int plane,int alongX,int facing,int start,int end){
    int center=(start+end)/2,width=ggMeshDoorHalfWidth((end-start)/2);
    plane-=facing*GG_MESH_DOOR_OFFSET;
    // The sill is entirely on one straight sector of the terrain fan.
    // Anchor it at the uphill end so the ground cannot bury the opening.
    int left=ggMeshGroundAtQ8(alongX?center-width:plane,
                             alongX?plane:center-width);
    int right=ggMeshGroundAtQ8(alongX?center+width:plane,
                              alongX?plane:center+width);
    return left>right?left:right;
}
static void ggMeshDoorway(GgMeshContext*m,int plane,int alongX,int facing,
                           int middle,int half,int base){
    int width=ggMeshDoorHalfWidth(half);
    // Thick jambs and a lintel surround the recessed door. The opening
    // uses the same floor, depth and height as the walking residents.
    int panel=plane-facing*GG_MESH_DOOR_OFFSET;
    int front=panel-facing*6,a=middle-width,b=middle+width;
    int top=base+GG_WORLD_DOOR_HEIGHT;
    GgMeshVertex al=ggMeshV(alongX?a:panel,alongX?panel:a,base);
    GgMeshVertex bl=ggMeshV(alongX?b:panel,alongX?panel:b,base);
    GgMeshVertex at=al,bt=bl;at.z=bt.z=top;
    GgMeshVertex afl=al,bfl=bl,aft=at,bft=bt;
    if(alongX)afl.y=bfl.y=aft.y=bft.y=front;
    else afl.x=bfl.x=aft.x=bft.x=front;
    ggMeshQuad(m,al,bl,bt,at,alongX?42:46,alongX?45:51,alongX?53:58);
    ggMeshQuad(m,al,afl,aft,at,91,70,52);
    ggMeshQuad(m,bfl,bl,bt,bft,106,80,58);
    ggMeshQuad(m,aft,bft,bt,at,82,61,48);
    GgMeshVertex outsideLeft=afl,outsideRight=bfl;
    if(alongX){outsideLeft.x-=5;outsideRight.x+=5;}
    else{outsideLeft.y-=5;outsideRight.y+=5;}
    GgMeshVertex upperLeft=outsideLeft,upperRight=outsideRight;
    GgMeshVertex lintelLeft=aft,lintelRight=bft;
    upperLeft.z=lintelLeft.z=upperRight.z=lintelRight.z=top+2;
    ggMeshQuad(m,outsideLeft,afl,lintelLeft,upperLeft,120,89,62);
    ggMeshQuad(m,bfl,outsideRight,upperRight,lintelRight,120,89,62);
    ggMeshQuad(m,aft,bft,lintelRight,lintelLeft,133,102,70);
}
static void ggMeshWindow(GgMeshContext*m,int x0,int x1,
                          int frontY,int facing,int ground,int height){
    if(height<GG_WORLD_DOOR_HEIGHT+2)return;
    int oldBias=m->depthBias,oldMat=m->material;
    m->depthBias=1;m->material=GG_MAT_NONE;
    int y=frontY-facing*3;
    int winBot=ground+9+(height>=35)*2,winTop=winBot+7;
    int cx=(x0+x1)/2;
    int half=(x1-x0)/2;
    int glassHalf=half>=90?20:10;
    ggMeshDoorway(m,frontY,1,facing,cx,half,ground);
    for(int side=-1;side<=1;side+=2){
        int center=cx+side*half*2/3;
        int a=center-glassHalf,b=center+glassHalf;
        ggMeshQuad(m,ggMeshV(a-4,y,winBot-1),
                   ggMeshV(b+4,y,winBot-1),
                   ggMeshV(b+4,y,winTop+1),
                   ggMeshV(a-4,y,winTop+1),63,67,75);
        int paneY=y-facing*2;
        ggMeshQuad(m,ggMeshV(a,paneY,winBot),
                   ggMeshV(b,paneY,winBot),
                   ggMeshV(b,paneY,winTop),
                   ggMeshV(a,paneY,winTop),91,132,143);
        ggMeshQuad(m,ggMeshV(center-2,paneY-facing,winBot),
                   ggMeshV(center+2,paneY-facing,winBot),
                   ggMeshV(center+2,paneY-facing,winTop),
                   ggMeshV(center-2,paneY-facing,winTop),180,161,124);
        ggMeshQuad(m,ggMeshV(a-5,paneY-facing*2,winBot-1),
                   ggMeshV(b+5,paneY-facing*2,winBot-1),
                   ggMeshV(b+5,paneY-facing*2,winBot),
                   ggMeshV(a-5,paneY-facing*2,winBot),196,181,151);
    }
    m->depthBias=oldBias;m->material=oldMat;
}
static void ggMeshSideFacade(GgMeshContext*m,int wallX,int facing,
                             int y0,int y1,int ground,int height){
    // A physically rotated version of the front glazing, with its
    // own x-plane. The original y-only windows left the wall directly
    // facing the default eastward camera completely blank.
    if(height<GG_WORLD_DOOR_HEIGHT+2)return;
    int oldBias=m->depthBias,oldMat=m->material;
    m->depthBias=1;m->material=GG_MAT_NONE;
    int face=wallX-facing*3,pane=face-facing*3;
    int cy=(y0+y1)/2;
    int half=(y1-y0)/2,glassHalf=half>=90?19:10;
    int low=ground+9+(height>=35)*2,high=low+7;
    ggMeshDoorway(m,wallX,0,facing,cy,half,ground);
    for(int n=-1;n<=1;n+=2){
        int q=cy+n*(y1-y0)/3;
        int a=q-glassHalf,b=q+glassHalf;
        ggMeshQuad(m,ggMeshV(face,a-4,low-1),
            ggMeshV(face,b+4,low-1),
            ggMeshV(face,b+4,high+1),
            ggMeshV(face,a-4,high+1),80,77,74);
        ggMeshQuad(m,ggMeshV(pane,a,low),
            ggMeshV(pane,b,low),
            ggMeshV(pane,b,high),
            ggMeshV(pane,a,high),90,132,145);
        ggMeshQuad(m,ggMeshV(pane-facing,q-2,low),
            ggMeshV(pane-facing,q+2,low),
            ggMeshV(pane-facing,q+2,high),
            ggMeshV(pane-facing,q-2,high),181,162,126);
        ggMeshQuad(m,ggMeshV(pane-facing*2,a-5,low-1),
            ggMeshV(pane-facing*2,b+5,low-1),
            ggMeshV(pane-facing*2,b+5,low),
            ggMeshV(pane-facing*2,a-5,low),197,182,151);
    }
    m->depthBias=oldBias;m->material=oldMat;
}
// Geometric masonry/wood courses use real world height and wall
// normals, so highlights, seams, and framing stay planted during
// camera turns. No UV atlas, screen-space noise or extra framebuffer.
static void ggMeshWallFinish(GgMeshContext*m,int x0,int y0,
                             int x1,int y1,int ground,int height,
                             int wood){
    int spacing=wood?6:5;
    int lower=ground+GG_WORLD_DOOR_HEIGHT+2,upper=ground+height-3;
    for(int z=lower;z<upper;z+=spacing){
        int r=wood?104:126,g=wood?80:124,b=wood?63:117;
        if((z/spacing)&1){r+=10;g+=9;b+=8;}
        ggMeshQuad(m,ggMeshV(x0+9,y0-4,z),
            ggMeshV(x1-9,y0-4,z),
            ggMeshV(x1-9,y0-4,z+1),
            ggMeshV(x0+9,y0-4,z+1),r,g,b);
        ggMeshQuad(m,ggMeshV(x0+9,y1+4,z),
            ggMeshV(x1-9,y1+4,z),
            ggMeshV(x1-9,y1+4,z+1),
            ggMeshV(x0+9,y1+4,z+1),r,g,b);
        ggMeshQuad(m,ggMeshV(x0-4,y0+9,z),
            ggMeshV(x0-4,y1-9,z),
            ggMeshV(x0-4,y1-9,z+1),
            ggMeshV(x0-4,y0+9,z+1),r,g,b);
        ggMeshQuad(m,ggMeshV(x1+4,y0+9,z),
            ggMeshV(x1+4,y1-9,z),
            ggMeshV(x1+4,y1-9,z+1),
            ggMeshV(x1+4,y0+9,z+1),r,g,b);
    }
}
// A second storey has its own window row, and timbered houses carry
// diagonal braces. Only outward faces visible from this camera receive
// these details; adding architecture does not quadruple hidden work.
static void ggMeshFacadeDetail(GgMeshContext*m,int plane,int axis,
                                int u0,int u1,int ground,int height,
                                int wood){
    int prior=m->material,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    for(int side=-1;side<=1;side+=2){
        int u=(u0+u1)/2+side*(u1-u0)/3;
        int low=ground+height-11,high=low+5;
        if(height>=30){
            GgMeshVertex a=ggMeshV(axis?u-17:plane,axis?plane:u-17,low);
            GgMeshVertex b=ggMeshV(axis?u+17:plane,axis?plane:u+17,low);
            GgMeshVertex c=b,d=a;c.z=d.z=high;
            ggMeshQuad(m,a,b,c,d,65,95,108);
            a.z=b.z=low-1;c.z=d.z=low;
            ggMeshQuad(m,a,b,c,d,192,177,150);
        }
        if(wood&&height>=18){
            int lo=ground+2,hi=ground+height/3;
            int aa=u-28,bb=u+28;
            GgMeshVertex a=ggMeshV(axis?aa:plane,axis?plane:aa,lo);
            GgMeshVertex b=ggMeshV(axis?aa+8:plane,axis?plane:aa+8,lo);
            GgMeshVertex c=ggMeshV(axis?bb:plane,axis?plane:bb,hi);
            GgMeshVertex d=ggMeshV(axis?bb-8:plane,axis?plane:bb-8,hi);
            ggMeshQuad(m,a,b,c,d,99,76,56);
        }
    }
    m->material=prior;m->depthBias=bias;
}
static void ggMeshBuildingLanding(GgMeshContext*m,int x0,int y0,
                                   int y1){
    // Short physical flagstone approach at the actual entrance. The
    // landing samples the SAME sloping world-surface triangle plane
    // rather than floating at the nominal tile center height.
    int centerY=(y0+y1)/2;
    for(int tile=0;tile<4;tile++){
        int x=x0-13-tile*20;
        int half=23+((tile&1)?3:0);
        int top=centerY-half,bottom=centerY+half;
        GgMeshVertex a=ggMeshV(x,top,ggMeshGroundAtQ8(x,top)+1);
        GgMeshVertex b=ggMeshV(x+16,top,
                                ggMeshGroundAtQ8(x+16,top)+1);
        GgMeshVertex cc=ggMeshV(x+16,bottom,
                                 ggMeshGroundAtQ8(x+16,bottom)+1);
        GgMeshVertex d=ggMeshV(x,bottom,
                                ggMeshGroundAtQ8(x,bottom)+1);
        ggMeshQuad(m,a,b,cc,d,tile&1?133:146,
                   tile&1?128:136,tile&1?116:121);
    }
}
// Clip a shadow footprint to one of the actual rendered ground faces.
// Allow room for quantized 2D intersections.
static void ggMeshShadowSector(GgMeshContext*m,const GgMeshVertex edge[6],
                                 const GgMeshVertex ground[3]){
    GgMeshVertex from[20],to[20];
    memcpy(from,edge,6*sizeof *from);
    int count=6;
    for(int i=0;i<3&&count>=3;i++){
        GgMeshVertex a=ground[i],b=ground[(i+1)%3];
        GgMeshVertex prev=from[count-1];
        int p=ggMeshGroundCross(a,b,prev),used=0;
        for(int j=0;j<count;j++){
            GgMeshVertex v=from[j];
            int c=ggMeshGroundCross(a,b,v);
            if((p<0&&c>0)||(p>0&&c<0)){
                int den=p-c;
                to[used++]=ggMeshV(prev.x+(v.x-prev.x)*p/den,
                                    prev.y+(v.y-prev.y)*p/den,0);
            }
            if(c>=0)to[used++]=v;
            prev=v;p=c;
        }
        count=used;
        memcpy(from,to,(size_t)count*sizeof *from);
    }
    int area=ggMeshGroundCross(ground[0],ground[1],ground[2]);
    if(count<3||area<=0)return;
    for(int i=0;i<count;i++){
        int a=ggMeshGroundCross(ground[1],ground[2],from[i]);
        int b=ggMeshGroundCross(ground[2],ground[0],from[i]);
        from[i].z=(ground[0].z*a+ground[1].z*b+
                   ground[2].z*(area-a-b)+area/2)/area;
    }
    for(int i=1;i+1<count;i++)
        ggMeshTriangle(m,from[0],from[i],from[i+1],29,36,44);
}
static void ggMeshShadowDrape(GgMeshContext*m,const GgMeshVertex edge[6],
                              int left,int right,int top,int bottom){
    for(int ty=top;ty<=bottom;ty++)for(int tx=left;tx<=right;tx++){
        if((unsigned)tx>=WW||(unsigned)ty>=WH)continue;
        int type=ggMeshTerrainAt(tx,ty);
        GgMeshVertex center=ggMeshV(tx*256+128,ty*256+128,ggMeshGroundCenter(type));
        GgMeshVertex corner[4];
        for(int i=0;i<4;i++)corner[i]=ggMeshGroundCorner(tx,ty,i);
        for(int sector=0;sector<4;sector++){
            GgMeshVertex point[5]={center,corner[sector],corner[(sector+1)&3]};
            int detail=ggMeshGroundDetail(tx,ty,sector,type,point);
            for(int i=0;i<ggMeshGroundFaceCount[detail];i++){
                const unsigned char *face=ggMeshGroundFaces[detail][i];
                GgMeshVertex triangle[3]={point[face[0]],point[face[1]],point[face[2]]};
                ggMeshShadowSector(m,edge,triangle);
            }
        }
    }
}
static int ggMeshShadowFlat(int left,int right,int top,int bottom,int z){
    if(left<0||right>=WW||top<0||bottom>=WH)return 0;
    // Include every cell contributing to the corners inside this patch.
    // Six edge samples can all round to z while missing a shallow dip.
    for(int y=top-1;y<=bottom+1;y++)for(int x=left-1;x<=right+1;x++){
        if((unsigned)x>=WW||(unsigned)y>=WH)continue;
        if(ggMeshGroundCenter(ggMeshTerrainAt(x,y))!=z)return 0;
    }
    return 1;
}
// Dark contact stays at the foot. The softer outer patch trails away
// from the light. Both sample the terrain and leave its depth intact.
static void ggMeshGroundShadows(GgMeshContext*m,int cx,int cy,
                                 int radius,int footRadius,int height){
    if(m->scale>1||footRadius<10)return;
    int keepMat=m->material,keepOpacity=m->opacity,keepBias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=2;
    int sun=m->daylight.cover>=0?m->daylight.sun:218;
    for(int ring=height?0:1;ring<2;ring++){
        int cast=ring?0:height*2;
        int acrossRadius=ring?footRadius:radius;
        int wide=acrossRadius+(ring?0:cast+12);
        int centerX=cx+cast,centerY=cy-cast*3/5;
        int z=ggMeshGroundCenter(ggMeshTerrainAt(centerX>>8,centerY>>8));
        GgMeshVertex center=ggMeshV(centerX,centerY,z);
        GgMeshVertex edge[6];
        int left=centerX,right=centerX,top=centerY,bottom=centerY;
        m->opacity=ring?74:(sun+40)/8;
        for(int n=0;n<6;n++){
            int a=n*256/6;
            int along=ggExpCos[a]*wide/4096;
            int across=ggExpSin[a]*(acrossRadius*3/4+3)/4096;
            int x=centerX+along+across/2;
            int y=centerY+across-along*3/5;
            edge[n]=ggMeshV(x,y,z);
            if(x<left)left=x;if(x>right)right=x;
            if(y<top)top=y;if(y>bottom)bottom=y;
        }
        left>>=8;right>>=8;top>>=8;bottom>>=8;
        if(ggMeshShadowFlat(left,right,top,bottom,z)){
            for(int n=0;n<6;n++)
                ggMeshTriangle(m,center,edge[n],edge[(n+1)%6],29,36,44);
        }else ggMeshShadowDrape(m,edge,left,right,top,bottom);
    }
    m->opacity=keepOpacity;m->material=keepMat;m->depthBias=keepBias;
}
static void ggMeshContactShadow(GgMeshContext*m,int cx,int cy,
                                 int radius,int height){
    ggMeshGroundShadows(m,cx,cy,radius,radius,height);
}
#include "mesh_architecture3d.h"
static void ggMeshDockBox(GgMeshContext*m,int x,int y,int dx,int dy,
                          int p0,int p1,int u0,int u1,int z0,int z1,
                          int r,int g,int b){
    int a=x+p0*dx+u0*dy,bx=x+p1*dx+u1*dy;
    int c=y+p0*dy+u0*dx,d=y+p1*dy+u1*dx;
    ggMeshArchitectureBlock(m,a<bx?a:bx,c<d?c:d,a>bx?a:bx,c>d?c:d,
                            z0,z1,r,g,b);
}
static void ggMeshDock(GgMeshContext*m,int tx,int ty,int ground,int detailed){
    unsigned shores=ggOutdoorShoreSides(tx,ty,0);
    int side=2;
    if(shores){
        side=0;
        while(!(shores&(1u<<side)))side++;
    }
    int dx=ggOutdoorSideX[side],dy=ggOutdoorSideY[side];
    int x=tx*256+128,y=ty*256+128;
    int end=shores?205:82,deck=ground+6,start=-83,join=end-66;
    int width=shores?96:71;
    int material=m->material;
    m->material=GG_MAT_WOOD;
    for(int row=0;row<4;row++)for(int edge=-1;edge<=1;edge+=2){
        int along=row<2?start+19+(join-start-28)*row:end-49+35*(row-2);
        int across=edge*(row<2?33:width-14);
        int px=x+along*dx+across*dy,py=y+along*dy+across*dx;
        int top=deck+(row>=2?4:0);
        ggMeshArchitectureSupport(m,px-8,py-8,px+8,py+8,top,114,85,58);
        if(detailed&&row>=2)
            ggMeshArchitectureBlock(m,px-10,py-10,px+10,py+10,top-1,top+1,166,127,84);
    }
    ggMeshDockBox(m,x,y,dx,dy,start,join,-49,49,deck-4,deck,160,119,78);
    ggMeshDockBox(m,x,y,dx,dy,join,end,-width,width,deck-4,deck,174,132,87);
    if(detailed){
        int priorBias=m->depthBias;
        for(int edge=-1;edge<=1;edge+=2){
            int u=edge*31;
            ggMeshDockBox(m,x,y,dx,dy,start,join,u-5,u+5,
                          deck-6,deck-4,105,78,58);
        }
        m->material=GG_MAT_NONE;m->depthBias=1;
        for(int along=start+14;along<end-7;along+=22){
            int px=x+along*dx,py=y+along*dy;
            int half=(along>=join?width:49)-2;
            ggMeshQuad(m,ggMeshV(px-half*dy,py-half*dx,deck),
                ggMeshV(px+half*dy,py+half*dx,deck),
                ggMeshV(px+half*dy+2*dx,py+half*dx+2*dy,deck),
                ggMeshV(px-half*dy+2*dx,py-half*dx+2*dy,deck),105,78,58);
        }
        m->depthBias=priorBias;
    }
    m->material=material;
}
enum { GG_MESH_BUILDING_MARGIN=28 };
typedef struct { int x0,y0,x1,y1; } GgMeshBuildingFootprint;
static inline GgMeshBuildingFootprint ggMeshBuildingFootprint(int tx,int ty,int build){
    int margin=GG_MESH_BUILDING_MARGIN;
    GgMeshBuildingFootprint p={tx*256+margin,ty*256+margin,
                              (tx+1)*256-margin,(ty+1)*256-margin};
    // Building type changes actual footprint/volume, not merely a
    // pixel tint: small huts, strong towers, open ruins and temples
    // remain distinguishable even when viewed with flat materials.
    if(build==B_HUT){p.x0+=35;p.x1-=35;p.y0+=30;p.y1-=30;}
    if(build==B_TOWER){p.x0+=39;p.x1-=39;p.y0+=38;p.y1-=38;}
    if(build==B_FARM){p.y0+=33;p.y1-=35;}
    if(build==B_STATUE){p.x0+=42;p.x1-=42;p.y0+=38;p.y1-=38;}
    // Broaden street fronts along the entrance plane. The hall's north
    // porch and the temple's west porch keep their column and aisle positions.
    if(build==B_HALL){p.x0-=10;p.x1+=10;}
    if(build==B_TEMPLE||build==B_LIBRARY){p.y0-=10;p.y1+=10;}
    return p;
}
static inline int ggMeshBuildingFoundation(GgMeshBuildingFootprint p,int ground){
    return ggMeshFoundationGround(p.x0-14,p.y0-14,p.x1+14,p.y1+14,ground);
}
static inline int ggMeshBuildingWallTop(GgMeshBuildingFootprint p,int tx,int ty,
                                       int build,int ground,int doorFloor[4]){
    int x0=p.x0,y0=p.y0,x1=p.x1,y1=p.y1;
    int wallTop=ground+ggExpBuilding(build)+(build==B_FARM?6:0);
    for(int i=0;i<4;i++)doorFloor[i]=ground;
    if(wallTop-ground>=GG_WORLD_DOOR_HEIGHT+2&&build!=B_STATUE&&
       !ggMeshShadowFlat(tx,tx,ty,ty,ground)){
        doorFloor[0]=ggMeshDoorFloor(x0,0,1,y0,y1);
        doorFloor[1]=ggMeshDoorFloor(x1,0,-1,y0,y1);
        doorFloor[2]=ggMeshDoorFloor(y0,1,1,x0,x1);
        doorFloor[3]=ggMeshDoorFloor(y1,1,-1,x0,x1);
        for(int side=0;side<4;side++){
            int top=doorFloor[side]+GG_WORLD_DOOR_HEIGHT+3;
            if(top>wallTop)wallTop=top;
        }
    }
    return wallTop;
}
#include "mesh_settlement3d.h"
#include "mesh_fort3d.h"
#include "mesh_residential_lots3d.h"

enum { GG_WELL_OUTER=62, GG_WELL_OUTER_CUT=34,
       GG_WELL_SHAFT=54, GG_WELL_SHAFT_CUT=30,
       GG_WELL_INNER=38, GG_WELL_INNER_CUT=22, GG_WELL_COPING_RISE=2 };

static inline int ggMeshWellRing(int x,int y,int radius,int cut){
    int ax=abs((x&255)-128),ay=abs((y&255)-128);
    int axial=ax>ay?ax:ay,diagonal=ax+ay;
    return axial<=radius&&diagonal<=radius+cut&&
           (axial>=GG_WELL_INNER||diagonal>=GG_WELL_INNER+GG_WELL_INNER_CUT);
}

static inline int ggMeshWellFoundation(int x,int y,int ground){
    return ggMeshFoundationGround(x-GG_WELL_OUTER,y-GG_WELL_OUTER,
                                  x+GG_WELL_OUTER,y+GG_WELL_OUTER,ground);
}

static int ggMeshWellSolid(int x,int y,int z,int ground){
    int top=ground+GG_WORLD_WELL_RIM;
    if(z>top)return 0;
    int lift=ggMeshClamp(z-top+GG_WELL_COPING_RISE,0,GG_WELL_COPING_RISE);
    int radius=GG_WELL_SHAFT+(GG_WELL_OUTER-GG_WELL_SHAFT)*lift/GG_WELL_COPING_RISE;
    int cut=GG_WELL_SHAFT_CUT+(GG_WELL_OUTER_CUT-GG_WELL_SHAFT_CUT)*lift/GG_WELL_COPING_RISE;
    if(!ggMeshWellRing(x,y,radius,cut))return 0;
    int cx=(x&~255)+128,cy=(y&~255)+128;
    return z>=ground||z>=ggMeshWellFoundation(cx,cy,ground);
}

static inline GgMeshVertex ggMeshWellVertex(int side,int x,int y,int z,
                                          int radius,int cut){
    static const signed char ring[8][2]={
        {-1,-2},{1,-2},{2,-1},{2,1},{1,2},{-1,2},{-2,1},{-2,-1}
    };
    int shift[5]={-radius,-cut,0,cut,radius};
    return ggMeshV(x+shift[ring[side][0]+2],y+shift[ring[side][1]+2],z);
}

static void ggMeshWell(GgMeshContext*m,int tx,int ty,int ground,int detailed){
    int x=tx*256+128,y=ty*256+128,top=ground+GG_WORLD_WELL_RIM;
    int bottom=ggMeshWellFoundation(x,y,ground),basin=ground+4;
    int neck=top-GG_WELL_COPING_RISE;
    int prior=m->material,seed=m->materialSeed;
    // Eight coping segments, outer and inner masonry, and a dark basin.
    // Both detail levels use the same 72 triangles and cover silhouette.
    for(int side=0;side<8;side++){
        int next=(side+1)&7;
        GgMeshVertex a=ggMeshWellVertex(side,x,y,top,GG_WELL_OUTER,GG_WELL_OUTER_CUT);
        GgMeshVertex b=ggMeshWellVertex(next,x,y,top,GG_WELL_OUTER,GG_WELL_OUTER_CUT);
        GgMeshVertex c=ggMeshWellVertex(next,x,y,top,GG_WELL_INNER,GG_WELL_INNER_CUT);
        GgMeshVertex d=ggMeshWellVertex(side,x,y,top,GG_WELL_INNER,GG_WELL_INNER_CUT);
        GgMeshVertex e=ggMeshWellVertex(side,x,y,neck,GG_WELL_SHAFT,GG_WELL_SHAFT_CUT);
        GgMeshVertex f=ggMeshWellVertex(next,x,y,neck,GG_WELL_SHAFT,GG_WELL_SHAFT_CUT);
        int variation=detailed?(side&1)*9:0;
        m->material=GG_MAT_STONE;m->materialSeed=seed;
        ggMeshQuad(m,ggMeshV(e.x,e.y,bottom),ggMeshV(f.x,f.y,bottom),f,e,122,127,128);
        ggMeshQuad(m,e,f,b,a,173+variation,161+variation,139+variation);
        ggMeshQuad(m,ggMeshV(c.x,c.y,basin),ggMeshV(d.x,d.y,basin),d,c,77,83,81);
        ggMeshQuad(m,a,b,c,d,191+variation,187+variation,167+variation);
        m->material=GG_MAT_WATER;m->materialSeed=ggExplore.frame/6;
        ggMeshTriangle(m,ggMeshV(x,y,basin),ggMeshV(d.x,d.y,basin),
                       ggMeshV(c.x,c.y,basin),40,89,93);
    }
    m->material=prior;m->materialSeed=seed;
}

static void ggMeshBuilding(GgMeshContext*m,int tx,int ty,int build,
                            int ground,int nearDetail){
    if(build<=B_NONE||build>=B_N||ggBuildingIsPaving(build))return;
    if(ggMeshLotDraw(m,tx,ty,build,ground,nearDetail)||
       ggMeshSettlementDraw(m,tx,ty,build,ground,nearDetail)||
       ggMeshFortDraw(m,tx,ty,build,ground,nearDetail)){
        ggMeshBuildingsDrawn++;
        return;
    }
    if(build==B_BORDER){
        ggMeshBorderMarker(m,tx*256+128,ty*256+128,ground);
        ggMeshBuildingsDrawn++;
        return;
    }
    if(build==B_DOCK){
        ggMeshDock(m,tx,ty,ground,nearDetail);
        ggMeshBuildingsDrawn++;
        return;
    }
    if(build==B_WELL){
        ggMeshWell(m,tx,ty,ground,nearDetail);
        ggMeshBuildingsDrawn++;
        return;
    }
    GgMeshBuildingFootprint bounds=ggMeshBuildingFootprint(tx,ty,build);
    int x0=bounds.x0,y0=bounds.y0,x1=bounds.x1,y1=bounds.y1;
    int baseGround=ggMeshBuildingFoundation(bounds,ground);
    int doorFloor[4];
    int wallTop=ggMeshBuildingWallTop(bounds,tx,ty,build,ground,doorFloor);
    int wood=build==B_HOUSE||build==B_HUT||build==B_FARM;
    int stone=build==B_TEMPLE||build==B_HALL||
              build==B_TOWER||build==B_LIBRARY;
    int r=wood?193:stone?181:126,g=wood?177:stone?177:128,
        bl=wood?143:stone?162:117;
    if(build==B_HUT){r=171;g=143;bl=104;}
    if(build==B_TEMPLE){r=209;g=197;bl=174;}
    if(build==B_LIBRARY){r=173;g=187;bl=174;}
    if(build==B_FARM){r=164;g=122;bl=87;}
    int cx=(x0+x1)/2,cy=(y0+y1)/2;
    if(build==B_STATUE){
        ggMeshStatue(m,cx,cy,baseGround,ground);
        ggMeshBuildingsDrawn++;return;
    }
    if(build==B_RUIN){
        ggMeshRuin(m,x0,y0,x1,y1,ground,wallTop,baseGround,nearDetail);
        ggMeshBuildingsDrawn++;return;
    }
    ggMeshArchitectureBlock(m,x0,y0,x1,y1,baseGround,wallTop,r,g,bl);
    if(build==B_TOWER)ggMeshWatchtower(m,x0,y0,x1,y1,wallTop,nearDetail);
    if(ggMeshHasRoof(build))
        ggMeshRoof(m,build,x0,y0,x1,y1,wallTop);
    if(build==B_HOUSE||build==B_HUT)ggMeshChimney(m,x0,y0,x1,y1,wallTop,build);
    if(build==B_TEMPLE)ggMeshPortico(m,0,x0,cy,doorFloor[0],wallTop-8,1);
    if(build==B_HALL)ggMeshPortico(m,1,y0,cx,doorFloor[2],wallTop-12,0);
    int h=wallTop-ground;
    // Even the medium distance silhouette keeps its entrances and
    // windows. The old LOD switched whole buildings into blank cubes.
    if(build!=B_FARM){
        if(m->cy<=y0)ggMeshWindow(m,x0,x1,y0,1,doorFloor[2],wallTop-doorFloor[2]);
        if(m->cy>=y1)ggMeshWindow(m,x0,x1,y1,-1,doorFloor[3],wallTop-doorFloor[3]);
        if(m->cx<=x0)ggMeshSideFacade(m,x0,1,y0,y1,doorFloor[0],wallTop-doorFloor[0]);
        if(m->cx>=x1)ggMeshSideFacade(m,x1,-1,y0,y1,doorFloor[1],wallTop-doorFloor[1]);
    }else{
        if(m->cx<=x0)ggMeshBarnGate(m,0,x0,1,cy,doorFloor[0]);
        if(m->cx>=x1)ggMeshBarnGate(m,0,x1,-1,cy,doorFloor[1]);
        if(m->cy<=y0)ggMeshBarnGate(m,1,y0,1,cx,doorFloor[2]);
        if(m->cy>=y1)ggMeshBarnGate(m,1,y1,-1,cx,doorFloor[3]);
    }
    if(nearDetail){
        // Ground-contact plinth and roof cornice extend beyond every
        // wall face, showing real parallax, height and shade.
        ggMeshArchitectureBlock(m,x0-5,y0-5,x1+5,y1+5,
                  baseGround,ground+2,107,105,98);
        ggMeshArchitectureBlock(m,x0-6,y0-6,x1+6,y1+6,
                  wallTop-3,wallTop-1,177,164,137);
        if(build==B_HALL||build==B_LIBRARY)
            ggMeshWallFinish(m,x0,y0,x1,y1,ground,h,wood);
        if(m->cy<=y0)ggMeshFacadeDetail(m,y0-7,1,x0,x1,ground,h,wood);
        if(m->cy>=y1)ggMeshFacadeDetail(m,y1+7,1,x0,x1,ground,h,wood);
        if(m->cx<=x0)ggMeshFacadeDetail(m,x0-7,0,y0,y1,ground,h,wood);
        if(m->cx>=x1)ggMeshFacadeDetail(m,x1+7,0,y0,y1,ground,h,wood);
        if(build==B_HOUSE||build==B_HUT||build==B_LIBRARY)
            ggMeshBuildingLanding(m,x0,y0,y1);
        // Vertical corner quoins or timber beams connect floor to
        // eaves and keep every view from looking like a plain cube.
        int trimR=wood?102:181,trimG=wood?77:170,trimB=wood?58:149;
        ggMeshArchitectureBlock(m,x0-7,y0-7,x0+10,y0+10,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshArchitectureBlock(m,x1-10,y0-7,x1+7,y0+10,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshArchitectureBlock(m,x0-7,y1-10,x0+10,y1+7,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshArchitectureBlock(m,x1-10,y1-10,x1+7,y1+7,baseGround,wallTop,
                  trimR,trimG,trimB);
        if(build==B_FARM){
            ggMeshArchitectureSupport(m,x0-28,y1-28,x0-4,y1-4,ground+4,129,95,59);
            ggMeshArchitectureSupport(m,x0-23,y1-21,x0-8,y1-9,ground+7,144,105,67);
        }
    }
    ggMeshBuildingsDrawn++;
}
// Two irregular latitude rings give crowns a rounded, leafy silhouette.
// At native detail the oak has three overlapping crowns; distant trees
// keep one six-sided volume. The fixed prop queue still caps total work.
static void ggMeshLeafCrown(GgMeshContext*m,int x,int y,int base,
                             int radius,int height,uint32_t key,
                             int sides,int r,int g,int b){
    int lower=base+height/3,upper=base+height*2/3;
    int rotation=(int)((key>>24)&255u);
    for(int n=0;n<sides;n++){
        int next=(n+1)%sides;
        int a=(rotation+n*256/sides)&255,c=(rotation+next*256/sides)&255;
        int ra=radius*(83+(int)((key>>(n*4))&31u))/100;
        int rb=radius*(83+(int)((key>>(next*4))&31u))/100;
        int ax=x+ggExpCos[a]*ra/4096,ay=y+ggExpSin[a]*ra/4096;
        int bx=x+ggExpCos[c]*rb/4096,by=y+ggExpSin[c]*rb/4096;
        int stepA=(int)((key>>(n*3))&3u),stepB=(int)((key>>(next*3))&3u);
        GgMeshVertex A=ggMeshV(ax,ay,lower+stepA);
        GgMeshVertex B=ggMeshV(bx,by,lower+stepB);
        GgMeshVertex C=ggMeshV(x+(bx-x)*3/4,y+(by-y)*3/4,upper+stepB/2);
        GgMeshVertex D=ggMeshV(x+(ax-x)*3/4,y+(ay-y)*3/4,upper+stepA/2);
        ggMeshTriangle(m,ggMeshV(x+radius/9,y-radius/10,base+height),
                        D,C,r+12+stepA*2,g+15+stepA*2,b+4+stepA);
        ggMeshQuad(m,A,B,C,D,r+stepA*2,g+stepA*3,b+stepA);
        ggMeshTriangle(m,B,A,ggMeshV(x,y,base+height/8),
                        r-9,g-5,b-2);
    }
}
static void ggMeshBough(GgMeshContext*m,int x,int y,int z,int top,
                        int radius,int rise,int sway,uint32_t key,
                        int r,int g,int b){
    int turn=(int)((key>>18)&255u);
    for(int i=0;i<6;i++){
        int next=(i+1)%6;
        int a=(turn+i*256/6)&255,c=(turn+next*256/6)&255;
        int sa=radius*(84+(int)((key>>(i*4))&31u))/100;
        int sb=radius*(84+(int)((key>>(next*4))&31u))/100;
        GgMeshVertex u=ggMeshV(x+ggExpCos[a]*sa/4096,
                              y+ggExpSin[a]*sa/4096,z+(i&1)*rise);
        GgMeshVertex v=ggMeshV(x+ggExpCos[c]*sb/4096,
                              y+ggExpSin[c]*sb/4096,z+(next&1)*rise);
        ggMeshTriangle(m,u,v,ggMeshV(x+sway,y-5,top),
                        r+(i&1)*6,g+(i&1)*8,b+(i&1)*3);
        ggMeshTriangle(m,v,u,ggMeshV(x,y,z-2),r-6,g-6,b-2);
    }
}
static void ggMeshTrunk(GgMeshContext*m,int x,int y,int z,
                        int dx,int dy,int height,int width,int curved){
    GgMeshVertex foot[5],bend[5],crown[5];
    for(int i=0;i<5;i++){
        int a=i*256/5;
        int wx=ggExpCos[a]*width/4096,wy=ggExpSin[a]*width/4096;
        int rootX=x+wx+wx/3,rootY=y+wy+wy/3;
        foot[i]=ggMeshV(rootX,rootY,ggMeshGroundAtQ8(rootX,rootY)-1);
        crown[i]=ggMeshV(x+dx+wx/2,y+dy+wy/2,z+height);
        if(curved)bend[i]=ggMeshV(x+dx*2/3+wx*3/4,
                                   y+dy*2/3+wy*3/4,z+height/2);
    }
    for(int i=0;i<5;i++){
        int next=(i+1)%5;
        if(curved){
            ggMeshQuad(m,foot[i],foot[next],bend[next],bend[i],111,85,60);
            ggMeshQuad(m,bend[i],bend[next],crown[next],crown[i],123,96,65);
        }else ggMeshQuad(m,foot[i],foot[next],crown[next],crown[i],113,84,58);
    }
}
static void ggMeshTreeRoots(GgMeshContext*m,int x,int y,int ground,
                            int width,int turn){
    for(int i=0;i<3;i++){
        int a=(turn+i*85)&255;
        int dx=ggExpCos[a],dy=ggExpSin[a];
        int span=width*2+4+(i&1)*5;
        int tipX=x+dx*span/4096,tipY=y+dy*span/4096;
        int leftX=x-dy*width/6144,leftY=y+dx*width/6144;
        int rightX=x+dy*width/6144,rightY=y-dx*width/6144;
        GgMeshVertex left=ggMeshV(leftX,leftY,ggMeshGroundAtQ8(leftX,leftY)-1);
        GgMeshVertex right=ggMeshV(rightX,rightY,ggMeshGroundAtQ8(rightX,rightY)-1);
        GgMeshVertex tip=ggMeshV(tipX,tipY,ggMeshGroundAtQ8(tipX,tipY)-1);
        GgMeshVertex shoulder=ggMeshV(x+dx*width/6144,y+dy*width/6144,ground+5);
        ggMeshTriangle(m,right,tip,shoulder,105,80,53);
        ggMeshTriangle(m,tip,left,shoulder,122,90,59);
    }
}
static void ggMeshBranch(GgMeshContext*m,int x,int y,int z,
                         int tipX,int tipY,int tipZ){
    int dx=tipX-x,dy=tipY-y;
    int span=abs(dx)+abs(dy);
    if(!span)return;
    int sideX=-dy*7/span,sideY=dx*7/span;
    GgMeshVertex a=ggMeshV(x+sideX,y+sideY,z);
    GgMeshVertex b=ggMeshV(x-sideX,y-sideY,z);
    GgMeshVertex c=ggMeshV(tipX,tipY,tipZ);
    ggMeshTriangle(m,a,b,c,119,91,60);
    ggMeshTriangle(m,b,ggMeshV(x,y,z-1),c,91,69,49);
    ggMeshTriangle(m,ggMeshV(x,y,z-1),a,c,101,77,52);
}
static void ggMeshPalmCrown(GgMeshContext*m,int x,int y,int z,int sway,
                            uint32_t key,int quality){
    int count=quality>=2?7:5;
    int turn=(int)((key>>17)&255u);
    GgMeshVertex stem=ggMeshV(x,y,z-1);
    for(int i=0;i<count;i++){
        int a=(turn+i*256/count)&255;
        int dx=ggExpCos[a],dy=ggExpSin[a];
        int span=75+(int)((key>>(i*4))&23u);
        int bx=x+sway/2+dx*span/8192,by=y+dy*span/8192;
        int width=10+(i%3)*3;
        GgMeshVertex right=ggMeshV(bx+dy*width/4096,by-dx*width/4096,z+1);
        GgMeshVertex left=ggMeshV(bx-dy*width/4096,by+dx*width/4096,z+1);
        GgMeshVertex bend=ggMeshV(bx,by,z+5+(i&1)*2);
        GgMeshVertex tip=ggMeshV(x+sway+dx*span/4096,y+dy*span/4096,
                                 z-4-(i%3)*2);
        ggMeshTriangle(m,stem,right,bend,52,113,62);
        ggMeshTriangle(m,bend,right,tip,60,126,69);
        ggMeshTriangle(m,stem,bend,left,70,132,70);
        ggMeshTriangle(m,bend,tip,left,76,141,74);
    }
    ggMeshLeafCrown(m,x,y,z-5,19,12,key,5,65,116,65);
}
static void ggMeshFoliage(GgMeshContext*m,int x,int y,int ground,
                           int form,uint32_t key,int quality){
    int pine=form==1,tropical=form==2;
    int keep=m->material;
    int age=(int)((key>>5)&7u),turn=(int)((key>>9)&255u);
    int lean=tropical?18+age:4+age;
    int leanX=ggExpCos[turn]*lean/4096,leanY=ggExpSin[turn]*lean/4096;
    int trunkH=(pine?16:tropical?36:23)+age/2;
    int width=(tropical?8:11)+age/2;
    m->material=GG_MAT_BARK;
    ggMeshTrunk(m,x,y,ground,leanX,leanY,trunkH,width,tropical&&quality>=2);
    if(quality>=2)ggMeshTreeRoots(m,x,y,ground,width,turn);
    m->material=GG_MAT_LEAVES;
    int sway=ggFilmScenicWind(key,ggExplore.frame)*3;
    int hue=(int)((key>>15)&7u)-3;
    int cx=x+leanX,cy=y+leanY;
    if(pine){
        for(int tier=0;tier<3;tier++){
            int low=ground+12+tier*(8+age/2);
            ggMeshBough(m,cx+sway*tier/3,cy,low,low+15+age/2,
                76+age*2-tier*19,quality>=2?3+tier:1,sway,
                key+(uint32_t)tier*37u,
                48+hue+tier*3,86+hue+tier*5,72+hue+tier*3);
        }
    }else if(tropical){
        ggMeshPalmCrown(m,cx,cy,ground+trunkH,sway,key,quality);
    }else{
        int limb=43+age*3;
        int spread=ggExpCos[turn]*limb/4096;
        int stand=ggExpSin[turn]*limb/4096;
        int base=ground+trunkH-5;
        int r=86+hue,g=123+hue,b=53+hue;
        ggMeshLeafCrown(m,cx+sway,cy,base,70+age*2,16+age,
                        key,6,r,g,b);
        if(quality>=2){
            m->material=GG_MAT_BARK;
            ggMeshBranch(m,x+leanX/2,y+leanY/2,ground+trunkH/2,
                         cx-spread+sway,cy-stand,base+4);
            ggMeshBranch(m,x+leanX/2,y+leanY/2,ground+trunkH/2+2,
                         cx+stand+sway,cy-spread,base+8);
            m->material=GG_MAT_LEAVES;
            ggMeshLeafCrown(m,cx-spread+sway,cy-stand,base-2,
                            53+age,13+age/2,key+733u,5,r-10,g-9,b+4);
            ggMeshLeafCrown(m,cx+stand+sway,cy-spread,base+4,
                            45+age*2,12+age/2,key+1471u,5,r+15,g+11,b+6);
        }
    }
    m->material=keep;
}
static void ggMeshShrub(GgMeshContext*m,int x,int y,int ground,
                          uint32_t key,int quality){
    int previous=m->material;
    int turn=(int)((key>>24)&255u),height=8+(int)((key>>7)&3u);
    int dx=ggExpCos[turn]*22/4096,dy=ggExpSin[turn]*22/4096;
    m->material=GG_MAT_BARK;
    ggMeshGroundBox(m,x-5,y-5,x+5,y+5,ground+height/2+2,92,74,49);
    int a=ground,b=ground;
    if(quality>=2){
        a=ggMeshGroundAtQ8(x+dx,y+dy);
        b=ggMeshGroundAtQ8(x-dx,y-dy);
        ggMeshBranch(m,x,y,ground+1,x+dx,y+dy,a+3);
        ggMeshBranch(m,x,y,ground+1,x-dx,y-dy,b+4);
    }
    m->material=GG_MAT_LEAVES;
    ggMeshLeafCrown(m,x,y,ground-2,39,height,key,5,69,107,56);
    if(quality>=2){
        ggMeshLeafCrown(m,x+dx,y+dy,a-2,27,height-3,
                        key+691u,5,81,119,59);
        ggMeshLeafCrown(m,x-dx,y-dy,b-2,24,height-2,
                        key+137u,5,63,97,55);
    }
    if((key&3u)==0u){
        m->material=GG_MAT_NONE;
        ggMeshTriangle(m,ggMeshV(x-7,y-4,ground+height-2),
            ggMeshV(x+4,y-3,ground+height-2),
            ggMeshV(x,y+7,ground+height-1),191,169,102);
    }
    m->material=previous;
}
static void ggMeshRock(GgMeshContext*m,int x,int y,int z,int type,
                       uint32_t key,int quality){
    int crag=(type==T_MOUNT||type==T_HILL)&&(key&7u)<3u;
    int slab=type==T_SAND||type==T_DESERT;
    int height=crag?14+(int)((key>>14)&7u):5+(int)((key>>18)&3u);
    int radius=31+(int)((key>>14)&15u);
    int rotation=(int)((key>>22)&255u);
    int r=slab?143:123,g=slab?121:124,b=slab?84:119;
    int rim=quality>=2?6:5;
    int split=rim/2;
    int dx=ggExpCos[(rotation+64)&255]*radius/15360;
    int dy=ggExpSin[(rotation+64)&255]*radius/15360;
    GgMeshVertex crest[2]={ggMeshV(x+dx,y+dy,z+height),
        ggMeshV(x-dx,y-dy,z+height-(crag?3:1))};
    GgMeshVertex foot[6],shoulder[6];
    for(int i=0;i<rim;i++){
        int a=(rotation+i*256/rim)&255;
        int u=radius+(int)((key>>(i*4))&7u);
        int ux=ggExpCos[a]*u/4096,uy=ggExpSin[a]*u*(slab?2:3)/(4096*4);
        int base=ggMeshGroundAtQ8(x+ux,y+uy)-1;
        int ledge=height*(slab?4:3)/5-(crag?i%3:0);
        foot[i]=ggMeshV(x+ux,y+uy,base<z-2?base:z-2);
        shoulder[i]=ggMeshV(x+ux*(slab?5:4)/6,y+uy*(slab?5:4)/6,z+ledge);
    }
    for(int i=0;i<rim;i++){
        int n=(i+1)%rim;
        int tone=(int)((key>>(i*4))&7u)-3;
        ggMeshQuad(m,foot[i],foot[n],shoulder[n],shoulder[i],
                   r-18+tone,g-14+tone,b-12+tone);
        ggMeshTriangle(m,shoulder[i],shoulder[n],crest[i>=split],r+tone,g+tone,b+tone);
    }
    ggMeshTriangle(m,shoulder[0],crest[0],crest[1],r,g,b);
    ggMeshTriangle(m,shoulder[split],crest[1],crest[0],r,g,b);
}
static void ggMeshFence(GgMeshContext*m,int x,int y){
    int left=x-54,right=x+54;
    int low=ggMeshGroundAtQ8(left,y),high=ggMeshGroundAtQ8(right,y);
    ggMeshGroundBox(m,left-6,y-6,left+6,y+6,low+10,116,84,58);
    ggMeshGroundBox(m,right-6,y-6,right+6,y+6,high+10,116,84,58);
    for(int rail=5;rail<=8;rail+=3){
        GgMeshVertex a=ggMeshV(left,y-4,low+rail-1);
        GgMeshVertex b=ggMeshV(right,y-4,high+rail-1);
        GgMeshVertex c=ggMeshV(right,y-4,high+rail);
        GgMeshVertex d=ggMeshV(left,y-4,low+rail);
        GgMeshVertex backC=ggMeshV(right,y+4,high+rail);
        GgMeshVertex backD=ggMeshV(left,y+4,low+rail);
        ggMeshQuad(m,a,b,c,d,144,107,73);
        ggMeshQuad(m,d,c,backC,backD,154,118,81);
        ggMeshQuad(m,ggMeshV(right,y+4,high+rail-1),
            ggMeshV(left,y+4,low+rail-1),backD,backC,132,100,72);
    }
}
static void ggMeshCartWheel(GgMeshContext*m,int x,int y,int z,int edge,int quality){
    int sides=quality>=2?8:6;
    GgMeshVertex rim[8],inner[8],back[8];
    for(int i=0;i<sides;i++){
        int a=(192+i*256/sides)&255;
        int dx=ggExpCos[a],dz=ggExpSin[a];
        int wx=x+dx*23/4096,wz=z+dz*4/4096;
        rim[i]=ggMeshV(wx,y+edge*4,wz);
        back[i]=ggMeshV(wx,y-edge*3,wz);
        inner[i]=ggMeshV(x+dx*16/4096,rim[i].y,z+dz*3/4096);
    }
    for(int i=0;i<sides;i++){
        int n=(i+1)%sides;
        int a=edge>0?n:i,b=edge>0?i:n;
        ggMeshQuad(m,rim[a],rim[b],inner[b],inner[a],113,85,61);
        ggMeshQuad(m,rim[b],rim[a],back[a],back[b],75,62,49);
        if((i&1)==0){
            GgMeshVertex tip=inner[i],tipEdge=ggMeshV(
                tip.x+(inner[n].x-tip.x)/3,tip.y,tip.z+(inner[n].z-tip.z)/2);
            GgMeshVertex hub=ggMeshV(x,tip.y,z);
            ggMeshTriangle(m,hub,edge>0?tipEdge:tip,edge>0?tip:tipEdge,151,111,72);
        }
    }
    ggMeshBox(m,x-5,y-6,x+5,y+6,z-1,z+1,118,88,56);
}
typedef struct {int x,y,front,rear,bank;} GgMeshCartFrame;
static void ggMeshCartTimber(GgMeshContext*m,const GgMeshCartFrame*p,
                            int x0,int y0,int x1,int y1,int z,int h,
                            int r,int g,int b){
    int left=p->front+(p->rear-p->front)*(x0+37)/54;
    int right=p->front+(p->rear-p->front)*(x1+37)/54;
    int near=p->bank*y0/64,far=p->bank*y1/64;
    GgMeshVertex a=ggMeshV(p->x+x0,p->y+y0,left+near+z);
    GgMeshVertex c=ggMeshV(p->x+x1,p->y+y0,right+near+z);
    GgMeshVertex d=ggMeshV(p->x+x1,p->y+y1,right+far+z);
    GgMeshVertex e=ggMeshV(p->x+x0,p->y+y1,left+far+z);
    GgMeshVertex A=a,C=c,D=d,E=e;
    A.z+=h;C.z+=h;D.z+=h;E.z+=h;
    ggMeshQuad(m,a,c,C,A,r,g,b);
    ggMeshQuad(m,c,d,D,C,r-12,g-12,b-8);
    ggMeshQuad(m,d,e,E,D,r-7,g-7,b-5);
    ggMeshQuad(m,e,a,A,E,r,g,b);
    ggMeshQuad(m,A,C,D,E,r+10,g+9,b+7);
}
static void ggMeshCart(GgMeshContext*m,int x,int y,int quality){
    int axleX=x+17,front=ggMeshGroundAtQ8(x-37,y)+7;
    int nearZ=ggMeshGroundAtQ8(axleX,y-32)+4;
    int farZ=ggMeshGroundAtQ8(axleX,y+32)+4;
    // A sloped chassis meets the axle and the third contact at the stand.
    GgMeshCartFrame frame={x,y,front,(nearZ+farZ)/2+3,farZ-nearZ};
    ggMeshCartTimber(m,&frame,-44,-25,41,25,-2,2,150,106,71);
    ggMeshGroundBox(m,x-40,y-7,x-33,y+7,front,108,83,61);
    for(int edge=-1;edge<=1;edge+=2){
        int railY=edge*21;
        ggMeshCartTimber(m,&frame,-57,railY-3,41,railY+3,-1,3,139,101,64);
        int z=edge<0?nearZ:farZ;
        ggMeshCartTimber(m,&frame,13,railY-4,21,railY+4,-4,3,111,86,61);
        ggMeshCartWheel(m,axleX,y+edge*32,z,edge,quality);
    }
    GgMeshVertex a=ggMeshV(axleX-4,y-32,nearZ+1),b=ggMeshV(axleX+4,y-32,nearZ+1);
    GgMeshVertex c=ggMeshV(axleX+4,y+32,farZ+1),d=ggMeshV(axleX-4,y+32,farZ+1);
    GgMeshVertex A=a,B=b,C=c,D=d;
    A.z-=2;B.z-=2;C.z-=2;D.z-=2;
    ggMeshQuad(m,a,b,c,d,111,82,56);
    ggMeshQuad(m,A,a,d,D,102,73,51);
    ggMeshQuad(m,b,B,C,c,115,85,57);
    ggMeshQuad(m,D,C,B,A,95,71,51);
}
static void ggMeshStall(GgMeshContext*m,int x,int y,int ground,int quality){
    int eave=ground+GG_WORLD_DOOR_HEIGHT+1;
    for(int dx=-44;dx<=44;dx+=88)for(int dy=-24;dy<=24;dy+=48)
        ggMeshGroundBox(m,x+dx-4,y+dy-4,x+dx+4,y+dy+4,
                         eave+3+(dy>0),108,81,59);
    ggMeshBox(m,x-47,y-26,x+47,y-7,ground+7,ground+9,146,102,67);
    ggMeshGroundBox(m,x-39,y-25,x-31,y-12,ground+8,119,85,53);
    ggMeshGroundBox(m,x+31,y-25,x+39,y-12,ground+8,119,85,53);
    for(int dx=-44;dx<=44;dx+=88){
        GgMeshVertex a=ggMeshV(x+dx,y-24,eave),b=ggMeshV(x+dx,y+24,eave+1);
        ggMeshTriangle(m,a,b,ggMeshV(x+dx,y+5,eave+7),151,108,75);
    }
    ggMeshBox(m,x-46,y+3,x+46,y+7,eave+5,eave+7,141,102,70);
    int previous=m->material;
    m->material=GG_MAT_FABRIC;
    int strips=quality>=2?4:1;
    for(int i=0;i<strips;i++){
        int left=x-52+104*i/strips,right=x-52+104*(i+1)/strips;
        int r=i&1?206:158,g=i&1?164:82,b=i&1?115:71;
        GgMeshVertex a=ggMeshV(left,y-33,eave);
        GgMeshVertex b0=ggMeshV(right,y-33,eave);
        GgMeshVertex c=ggMeshV(right,y+5,eave+7);
        GgMeshVertex d=ggMeshV(left,y+5,eave+7);
        ggMeshQuad(m,a,b0,c,d,r,g,b);
        ggMeshQuad(m,d,c,ggMeshV(right,y+33,eave+1),
            ggMeshV(left,y+33,eave+1),r-5,g-4,b-4);
        ggMeshQuad(m,ggMeshV(left,y-33,eave-2),
            ggMeshV(right,y-33,eave-2),b0,a,r-12,g-10,b-8);
    }
    m->material=previous;
}
// People move between tile centres, including diagonal steps. Keep
// rooted scenery away from those centres without looking at occupancy:
// a walking citizen must never make a tree disappear or move. Narrow
// trunks use unequal offsets to clear diagonal as well as cardinal
// lanes; broad furniture clears the main cardinal lanes. This is visual
// placement, not a new collision/navigation simulation.
static inline void ggMeshPropAnchor(int tx,int ty,int kind,uint32_t key,
                                    int *wx,int *wy){
    int dx=(int)((key>>12)&63u)-32;
    int dy=(int)((key>>20)&63u)-32;
    if(kind==GG_SCENE_REEDS&&ggMeshTerrainAt(tx,ty)==T_WATER){
        unsigned shore=ggOutdoorShoreSides(tx,ty,1);
        int first=(int)((key>>9)&3u);
        for(int i=0;i<4;i++){
            int side=(first+i)&3;
            if(!(shore&(1u<<side)))continue;
            dx+=ggOutdoorSideX[side]*86;
            dy+=ggOutdoorSideY[side]*86;
            break;
        }
    }
    if(kind!=GG_SCENE_NONE&&kind!=GG_SCENE_GRASS&&
       kind!=GG_SCENE_CROPS&&kind!=GG_SCENE_REEDS){
        int narrow=kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
                   kind==GG_SCENE_LAMP||kind==GG_SCENE_BRAZIER||
                   kind==GG_SCENE_BANNER;
        dx=(narrow?96:84)+(int)(((key>>13)&7u)%(narrow?8u:5u));
        dy=(narrow?40:84)+(int)(((key>>21)&7u)%(narrow?8u:5u));
        if(narrow&&((key>>5)&1u)){int swap=dx;dx=dy;dy=swap;}
        if((key>>8)&1u)dx=-dx;
        if((key>>19)&1u)dy=-dy;
    }
    *wx=tx*256+128+dx;*wy=ty*256+128+dy;
}
static void ggMeshProp(GgMeshContext*m,int tx,int ty,int kind,
                       int ground,uint32_t key,int quality){
    int x,y;ggMeshPropAnchor(tx,ty,kind,key,&x,&y);
    // Every prop is placed at its jittered Q8 position, not the flat
    // center height of the whole tile. This prevents trees, fences and
    // market furniture floating over a hillside or sinking into it.
    int z=ggMeshGroundAtQ8(x,y);
    (void)ground;
    int tree=kind==GG_SCENE_OAK||kind==GG_SCENE_PINE;
    int low=kind==GG_SCENE_SHRUB||kind==GG_SCENE_STONE;
    int furniture=kind==GG_SCENE_STALL||kind==GG_SCENE_CART||kind==GG_SCENE_CRATES;
    if((tree||low||furniture)&&(quality>=2||
       (m->scale==1&&ggMeshCameraPoint(m,ggMeshV(x,y,z)).f<3*256))){
        int footprint=tree?21:low?29:kind==GG_SCENE_STALL?48:38;
        int canopy=tree?63:footprint;
        int height=quality<2?0:tree?18:low?6:11;
        ggMeshGroundShadows(m,x,y,canopy,footprint,height);
    }
    int previous=m->material,previousSeed=m->materialSeed;
    m->materialSeed=(int)key;
    m->material=(kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
                 kind==GG_SCENE_SHRUB||kind==GG_SCENE_CROPS||
                 kind==GG_SCENE_GRASS||kind==GG_SCENE_REEDS)?
                GG_MAT_LEAVES:
                kind==GG_SCENE_STONE?GG_MAT_ROCK:
                kind==GG_SCENE_COLUMN?GG_MAT_STONE:
                kind==GG_SCENE_BANNER?GG_MAT_FABRIC:GG_MAT_WOOD;
    switch(kind){
    case GG_SCENE_OAK:case GG_SCENE_PINE:
        ggMeshFoliage(m,x,y,z,kind==GG_SCENE_PINE?1:
                      ggMeshTerrainAt(tx,ty)==T_JUNGLE?2:0,key,quality);
        break;
    case GG_SCENE_SHRUB:
        ggMeshShrub(m,x,y,z,key,quality);break;
    case GG_SCENE_COLUMN:
        ggMeshGroundBox(m,x-22,y-22,x+22,y+22,z+3,135,135,126);
        ggMeshBox(m,x-14,y-14,x+14,y+14,z+3,z+16,154,150,136);
        ggMeshBox(m,x-21,y-21,x+21,y+21,z+16,z+19,169,160,144);
        break;
    case GG_SCENE_STONE:
        ggMeshRock(m,x,y,z,ggMeshTerrainAt(tx,ty),key,quality);
        break;
    case GG_SCENE_LAMP:case GG_SCENE_BRAZIER:{
        int flicker=(int)((key+(uint32_t)(ggExplore.frame/9))&3u);
        ggMeshGroundBox(m,x-8,y-8,x+8,y+8,z+2,86,84,76);
        ggMeshBox(m,x-5,y-5,x+5,y+5,z+2,z+12,101,91,75);
        ggMeshBox(m,x-13,y-13,x+13,y+13,z+12,z+15,
                  184+flicker*9,128+flicker*9,64);
        ggMeshTriangle(m,ggMeshV(x-11,y,z+15),
                       ggMeshV(x+11,y,z+15),
                       ggMeshV(x,y,z+19+flicker),250,187,83);
        break;
    }
    case GG_SCENE_BANNER:{
        int gust=ggFilmScenicWind(key,ggExplore.frame)*11;
        ggMeshGroundBox(m,x-3,y-3,x+3,y+3,z+19,109,107,99);
        ggMeshTriangle(m,ggMeshV(x,y,z+17),
                       ggMeshV(x+64+gust,y+gust,z+16),
                       ggMeshV(x+6+gust/3,y,z+9),146,65,80);
        break;
    }
    case GG_SCENE_FENCE:
        ggMeshFence(m,x,y);
        break;
    case GG_SCENE_CRATES:
        ggMeshGroundBox(m,x-40,y-27,x+7,y+27,z+7,153,112,77);
        ggMeshGroundBox(m,x+12,y-20,x+37,y+18,z+5,132,98,69);
        if(quality>=2)
            ggMeshBox(m,x-34,y-20,x-5,y+19,z+7,z+11,172,129,88);
        break;
    case GG_SCENE_CART:
        ggMeshCart(m,x,y,quality);
        break;
    case GG_SCENE_STALL:
        ggMeshStall(m,x,y,z,quality);
        break;
    case GG_SCENE_CROPS:case GG_SCENE_GRASS:case GG_SCENE_REEDS:
        for(int i=0;i<(quality>=2?5:3);i++){
            int a=((int)((key>>7)&255u)+i*51)&255;
            int ox=ggExpCos[a]/156,oy=ggExpSin[a]/156;
            int xx=x+ox,yy=y+oy;
            int sway=ggFilmScenicWind(key+(uint32_t)i,ggExplore.frame)*3;
            int reed=kind==GG_SCENE_REEDS,crop=kind==GG_SCENE_CROPS;
            int height=(reed?4:crop?3:1)+(int)((key>>(i*4))&3u);
            int width=reed?3:5;
            int foot=ggMeshGroundAtQ8(xx,yy);
            int tip=xx+2+sway+ox/3;
            ggMeshTriangle(m,ggMeshV(xx-width,yy-3,ggMeshGroundAtQ8(xx-width,yy-3)-1),
                           ggMeshV(xx+width,yy+3,ggMeshGroundAtQ8(xx+width,yy+3)-1),
                           ggMeshV(tip,yy+oy/3,foot+height),
                           90+i*6,120+i*3,67+i*2);
            if(reed||crop)
                ggMeshTriangle(m,ggMeshV(tip-3,yy-2,foot+height-1),
                    ggMeshV(tip+3,yy+2,foot+height-1),
                    ggMeshV(tip+sway,yy+oy/3,foot+height+1),
                    reed?133:173,reed?103:143,67);
        }
        break;
    default:break;
    }
    m->material=previous;m->materialSeed=previousSeed;
    ggMeshPropsDrawn++;
}
static void ggMeshCloudPuff(GgMeshContext*m,int x,int y,int z,
                            int radius,int height,int r,int g,int b){
    // Fully enclosed eight-sided cloud volume, no single huge kite-
    // shaped triangles or near-plane spikes. Low-contrast faceted haze
    // catches the sky light without turning the horizon into a polygon
    // artifact. Visible from either side of the camera.
    for(int i=0;i<8;i++){
        int a=i*32,n=((i+1)*32)&255;
        GgMeshVertex u=ggMeshV(x+ggExpCos[a]*radius/4096,
                              y+ggExpSin[a]*radius/4096,z);
        GgMeshVertex v=ggMeshV(x+ggExpCos[n]*radius/4096,
                              y+ggExpSin[n]*radius/4096,z);
        int lit=(i+5)%4;
        ggMeshTriangle(m,ggMeshV(x,y,z+height),u,v,
                       r+lit*3,g+lit*2,b+lit*2);
        ggMeshTriangle(m,u,ggMeshV(x,y,z-height/2),v,
                       r-9,g-8,b-7);
    }
}
static void ggMeshCloudBank(GgMeshContext*m,int camTileX,int camTileY,
                             int frame,int hazeR,int hazeG,int hazeB){
    int gx=(camTileX/16)*16,gy=(camTileY/16)*16;
    int sourceX=m->cx,sourceY=m->cy;
    for(int row=-2;row<=2;row++){
        for(int col=-2;col<=2;col++){
            int tx=gx+col*8+4,ty=gy+row*8+4;
            uint32_t key=ggFilmScenicHash(tx,ty,31);
            if(((key>>16)&3u)==0u)continue;
            int x=tx*256+(int)(key&255u)+frame/24;
            int y=ty*256+(int)((key>>8)&255u);
            int dx=x-sourceX,dy=y-sourceY;
            int forward=(dx*m->cosine+dy*m->sine)/4096;
            int side=(-dx*m->sine+dy*m->cosine)/4096;
            // Only distant geometry. Close, above-camera clouds caused
            // giant clipped sky triangles in the prior preview.
            if(forward<6*256||forward>16*256||
               side<-forward*3/2||side>forward*3/2)continue;
            int z=75+(int)((key>>20)&11u);
            int radius=130+(int)((key>>11)&63u);
            int r=ggMeshClamp(hazeR+16,0,255);
            int g=ggMeshClamp(hazeG+17,0,255);
            int b=ggMeshClamp(hazeB+17,0,255);
            ggMeshCloudPuff(m,x,y,z,radius,7,r,g,b);
            ggMeshCloudPuff(m,x+radius*2/3,y+radius/3,z-2,
                            radius*3/4,5,r+5,g+4,b+3);
        }
    }
}
typedef struct {
    int tx,ty,kind,ground,forward;
    uint32_t key;
} GgMeshVisibleProp;
// Stable nearest-on-screen selection, rather than taking the first
// 84 entries of an x/y tile scan. This prevents an empty right-hand
// camera view despite thousands of trees left behind the camera.
static GgMeshVisibleProp ggMeshVisibleProps[GG_SCENE_LIMIT];
// Test the ACTUAL jittered center of a prop, not the tile center.
// Apparent half-width allows crowns, awnings and carts to cross a screen
// edge without popping. Offscreen scenery never consumes the fixed queue.
// Entirely integer math; no extra allocations or per-frame world scans.
static inline int ggMeshSceneryVisible(const GgMeshContext*m,
                                       int wx,int wy,int radius,
                                       int *forwardOut){
    int dx=wx-m->cx,dy=wy-m->cy;
    int forward=(int)(((int64_t)dx*m->cosine+
                        (int64_t)dy*m->sine)/4096);
    int side=(int)((-(int64_t)dx*m->sine+
                    (int64_t)dy*m->cosine)/4096);
    if(forwardOut)*forwardOut=forward;
    if(forward<192||forward>11*256)return 0;
    int64_t across=(int64_t)(side<0?-side:side)*m->focal;
    int64_t visible=(int64_t)forward*(m->w/2+14)+
                    (int64_t)radius*m->focal;
    return across<=visible;
}
static void ggMeshWorldDraw(unsigned char *out,int cinematic){
    // New 3DS holds 400x240 while reducing surface samples and scenery.
    // The legacy 200x120 path uses the same RGB target and world data.
    int quality=ggExpScenicQuality>=2?2:1;
    int cx=ggExplore.px>>8,cy=ggExplore.py>>8;
    int terrain=ggMeshTerrainAt(cx,cy);
    GgFilmScenicAtmosphere atmosphere=ggFilmScenicAtmosphere(terrain);
    // Use the exact triangular terrain beneath the camera. Nominal
    // tile height caused a visible vertical pop at slopes/biome borders.
    int eye=ggMeshGroundAtQ8(ggExplore.px,ggExplore.py)+GG_WORLD_EYE_HEIGHT+
            (ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0);
    int angle=ggExplore.angle&255;
    int cover=terrain==T_SAND||terrain==T_DESERT?0:
              terrain==T_SNOW?3:
              terrain==T_SWAMP||terrain==T_FOREST||terrain==T_JUNGLE?2:1;
    GgMeshContext*m=&ggMeshContext;
    ggMeshBeginDaylight(m,out,quality,ggExplore.px,ggExplore.py,eye,
                        ggExpCos[angle],ggExpSin[angle],
                        49+atmosphere.skyR,78+atmosphere.skyG,
                        108+atmosphere.skyB,cover);
    ggMeshSkyWeather(m,ggExplore.frame);
    m->fogMax=(terrain==T_SWAMP||terrain==T_SNOW)?174:132;
    ggMeshTilesDrawn=ggMeshBuildingsDrawn=ggMeshPropsDrawn=0;
    enum { VIEW_RADIUS=9, VIEW_WIDTH=VIEW_RADIUS*2+1,
           LOT_REACH=VIEW_RADIUS+1, LOT_WIDTH=LOT_REACH*2+1 };
    struct { short x,y,forward; } buildings[VIEW_WIDTH*VIEW_WIDTH];
    // A visible wing queues its parent even when the parent tile is
    // outside the scan. The extra one-cell border holds those origins.
    unsigned char buildingSeen[(LOT_WIDTH*LOT_WIDTH+7)/8]={0};
    int buildingCount=0;
    int radius=quality>=2?VIEW_RADIUS:7;
    int propCap=ggFilmScenicBudget(ggExpScenicQuality,0);
    int sceneLimit=0;
    // A deterministic local scene cache is generated from the SAME
    // authoritative world/settlements on each real world camera frame.
    // The fixed 19x19 or 15x15 scan is independent of creature count.
    for(int ty=cy-radius;ty<=cy+radius;ty++){
        if((unsigned)ty>=WH)continue;
        for(int tx=cx-radius;tx<=cx+radius;tx++){
            if((unsigned)tx>=WW)continue;
            int dx=(tx-cx)*256,dy=(ty-cy)*256;
            int forward=(dx*m->cosine+dy*m->sine)/4096;
            int side=(-dx*m->sine+dy*m->cosine)/4096;
            if(forward<-256||forward>radius*256||
               side<-(forward+512)*2||side>(forward+512)*2)
                continue;
            int t=ggMeshTerrainAt(tx,ty);
            uint32_t key=ggFilmScenicHash(tx,ty,17);
            ggMeshGroundTile(m,tx,ty,t,key);
            int build=bld[ty][tx];
            if(ggBuildingIsPaving(build))continue;
            if(!build&&ggExpSceneryEnabled&&quality>=2&&
               forward>=0&&forward<4*256)
                ggMeshGroundLife(m,tx,ty,t,key);
            if(build){
                int rootX=tx,rootY=ty;
                int parent=ggResidenceResolveWing(tx,ty,build,&rootX,&rootY);
                int slot=(rootY-cy+LOT_REACH)*LOT_WIDTH+rootX-cx+LOT_REACH;
                unsigned bit=1u<<(slot&7);
                if(buildingSeen[slot>>3]&bit)continue;
                buildingSeen[slot>>3]|=(unsigned char)bit;
                if(parent)forward=((rootX-cx)*256*m->cosine+
                                   (rootY-cy)*256*m->sine)/4096;
                buildings[buildingCount].x=(short)rootX;
                buildings[buildingCount].y=(short)rootY;
                buildings[buildingCount++].forward=(short)forward;
            }else if(ggExpSceneryEnabled){
                uint32_t scenic=ggFilmScenicHash(tx,ty,7);
                if(((scenic>>24)&7u)>=5u)continue;
                int kind=ggOutdoorPropKind(tx,ty,t,scenic);
                if(!kind)continue;
                int propX,propY,propForward;
                ggMeshPropAnchor(tx,ty,kind,scenic,&propX,&propY);
                if(!ggMeshSceneryVisible(m,propX,propY,136,
                                          &propForward))continue;
                GgMeshVisibleProp candidate={
                    tx,ty,kind,ggExpGround(t),propForward,scenic
                };
                if(sceneLimit<propCap){
                    ggMeshVisibleProps[sceneLimit++]=candidate;
                }else if(sceneLimit){
                    int farthest=0;
                    for(int i=1;i<sceneLimit;i++)
                        if(ggMeshVisibleProps[i].forward>
                           ggMeshVisibleProps[farthest].forward)
                            farthest=i;
                    if(candidate.forward<
                       ggMeshVisibleProps[farthest].forward)
                        ggMeshVisibleProps[farthest]=candidate;
                }
            }
        }
    }
    // All terrain is in the depth buffer before building contact shade.
    // This keeps neighbouring tiles from painting over the cast patch.
    int oldMaterial=m->material,oldSeed=m->materialSeed;
    for(int i=0;i<buildingCount;i++){
        int tx=buildings[i].x,ty=buildings[i].y,forward=buildings[i].forward;
        int build=bld[ty][tx];
        int detailed=forward<(quality>=2?6:4)*256&&forward>-128;
        m->material=(build==B_HOUSE||build==B_HUT||
                     build==B_DOCK||build==B_FARM)?GG_MAT_WOOD:GG_MAT_STONE;
        m->materialSeed=(int)ggFilmScenicHash(tx,ty,17);
        if(detailed&&forward>128&&build!=B_DOCK&&build!=B_BORDER&&
           build!=B_WALL&&!ggBuildingIsGate(build))
            ggMeshContactShadow(m,tx*256+128,ty*256+128,build==B_WELL?GG_WELL_OUTER:105,
                                ggExpBuilding(build));
        ggMeshBuilding(m,tx,ty,build,ggExpGround(ggMeshTerrainAt(tx,ty)),detailed);
    }
    m->material=oldMaterial;m->materialSeed=oldSeed;
    for(int i=0;i<sceneLimit;i++){
        const GgMeshVisibleProp *p=&ggMeshVisibleProps[i];
        // Full foliage silhouette near camera, cheaper distant facets.
        // Preserve the selected object and its world contact at all LODs.
        int detail=ggExpScenicQuality>=2&&p->forward<5*256?2:1;
        ggMeshProp(m,p->tx,p->ty,p->kind,p->ground,p->key,
                   detail);
    }
}
#endif

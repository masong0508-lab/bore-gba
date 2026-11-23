// Fortification masonry is emitted from one layout for drawing and cover.
#ifndef GG_MESH_FORT3D_H
#define GG_MESH_FORT3D_H

enum { GG_FORT_GATE_HALF=52, GG_FORT_WALL_HALF=24, GG_FORT_WALL_TOP=28 };
typedef struct {
    GgMeshContext *mesh;
    int x,y,z,solid,detail;
} GgMeshFort;

static inline int ggMeshFortType(int build){
    return build==B_KEEP||build==B_WALL||ggBuildingIsGate(build);
}

// Seated stone touches a solid course over its entire base. The
// buried interface has no visible underside in the combined mesh.
static void ggMeshFortMasonry(GgMeshFort *f,int x0,int y0,int x1,int y1,
                               int bottom,int top,int cut,int seated,
                               int r,int g,int b){
    if(bottom>=top||x0>=x1||y0>=y1)return;
    GgMeshContext*m=f->mesh;
    if(!m){
        if(f->solid||f->x<x0||f->x>x1||f->y<y0||f->y>y1||
           f->z<bottom||f->z>top)return;
        int dx=f->x-x0<x1-f->x?f->x-x0:x1-f->x;
        int dy=f->y-y0<y1-f->y?f->y-y0:y1-f->y;
        f->solid=dx+dy>=cut;
        return;
    }
    if(!cut&&!seated){
        ggMeshArchitectureBlock(m,x0,y0,x1,y1,bottom,top,r,g,b);
        return;
    }
    int x[8]={x0,x1,x1,x0},y[8]={y0,y0,y1,y1},count=4;
    if(cut){
        int ox[]={x0+cut,x1-cut,x1,x1,x1-cut,x0+cut,x0,x0};
        int oy[]={y0,y0,y0+cut,y1-cut,y1,y1,y1-cut,y0+cut};
        for(int i=0;i<8;i++){x[i]=ox[i];y[i]=oy[i];}
        count=8;
    }
    GgMeshVertex floor[8],roof[8];
    for(int i=0;i<count;i++){
        floor[i]=ggMeshV(x[i],y[i],bottom);
        roof[i]=ggMeshV(x[i],y[i],top);
    }
    GgMeshVertex closest=ggMeshV(m->cosine>=0?x0:x1,m->sine>=0?y0:y1,bottom);
    int clipped=ggMeshCameraPoint(m,closest).f<=m->near;
    static const unsigned char light[]={231,213,191,201,216,228,253,247};
    for(int i=0;i<count;i++){
        int j=(i+1)%count;
        int cross=(x[j]-x[i])*(m->cy-y[i])-(y[j]-y[i])*(m->cx-x[i]);
        int shade=light[cut?i:i*2];
        if(clipped||cross<=0)
            ggMeshQuad(m,floor[i],floor[j],roof[j],roof[i],
                        r*shade/256,g*shade/256,b*shade/256);
    }
    for(int i=1;i+1<count;i++){
        if(clipped||m->eye>=top)
            ggMeshTriangle(m,roof[0],roof[i],roof[i+1],r+12,g+12,b+12);
        if(!seated&&(clipped||m->eye<=bottom))
            ggMeshTriangle(m,floor[0],floor[i+1],floor[i],r*3/4,g*3/4,b*3/4);
    }
}
static void ggMeshFortPrism(GgMeshFort *f,int x0,int y0,int x1,int y1,
                             int bottom,int top,int cut,int r,int g,int b){
    ggMeshFortMasonry(f,x0,y0,x1,y1,bottom,top,cut,0,r,g,b);
}
static void ggMeshFortOriented(GgMeshFort*f,int axis,int p0,int u0,
                                int p1,int u1,int bottom,int top,int seated,
                                int r,int g,int b){
    if(axis)ggMeshFortMasonry(f,u0,p0,u1,p1,bottom,top,0,seated,r,g,b);
    else ggMeshFortMasonry(f,p0,u0,p1,u1,bottom,top,0,seated,r,g,b);
}
static int ggMeshFortConnects(int tx,int ty,int axis){
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 0;
    int neighbor=bld[ty][tx];
    return neighbor==B_WALL||neighbor==(axis?B_GATE_NS:B_GATE_EW);
}

static void ggMeshFortRail(GgMeshFort*f,int axis,int p,int u0,int u1,int top){
    if(u1<=u0)return;
    ggMeshFortOriented(f,axis,p-5,u0,p+5,u1,top,top+4,1,176,168,146);
    for(int start=u0+8;start+18<=u1;start+=46){
        ggMeshFortOriented(f,axis,p-5,start,p+5,start+18,top+4,top+10,
                           1,196,185,158);
        if(f->solid)return;
    }
}
static void ggMeshFortCurtain(GgMeshFort*f,int axis,int p,int u0,int u1,
                               int base,int top){
    if(u1<=u0)return;
    int w=GG_FORT_WALL_HALF;
    ggMeshFortOriented(f,axis,p-w-4,u0,p+w+4,u1,base,base+7,0,141,132,113);
    ggMeshFortOriented(f,axis,p-w,u0,p+w,u1,base,top,0,172,161,137);
    ggMeshFortOriented(f,axis,p-w-3,u0,p+w+3,u1,top-3,top,0,199,184,153);
    if(f->solid)return;
    ggMeshFortRail(f,axis,p-w+4,u0,u1,top);
    ggMeshFortRail(f,axis,p+w-4,u0,u1,top);
}
static void ggMeshFortWall(GgMeshFort*f,int tx,int ty,int ground){
    int x=tx*256+128,y=ty*256+128;
    int west=ggMeshFortConnects(tx-1,ty,1),east=ggMeshFortConnects(tx+1,ty,1);
    int north=ggMeshFortConnects(tx,ty-1,0),south=ggMeshFortConnects(tx,ty+1,0);
    int lone=!(north||south||west||east);
    int base=ggMeshFoundationGround(x-128,y-128,x+128,y+128,ground);
    int top=ground+GG_FORT_WALL_TOP;
    if(west||east||lone)
        ggMeshFortCurtain(f,1,y,x-(west?128:lone?76:22),
                           x+(east?128:lone?76:22),base,top);
    if(f->solid)return;
    if(north||south)
        ggMeshFortCurtain(f,0,x,y-(north?128:22),y+(south?128:22),base,top);
    if(f->solid)return;
    // Corner and junction piers tie transverse wall walks together.
    if((north||south)&&(west||east)){
        ggMeshFortPrism(f,x-35,y-35,x+35,y+35,base,top+2,8,158,149,127);
        ggMeshFortPrism(f,x-37,y-37,x+37,y+37,top-2,top+3,9,196,184,156);
        ggMeshFortPrism(f,x-12,y-12,x+12,y+12,top+3,top+10,3,187,165,137);
    }
}

static void ggMeshFortBattlements(GgMeshFort*f,int x,int y,int half,
                                    int z,int merlon){
    ggMeshFortMasonry(f,x-half,y-half,x+half,y+half,z,z+4,0,1,168,157,136);
    if(f->solid)return;
    for(int dx=-1;dx<=1;dx+=2)for(int dy=-1;dy<=1;dy+=2){
        int bx=x+dx*(half-merlon/2),by=y+dy*(half-merlon/2);
        ggMeshFortMasonry(f,bx-merlon/2,by-merlon/2,bx+merlon/2,by+merlon/2,
                         z+4,z+10,0,1,190,177,150);
        if(f->solid)return;
    }
}
static void ggMeshFortTowerSlits(GgMeshFort*f,int x,int y,int half,int z){
    GgMeshContext*m=f->mesh;
    if(!m||!f->detail)return;
    int material=m->material,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    for(int axis=0;axis<2;axis++)for(int side=-1;side<=1;side+=2){
        int p=(axis?y:x)+side*half,u=axis?x:y;
        int cameraSide=(axis?m->cy:m->cx)-p;
        if(cameraSide*side<0)continue;
        GgMeshVertex a=ggMeshFacadePoint(axis,p,u-4,z-5);
        GgMeshVertex b=ggMeshFacadePoint(axis,p,u+4,z-5);
        GgMeshVertex c=ggMeshFacadePoint(axis,p,u+3,z+6);
        GgMeshVertex d=ggMeshFacadePoint(axis,p,u-3,z+6);
        ggMeshQuad(m,a,b,c,d,62,65,64);
    }
    m->material=material;m->depthBias=bias;
}
static void ggMeshFortTower(GgMeshFort*f,int x,int y,int half,int base,int floor,
                              int top,int rise){
    ggMeshFortPrism(f,x-half-4,y-half-4,x+half+4,y+half+4,base,floor+6,
                   10,142,135,119);
    ggMeshFortPrism(f,x-half,y-half,x+half,y+half,base,top-4,9,176,164,143);
    ggMeshFortPrism(f,x-half-6,y-half-6,x+half+6,y+half+6,top-5,top+rise,
                   10,190,176,151);
    if(f->solid)return;
    ggMeshFortBattlements(f,x,y,half+1,top+rise,16);
    ggMeshFortTowerSlits(f,x,y,half,top-17);
}
static void ggMeshFortKeep(GgMeshFort*f,int tx,int ty,int ground){
    int x=tx*256+128,y=ty*256+128;
    int base=ggMeshFoundationGround(x-116,y-116,x+116,y+116,ground);
    ggMeshFortPrism(f,x-87,y-88,x+87,y+88,base,ground+6,10,138,131,113);
    ggMeshFortPrism(f,x-82,y-84,x+82,y+84,base,ground+27,7,171,159,136);
    ggMeshFortPrism(f,x-85,y-87,x+85,y+87,ground+24,ground+27,7,194,180,152);
    if(f->solid)return;
    for(int dx=-1;dx<=1;dx+=2)for(int dy=-1;dy<=1;dy+=2){
        ggMeshFortTower(f,x+dx*76,y+dy*76,28,base,ground,
                        ground+(dy>0?53:49),2);
        if(f->solid)return;
    }
    // The narrower central donjon reads above the corner towers.
    ggMeshFortPrism(f,x-48,y-48,x+48,y+48,ground+27,ground+61,5,
                   188,177,155);
    ggMeshFortPrism(f,x-54,y-54,x+54,y+54,ground+58,ground+62,6,
                   156,151,138);
    ggMeshFortBattlements(f,x,y,50,ground+62,20);
    if(f->solid)return;
    ggMeshFortRail(f,1,y-81,x-48,x+48,ground+27);
    ggMeshFortRail(f,1,y+81,x-48,x+48,ground+27);
    ggMeshFortPrism(f,x-22,y-89,x+22,y-84,base,ground+16,0,107,83,61);
    ggMeshFortPrism(f,x-26,y-90,x+26,y-84,ground+16,ground+19,0,186,163,128);
    ggMeshFortTowerSlits(f,x,y,48,ground+49);
}

// One faceted arch profile defines both the vaulted mesh and its cover.
// Height increases towards the middle, with a flat upper crown.
static const unsigned char ggMeshFortVaultRun[]={0,22,39,GG_FORT_GATE_HALF};
static const unsigned char ggMeshFortVaultRise[]={32,30,26,22};
enum { GG_FORT_VAULT_PARTS=sizeof ggMeshFortVaultRun/sizeof ggMeshFortVaultRun[0]-1 };
static int ggMeshFortVaultHit(int across,int height,int ceiling){
    int u=abs(across);
    if(u>GG_FORT_GATE_HALF)return 0;
    for(int i=0;i<GG_FORT_VAULT_PARTS;i++){
        int a=ggMeshFortVaultRun[i],b=ggMeshFortVaultRun[i+1];
        if(u>b)continue;
        int span=b-a;
        int lower=ggMeshFortVaultRise[i]*(b-u)+ggMeshFortVaultRise[i+1]*(u-a);
        int upper=ceiling?ceiling*span:lower+4*span;
        return height*span>=lower&&height*span<=upper;
    }
    return 0;
}
static void ggMeshFortOrientedQuad(GgMeshContext*m,int axis,int forward,
                                   GgMeshVertex a,GgMeshVertex b,
                                   GgMeshVertex c,GgMeshVertex d,
                                   int red,int green,int blue){
    if(axis==forward)ggMeshQuad(m,a,b,c,d,red,green,blue);
    else ggMeshQuad(m,d,c,b,a,red,green,blue);
}
static void ggMeshFortVault(GgMeshFort*f,int axis,int p0,int p1,int mid,
                             int floor,int ceiling,int rimSide){
    GgMeshContext*m=f->mesh;
    if(!m){
        int p=axis?f->y:f->x,u=axis?f->x:f->y;
        if(p>=p0&&p<=p1&&ggMeshFortVaultHit(u-mid,f->z-floor,ceiling))
            f->solid=1;
        return;
    }
    int plane=axis?m->cy:m->cx;
    int across=axis?m->cx:m->cy;
    int r=ceiling?171:216,g=ceiling?161:200,b=ceiling?138:169;
    int front=(axis?m->sine:m->cosine)>=0?p0:p1;
    int sideSign=(axis?m->cosine:m->sine)>=0?-1:1;
    GgMeshVertex closest=ggMeshFacadePoint(axis,front,
                        mid+sideSign*GG_FORT_GATE_HALF,floor);
    int clipped=ggMeshCameraPoint(m,closest).f<=m->near;
    for(int side=-1;side<=1;side+=2)for(int i=0;i<GG_FORT_VAULT_PARTS;i++){
        int a=i+(side<0),c=i+(side>0);
        int u0=mid+side*ggMeshFortVaultRun[a],u1=mid+side*ggMeshFortVaultRun[c];
        int z0=floor+ggMeshFortVaultRise[a],z1=floor+ggMeshFortVaultRise[c];
        int cap0=ceiling?floor+ceiling:z0+4,cap1=ceiling?floor+ceiling:z1+4;
        GgMeshVertex A=ggMeshFacadePoint(axis,p0,u0,z0),B=ggMeshFacadePoint(axis,p0,u1,z1);
        GgMeshVertex C=ggMeshFacadePoint(axis,p1,u1,z1),D=ggMeshFacadePoint(axis,p1,u0,z0);
        GgMeshVertex E=A,F=B,G=C,H=D;
        E.z=H.z=cap0;F.z=G.z=cap1;
        int shade=ceiling?0:i%2*13;
        if(rimSide<=0&&(plane<=p0||clipped))
            ggMeshFortOrientedQuad(m,axis,1,A,B,F,E,r-shade,g-shade,b-shade);
        if(rimSide>=0&&(plane>=p1||clipped))
            ggMeshFortOrientedQuad(m,axis,0,D,C,G,H,r-10-shade,g-10-shade,b-8-shade);
        if(m->eye>=cap0||m->eye>=cap1||clipped)
            ggMeshFortOrientedQuad(m,axis,1,E,F,G,H,r+8,g+8,b+8);
        if(m->eye<=z0||m->eye<=z1||clipped)
            ggMeshFortOrientedQuad(m,axis,0,A,B,C,D,r*3/4,g*3/4,b*3/4);
        if(i==GG_FORT_VAULT_PARTS-1&&
           (clipped||(across-mid)*side>=GG_FORT_GATE_HALF)){
            if(side>0)ggMeshFortOrientedQuad(m,axis,1,B,C,G,F,r,g,b);
            else ggMeshFortOrientedQuad(m,axis,0,A,D,H,E,r,g,b);
        }
    }
}
// The gate passage follows the very same terrain fan as the player's
// feet. A single flat bridge slab floated above low banks and vanished
// into mountain slopes. Four short stone courses follow the actual
// interpolated heights; inset roadside trims are restricted to near LOD.
// This is paving, not a new collision solid: combat and navigation still
// pass through the correctly vaulted center of the gate.
enum { GG_FORT_APPROACH_HALF=42, GG_FORT_APPROACH_STEPS=4 };
static inline GgMeshVertex ggMeshFortApproachVertex(int axis,int p,int u){
    GgMeshVertex v=ggMeshFacadePoint(axis,p,u,0);
    v.z=ggMeshGroundAtQ8(v.x,v.y)+1;
    return v;
}
static void ggMeshFortApproach(GgMeshFort*f,int axis,int p,int u){
    GgMeshContext*m=f->mesh;
    if(!m)return;
    int prior=m->material,bias=m->depthBias;
    m->material=GG_MAT_PAVING;
    m->depthBias=1;
    for(int i=0;i<GG_FORT_APPROACH_STEPS;i++){
        int p0=p-64+i*128/GG_FORT_APPROACH_STEPS;
        int p1=p-64+(i+1)*128/GG_FORT_APPROACH_STEPS;
        GgMeshVertex a=ggMeshFortApproachVertex(axis,p0,u-GG_FORT_APPROACH_HALF);
        GgMeshVertex b=ggMeshFortApproachVertex(axis,p1,u-GG_FORT_APPROACH_HALF);
        GgMeshVertex c=ggMeshFortApproachVertex(axis,p1,u+GG_FORT_APPROACH_HALF);
        GgMeshVertex d=ggMeshFortApproachVertex(axis,p0,u+GG_FORT_APPROACH_HALF);
        ggMeshFortOrientedQuad(m,axis,0,a,b,c,d,158,145,122);
        if(f->detail&&m->detailQuality>=2){
            // The narrow raised edge is outside the walkable central strip.
            for(int side=-1;side<=1;side+=2){
                int inner=u+side*(GG_FORT_APPROACH_HALF-7);
                int outer=u+side*GG_FORT_APPROACH_HALF;
                GgMeshVertex A=ggMeshFortApproachVertex(axis,p0,inner);
                GgMeshVertex B=ggMeshFortApproachVertex(axis,p1,inner);
                GgMeshVertex C=ggMeshFortApproachVertex(axis,p1,outer);
                GgMeshVertex D=ggMeshFortApproachVertex(axis,p0,outer);
                A.z++;B.z++;C.z++;D.z++;
                ggMeshFortOrientedQuad(m,axis,0,A,B,C,D,192,177,147);
            }
        }
    }
    m->material=prior;m->depthBias=bias;
}
static int ggMeshFortGateFloor(int axis,int p,int u,int ground){
    int highest=ground;
    for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++){
        GgMeshVertex foot=ggMeshFacadePoint(axis,p+i*62,u+j*GG_FORT_GATE_HALF,0);
        int z=ggMeshGroundAtQ8(foot.x,foot.y);
        if(z>highest)highest=z;
    }
    return highest;
}
static void ggMeshFortGate(GgMeshFort*f,int tx,int ty,int build,int ground){
    int axis=build==B_GATE_NS,x=tx*256+128,y=ty*256+128;
    int p=axis?y:x,u=axis?x:y,half=GG_FORT_GATE_HALF;
    int floor=ggMeshFortGateFloor(axis,p,u,ground);
    int top=floor+42;
    int base=ggMeshFoundationGround(x-128,y-128,x+128,y+128,ground);
    for(int side=-1;side<=1;side+=2){
        int pier=u+side*78;
        ggMeshFortOriented(f,axis,p-63,pier-26,p+63,pier+26,base,top,0,172,160,137);
        ggMeshFortOriented(f,axis,p-69,pier-26,p+69,pier+26,base,floor+6,0,141,134,118);
        ggMeshFortOriented(f,axis,p-70,pier-28,p+70,pier+28,top-4,top+2,0,196,182,154);
        if(f->solid)return;
        for(int face=-1;face<=1;face+=2){
            ggMeshFortOriented(f,axis,p+face*64-6,pier-25,p+face*64+6,pier+25,
                               top+2,top+6,1,172,159,136);
            for(int dx=-1;dx<=1;dx+=2){
                int q=pier+dx*16;
                ggMeshFortOriented(f,axis,p+face*64-6,q-8,p+face*64+6,q+8,
                                   top+6,top+12,1,188,175,149);
            }
        }
        if(ggMeshFortConnects(tx+(axis?side:0),ty+(axis?0:side),axis)){
            int left=u+(side>0?99:-128),right=u+(side>0?128:-99);
            ggMeshFortCurtain(f,axis,p,left,right,base,ground+GG_FORT_WALL_TOP);
        }
        if(f->solid)return;
    }
    ggMeshFortVault(f,axis,p-53,p+53,u,floor,42,0);
    if(f->solid)return;
    // Each rim embeds its interior face into the main barrel.
    ggMeshFortVault(f,axis,p-67,p-52,u,floor,0,-1);
    ggMeshFortVault(f,axis,p+52,p+67,u,floor,0,1);
    ggMeshFortApproach(f,axis,p,u);
    ggMeshFortRail(f,axis,p-48,u-half,u+half,top);
    ggMeshFortRail(f,axis,p+48,u-half,u+half,top);
    if(f->solid)return;
    // A raised wooden portcullis hangs high inside the real passage.
    GgMeshContext*m=f->mesh;
    if(m)m->material=GG_MAT_WOOD;
    ggMeshFortOriented(f,axis,p-5,u-38,p+5,u+38,floor+28,floor+30,
                       0,83,76,62);
    for(int bar=-32;bar<=32;bar+=16){
        ggMeshFortOriented(f,axis,p-4,u+bar-2,p+4,u+bar+2,floor+26,floor+34,
                           0,98,86,70);
        if(f->solid)return;
    }
    if(m)m->material=GG_MAT_STONE;
}
static void ggMeshFortLayout(GgMeshFort*f,int tx,int ty,int build,int ground){
    if(build==B_KEEP)ggMeshFortKeep(f,tx,ty,ground);
    else if(build==B_WALL)ggMeshFortWall(f,tx,ty,ground);
    else ggMeshFortGate(f,tx,ty,build,ground);
}
static int ggMeshFortDraw(GgMeshContext*m,int tx,int ty,int build,int ground,
                           int nearDetail){
    if(!ggMeshFortType(build))return 0;
    int material=m->material;
    m->material=GG_MAT_STONE;
    GgMeshFort f={.mesh=m,.detail=nearDetail};
    ggMeshFortLayout(&f,tx,ty,build,ground);
    m->material=material;
    return 1;
}
static int ggMeshFortSolid(int tx,int ty,int build,int x,int y,int z,int ground){
    if(!ggMeshFortType(build))return -1;
    if(x<tx*256||x>(tx+1)*256||y<ty*256||y>(ty+1)*256||z>ground+100)
        return 0;
    if(ggBuildingIsGate(build)&&z<ground+ggMeshFortVaultRise[GG_FORT_VAULT_PARTS]){
        int across=build==B_GATE_NS?x-(tx*256+128):y-(ty*256+128);
        if(abs(across)<GG_FORT_GATE_HALF)return 0;
    }
    GgMeshFort f={.x=x,.y=y,.z=z};
    ggMeshFortLayout(&f,tx,ty,build,ground);
    return f.solid;
}
#endif

#ifndef GG_MESH_ARCHITECTURE3D_H
#define GG_MESH_ARCHITECTURE3D_H
// Building-specific solid pieces share the terrain sampler and depth buffer
// used by the shell. Dimensions are Q8 horizontally and world height in Z.
static inline GgMeshVertex ggMeshFacadePoint(int axis,int plane,int across,int z){
    return axis?ggMeshV(across,plane,z):ggMeshV(plane,across,z);
}
static void ggMeshArchitectureBlock(GgMeshContext*m,int x0,int y0,int x1,int y1,
                                    int bottom,int top,int r,int g,int b){
    if(bottom>=top)return;
    GgMeshVertex a=ggMeshV(x0,y0,bottom),c=ggMeshV(x1,y0,bottom),
        d=ggMeshV(x1,y1,bottom),e=ggMeshV(x0,y1,bottom);
    GgMeshVertex A=ggMeshV(x0,y0,top),C=ggMeshV(x1,y0,top),
        D=ggMeshV(x1,y1,top),E=ggMeshV(x0,y1,top);
    // The rear face can become visible when near clipping removes
    // the front of an otherwise opaque block.
    GgMeshVertex closest=ggMeshV(m->cosine>=0?x0:x1,m->sine>=0?y0:y1,bottom);
    int clipped=ggMeshCameraPoint(m,closest).f<=m->near;
    if(clipped||m->cy<=y0)ggMeshQuad(m,a,c,C,A,r*9/10,g*9/10,b*9/10);
    if(clipped||m->cx>=x1)ggMeshQuad(m,c,d,D,C,r*7/10,g*7/10,b*7/10);
    if(clipped||m->cy>=y1)ggMeshQuad(m,d,e,E,D,r*8/10,g*8/10,b*8/10);
    if(clipped||m->cx<=x0)ggMeshQuad(m,e,a,A,E,r,g,b);
    if(clipped||m->eye>=top)ggMeshQuad(m,A,C,D,E,r*11/10,g*11/10,b*11/10);
    if(clipped||m->eye<bottom)ggMeshQuad(m,c,a,e,d,r*3/4,g*3/4,b*3/4);
}
static void ggMeshFacadeBox(GgMeshContext*m,int axis,int p0,int u0,
                           int p1,int u1,int z0,int z1,int r,int g,int b){
    if(axis)ggMeshArchitectureBlock(m,u0,p0,u1,p1,z0,z1,r,g,b);
    else ggMeshArchitectureBlock(m,p0,u0,p1,u1,z0,z1,r,g,b);
}
static void ggMeshArchitectureSupport(GgMeshContext*m,int x0,int y0,int x1,int y1,
                                      int top,int r,int g,int b){
    int base=ggMeshFoundationGround(x0,y0,x1,y1,top);
    ggMeshArchitectureBlock(m,x0,y0,x1,y1,base,top,r,g,b);
}
static int ggMeshFacadeFoundation(int axis,int p0,int u0,int p1,int u1,int z){
    return axis?ggMeshFoundationGround(u0,p0,u1,p1,z):
                ggMeshFoundationGround(p0,u0,p1,u1,z);
}
static void ggMeshPortico(GgMeshContext*m,int axis,int plane,int center,
                          int ground,int top,int temple){
    int clearance=ground+GG_WORLD_DOOR_HEIGHT+3;
    if(top<clearance)top=clearance;
    int reach=temple?52:34,half=temple?79:66;
    int corner=center-half,far=center+half,pier=plane-reach;
    int col=temple?62:51;
    int r=temple?203:153,g=temple?189:142,b=temple?165:121;
    int material=m->material;
    m->material=temple?GG_MAT_STONE:GG_MAT_WOOD;
    int base=ggMeshFacadeFoundation(axis,pier-9,corner-5,plane,far+5,ground);
    ggMeshFacadeBox(m,axis,pier-9,corner-5,plane,far+5,base,ground+1,
                   r-25,g-23,b-20);
    for(int n=-1;n<=1;n+=2){
        int u=center+n*col,p=pier+9;
        ggMeshFacadeBox(m,axis,p-10,u-11,p+10,u+11,ground,ground+3,r,g,b);
        ggMeshFacadeBox(m,axis,p-7,u-8,p+7,u+8,ground+3,top-4,
                       r-17,g-16,b-14);
        ggMeshFacadeBox(m,axis,p-11,u-11,p+10,u+11,top-4,top-1,r,g,b);
    }
    ggMeshFacadeBox(m,axis,pier,corner,plane+6,far,top-1,top+2,r,g,b);
    if(temple){
        GgMeshVertex left=ggMeshFacadePoint(axis,pier-2,corner-3,top+2);
        GgMeshVertex peak=ggMeshFacadePoint(axis,pier-2,center,top+11);
        GgMeshVertex right=ggMeshFacadePoint(axis,pier-2,far+3,top+2);
        GgMeshVertex innerLeft=left,innerPeak=peak,innerRight=right;
        if(axis)innerLeft.y=innerPeak.y=innerRight.y=plane+8;
        else innerLeft.x=innerPeak.x=innerRight.x=plane+8;
        if(axis)ggMeshTriangle(m,left,right,peak,r,g,b);
        else ggMeshTriangle(m,left,peak,right,r,g,b);
        m->material=GG_MAT_ROOF;
        if(axis){
            ggMeshQuad(m,left,peak,innerPeak,innerLeft,112,132,105);
            ggMeshQuad(m,peak,right,innerRight,innerPeak,112,132,105);
        }else{
            ggMeshQuad(m,left,innerLeft,innerPeak,peak,112,132,105);
            ggMeshQuad(m,peak,innerPeak,innerRight,right,112,132,105);
        }
    }
    m->material=material;
}
static void ggMeshChimney(GgMeshContext*m,int x0,int y0,int x1,int y1,
                          int wallTop,int build){
    int x=x0+(x1-x0)*3/10,y=y0+(y1-y0)*4/5;
    int top=wallTop+(build==B_HUT?23:27);
    int material=m->material;
    m->material=GG_MAT_STONE;
    ggMeshArchitectureBlock(m,x-10,y-12,x+10,y+12,wallTop,top,131,105,87);
    ggMeshArchitectureBlock(m,x-14,y-16,x+14,y+16,top-3,top,180,143,107);
    ggMeshArchitectureBlock(m,x-8,y-9,x+8,y+9,top,top+2,86,70,65);
    m->material=material;
}
static void ggMeshBarnGate(GgMeshContext*m,int axis,int plane,int facing,
                           int center,int base){
    int outer=plane-facing*8,frame=outer-facing*3;
    int p0=outer<frame?outer:frame,p1=outer>frame?outer:frame;
    int a=center-43,b=center+43,top=base+17;
    ggMeshFacadeBox(m,axis,p0,a-6,p1,a,base,top+2,132,92,63);
    ggMeshFacadeBox(m,axis,p0,b,p1,b+6,base,top+2,132,92,63);
    ggMeshFacadeBox(m,axis,p0,a,p1,b,top,top+2,151,106,66);
    GgMeshVertex a0=ggMeshFacadePoint(axis,outer,a,base);
    GgMeshVertex b0=ggMeshFacadePoint(axis,outer,b,base);
    GgMeshVertex a1=ggMeshFacadePoint(axis,outer,a,top);
    GgMeshVertex b1=ggMeshFacadePoint(axis,outer,b,top);
    ggMeshQuad(m,a0,b0,b1,a1,79,58,48);
    for(int side=-1;side<=1;side+=2){
        int u=center+side*5,v=center+side*41;
        ggMeshQuad(m,ggMeshFacadePoint(axis,frame,u,base+1),
                     ggMeshFacadePoint(axis,frame,u+side*7,base+1),
                     ggMeshFacadePoint(axis,frame,v,top-1),
                     ggMeshFacadePoint(axis,frame,v-side*7,top-1),133,94,63);
    }
}
static void ggMeshWatchtower(GgMeshContext*m,int x0,int y0,int x1,int y1,
                             int wallTop,int nearDetail){
    int cap=wallTop+3;
    // The projecting course supports a recessed fighting platform.
    ggMeshArchitectureBlock(m,x0-11,y0-11,x1+11,y1+11,wallTop-8,wallTop-5,143,141,127);
    ggMeshArchitectureBlock(m,x0-22,y0-22,x1+22,y1+22,wallTop-5,cap,184,181,159);
    int l=x0-18,r=x1+18,t=y0-18,b=y1+18;
    ggMeshArchitectureBlock(m,l,t,l+12,b,cap,cap+4,157,155,141);
    ggMeshArchitectureBlock(m,r-12,t,r,b,cap,cap+4,157,155,141);
    ggMeshArchitectureBlock(m,l+12,t,r-12,t+12,cap,cap+4,157,155,141);
    ggMeshArchitectureBlock(m,l+12,b-12,r-12,b,cap,cap+4,157,155,141);
    for(int i=0;i<2;i++)for(int j=0;j<2;j++){
        int x=i?r-26:l,y=j?b-25:t;
        ggMeshArchitectureBlock(m,x,y,x+26,y+25,cap+4,cap+10,178,174,151);
    }
    if(!nearDetail)return;
    int cx=(x0+x1)/2,cy=(y0+y1)/2;
    ggMeshArchitectureBlock(m,cx-13,t,cx+13,t+12,cap+4,cap+10,172,168,148);
    ggMeshArchitectureBlock(m,cx-13,b-12,cx+13,b,cap+4,cap+10,172,168,148);
    ggMeshArchitectureBlock(m,l,cy-13,l+12,cy+13,cap+4,cap+10,172,168,148);
    ggMeshArchitectureBlock(m,r-12,cy-13,r,cy+13,cap+4,cap+10,172,168,148);
}
static void ggMeshBrokenWall(GgMeshContext*m,int axis,int p0,int p1,
                             int u0,int u1,int bottom,int top,
                             int r,int g,int b){
    int span=u1-u0;
    int u[6]={u0,u0+span/5,u0+span*2/5,u0+span*3/5,u0+span*4/5,u1};
    int z[6]={top-4,top+1,top-1,top+3,top-2,top-6};
    int across=axis?m->cx:m->cy,normal=axis?m->cy:m->cx;
    for(int i=0;i<5;i++){
        GgMeshVertex a=ggMeshFacadePoint(axis,p0,u[i],z[i]);
        GgMeshVertex b0=ggMeshFacadePoint(axis,p0,u[i+1],z[i+1]);
        GgMeshVertex c=ggMeshFacadePoint(axis,p1,u[i+1],z[i+1]);
        GgMeshVertex d=ggMeshFacadePoint(axis,p1,u[i],z[i]);
        if(axis)ggMeshQuad(m,a,b0,c,d,r+24,g+22,b+18);
        else ggMeshQuad(m,a,d,c,b0,r+24,g+22,b+18);
        GgMeshVertex lowA=a,lowB=b0,lowC=c,lowD=d;
        lowA.z=lowB.z=lowC.z=lowD.z=bottom;
        if(axis){
            if(normal<=p0)ggMeshQuad(m,lowA,lowB,b0,a,r,g,b);
            if(normal>=p1)ggMeshQuad(m,lowC,lowD,d,c,r-9,g-8,b-7);
            if(i==0&&across<=u0)ggMeshQuad(m,lowD,lowA,a,d,r-3,g-3,b-3);
            if(i==4&&across>=u1)ggMeshQuad(m,lowB,lowC,c,b0,r-3,g-3,b-3);
        }else{
            if(normal<=p0)ggMeshQuad(m,lowB,lowA,a,b0,r,g,b);
            if(normal>=p1)ggMeshQuad(m,lowD,lowC,c,d,r-9,g-8,b-7);
            if(i==0&&across<=u0)ggMeshQuad(m,lowA,lowD,d,a,r-3,g-3,b-3);
            if(i==4&&across>=u1)ggMeshQuad(m,lowC,lowB,b0,c,r-3,g-3,b-3);
        }
    }
}
static void ggMeshRuin(GgMeshContext*m,int x0,int y0,int x1,int y1,
                       int ground,int wallTop,int base,int nearDetail){
    int cy=(y0+y1)/2;
    ggMeshBrokenWall(m,0,x0,x0+24,y0,cy-34,base,wallTop+2,125,126,114);
    ggMeshBrokenWall(m,0,x0,x0+27,cy+42,y1-17,base,wallTop-2,119,120,108);
    ggMeshBrokenWall(m,1,y1-25,y1,x0+11,x1-15,base,wallTop+4,115,118,109);
    ggMeshArchitectureBlock(m,x1-32,y0,x1,y0+31,base,wallTop+6,136,132,118);
    ggMeshArchitectureBlock(m,x1-30,y1-33,x1,y1,base,wallTop,124,125,116);
    if(nearDetail){
        ggMeshArchitectureSupport(m,x0+12,cy-13,x0+33,cy+5,ground+3,135,128,115);
        ggMeshArchitectureSupport(m,x0-17,cy+18,x0+6,cy+36,ground+2,122,121,109);
        ggMeshArchitectureSupport(m,x1-55,y0+8,x1-36,y0+25,ground+4,151,145,128);
    }
}
static void ggMeshStatue(GgMeshContext*m,int x,int y,int base,int ground){
    ggMeshArchitectureBlock(m,x-67,y-62,x+67,y+62,base,ground+4,135,131,118);
    ggMeshArchitectureBlock(m,x-53,y-49,x+53,y+49,ground+4,ground+8,176,165,142);
    ggMeshArchitectureBlock(m,x-39,y-39,x+39,y+39,ground+8,ground+17,158,148,130);
    ggMeshArchitectureBlock(m,x-44,y-44,x+44,y+44,ground+17,ground+19,184,169,144);
    ggMeshArchitectureBlock(m,x-15,y-18,x+14,y-4,ground+19,ground+25,156,163,144);
    ggMeshArchitectureBlock(m,x-16,y+4,x+13,y+18,ground+19,ground+25,156,163,144);
    ggMeshArchitectureBlock(m,x-16,y-22,x+18,y+22,ground+24,ground+33,169,176,152);
    ggMeshArchitectureBlock(m,x-13,y-14,x+13,y+14,ground+33,ground+39,182,193,165);
    ggMeshArchitectureBlock(m,x-23,y+18,x-14,y+39,ground+27,ground+31,173,181,156);
}
static void ggMeshBorderMarker(GgMeshContext*m,int x,int y,int ground){
    int prior=m->material;
    m->material=GG_MAT_STONE;
    ggMeshArchitectureSupport(m,x-13,y-13,x+13,y+13,ground+2,142,134,113);
    m->material=GG_MAT_WOOD;
    ggMeshArchitectureBlock(m,x-5,y-5,x+5,y+5,ground,ground+16,99,72,48);
    m->material=GG_MAT_FABRIC;
    GgMeshVertex a=ggMeshV(x,y,ground+15),b=ggMeshV(x+4,y+22,ground+15);
    GgMeshVertex c=ggMeshV(x+4,y+22,ground+11),d=ggMeshV(x,y,ground+11);
    ggMeshQuad(m,a,b,c,d,219,178,91);
    ggMeshTriangle(m,b,ggMeshV(x-2,y+39,ground+12),c,201,153,69);
    m->material=prior;
}
#endif

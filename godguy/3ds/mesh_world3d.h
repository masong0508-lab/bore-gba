// Real mesh-based GodGuy world. Each visible tile contributes physical
// 3D ground triangles, architecture and low-poly vegetation to the same
// z-tested perspective camera. No second simulation, texture atlas,
// allocation, full RGB buffer or arbitrary floating 2D scenery stickers.
// Old 3DS draws 200x120 polygons at 2x; New 3DS cinematic draws 400x240.
// Requires explore3d.h's world, ground, trig, palette and scene definitions.
#ifndef GG_MESH_WORLD3D_H
#define GG_MESH_WORLD3D_H
static int ggMeshTilesDrawn,ggMeshBuildingsDrawn,ggMeshPropsDrawn;
static inline int ggMeshTerrainAt(int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH)return T_DIRT;
    int v=world[y][x];return (unsigned)v<T_N?v:T_DIRT;
}
static inline int ggMeshCornerHeight(int x,int y){
    int sum=0,n=0;
    for(int oy=-1;oy<=0;oy++)
        for(int ox=-1;ox<=0;ox++){
            if((unsigned)(x+ox)<WW&&(unsigned)(y+oy)<WH){
                sum+=ggExpGround(ggMeshTerrainAt(x+ox,y+oy));n++;
            }
        }
    return n?sum/n:8;
}
// Exact height on the four triangle sectors around a world-cell
// center. Road markings, plants, and later actor contacts can use this
// same surface instead of hovering at a whole-tile average altitude.
static inline int ggMeshGroundAtQ8(int wx,int wy){
    int tx=wx>>8,ty=wy>>8;
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 8;
    int a=ggMeshCornerHeight(tx,ty);
    int b=ggMeshCornerHeight(tx+1,ty);
    int cc=ggMeshCornerHeight(tx+1,ty+1);
    int d=ggMeshCornerHeight(tx,ty+1);
    int center=ggExpGround(ggMeshTerrainAt(tx,ty));
    int u=(wx&255)-128,v=(wy&255)-128;
    int au=u<0?-u:u,av=v<0?-v:v;
    if(au>=av){
        if(u>=0)
            return (center*(128-u)*2+
                    b*(u-v)+cc*(u+v)+128)/256;
        int n=-u;
        return (center*(128-n)*2+
                a*(n-v)+d*(n+v)+128)/256;
    }
    if(v>=0)
        return (center*(128-v)*2+
                d*(v-u)+cc*(v+u)+128)/256;
    int n=-v;
    return (center*(128-n)*2+
            a*(n-u)+b*(n+u)+128)/256;
}
// Anchor the foundation to the lowest physical ground under its
// footprint. A building keeps its intended roof height but the walls
// extend into a sloped site rather than hovering above the downhill edge.
// Four corner samples, no new geometry, allocations or persistent state.
static inline int ggMeshFoundationGround(int x0,int y0,int x1,int y1,
                                          int nominal){
    int base=nominal,z;
    z=ggMeshGroundAtQ8(x0,y0);if(z<base)base=z;
    z=ggMeshGroundAtQ8(x1,y0);if(z<base)base=z;
    z=ggMeshGroundAtQ8(x1,y1);if(z<base)base=z;
    z=ggMeshGroundAtQ8(x0,y1);if(z<base)base=z;
    return base-1; // bury the final one-unit seam under the terrain
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
static inline int ggMeshShade(int c,int light){
    return ggMeshClamp(c*light/256,0,255);
}
// Small world-space plants, cobbles and weathering instead of a
// monolithic triangle carpet. Only nearby native-detail tiles get these
// facets; distant / Old 3DS tiles retain the original four triangles.
static void ggMeshGroundLife(GgMeshContext*m,int tx,int ty,int type,
                             int ground,uint32_t key){
    if(type<=T_WATER||type==T_LAVA)return;
    for(int i=0;i<3;i++){
        int sx=tx*256+36+(int)((key>>(i*5))%184u);
        int sy=ty*256+32+(int)((key>>(i*7+2))%192u);
        int z=ggMeshGroundAtQ8(sx,sy)+1;
        if(type==T_GRASS||type==T_FOREST||type==T_JUNGLE||
           type==T_SWAMP){
            int height=type==T_JUNGLE?3:1+(int)((key>>(i*3))&1u);
            ggMeshTriangle(m,
                ggMeshV(sx-8,sy-4,
                        ggMeshGroundAtQ8(sx-8,sy-4)+1),
                ggMeshV(sx+7,sy+4,
                        ggMeshGroundAtQ8(sx+7,sy+4)+1),
                ggMeshV(sx+3,sy-3,z+height),
                84+(i*7),121+(i*5),67+(i*3));
            if((key>>(i*6))&1u)
                ggMeshTriangle(m,
                    ggMeshV(sx-3,sy+7,
                            ggMeshGroundAtQ8(sx-3,sy+7)+1),
                    ggMeshV(sx+6,sy-4,
                            ggMeshGroundAtQ8(sx+6,sy-4)+1),
                    ggMeshV(sx-4,sy+2,z+height),92,130,73);
            if(type==T_GRASS&&(key&31u)==(unsigned)i){
                // Rare low wildflowers, scaled to ankles rather than
                // the former person-sized triangular grass blades.
                ggMeshTriangle(m,ggMeshV(sx-5,sy-2,z+2),
                    ggMeshV(sx+5,sy-2,z+2),
                    ggMeshV(sx,sy+3,z+3),206,175,103);
            }
        }else if(type==T_SAND||type==T_DESERT){
            ggMeshQuad(m,ggMeshV(sx-28,sy-5,z),
                ggMeshV(sx+28,sy-5,z),
                ggMeshV(sx+26,sy+2,z),
                ggMeshV(sx-25,sy+2,z),164,140,91);
        }else if(type==T_SNOW){
            ggMeshTriangle(m,ggMeshV(sx-24,sy-12,z),
                ggMeshV(sx+23,sy-10,z),
                ggMeshV(sx+7,sy+15,z),205,217,222);
        }else if(type==T_MOUNT||type==T_HILL){
            ggMeshTriangle(m,ggMeshV(sx-22,sy-15,z),
                ggMeshV(sx+20,sy-13,z),
                ggMeshV(sx,sy+14,z+2),102,112,109);
        }else if(type==T_DIRT){
            ggMeshQuad(m,ggMeshV(sx-14,sy-12,z),
                ggMeshV(sx+19,sy-12,z),
                ggMeshV(sx+19,sy+7,z),
                ggMeshV(sx-14,sy+7,z),118,100,77);
        }
    }
}
static inline int ggMeshGroundMaterial(int type){
    switch(type){
    case T_GRASS:case T_FOREST:case T_JUNGLE:case T_SWAMP:return GG_MAT_MEADOW;
    case T_SAND:case T_DESERT:return GG_MAT_SAND;
    case T_SNOW:return GG_MAT_SNOW;
    case T_MOUNT:case T_HILL:return GG_MAT_ROCK;
    case T_DEEP:case T_WATER:return GG_MAT_WATER;
    case T_DIRT:return GG_MAT_STONE;
    default:return GG_MAT_ROCK;
    }
}
// Narrow shoreline bands use the same triangular physical surface as
// the water and land. Their two sampled ends follow a sloping bank;
// there is no flat decal hovering above a coast, or screen-space foam.
static void ggMeshShore(GgMeshContext*m,int tx,int ty,int side,int foam){
    int left=tx*256,top=ty*256;
    int keep=m->material,opacity=m->opacity;
    m->material=GG_MAT_NONE;
    for(int band=0;band<2;band++){
        int inset=foam?5+band*12:8+band*17;
        int width=foam?5:14;
        int ax=left+inset,ay=top+14,bx=ax+width,by=top+242;
        if(side==1){ax=left+256-inset-width;bx=ax+width;}
        if(side>=2){
            ax=left+14;bx=left+242;
            ay=top+(side==2?inset:256-inset-width);by=ay+width;
        }
        m->opacity=foam?(band?42:105):(band?35:67);
        int r=foam?197:101,g=foam?218:125,b=foam?209:103;
        ggMeshQuad(m,
            ggMeshV(ax,ay,ggMeshGroundAtQ8(ax,ay)+1),
            ggMeshV(bx,ay,ggMeshGroundAtQ8(bx,ay)+1),
            ggMeshV(bx,by,ggMeshGroundAtQ8(bx,by)+1),
            ggMeshV(ax,by,ggMeshGroundAtQ8(ax,by)+1),r,g,b);
    }
    m->material=keep;m->opacity=opacity;
}
static void ggMeshGroundTile(GgMeshContext*m,int tx,int ty,
                              int type,uint32_t key){
    int prior=m->material,priorSeed=m->materialSeed;
    m->material=ggMeshGroundMaterial(type);
    // One material field per biome; using the cell hash here made
    // neighbouring meadow tiles visibly reset their colour and grain.
    m->materialSeed=type*733+17;
    int left=tx*256,right=left+256,top=ty*256,bottom=top+256;
    int center=ggExpGround(type);
    int a=ggMeshCornerHeight(tx,ty);
    int b=ggMeshCornerHeight(tx+1,ty);
    int c=ggMeshCornerHeight(tx+1,ty+1);
    int d=ggMeshCornerHeight(tx,ty+1);
    GgMeshVertex V=ggMeshV(left+128,top+128,center);
    GgMeshVertex corners[4]={
        ggMeshV(left,top,a),ggMeshV(right,top,b),
        ggMeshV(right,bottom,c),ggMeshV(left,bottom,d)
    };
    int r=(terrCol[type][0]*8*4+72)/5;
    int g=(terrCol[type][1]*8*3+71)/4;
    int bl=(terrCol[type][2]*8*4+91)/5;
    if(type==T_GRASS||type==T_FOREST||type==T_JUNGLE){
        g=(g*4+r)/5;bl=(bl*3+g)/4;
    }
    for(int i=0;i<4;i++){
        // Sunlight is already evaluated from each actual 3D normal by
        // the rasterizer. A second four-sector shade creates a pinwheel
        // on perfectly flat ground and makes every cell a fake pyramid.
        ggMeshTriangle(m,V,corners[i],corners[(i+1)&3],
            r,g,bl);
    }
    if(type==T_WATER||type==T_DEEP){
        int ripple=(int)((key>>17)&3u);
        int priorOpacity=m->opacity;
        m->opacity=80;
        ggMeshQuad(m,ggMeshV(left+28,top+64+24*ripple,center+1),
                   ggMeshV(left+109,top+64+24*ripple,center+1),
                   ggMeshV(left+105,top+68+24*ripple,center+1),
                   ggMeshV(left+26,top+68+24*ripple,center+1),
                   147,188,196);
        m->opacity=priorOpacity;
    }
    if(m->detailQuality>=2){
        static const signed char dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
        for(int side=0;side<4;side++){
            int neighbor=ggMeshTerrainAt(tx+dx[side],ty+dy[side]);
            if(type==T_WATER&&neighbor>T_WATER)
                ggMeshShore(m,tx,ty,side,1);
            else if(type>T_WATER&&type!=T_LAVA&&neighbor==T_WATER)
                ggMeshShore(m,tx,ty,side,0);
        }
    }
    m->material=prior;m->materialSeed=priorSeed;
    ggMeshTilesDrawn++;
}
static void ggMeshRoof(GgMeshContext*m,int build,
                       int x0,int y0,int x1,int y1,int wallTop){
    int prior=m->material;
    m->material=GG_MAT_ROOF;
    int ridgeY=(y0+y1)/2;
    int roofRise=build==B_TEMPLE?9:
                 build==B_HALL?12:build==B_TOWER?3:11;
    int roofR=build==B_HOUSE||build==B_HUT?157:
              build==B_TEMPLE?177:78;
    int roofG=build==B_HOUSE||build==B_HUT?89:
              build==B_TEMPLE?135:108;
    int roofB=build==B_HOUSE||build==B_HUT?65:
              build==B_TEMPLE?95:121;
    ggMeshQuad(m,ggMeshV(x0-10,y0-10,wallTop),
               ggMeshV(x1+10,y0-10,wallTop),
               ggMeshV(x1+10,ridgeY,wallTop+roofRise),
               ggMeshV(x0-10,ridgeY,wallTop+roofRise),
               roofR,roofG,roofB);
    ggMeshQuad(m,ggMeshV(x0-10,ridgeY,wallTop+roofRise),
               ggMeshV(x1+10,ridgeY,wallTop+roofRise),
               ggMeshV(x1+10,y1+10,wallTop),
               ggMeshV(x0-10,y1+10,wallTop),
               roofR*3/4,roofG*3/4,roofB*3/4);
    // Narrow courses sit on the actual roof slope. Their far edge has
    // its own height rather than a horizontal hovering strip.
    if(m->detailQuality>=2&&(build==B_HOUSE||build==B_HUT||
       build==B_HALL||build==B_LIBRARY)){
        for(int row=1;row<=3;row++){
            int yy=y0+(ridgeY-y0)*row/4;
            int z=wallTop+roofRise*(yy-3-y0)/(ridgeY-y0)+1;
            int z2=wallTop+roofRise*(yy+3-y0)/(ridgeY-y0)+1;
            ggMeshQuad(m,ggMeshV(x0-10,yy-3,z),
                ggMeshV(x1+10,yy-3,z),
                ggMeshV(x1+10,yy+3,z2),
                ggMeshV(x0-10,yy+3,z2),
                roofR*4/5,roofG*4/5,roofB*4/5);
            yy=y1-(y1-ridgeY)*row/4;
            ggMeshQuad(m,ggMeshV(x0-10,yy-3,z2),
                ggMeshV(x1+10,yy-3,z2),
                ggMeshV(x1+10,yy+3,z),
                ggMeshV(x0-10,yy+3,z),
                roofR*3/5,roofG*3/5,roofB*3/5);
        }
    }
    // Actual closed gables, with triangular geometry rather than a
    // 2D roof sticker floating above an otherwise flat cube.
    ggMeshTriangle(m,ggMeshV(x0,y0,wallTop),
                   ggMeshV(x0,y1,wallTop),
                   ggMeshV(x0,ridgeY,wallTop+roofRise),
                   188,171,143);
    ggMeshTriangle(m,ggMeshV(x1,y0,wallTop),
                   ggMeshV(x1,y1,wallTop),
                   ggMeshV(x1,ridgeY,wallTop+roofRise),
                   151,150,143);
    // A strong ridge, and a thin verge on the two actual gable faces,
    // turn the previous unadorned triangular cap into a legible roof.
    m->material=GG_MAT_WOOD;
    ggMeshBox(m,x0-12,ridgeY-4,x1+12,ridgeY+4,
              wallTop+roofRise,wallTop+roofRise+1,
              roofR*4/5,roofG*4/5,roofB*4/5);
    for(int end=0;end<2;end++){
        int xx=end?x1+2:x0-2;
        ggMeshQuad(m,ggMeshV(xx,y0,wallTop),
            ggMeshV(xx,y0+7,wallTop),
            ggMeshV(xx,ridgeY+4,wallTop+roofRise),
            ggMeshV(xx,ridgeY-4,wallTop+roofRise),87,73,59);
        ggMeshQuad(m,ggMeshV(xx,ridgeY-4,wallTop+roofRise),
            ggMeshV(xx,ridgeY+4,wallTop+roofRise),
            ggMeshV(xx,y1-7,wallTop),
            ggMeshV(xx,y1,wallTop),87,73,59);
    }
    m->material=prior;
}
static void ggMeshWindow(GgMeshContext*m,int x0,int x1,
                          int frontY,int facing,int ground,int height){
    if(height<15)return;
    int oldBias=m->depthBias,oldMat=m->material;
    m->depthBias=1;m->material=GG_MAT_NONE;
    int y=frontY-facing*3;
    int winBot=ground+height*2/5,winTop=winBot+7;
    int cx=(x0+x1)/2;
    int half=(x1-x0)/2;
    // Black recessed door plus narrower framed amber windows. Their
    // vertices are physically ahead of the wall: no shader overlays.
    ggMeshQuad(m,ggMeshV(cx-21,y,ground+1),
               ggMeshV(cx+21,y,ground+1),
               ggMeshV(cx+21,y,ground+10),
               ggMeshV(cx-21,y,ground+10),42,45,53);
    for(int side=-1;side<=1;side+=2){
        int center=cx+side*half*2/3;
        int a=center-20,b=center+20;
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
    if(height<13)return;
    int oldBias=m->depthBias,oldMat=m->material;
    m->depthBias=1;m->material=GG_MAT_NONE;
    int face=wallX-facing*3,pane=face-facing*3;
    int cy=(y0+y1)/2;
    int low=ground+height*2/5,high=low+8;
    ggMeshQuad(m,ggMeshV(face,cy-22,ground+1),
        ggMeshV(face,cy+22,ground+1),
        ggMeshV(face,cy+22,ground+10),
        ggMeshV(face,cy-22,ground+10),46,51,58);
    for(int n=-1;n<=1;n+=2){
        int q=cy+n*(y1-y0)/3;
        ggMeshQuad(m,ggMeshV(face,q-23,low-1),
            ggMeshV(face,q+23,low-1),
            ggMeshV(face,q+23,high+1),
            ggMeshV(face,q-23,high+1),80,77,74);
        ggMeshQuad(m,ggMeshV(pane,q-19,low),
            ggMeshV(pane,q+19,low),
            ggMeshV(pane,q+19,high),
            ggMeshV(pane,q-19,high),90,132,145);
        ggMeshQuad(m,ggMeshV(pane-facing,q-2,low),
            ggMeshV(pane-facing,q+2,low),
            ggMeshV(pane-facing,q+2,high),
            ggMeshV(pane-facing,q-2,high),181,162,126);
        ggMeshQuad(m,ggMeshV(pane-facing*2,q-24,low-1),
            ggMeshV(pane-facing*2,q+24,low-1),
            ggMeshV(pane-facing*2,q+24,low),
            ggMeshV(pane-facing*2,q-24,low),197,182,151);
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
    int lower=ground+3,upper=ground+height-3;
    int windowLow=ground+height*2/5;
    for(int z=lower;z<upper;z+=spacing){
        if(z>=windowLow-2&&z<=windowLow+10)continue;
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
                                   int y1,int ground){
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
    (void)ground;
}
// Real 3D world-contact penumbra, not a painted 2D oval billboard.
// Two small translucent polygons are projected onto the exact live
// sloping terrain. The depth buffer hides them when architecture covers
// the same footprint. No shadow-map framebuffer or texture allocation.
static void ggMeshContactShadow(GgMeshContext*m,int cx,int cy,
                                 int radius,int height){
    if(m->scale>1||radius<10)return;
    int keepMat=m->material,keepOpacity=m->opacity;
    m->material=GG_MAT_NONE;
    for(int ring=0;ring<2;ring++){
        int wide=radius+(ring?0:28);
        int centerX=cx-height*2;
        int centerY=cy+height;
        int centerZ=ggMeshGroundAtQ8(centerX,centerY)+2;
        m->opacity=ring?75:28;
        for(int n=0;n<8;n++){
            int a=n*32,b=((n+1)*32)&255;
            int ax=centerX+ggExpCos[a]*wide/4096,
                ay=centerY+ggExpSin[a]*(wide*2/3)/4096;
            int bx=centerX+ggExpCos[b]*wide/4096,
                by=centerY+ggExpSin[b]*(wide*2/3)/4096;
            ggMeshTriangle(m,ggMeshV(centerX,centerY,centerZ),
                ggMeshV(ax,ay,ggMeshGroundAtQ8(ax,ay)+2),
                ggMeshV(bx,by,ggMeshGroundAtQ8(bx,by)+2),
                25,32,36);
        }
    }
    m->opacity=keepOpacity;m->material=keepMat;
}
static void ggMeshBuilding(GgMeshContext*m,int tx,int ty,int build,
                            int ground,int nearDetail){
    int x0=tx*256+28,x1=tx*256+228;
    int y0=ty*256+28,y1=ty*256+228;
    // Building type changes actual footprint/volume, not merely a
    // pixel tint: small huts, strong towers, open ruins and temples
    // remain distinguishable even when viewed with flat materials.
    if(build==B_HUT){x0+=35;x1-=35;y0+=30;y1-=30;}
    if(build==B_TOWER){x0+=39;x1-=39;y0+=38;y1-=38;}
    if(build==B_FARM){y0+=33;y1-=35;}
    if(build==B_DOCK){x0+=28;x1-=28;}
    if(build==B_STATUE){x0+=42;x1-=42;y0+=38;y1-=38;}
    int baseGround=ggMeshFoundationGround(x0-14,y0-14,x1+14,y1+14,ground);
    int wallTop=ground+ggExpBuilding(build);
    int wood=build==B_HOUSE||build==B_HUT||
             build==B_DOCK||build==B_FARM;
    int stone=build==B_TEMPLE||build==B_HALL||
              build==B_TOWER||build==B_LIBRARY;
    int r=wood?193:stone?181:126,g=wood?177:stone?177:128,
        bl=wood?143:stone?162:117;
    if(build==B_HUT){r=171;g=143;bl=104;}
    if(build==B_TEMPLE){r=209;g=197;bl=174;}
    if(build==B_LIBRARY){r=173;g=187;bl=174;}
    if(build==B_RUIN){r=107;g=109;bl=108;}
    if(build==B_STATUE){r=156;g=152;bl=140;}
    if(build==B_STATUE){
        // Stone plinth, tapering waist and an actual angular sculpture
        // create a readable monument instead of an enormous solid cube.
        ggMeshBox(m,x0-14,y0-14,x1+14,y1+14,
                  baseGround,ground+6,134,134,127);
        ggMeshBox(m,x0+8,y0+8,x1-8,y1-8,
                  ground+6,ground+13,174,166,148);
        int cx=(x0+x1)/2,cy=(y0+y1)/2;
        ggMeshTriangle(m,ggMeshV(x0+8,y0+8,ground+13),
            ggMeshV(x1-8,y0+8,ground+13),
            ggMeshV(cx,cy,wallTop+3),192,181,159);
        ggMeshTriangle(m,ggMeshV(x1-8,y0+8,ground+13),
            ggMeshV(x1-8,y1-8,ground+13),
            ggMeshV(cx,cy,wallTop+3),143,138,135);
        ggMeshTriangle(m,ggMeshV(x1-8,y1-8,ground+13),
            ggMeshV(x0+8,y1-8,ground+13),
            ggMeshV(cx,cy,wallTop+3),127,126,124);
        ggMeshTriangle(m,ggMeshV(x0+8,y1-8,ground+13),
            ggMeshV(x0+8,y0+8,ground+13),
            ggMeshV(cx,cy,wallTop+3),164,159,145);
        ggMeshBuildingsDrawn++;return;
    }
    if(build==B_RUIN){
        // Deliberately incomplete, hollow ruin. Walking past the wall
        // reveals true spatial gaps instead of a tinted complete house.
        ggMeshBox(m,x0,y0,x0+23,y1,baseGround,wallTop,
                  130,129,121);
        ggMeshBox(m,x0,y1-24,x1,y1,baseGround,wallTop-4,
                  104,108,104);
        ggMeshBox(m,x1-30,y0,x1,y0+27,baseGround,wallTop+5,
                  148,145,132);
        ggMeshBox(m,x1-35,y1-29,x1,y1,baseGround,wallTop-5,
                  112,112,108);
        ggMeshBuildingsDrawn++;return;
    }
    ggMeshBox(m,x0,y0,x1,y1,baseGround,wallTop,r,g,bl);
    if(build==B_HOUSE||build==B_HUT||build==B_HALL||
       build==B_TEMPLE||build==B_LIBRARY||build==B_FARM)
        ggMeshRoof(m,build,x0,y0,x1,y1,wallTop);
    if(build==B_DOCK){
        // Pier planks and offset pilings, still part of a real world
        // tile, so coast settlements have space rather than a stone
        // high-rise sitting on the waterline.
        for(int i=0;i<4;i++){
            int x=x0+18+i*(x1-x0-36)/3;
            ggMeshBox(m,x-7,y0-14,x+7,y0+14,
                      ground-3,ground+4,115,91,64);
        }
        ggMeshBox(m,x0-10,y0-28,x1+10,y1+18,
                  ground+5,ground+7,153,119,81);
    }
    if(build==B_TOWER){
        // Four real corner parapets, not a one-pixel tower trim stripe.
        int h=wallTop;
        ggMeshBox(m,x0,y0,x0+34,y0+34,h,h+5,164,163,154);
        ggMeshBox(m,x1-34,y0,x1,y0+34,h,h+5,164,163,154);
        ggMeshBox(m,x0,y1-34,x0+34,y1,h,h+5,164,163,154);
        ggMeshBox(m,x1-34,y1-34,x1,y1,h,h+5,164,163,154);
    }
    int h=wallTop-ground;
    // Even the medium distance silhouette keeps its entrances and
    // windows. The old LOD switched whole buildings into blank cubes.
    if(build!=B_DOCK&&build!=B_FARM){
        if(m->cy<=y0)ggMeshWindow(m,x0,x1,y0,1,ground,h);
        if(m->cy>=y1)ggMeshWindow(m,x0,x1,y1,-1,ground,h);
        if(m->cx<=x0)ggMeshSideFacade(m,x0,1,y0,y1,ground,h);
        if(m->cx>=x1)ggMeshSideFacade(m,x1,-1,y0,y1,ground,h);
    }
    if(nearDetail){
        // Ground-contact plinth and roof cornice extend beyond every
        // wall face, showing real parallax, height and shade.
        ggMeshBox(m,x0-5,y0-5,x1+5,y1+5,
                  baseGround,ground+2,107,105,98);
        ggMeshBox(m,x0-6,y0-6,x1+6,y1+6,
                  wallTop-3,wallTop-1,177,164,137);
        ggMeshWallFinish(m,x0,y0,x1,y1,ground,h,wood);
        if(m->cy<=y0)ggMeshFacadeDetail(m,y0-7,1,x0,x1,ground,h,wood);
        if(m->cy>=y1)ggMeshFacadeDetail(m,y1+7,1,x0,x1,ground,h,wood);
        if(m->cx<=x0)ggMeshFacadeDetail(m,x0-7,0,y0,y1,ground,h,wood);
        if(m->cx>=x1)ggMeshFacadeDetail(m,x1+7,0,y0,y1,ground,h,wood);
        if(build==B_HOUSE||build==B_HUT||build==B_HALL||
           build==B_TEMPLE||build==B_LIBRARY)
            ggMeshBuildingLanding(m,x0,y0,y1,ground);
        // Vertical corner quoins or timber beams connect floor to
        // eaves and keep every view from looking like a plain cube.
        int trimR=wood?102:181,trimG=wood?77:170,trimB=wood?58:149;
        ggMeshBox(m,x0-7,y0-7,x0+10,y0+10,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x1-10,y0-7,x1+7,y0+10,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x0-7,y1-10,x0+10,y1+7,baseGround,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x1-10,y1-10,x1+7,y1+7,baseGround,wallTop,
                  trimR,trimG,trimB);
        if(build==B_TEMPLE){
            // A broad stone stair procession toward the portico.
            for(int stair=0;stair<3;stair++){
                ggMeshBox(m,x0+20+stair*5,
                           y0-58+stair*13,
                           x1-20-stair*5,y0-45+stair*13,
                           ground,ground+2+stair,
                           160+stair*7,157+stair*4,146);
            }
        }
        if(build==B_FARM){
            // True depth barn braces and stocked crates by the door.
            ggMeshBox(m,x0+9,y0-20,x0+24,y0-2,
                      ground,wallTop+2,114,84,58);
            ggMeshBox(m,x1-24,y0-20,x1-9,y0-2,
                      ground,wallTop+2,114,84,58);
            ggMeshBox(m,x0+35,y0-38,x0+75,y0-13,
                      ground,ground+6,142,100,68);
        }
        if(build==B_TEMPLE||build==B_HALL){
            // Real colonnade projecting ahead of the wall, with an
            // enclosed upper lintel that receives perspective shading.
            for(int i=0;i<3;i++){
                int x=x0+43+i*57;
                ggMeshBox(m,x-8,y0-25,x+8,y0-10,
                          ground,ground+20,181,173,155);
            }
            ggMeshBox(m,x0+26,y0-30,x1-26,y0-6,
                      ground+20,ground+23,182,170,145);
        }
        if(build==B_HOUSE||build==B_HUT){
            // A small masonry chimney gives every cottage a readable
            // domestic roofline; it shares the building's fixed volume.
            int cx=x0+(x1-x0)*3/4,cy=(y0+y1)/2;
            int chimney=wallTop+(build==B_HUT?10:12);
            ggMeshBox(m,cx-13,cy-13,cx+13,cy+13,
                      wallTop+2,chimney+4,142,120,99);
            ggMeshBox(m,cx-17,cy-17,cx+17,cy+17,
                      chimney+3,chimney+5,186,161,126);
        }
    }
    ggMeshBuildingsDrawn++;
}
// Two irregular latitude rings give crowns a rounded, leafy silhouette.
// At native detail the oak has three overlapping crowns; distant trees
// keep one six-sided volume. The fixed prop queue still caps total work.
static void ggMeshLeafCrown(GgMeshContext*m,int x,int y,int base,
                             int radius,int height,uint32_t key,
                             int sides,int pine,int r,int g,int b){
    int lower=base+height/(pine?4:3),upper=base+height*(pine?3:2)/(pine?5:3);
    for(int n=0;n<sides;n++){
        int a=n*256/sides,c=((n+1)*256/sides)&255;
        int ra=radius*(93+(int)((key>>(n*3%22))&15u))/100;
        int rb=radius*(93+(int)((key>>(((n+1)*3)%22))&15u))/100;
        int ax=x+ggExpCos[a]*ra/4096,ay=y+ggExpSin[a]*ra/4096;
        int bx=x+ggExpCos[c]*rb/4096,by=y+ggExpSin[c]*rb/4096;
        int taper=pine?45:82;
        GgMeshVertex A=ggMeshV(ax,ay,lower),B=ggMeshV(bx,by,lower);
        GgMeshVertex C=ggMeshV(x+(bx-x)*taper/100,
                              y+(by-y)*taper/100,upper);
        GgMeshVertex D=ggMeshV(x+(ax-x)*taper/100,
                              y+(ay-y)*taper/100,upper);
        int light=(int)((key>>(n%18))&3u);
        ggMeshTriangle(m,ggMeshV(x+radius/9,y-radius/10,base+height),
                        D,C,r+7+light*2,g+11+light*2,b+3+light);
        ggMeshQuad(m,A,B,C,D,r+light*2,g+light*3,b+light);
        ggMeshTriangle(m,B,A,ggMeshV(x,y,base+height/8),
                        r+16,g+20,b+12);
    }
}
static void ggMeshFoliage(GgMeshContext*m,int x,int y,int ground,
                           int pine,uint32_t key,int quality){
    int keep=m->material;
    int trunkH=pine?13:17;
    m->material=GG_MAT_WOOD;
    ggMeshBox(m,x-10,y-10,x+10,y+10,ground,ground+trunkH,
              118,91,65);
    m->material=GG_MAT_LEAVES;
    int sway=ggFilmScenicWind(key,ggExplore.frame)*4;
    int sides=6;
    int hue=(int)((key>>15)&7u)-3;
    if(pine){
        for(int tier=0;tier<3;tier++)
            ggMeshLeafCrown(m,x+sway*(tier+1)/3,y,
                ground+8+tier*7,75-tier*17,19-tier,
                key+(uint32_t)tier*31u,sides,1,
                63+hue,103+hue,77+hue);
    }else{
        ggMeshLeafCrown(m,x+sway,y,ground+10,77,20,
                        key,sides,0,91+hue,130+hue,74+hue);
        if(quality>=2){
            int angle=(int)((key>>9)&255u);
            int dx=ggExpCos[angle]*49/4096;
            int dy=ggExpSin[angle]*49/4096;
            // Individual crown axes rotate with the tree identity,
            // preventing every grove from sharing one stamped shape.
            ggMeshLeafCrown(m,x-dx+sway,y-dy,ground+9,63,15,
                            key+733u,6,0,76+hue,119+hue,69+hue);
            ggMeshLeafCrown(m,x+dy+sway,y-dx,ground+10,62,16,
                            key+1471u,6,0,98+hue,137+hue,79+hue);
        }
    }
    m->material=keep;
}
static void ggMeshShrub(GgMeshContext*m,int x,int y,int ground,
                          uint32_t key){
    ggMeshLeafCrown(m,x,y,ground+1,44,9,key,6,0,77,115,65);
    // Small warm seed heads break up the otherwise uniform green.
    if((key&3u)==0u){
        ggMeshTriangle(m,ggMeshV(x-7,y-4,ground+10),
            ggMeshV(x+4,y-3,ground+10),
            ggMeshV(x,y+7,ground+11),191,169,102);
    }
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
    if(quality>=2&&
       (kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
        kind==GG_SCENE_SHRUB||kind==GG_SCENE_STALL||
        kind==GG_SCENE_CART))
        ggMeshContactShadow(m,x,y,
                             kind==GG_SCENE_SHRUB?30:
                             kind==GG_SCENE_STALL?55:70,
                             kind==GG_SCENE_PINE?19:13);
    int previous=m->material,previousSeed=m->materialSeed;
    m->materialSeed=(int)key;
    m->material=(kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
                 kind==GG_SCENE_SHRUB||kind==GG_SCENE_CROPS||
                 kind==GG_SCENE_GRASS||kind==GG_SCENE_REEDS)?
                GG_MAT_LEAVES:
                (kind==GG_SCENE_STONE||kind==GG_SCENE_COLUMN)?
                GG_MAT_STONE:
                kind==GG_SCENE_BANNER?GG_MAT_FABRIC:GG_MAT_WOOD;
    switch(kind){
    case GG_SCENE_OAK:case GG_SCENE_PINE:
        ggMeshFoliage(m,x,y,z,kind==GG_SCENE_PINE,key,quality);break;
    case GG_SCENE_SHRUB:
        ggMeshShrub(m,x,y,z,key);break;
    case GG_SCENE_COLUMN:
        ggMeshBox(m,x-15,y-15,x+15,y+15,z,z+16,144,141,131);
        ggMeshBox(m,x-23,y-23,x+23,y+23,z+16,z+19,156,153,138);
        break;
    case GG_SCENE_STONE:
        ggMeshBox(m,x-26,y-19,x+24,y+17,z,z+7,117,120,113);
        break;
    case GG_SCENE_LAMP:case GG_SCENE_BRAZIER:{
        int flicker=(int)((key+(uint32_t)(ggExplore.frame/9))&3u);
        ggMeshBox(m,x-5,y-5,x+5,y+5,z,z+12,89,86,74);
        ggMeshBox(m,x-13,y-13,x+13,y+13,z+12,z+15,
                  184+flicker*9,128+flicker*9,64);
        ggMeshTriangle(m,ggMeshV(x-11,y,z+15),
                       ggMeshV(x+11,y,z+15),
                       ggMeshV(x,y,z+19+flicker),250,187,83);
        break;
    }
    case GG_SCENE_BANNER:{
        int gust=ggFilmScenicWind(key,ggExplore.frame)*11;
        ggMeshBox(m,x-3,y-3,x+3,y+3,z,z+19,109,107,99);
        ggMeshTriangle(m,ggMeshV(x,y,z+17),
                       ggMeshV(x+64+gust,y+gust,z+16),
                       ggMeshV(x+6+gust/3,y,z+9),146,65,80);
        break;
    }
    case GG_SCENE_FENCE:
        ggMeshBox(m,x-60,y-6,x-49,y+6,z,z+9,116,84,58);
        ggMeshBox(m,x+49,y-6,x+60,y+6,z,z+9,116,84,58);
        ggMeshBox(m,x-55,y-4,x+55,y+4,z+4,z+6,146,108,75);
        break;
    case GG_SCENE_CART:case GG_SCENE_CRATES:
        ggMeshBox(m,x-44,y-32,x+44,y+32,z+2,z+9,150,106,71);
        if(kind==GG_SCENE_CART){
            ggMeshBox(m,x-36,y-35,x-16,y-27,z,z+3,52,54,59);
            ggMeshBox(m,x+16,y-35,x+36,y-27,z,z+3,52,54,59);
        }
        break;
    case GG_SCENE_STALL:
        ggMeshBox(m,x-52,y-33,x+52,y+33,z+9,z+13,149,81,74);
        ggMeshBox(m,x-49,y-30,x-42,y-23,z,z+9,109,82,59);
        ggMeshBox(m,x+42,y+23,x+49,y+30,z,z+9,109,82,59);
        break;
    case GG_SCENE_CROPS:case GG_SCENE_GRASS:case GG_SCENE_REEDS:
        for(int i=-1;i<=1;i++){
            int xx=x+i*19;
            int sway=ggFilmScenicWind(key+(uint32_t)i,
                                      ggExplore.frame)*3;
            int height=kind==GG_SCENE_REEDS?5:
                       kind==GG_SCENE_CROPS?3:2;
            int width=kind==GG_SCENE_REEDS?2:5;
            ggMeshTriangle(m,ggMeshV(xx-width,y,z),
                           ggMeshV(xx+width,y,z),
                           ggMeshV(xx+2+sway,y+2,z+height),
                           93,131,75);
            if(kind==GG_SCENE_REEDS)
                ggMeshTriangle(m,ggMeshV(xx+sway-2,y,z+height-1),
                    ggMeshV(xx+sway+3,y,z+height-1),
                    ggMeshV(xx+sway,y,z+height+1),139,109,72);
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
    // New 3DS gets genuine 400x240 geometry even in exploration.
    // Under frame-clock pressure (or on Old 3DS), the SAME renderer
    // drops to 200x120 and scales 2x without a new framebuffer.
    int quality=ggExpScenicQuality>=2?2:1;
    int cx=ggExplore.px>>8,cy=ggExplore.py>>8;
    int terrain=ggMeshTerrainAt(cx,cy);
    GgFilmScenicAtmosphere atmosphere=ggFilmScenicAtmosphere(terrain);
    // Use the exact triangular terrain beneath the camera. Nominal
    // tile height caused a visible vertical pop at slopes/biome borders.
    int eye=ggMeshGroundAtQ8(ggExplore.px,ggExplore.py)+13+
            (ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0);
    int angle=ggExplore.angle&255;
    GgMeshContext*m=&ggMeshContext;
    ggMeshBegin(m,out,quality,ggExplore.px,ggExplore.py,eye,
                ggExpCos[angle],ggExpSin[angle],
                49+atmosphere.skyR,78+atmosphere.skyG,
                108+atmosphere.skyB);
    ggMeshSkyWeather(m,ggExplore.frame,
        terrain==T_SAND||terrain==T_DESERT?0:
        terrain==T_SNOW?3:
        terrain==T_SWAMP||terrain==T_FOREST||terrain==T_JUNGLE?2:1);
    // Match the new bright horizon instead of fading distant buildings
    // into the previous dark grey-blue strip above the ground.
    m->fogR=ggMeshClamp(164+atmosphere.skyR/2,0,255);
    m->fogG=ggMeshClamp(182+atmosphere.skyG/2,0,255);
    m->fogB=ggMeshClamp(193+atmosphere.skyB/2,0,255);
    m->fogMax=(terrain==T_SWAMP||terrain==T_SNOW)?174:132;
    m->ambientLight=(terrain==T_SNOW||terrain==T_SAND||
                     terrain==T_DESERT)?236:
                    terrain==T_SWAMP||terrain==T_FOREST?198:218;
    ggMeshTilesDrawn=ggMeshBuildingsDrawn=ggMeshPropsDrawn=0;
    int radius=quality>=2?9:7;
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
            if(ggExpSceneryEnabled&&quality>=2&&forward>0&&forward<4*256)
                ggMeshGroundLife(m,tx,ty,t,ggExpGround(t),key);
            int build=bld[ty][tx];
            if(build){
                int detailed=forward<6*256&&forward>-128;
                int oldMaterial=m->material,oldSeed=m->materialSeed;
                m->material=(build==B_HOUSE||build==B_HUT||
                             build==B_DOCK||build==B_FARM)?
                             GG_MAT_WOOD:GG_MAT_STONE;
                m->materialSeed=(int)key;
                if(detailed&&forward>128)
                    ggMeshContactShadow(m,tx*256+128,
                                         ty*256+128,105,
                                         ggExpBuilding(build));
                ggMeshBuilding(m,tx,ty,build,ggExpGround(t),detailed);
                m->material=oldMaterial;m->materialSeed=oldSeed;
            }else if(ggExpSceneryEnabled){
                uint32_t scenic=ggFilmScenicHash(tx,ty,7);
                if(((scenic>>24)&7u)>=5u)continue;
                int near=0;
                if(tx>0&&bld[ty][tx-1])near=bld[ty][tx-1];
                else if(tx+1<WW&&bld[ty][tx+1])near=bld[ty][tx+1];
                else if(ty>0&&bld[ty-1][tx])near=bld[ty-1][tx];
                else if(ty+1<WH&&bld[ty+1][tx])near=bld[ty+1][tx];
                int kind=ggFilmScenicKind(t,near,scenic);
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

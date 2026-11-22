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
    int center=ggMeshGroundCenter(ggMeshTerrainAt(tx,ty));
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
        ggMeshTriangle(m,ggMeshV(sx-4,sy-4,ggMeshGroundAtQ8(sx-4,sy-4)-1),
            ggMeshV(sx+4,sy+3,ggMeshGroundAtQ8(sx+4,sy+3)-1),
            ggMeshV(sx+sway+dx/3,sy+dy/3,z+height),
            109-18*tall+i*4,145-10*tall+i*2,66+i*3);
        if(flower&&i<3){
            ggMeshTriangle(m,ggMeshV(sx-3+sway,sy,z+height),
                ggMeshV(sx+3+sway,sy-3,z+height+1),
                ggMeshV(sx+6+sway,sy+5,z+height+1),
                (key&4u)?225:199,(key&4u)?182:165,(key&4u)?100:179);
        }
    }
}
static GgMeshVertex ggMeshShorePoint(int tx,int ty,int side,
                                     int along,int inset,int water){
    int x=tx*256+(side<2?(side==0?inset:255-inset):along);
    int y=ty*256+(side>=2?(side==2?inset:255-inset):along);
    int z=water?ggMeshGroundCenter(T_WATER):ggMeshGroundAtQ8(x,y);
    return ggMeshV(x,y,z);
}
// Grounded dark wet sediment and broken moving lines of foam share
// the actual shoreline depth. World position fixes gaps in the foam,
// while wind advances the thin crest across the water.
static void ggMeshShore(GgMeshContext*m,int tx,int ty,int side,int water,
                        uint32_t key){
    int material=m->material,opacity=m->opacity,bias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=1;
    m->opacity=108;
    int r=water?190:92,g=water?224:102,b=water?213:77;
    int run=water?2:1;
    int surge=1+ggMatWave(ggExplore.frame/5,15);
    if(water){
        m->opacity=62;
        ggMeshQuad(m,ggMeshShorePoint(tx,ty,side,0,0,1),
            ggMeshShorePoint(tx,ty,side,255,0,1),
            ggMeshShorePoint(tx,ty,side,255,36,1),
            ggMeshShorePoint(tx,ty,side,0,36,1),93,169,153);
        m->opacity=91;
    }
    for(int piece=0;piece<run;piece++){
        int a=water?16+piece*116+(int)((key>>(piece*5))&15u):0;
        int c=water?a+52+(int)((key>>(piece*6+10))&47u):255;
        int inner=water?5+surge+piece*7:0;
        int outer=water?inner+5:20+(int)((key>>16)&15u);
        ggMeshQuad(m,ggMeshShorePoint(tx,ty,side,a,inner,water),
            ggMeshShorePoint(tx,ty,side,c,inner+(water?4:0),water),
            ggMeshShorePoint(tx,ty,side,c,outer+4,water),
            ggMeshShorePoint(tx,ty,side,a,outer,water),r,g,b);
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
    m->material=ggMeshGroundMaterial(type);
    // One material field per biome; using the cell hash here made
    // neighbouring meadow tiles visibly reset their colour and grain.
    m->materialSeed=(int)ggOutdoorGroundSeed(type,ggExplore.frame);
    m->groundUV=1;
    int left=tx*256,right=left+256,top=ty*256,bottom=top+256;
    int center=ggMeshGroundCenter(type);
    int a=ggMeshCornerHeight(tx,ty);
    int b=ggMeshCornerHeight(tx+1,ty);
    int c=ggMeshCornerHeight(tx+1,ty+1);
    int d=ggMeshCornerHeight(tx,ty+1);
    GgMeshVertex V=ggMeshV(left+128,top+128,center);
    GgMeshVertex corners[4]={
        ggMeshV(left,top,a),ggMeshV(right,top,b),
        ggMeshV(right,bottom,c),ggMeshV(left,bottom,d)
    };
    int r,g,bl;
    ggOutdoorGroundColor(type,&r,&g,&bl);
    for(int i=0;i<4;i++){
        // Sunlight is already evaluated from each actual 3D normal by
        // the rasterizer. A second four-sector shade creates a pinwheel
        // on perfectly flat ground and makes every cell a fake pyramid.
        ggMeshTriangle(m,V,corners[i],corners[(i+1)&3],
            r,g,bl);
    }
    if(type!=T_LAVA){
        int water=type<=T_WATER;
        unsigned shore=ggOutdoorShoreSides(tx,ty,water);
        for(int side=0;shore;side++,shore>>=1)
            if(shore&1u)ggMeshShore(m,tx,ty,side,water,key);
    }
    m->material=prior;m->materialSeed=priorSeed;m->groundUV=priorUV;
    m->near=priorNear;
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
              wallTop+roofRise-1,wallTop+roofRise+1,
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
static inline int ggMeshGroundCross(GgMeshVertex a,GgMeshVertex b,GgMeshVertex pt){
    return (b.x-a.x)*(pt.y-a.y)-(b.y-a.y)*(pt.x-a.x);
}
// Clip a shadow footprint to one of the same four sectors used by the
// ground renderer. Allow room for quantized 2D intersections.
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
        int x=tx*256,y=ty*256;
        GgMeshVertex vertex[4]={
            ggMeshV(x,y,ggMeshCornerHeight(tx,ty)),
            ggMeshV(x+256,y,ggMeshCornerHeight(tx+1,ty)),
            ggMeshV(x+256,y+256,ggMeshCornerHeight(tx+1,ty+1)),
            ggMeshV(x,y+256,ggMeshCornerHeight(tx,ty+1))
        };
        GgMeshVertex center=ggMeshV(x+128,y+128,ggMeshGroundCenter(ggMeshTerrainAt(tx,ty)));
        for(int i=0;i<4;i++){
            GgMeshVertex sector[3]={center,vertex[i],vertex[(i+1)&3]};
            ggMeshShadowSector(m,edge,sector);
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
static void ggMeshContactShadow(GgMeshContext*m,int cx,int cy,
                                 int radius,int height){
    if(m->scale>1||radius<10)return;
    int keepMat=m->material,keepOpacity=m->opacity,keepBias=m->depthBias;
    m->material=GG_MAT_NONE;m->depthBias=2;
    for(int ring=0;ring<2;ring++){
        int cast=ring?0:height*2;
        int wide=radius+(ring?0:cast+12);
        int centerX=cx+cast,centerY=cy-cast*3/5;
        int z=ggMeshGroundCenter(ggMeshTerrainAt(centerX>>8,centerY>>8));
        GgMeshVertex center=ggMeshV(centerX,centerY,z);
        GgMeshVertex edge[6];
        int left=centerX,right=centerX,top=centerY,bottom=centerY;
        m->opacity=ring?73:31;
        for(int n=0;n<6;n++){
            int a=n*256/6;
            int along=ggExpCos[a]*wide/4096;
            int across=ggExpSin[a]*(radius*3/4+3)/4096;
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
static void ggMeshDock(GgMeshContext*m,int tx,int ty,int ground,int detailed){
    unsigned shores=ggOutdoorShoreSides(tx,ty,0);
    int side=2;
    if(shores){
        side=0;
        while(!(shores&(1u<<side)))side++;
    }
    int dx=ggOutdoorSideX[side],dy=ggOutdoorSideY[side];
    int x=tx*256+128,y=ty*256+128;
    int end=shores?205:70;
    int deck=ground+3;
    for(int row=0;row<3;row++)for(int edge=-1;edge<=1;edge+=2){
        int along=-62+(end+42)*row/2;
        int px=x+along*dx+edge*48*dy;
        int py=y+along*dy+edge*48*dx;
        ggMeshGroundBox(m,px-8,py-8,px+8,py+8,deck,105,81,57);
        if(detailed&&row==2)
            ggMeshBox(m,px-9,py-9,px+9,py+9,deck,deck+4,121,93,67);
    }
    int start=-83;
    int endX=x+end*dx,startX=x+start*dx;
    int endY=y+end*dy,startY=y+start*dy;
    int x0=(endX<startX?endX:startX)-62*abs(dy);
    int x1=(endX>startX?endX:startX)+62*abs(dy);
    int y0=(endY<startY?endY:startY)-62*abs(dx);
    int y1=(endY>startY?endY:startY)+62*abs(dx);
    ggMeshBox(m,x0,y0,x1,y1,deck-2,deck,158,123,82);
    if(detailed){
        int previous=m->material,priorBias=m->depthBias;
        m->material=GG_MAT_NONE;m->depthBias=1;
        for(int along=start+14;along<end-7;along+=22){
            int px=x+along*dx,py=y+along*dy;
            ggMeshQuad(m,ggMeshV(px-61*dy,py-61*dx,deck),
                ggMeshV(px+61*dy,py+61*dx,deck),
                ggMeshV(px+61*dy+2*dx,py+61*dx+2*dy,deck),
                ggMeshV(px-61*dy+2*dx,py-61*dx+2*dy,deck),105,78,58);
        }
        m->material=previous;m->depthBias=priorBias;
    }
}
static void ggMeshBuilding(GgMeshContext*m,int tx,int ty,int build,
                            int ground,int nearDetail){
    if(build==B_DOCK){
        ggMeshDock(m,tx,ty,ground,nearDetail);
        ggMeshBuildingsDrawn++;
        return;
    }
    int x0=tx*256+28,x1=tx*256+228;
    int y0=ty*256+28,y1=ty*256+228;
    // Building type changes actual footprint/volume, not merely a
    // pixel tint: small huts, strong towers, open ruins and temples
    // remain distinguishable even when viewed with flat materials.
    if(build==B_HUT){x0+=35;x1-=35;y0+=30;y1-=30;}
    if(build==B_TOWER){x0+=39;x1-=39;y0+=38;y1-=38;}
    if(build==B_FARM){y0+=33;y1-=35;}
    if(build==B_STATUE){x0+=42;x1-=42;y0+=38;y1-=38;}
    int baseGround=ggMeshFoundationGround(x0-14,y0-14,x1+14,y1+14,ground);
    int wallTop=ground+ggExpBuilding(build);
    int wood=build==B_HOUSE||build==B_HUT||build==B_FARM;
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
    if(build!=B_FARM){
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
            ggMeshBuildingLanding(m,x0,y0,y1);
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
                ggMeshGroundBox(m,x0+20+stair*5,
                           y0-58+stair*13,
                           x1-20-stair*5,y0-45+stair*13,
                           ground+2+stair,
                           160+stair*7,157+stair*4,146);
            }
        }
        if(build==B_FARM){
            // True depth barn braces and stocked crates by the door.
            ggMeshGroundBox(m,x0+9,y0-20,x0+24,y0-2,wallTop+2,114,84,58);
            ggMeshGroundBox(m,x1-24,y0-20,x1-9,y0-2,wallTop+2,114,84,58);
            ggMeshGroundBox(m,x0+35,y0-38,x0+75,y0-13,ground+6,142,100,68);
        }
        if(build==B_TEMPLE||build==B_HALL){
            // Real colonnade projecting ahead of the wall, with an
            // enclosed upper lintel that receives perspective shading.
            for(int i=0;i<3;i++){
                int x=x0+43+i*57;
                ggMeshGroundBox(m,x-8,y0-25,x+8,y0-10,ground+20,181,173,155);
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
                        int dx,int dy,int height,int width){
    GgMeshVertex foot[5],crown[5];
    for(int i=0;i<5;i++){
        int a=i*256/5;
        int wx=ggExpCos[a]*width/4096,wy=ggExpSin[a]*width/4096;
        int rootX=x+wx+wx/3,rootY=y+wy+wy/3;
        foot[i]=ggMeshV(rootX,rootY,ggMeshGroundAtQ8(rootX,rootY)-1);
        crown[i]=ggMeshV(x+dx+wx/2,y+dy+wy/2,z+height);
    }
    for(int i=0;i<5;i++){
        int next=(i+1)%5;
        ggMeshQuad(m,foot[i],foot[next],crown[next],crown[i],113,84,58);
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
static void ggMeshFoliage(GgMeshContext*m,int x,int y,int ground,
                           int form,uint32_t key,int quality){
    int pine=form==1,tropical=form==2;
    int keep=m->material;
    int age=(int)((key>>5)&7u),turn=(int)((key>>9)&255u);
    int leanX=ggExpCos[turn]*(4+age)/4096;
    int leanY=ggExpSin[turn]*(4+age)/4096;
    int trunkH=(pine?12:tropical?25:17)+age/2;
    m->material=GG_MAT_BARK;
    ggMeshTrunk(m,x,y,ground,leanX,leanY,trunkH,9+age/2);
    m->material=GG_MAT_LEAVES;
    int sway=ggFilmScenicWind(key,ggExplore.frame)*3;
    int hue=(int)((key>>15)&7u)-3;
    int cx=x+leanX,cy=y+leanY;
    if(pine){
        for(int tier=0;tier<3;tier++){
            int low=ground+9+tier*(7+age/2);
            ggMeshBough(m,cx+sway*tier/3,cy,low,low+15+age/2,
                75+age*2-tier*18,quality>=2?3:1,sway,
                key+(uint32_t)tier*37u,
                48+hue+tier*3,86+hue+tier*5,72+hue+tier*3);
        }
    }else{
        int limb=38+age*3;
        int spread=ggExpCos[turn]*limb/4096;
        int stand=ggExpSin[turn]*limb/4096;
        int base=ground+trunkH-5;
        int r=(tropical?57:86)+hue;
        int g=(tropical?105:123)+hue;
        int b=(tropical?65:53)+hue;
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
    ggMeshGroundBox(m,x-5,y-5,x+5,y+5,ground+height/2,80,67,45);
    if(quality>=2){
        ggMeshBranch(m,x,y,ground+1,x+dx,y+dy,ground+3);
        ggMeshBranch(m,x,y,ground+1,x-dx,y-dy,ground+4);
    }
    m->material=GG_MAT_LEAVES;
    ggMeshLeafCrown(m,x,y,ground-2,42,height,key,5,70,109,55);
    if(quality>=2){
        ggMeshLeafCrown(m,x+dx,y+dy,ground-2,28,height-3,
                        key+691u,5,81,119,59);
        ggMeshLeafCrown(m,x-dx,y-dy,ground-1,24,height-2,
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
    int crag=(type==T_MOUNT||type==T_HILL)&&(key&7u)==0u;
    int height=crag?14+(int)((key>>14)&7u):5+(int)((key>>18)&3u);
    int radius=31+(int)((key>>14)&15u);
    int rotation=(int)((key>>22)&255u);
    int warm=type==T_SAND||type==T_DESERT;
    int r=warm?143:123,g=warm?121:124,b=warm?84:119;
    int rim=quality>=2?6:5;
    GgMeshVertex peak=ggMeshV(x+radius/4,y-radius/6,z+height);
    for(int i=0;i<rim;i++){
        int n=(i+1)%rim;
        int a=(rotation+i*256/rim)&255,c=(rotation+n*256/rim)&255;
        int u=radius+(int)((key>>(i*4))&7u);
        int v=radius+(int)((key>>(n*4))&7u);
        int ux=ggExpCos[a]*u/4096,uy=ggExpSin[a]*u*3/(4096*4);
        int vx=ggExpCos[c]*v/4096,vy=ggExpSin[c]*v*3/(4096*4);
        int footU=ggMeshGroundAtQ8(x+ux,y+uy)-1;
        int footV=ggMeshGroundAtQ8(x+vx,y+vy)-1;
        GgMeshVertex lowU=ggMeshV(x+ux,y+uy,footU<z-2?footU:z-2);
        GgMeshVertex lowV=ggMeshV(x+vx,y+vy,footV<z-2?footV:z-2);
        GgMeshVertex highU=ggMeshV(x+ux*2/3,y+uy*2/3,z+height*2/3+(i&1));
        GgMeshVertex highV=ggMeshV(x+vx*2/3,y+vy*2/3,z+height*2/3+(n&1));
        ggMeshQuad(m,lowU,lowV,highV,highU,r-18,g-14,b-12);
        ggMeshTriangle(m,highU,highV,peak,r,g,b);
    }
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
static void ggMeshCart(GgMeshContext*m,int x,int y,int ground){
    // A two-wheeled handcart with a resting foot. The axle is linked to
    // the raised bed even when one wheel is on the downhill side.
    int bed=ground+6;
    ggMeshBox(m,x-44,y-24,x+42,y+24,bed-2,bed+3,150,106,71);
    ggMeshGroundBox(m,x-40,y-5,x-33,y+5,bed,108,83,61);
    ggMeshBox(m,x-57,y-20,x-33,y-13,bed,bed+1,133,95,68);
    ggMeshBox(m,x-57,y+13,x-33,y+20,bed,bed+1,133,95,68);
    int wheelX=x+17;
    for(int edge=-1;edge<=1;edge+=2){
        int wheelY=y+edge*28;
        int hubZ=ggMeshGroundAtQ8(wheelX,wheelY)+2;
        ggMeshGroundBox(m,wheelX-5,wheelY-4,wheelX+5,wheelY+4,
                         hubZ>bed?hubZ+1:bed,100,75,53);
        GgMeshVertex hub=ggMeshV(wheelX,wheelY+edge*5,hubZ);
        GgMeshVertex edgeV[6];
        for(int i=0;i<6;i++){
            int a=(192+i*256/6)&255;
            int wx=wheelX+ggExpCos[a]*16/4096;
            int z=hubZ+ggExpSin[a]*3/4096;
            edgeV[i]=ggMeshV(wx,hub.y,z);
        }
        for(int i=0;i<6;i++){
            GgMeshVertex v=edgeV[i],next=edgeV[(i+1)%6];
            int lv=57+(i&1)*19;
            ggMeshTriangle(m,hub,edge>0?next:v,edge>0?v:next,lv,lv-6,lv-12);
            ggMeshQuad(m,v,next,ggMeshV(next.x,wheelY-edge*4,next.z),
                ggMeshV(v.x,wheelY-edge*4,v.z),61,54,47);
        }
        ggMeshBox(m,wheelX-4,wheelY-6,wheelX+4,wheelY+6,
                  hubZ,hubZ+1,144,108,73);
    }
}
static void ggMeshStall(GgMeshContext*m,int x,int y,int ground,int quality){
    for(int dx=-44;dx<=44;dx+=88)for(int dy=-24;dy<=24;dy+=48)
        ggMeshGroundBox(m,x+dx-4,y+dy-4,x+dx+4,y+dy+4,
                         ground+12+(dy>0),108,81,59);
    ggMeshBox(m,x-47,y-28,x+47,y+28,ground+4,ground+6,146,102,67);
    int previous=m->material;
    m->material=GG_MAT_FABRIC;
    int strips=quality>=2?4:1;
    for(int i=0;i<strips;i++){
        int left=x-52+104*i/strips,right=x-52+104*(i+1)/strips;
        int r=i&1?206:158,g=i&1?164:82,b=i&1?115:71;
        GgMeshVertex a=ggMeshV(left,y-33,ground+11);
        GgMeshVertex b0=ggMeshV(right,y-33,ground+11);
        GgMeshVertex c=ggMeshV(right,y+33,ground+13);
        GgMeshVertex d=ggMeshV(left,y+33,ground+13);
        ggMeshQuad(m,a,b0,c,d,r,g,b);
        ggMeshQuad(m,ggMeshV(left,y-33,ground+9),
            ggMeshV(right,y-33,ground+9),b0,a,r-12,g-10,b-8);
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
    if(quality>=2&&
       (kind==GG_SCENE_OAK||kind==GG_SCENE_PINE||
        kind==GG_SCENE_SHRUB||kind==GG_SCENE_STONE||kind==GG_SCENE_STALL||
        kind==GG_SCENE_CART||kind==GG_SCENE_CRATES))
        ggMeshContactShadow(m,x,y,
                             kind==GG_SCENE_SHRUB||kind==GG_SCENE_STONE?30:
                             kind==GG_SCENE_STALL?55:70,
                             kind==GG_SCENE_PINE?19:kind==GG_SCENE_SHRUB?7:13);
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
        ggMeshCart(m,x,y,z);
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
    int eye=ggMeshGroundAtQ8(ggExplore.px,ggExplore.py)+13+
            (ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0);
    int angle=ggExplore.angle&255;
    GgMeshContext*m=&ggMeshContext;
    ggMeshBegin(m,out,quality,ggExplore.px,ggExplore.py,eye,
                ggExpCos[angle],ggExpSin[angle],
                49+atmosphere.skyR,78+atmosphere.skyG,
                108+atmosphere.skyB);
    m->sunTint=1;
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
    enum { VIEW_RADIUS=9, VIEW_WIDTH=VIEW_RADIUS*2+1 };
    struct { short x,y,forward; } buildings[VIEW_WIDTH*VIEW_WIDTH];
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
            if(!build&&ggExpSceneryEnabled&&quality>=2&&
               forward>=0&&forward<4*256)
                ggMeshGroundLife(m,tx,ty,t,key);
            if(build){
                buildings[buildingCount].x=(short)tx;
                buildings[buildingCount].y=(short)ty;
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
        int detailed=forward<6*256&&forward>-128;
        m->material=(build==B_HOUSE||build==B_HUT||
                     build==B_DOCK||build==B_FARM)?GG_MAT_WOOD:GG_MAT_STONE;
        m->materialSeed=(int)ggFilmScenicHash(tx,ty,17);
        if(detailed&&forward>128&&build!=B_DOCK)
            ggMeshContactShadow(m,tx*256+128,ty*256+128,105,
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

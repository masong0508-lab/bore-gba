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
        int sx=tx*256+72+(int)((key>>(i*5))&95u);
        int sy=ty*256+66+(int)((key>>(i*7+2))&111u);
        int z=ground+1;
        if(type==T_GRASS||type==T_FOREST||type==T_JUNGLE||
           type==T_SWAMP){
            int height=type==T_JUNGLE?7:4+(int)((key>>(i*3))&3u);
            ggMeshTriangle(m,ggMeshV(sx-13,sy-8,z),
                ggMeshV(sx+11,sy+8,z),
                ggMeshV(sx+5,sy-5,z+height),
                51+(i*7),103+(i*5),60+(i*3));
            if((key>>(i*6))&1u)
                ggMeshTriangle(m,ggMeshV(sx-5,sy+12,z),
                    ggMeshV(sx+10,sy-8,z),
                    ggMeshV(sx-7,sy+3,z+height-1),70,116,71);
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
static void ggMeshGroundTile(GgMeshContext*m,int tx,int ty,
                              int type,uint32_t key){
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
    int variation=(int)((key>>9)&15u)-7;
    int light[4]={251,224,237,265};
    for(int i=0;i<4;i++){
        int amount=light[i]+variation*2;
        ggMeshTriangle(m,V,corners[i],corners[(i+1)&3],
            ggMeshShade(r,amount),
            ggMeshShade(g,amount),
            ggMeshShade(bl,amount));
    }
    // Sunlit earth tracks and paths are physical horizontal strips
    // with correct perspective. They don't crawl when camera turns.
    if(type==T_DIRT){
        int trackR=ggMeshShade(r,198),trackG=ggMeshShade(g,198),
            trackB=ggMeshShade(bl,188);
        for(int lane=0;lane<2;lane++){
            int x=left+(lane?175:70),z=center+1;
            ggMeshQuad(m,ggMeshV(x,top+13,z),
                       ggMeshV(x+8,top+13,z),
                       ggMeshV(x+8,bottom-13,z),
                       ggMeshV(x,bottom-13,z),
                       trackR,trackG,trackB);
        }
    }else if(type==T_WATER||type==T_DEEP){
        int ripple=(int)((key>>17)&3u);
        ggMeshQuad(m,ggMeshV(left+28,top+64+24*ripple,center+1),
                   ggMeshV(left+109,top+64+24*ripple,center+1),
                   ggMeshV(left+105,top+68+24*ripple,center+1),
                   ggMeshV(left+26,top+68+24*ripple,center+1),
                   80,130,154);
    }
    ggMeshTilesDrawn++;
}
static void ggMeshRoof(GgMeshContext*m,int build,
                       int x0,int y0,int x1,int y1,int wallTop){
    int ridgeY=(y0+y1)/2;
    int roofRise=build==B_TEMPLE?6:
                 build==B_HALL?9:build==B_TOWER?3:7;
    int roofR=build==B_HOUSE||build==B_HUT?129:
              build==B_TEMPLE?172:89;
    int roofG=build==B_HOUSE||build==B_HUT?79:
              build==B_TEMPLE?143:91;
    int roofB=build==B_HOUSE||build==B_HUT?65:
              build==B_TEMPLE?110:103;
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
    // Tile courses are real narrow roof geometry, not a vertical
    // stripe on a ray column. They keep the pitched surface legible
    // as the player walks around all four sides of a dwelling.
    if(build==B_HOUSE||build==B_HUT||build==B_HALL||
       build==B_LIBRARY){
        for(int row=1;row<=3;row++){
            int yy=y0+(ridgeY-y0)*row/4;
            int z=wallTop+roofRise*row/4+1;
            ggMeshQuad(m,ggMeshV(x0-10,yy-3,z),
                ggMeshV(x1+10,yy-3,z),
                ggMeshV(x1+10,yy+3,z),
                ggMeshV(x0-10,yy+3,z),
                roofR*4/5,roofG*4/5,roofB*4/5);
            yy=y1-(y1-ridgeY)*row/4;
            ggMeshQuad(m,ggMeshV(x0-10,yy-3,z),
                ggMeshV(x1+10,yy-3,z),
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
                   124,109,96);
    ggMeshTriangle(m,ggMeshV(x1,y0,wallTop),
                   ggMeshV(x1,y1,wallTop),
                   ggMeshV(x1,ridgeY,wallTop+roofRise),
                   88,88,97);
}
static void ggMeshWindow(GgMeshContext*m,int x0,int x1,
                          int frontY,int facing,int ground,int height){
    if(height<15)return;
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
                   ggMeshV(a,paneY,winTop),187,139,95);
        ggMeshQuad(m,ggMeshV(center-2,paneY-facing,winBot),
                   ggMeshV(center+2,paneY-facing,winBot),
                   ggMeshV(center+2,paneY-facing,winTop),
                   ggMeshV(center-2,paneY-facing,winTop),62,67,72);
    }
}
static void ggMeshSideFacade(GgMeshContext*m,int wallX,int facing,
                             int y0,int y1,int ground,int height){
    // A physically rotated version of the front glazing, with its
    // own x-plane. The original y-only windows left the wall directly
    // facing the default eastward camera completely blank.
    if(height<13)return;
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
            ggMeshV(pane,q-19,high),177,145,104);
        ggMeshQuad(m,ggMeshV(pane-facing,q-2,low),
            ggMeshV(pane-facing,q+2,low),
            ggMeshV(pane-facing,q+2,high),
            ggMeshV(pane-facing,q-2,high),62,65,66);
    }
}
static void ggMeshBuilding(GgMeshContext*m,int tx,int ty,int build,
                            int ground,int nearDetail){
    int x0=tx*256+28,x1=tx*256+228;
    int y0=ty*256+28,y1=ty*256+228;
    int wallTop=ground+ggExpBuilding(build);
    int wood=build==B_HOUSE||build==B_HUT||
             build==B_DOCK||build==B_FARM;
    int stone=build==B_TEMPLE||build==B_HALL||
              build==B_TOWER||build==B_LIBRARY;
    int r=wood?153:stone?147:111,g=wood?110:stone?140:113,
        bl=wood?78:stone?130:111;
    if(build==B_RUIN){r=107;g=109;bl=108;}
    if(build==B_STATUE){r=156;g=152;bl=140;}
    ggMeshBox(m,x0,y0,x1,y1,ground,wallTop,r,g,bl);
    if(build==B_HOUSE||build==B_HUT||build==B_HALL||
       build==B_TEMPLE||build==B_LIBRARY)
        ggMeshRoof(m,build,x0,y0,x1,y1,wallTop);
    if(build==B_TOWER){
        // Four real corner parapets, not a one-pixel tower trim stripe.
        int h=wallTop;
        ggMeshBox(m,x0,y0,x0+34,y0+34,h,h+5,164,163,154);
        ggMeshBox(m,x1-34,y0,x1,y0+34,h,h+5,164,163,154);
        ggMeshBox(m,x0,y1-34,x0+34,y1,h,h+5,164,163,154);
        ggMeshBox(m,x1-34,y1-34,x1,y1,h,h+5,164,163,154);
    }
    if(nearDetail){
        int h=wallTop-ground;
        // Ground-contact plinth and roof cornice extend beyond every
        // wall face, showing real parallax, height and shade.
        ggMeshBox(m,x0-5,y0-5,x1+5,y1+5,
                  ground,ground+2,107,105,98);
        ggMeshBox(m,x0-6,y0-6,x1+6,y1+6,
                  wallTop-3,wallTop-1,177,164,137);
        ggMeshWindow(m,x0,x1,y0,1,ground,h);
        ggMeshWindow(m,x0,x1,y1,-1,ground,h);
        ggMeshSideFacade(m,x0,1,y0,y1,ground,h);
        ggMeshSideFacade(m,x1,-1,y0,y1,ground,h);
        // Vertical corner quoins or timber beams connect floor to
        // eaves and keep every view from looking like a plain cube.
        int trimR=wood?102:181,trimG=wood?77:170,trimB=wood?58:149;
        ggMeshBox(m,x0-7,y0-7,x0+10,y0+10,ground,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x1-10,y0-7,x1+7,y0+10,ground,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x0-7,y1-10,x0+10,y1+7,ground,wallTop,
                  trimR,trimG,trimB);
        ggMeshBox(m,x1-10,y1-10,x1+7,y1+7,ground,wallTop,
                  trimR,trimG,trimB);
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
            int cx=(x0+x1)/2;
            ggMeshBox(m,cx-30,y0-13,cx-23,y0-3,
                      ground+12,ground+22,89,65,53);
        }
    }
    ggMeshBuildingsDrawn++;
}
static void ggMeshFoliage(GgMeshContext*m,int x,int y,int ground,
                           int pine,uint32_t key,int quality){
    int size=pine?42:53;
    int trunkH=pine?11:13;
    ggMeshBox(m,x-12,y-12,x+12,y+12,ground,ground+trunkH,
              99,74,58);
    // Physical octagonal crown: upper and lower triangles converge
    // around real 3D points, viewed from *any* camera heading.
    int levels=pine?3:1;
    for(int tier=0;tier<levels;tier++){
        int base=ground+trunkH+tier*(pine?7:0);
        int radius=pine?size-tier*9:size;
        int mid=base+(pine?5:10),top=base+(pine?18:21);
        int sides=quality>=2?8:6;
        int rr=pine?54:63,gg=pine?105:118,bb=pine?76:83;
        for(int n=0;n<sides;n++){
            int aa=n*256/sides,ab=(n+1)*256/sides;
            int ax=x+ggExpCos[aa]*radius/4096,
                ay=y+ggExpSin[aa]*radius/4096;
            int bx=x+ggExpCos[ab]*radius/4096,
                by=y+ggExpSin[ab]*radius/4096;
            int light=(int)((key>>(n%18))&3u);
            ggMeshTriangle(m,ggMeshV(x,y,top),
                ggMeshV(ax,ay,mid),ggMeshV(bx,by,mid),
                rr+light*3,gg+light*5,bb+light*4);
            ggMeshTriangle(m,ggMeshV(ax,ay,mid),
                ggMeshV(x,y,base),ggMeshV(bx,by,mid),
                rr*3/4,gg*3/4,bb*3/4);
        }
    }
}
static void ggMeshShrub(GgMeshContext*m,int x,int y,int ground,
                          uint32_t key){
    // Broad low foliage footprint, not a full-sized oak reused as a
    // shrub. Eight actual side facets give recognizable small-scale
    // understory without view-dependent sprite stretching.
    int radius=32,base=ground+1,mid=ground+5,tip=ground+10;
    for(int i=0;i<8;i++){
        int a=i*32,b=(i+1)*32;
        int ax=x+ggExpCos[a]*radius/4096,
            ay=y+ggExpSin[a]*radius/4096;
        int bx=x+ggExpCos[b]*radius/4096,
            by=y+ggExpSin[b]*radius/4096;
        int shade=(int)((key>>(i+8))&3u);
        ggMeshTriangle(m,ggMeshV(x,y,tip),
            ggMeshV(ax,ay,mid),ggMeshV(bx,by,mid),
            55+shade*4,95+shade*5,62+shade*3);
        ggMeshTriangle(m,ggMeshV(ax,ay,mid),
            ggMeshV(x,y,base),ggMeshV(bx,by,mid),
            36+shade*3,77+shade*3,53+shade*2);
    }
}
static void ggMeshProp(GgMeshContext*m,int tx,int ty,int kind,
                       int ground,uint32_t key,int quality){
    int x=tx*256+128+(int)((key>>12)&63u)-32;
    int y=ty*256+128+(int)((key>>20)&63u)-32;
    int z=ground;
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
    case GG_SCENE_LAMP:case GG_SCENE_BRAZIER:
        ggMeshBox(m,x-5,y-5,x+5,y+5,z,z+12,89,86,74);
        ggMeshBox(m,x-13,y-13,x+13,y+13,z+12,z+15,196,142,72);
        break;
    case GG_SCENE_BANNER:
        ggMeshBox(m,x-3,y-3,x+3,y+3,z,z+19,109,107,99);
        ggMeshTriangle(m,ggMeshV(x,y,z+17),
                       ggMeshV(x+64,y,z+16),
                       ggMeshV(x+6,y,z+9),146,65,80);
        break;
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
            ggMeshTriangle(m,ggMeshV(xx-8,y,z),
                           ggMeshV(xx+8,y,z),
                           ggMeshV(xx+4,y+3,z+5+(i&1)),70,124,78);
        }
        break;
    default:break;
    }
    ggMeshPropsDrawn++;
}
static void ggMeshCloudBank(GgMeshContext*m,int camTileX,int camTileY,
                             int frame,int hazeR,int hazeG,int hazeB){
    // Clouds are actually floating, depth-tested polygonal volumes at
    // deterministic sky-world positions. They drift slightly on the
    // global timeline and turn with parallax; they are not a static
    // screen overlay or a second image layer.
    int gx=(camTileX/16)*16,gy=(camTileY/16)*16;
    for(int row=-2;row<=2;row++){
        for(int col=-2;col<=2;col++){
            int tx=gx+col*8+4,ty=gy+row*8+4;
            uint32_t key=ggFilmScenicHash(tx,ty,31);
            if(((key>>16)&3u)==0u)continue;
            int cx=tx*256+(int)(key&255u)+frame/8;
            int cy=ty*256+(int)((key>>8)&255u);
            int z=78+(int)((key>>20)&15u);
            int width=150+(int)((key>>11)&63u);
            int height=7+(int)((key>>24)&7u);
            int dr=hazeR+23,dg=hazeG+25,db=hazeB+23;
            ggMeshTriangle(m,ggMeshV(cx-width,cy,z),
                ggMeshV(cx,cy-100,z+height),
                ggMeshV(cx+width,cy,z),dr,dg,db);
            ggMeshTriangle(m,ggMeshV(cx-width/2,cy+60,z),
                ggMeshV(cx,cy-90,z+height+3),
                ggMeshV(cx+width/2,cy+60,z),
                dr+9,dg+7,db+5);
            ggMeshTriangle(m,ggMeshV(cx-width,cy,z),
                ggMeshV(cx+width,cy,z),
                ggMeshV(cx,cy+110,z-3),
                dr-15,dg-14,db-12);
        }
    }
}
static void ggMeshWorldDraw(unsigned char *out,int cinematic){
    int quality=ggExpScenicQuality>=2&&cinematic?2:1;
    int cx=ggExplore.px>>8,cy=ggExplore.py>>8;
    int terrain=ggMeshTerrainAt(cx,cy);
    GgFilmScenicAtmosphere atmosphere=ggFilmScenicAtmosphere(terrain);
    int eye=ggExpGround(terrain)+13+
            (ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0);
    int angle=ggExplore.angle&255;
    GgMeshContext*m=&ggMeshContext;
    ggMeshBegin(m,out,quality,ggExplore.px,ggExplore.py,eye,
                ggExpCos[angle],ggExpSin[angle],
                35+atmosphere.skyR,59+atmosphere.skyG,
                90+atmosphere.skyB);
    ggMeshTilesDrawn=ggMeshBuildingsDrawn=ggMeshPropsDrawn=0;
    ggMeshCloudBank(m,cx,cy,ggExplore.frame,atmosphere.hazeR,
                    atmosphere.hazeG,atmosphere.hazeB);
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
            if(quality>=2&&forward>0&&forward<4*256)
                ggMeshGroundLife(m,tx,ty,t,ggExpGround(t),key);
            int build=bld[ty][tx];
            if(build){
                int detailed=forward<6*256&&forward>-128;
                ggMeshBuilding(m,tx,ty,build,ggExpGround(t),detailed);
            }else if(sceneLimit<propCap){
                uint32_t scenic=ggFilmScenicHash(tx,ty,7);
                if(((scenic>>24)&7u)>=5u)continue;
                int near=0;
                if(tx>0&&bld[ty][tx-1])near=bld[ty][tx-1];
                else if(tx+1<WW&&bld[ty][tx+1])near=bld[ty][tx+1];
                else if(ty>0&&bld[ty-1][tx])near=bld[ty-1][tx];
                else if(ty+1<WH&&bld[ty+1][tx])near=bld[ty+1][tx];
                int kind=ggFilmScenicKind(t,near,scenic);
                if(kind){
                    ggMeshProp(m,tx,ty,kind,ggExpGround(t),scenic,
                               ggExpScenicQuality);
                    sceneLimit++;
                }
            }
        }
    }
}
#endif

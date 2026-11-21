// GodGuy 3DS: project the existing *2D game creature sprites* into the
// live first-person world. Keep pixel-art individuality and animation;
// do not introduce block-headed replacement models, heap allocations or
// full-map per-frame creature scans. First-person actor never duplicates.
#ifndef GG_POPULATION3D_H
#define GG_POPULATION3D_H
enum { GG_EXP_MAX_VISIBLE=12, GG_EXP_NEAR_RADIUS=11 };
typedef struct {int index,forward,side;} GgExpNPC;
static int ggExpNpcsDrawn;
// The live 200x120 POV upscaled to 400x240 has the same 250px focal
// length and Q8 ground contract as the 3DS film, not the obsolete 140px.
static inline void ggExpNpcProject(int forward,int side,
                                   int cameraGround,int actorGround,
                                   int footBob,int scale,
                                   int *screenX,int *footY){
    *screenX=ggFilmProjectScreenX(side,forward)-4*scale;
    *footY=ggFilmProjectFootY(forward,cameraGround,actorGround)+
           footBob*2;
}
static void ggExpNpcContactShade(u8*out,int center,int feet,int span){
    if(feet<1||feet>=240)return;
    int y=feet-1;
    int from=ggExpClamp(center-span/2,0,400);
    int to=ggExpClamp(center+span/2,0,400);
    for(int x=from;x<to;x++){
        if(ggExpMeshActive&&
           ggMeshHidden(&ggMeshContext,x,y,ggExpFilmDrawDepth))
            continue;
        u8*p=out+(y*400+x)*3;
        p[0]=(u8)(p[0]*204/256);
        p[1]=(u8)(p[1]*204/256);
        p[2]=(u8)(p[2]*204/256);
    }
}
static int ggExpLineClear(int fromX,int fromY,int x,int y){
    int dx=x-fromX,dy=y-fromY;
    int n=abs(dx)>abs(dy)?abs(dx):abs(dy);
    if(n<=1)return 1;
    int here=ggExpGround(world[fromY][fromX])+13;
    int there=ggExpGround(world[y][x])+19;
    for(int q=1;q<n;q++){
        int tx=fromX+dx*q/n,ty=fromY+dy*q/n;
        if((unsigned)tx>=WW||(unsigned)ty>=WH)return 0;
        if(bld[ty][tx])return 0;
        if(ggExpGround(world[ty][tx])>
            here+(there-here)*q/n+4)return 0;
    }
    return 1;
}
static void ggExpRenderCreatures(u8 *top){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    int yaw=ggExplore.angle&255;
    int forwardX=ggExpCos[yaw],forwardY=ggExpSin[yaw];
    int eye=ggExpGround(world[py][px])+13;
    GgExpNPC draw[GG_EXP_MAX_VISIBLE];
    int count=0;
    ggExpNpcsDrawn=0;
    ggExpNpcClosestDepth=0;
    ggMeshPersonCount=0;
    ggMeshPersonPixels=0;
    ggMeshPersonWalkingDrawn=0;
    // O(23x23) cell probes independent of 3DS creature capacity; no
    // looping through all 12,000+ creatures during each rendered frame.
    for(int y=py-GG_EXP_NEAR_RADIUS;y<=py+GG_EXP_NEAR_RADIUS;y++){
        if((unsigned)y>=WH)continue;
        for(int x=px-GG_EXP_NEAR_RADIUS;x<=px+GG_EXP_NEAR_RADIUS;x++){
            if((unsigned)x>=WW||!occ[y][x])continue;
            int i=(int)occ[y][x]-1;
            if(i<0||i>=nCr||!cr[i].alive)continue;
            if(ggExplore.actorId&&crId[i]==ggExplore.actorId)continue;
            // A creature standing in a building still exists but cannot
            // float through a neighboring wall.
            int vx=(x<<8)+128-ggExplore.px;
            int vy=(y<<8)+128-ggExplore.py;
            int f=(vx*forwardX+vy*forwardY)/4096;
            if(f<180||f>GG_EXP_NEAR_RADIUS*256)continue;
            int side=(-vx*forwardY+vy*forwardX)/4096;
            if(abs(side)*4>f*5)continue;
            if(!ggExpLineClear(px,py,x,y))continue;
            if(count<GG_EXP_MAX_VISIBLE){
                draw[count++]=(GgExpNPC){i,f,side};
            }else{
                // Preserve the closest inhabitants as population grows.
                int far=0;
                for(int k=1;k<count;k++)
                    if(draw[k].forward>draw[far].forward)far=k;
                if(f<draw[far].forward)
                    draw[far]=(GgExpNPC){i,f,side};
            }
        }
    }
    // Back to front layering: nearby 2D cels properly cover distant
    // people, not the other way around. Maximum 12 actors, 8x8 each.
    while(count){
        int far=0;
        for(int k=1;k<count;k++)
            if(draw[k].forward>draw[far].forward)far=k;
        GgExpNPC npc=draw[far];
        draw[far]=draw[--count];
        const Cr *c=&cr[npc.index];
        int si=sp[c->sp].a;
        if(si<0||si>=ARCH_N)continue;
        int shape=arch[si].shape;
        if(shape<0||shape>=SHAPE_N)continue;
        int fam=creatureFamily(c)&15;
        int size=160*256/(npc.forward+256);
        if(size>72)size=72;
        int scale=size/8;
        if(scale<1)scale=1;
        int sx,feet;
        int ground=ggExpGround(world[c->y][c->x]);
        if(ggExpMeshActive){
            // Person: authored 32x48 procedural anatomical sprite with
            // stable clothing, complexion, facial features and eight
            // facing directions. Other families preserve actual low-poly
            // animal volumes. Both share real terrain, physical z depth.
            int wx=(c->x<<8)+128,wy=(c->y<<8)+128;
            uint32_t identity=(uint32_t)crId[npc.index];
            if(!identity)identity=(uint32_t)(npc.index+1);
            uint32_t stable=ggPersonMix(identity^0xa1379c5du);
            GgPersonMotion movement=ggPersonMotionSample(
                identity,wx,wy,ggExplore.frame,(int)(stable>>24));
            int motionDX=movement.x-ggExplore.px;
            int motionDY=movement.y-ggExplore.py;
            int motionF=(motionDX*forwardX+motionDY*forwardY)/4096;
            int motionSide=(-motionDX*forwardY+motionDY*forwardX)/4096;
            int bodyGround=ggMeshGroundAtQ8(movement.x,movement.y);
            int phase=(ggExplore.frame*2+npc.index*17)&255;
            if(arch[si].diet==4){
                // Real 3D depth and tilt now follow INTERPOLATED feet.
                // Direction is derived from the last creature move,
                // not its random ID. A stationary resident can breathe
                // or blink without perpetually moonwalking.
                ggMeshPersonDraw(&ggMeshContext,
                                 movement.x,movement.y,bodyGround,
                                 motionF,motionSide,identity,fam,
                                 movement.heading,ggExplore.frame,
                                 movement.walking,famCol[fam]);
            }else{
                ggMeshActorCharacter(&ggMeshContext,
                                     movement.x,movement.y,bodyGround,
                                     movement.heading,phase,fam,0,
                                     famCol[fam],
                                     ggExpScenicQuality>=2?2:1);
            }
            ggExpNpcsDrawn++;
            continue;
        }
        int walkBob=ggExplore.stepPhase?
                    ((ggExplore.stepPhase/5)&1):0;
        ggExpNpcProject(npc.forward,npc.side,eye-13,ground,
                        walkBob,scale,&sx,&feet);
        if(ggExpMeshActive){
            GgMeshCameraVertex point={npc.forward,npc.side,ground};
            GgMeshProjected projected=ggMeshProject(&ggMeshContext,point);
            sx=projected.x*ggMeshContext.scale-4*scale;
            feet=projected.y*ggMeshContext.scale+walkBob*2;
        }
        if(sx>=400||sx+8*scale<0||feet-8*scale>=240||feet<0)
            continue;
        ggExpFilmDrawDepth=npc.forward;
        ggExpNpcContactShade(top,sx+4*scale,feet,scale*6);
        int pose=((ggExplore.frame>>4)+npc.index)&1;
        for(int y=0;y<8;y++){
            const char *row=shpRow(shape,pose,y);
            for(int x=0;x<8;x++){
                int col=row[x]=='b'?0:row[x]=='d'?1:
                        row[x]=='a'?2:row[x]=='e'?3:-1;
                if(col<0)continue;
                int r=col==3?246:famCol[fam][col][0]*8;
                int g=col==3?244:famCol[fam][col][1]*8;
                int b=col==3?225:famCol[fam][col][2]*8;
                int xx=sx+x*scale,yy=feet-(8-y)*scale;
                ggExpRect(top,xx,yy,scale,scale,r,g,b);
                // Two subtle world-light edges at high magnification,
                // preserving the source game's real 8x8 creature shape
                // without ballooning it into a featureless colored cube.
                if(scale>=4&&col!=3){
                    ggExpRect(top,xx,yy,1,scale,
                              r*5/6,g*5/6,b*5/6);
                    ggExpRect(top,xx+scale-1,yy,1,scale,
                              r*3/4,g*3/4,b*3/4);
                }
            }
        }
        if(!ggExpNpcClosestDepth||npc.forward<ggExpNpcClosestDepth)
            ggExpNpcClosestDepth=npc.forward;
        ggExpNpcsDrawn++;
        ggExpFilmDrawDepth=0;
    }
}
#endif

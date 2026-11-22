// GodGuy 3DS: the existing live population in the first-person world.
// Directional citizen cels and species meshes share terrain and depth;
// the legacy renderer retains its original 2D sprites. No population
// duplication, heap allocations or full-map per-frame creature scans.
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
    int here=ggExpGround(world[fromY][fromX])+GG_WORLD_EYE_HEIGHT;
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
// Iconic creatures without an authored mesh keep the SAME source sprite
// (skeleton, dragon, mage, finger, printer, etc.), projected into actual
// world depth. Substituting a quadruped loses their identity. The bounded
// 8x8 painter uses 12 bytes of graded inks, no cel/cache/extra framebuffer.
static void ggMeshOriginalCreature(GgMeshContext*m,int x,int y,int ground,
                                   int forward,int side,int shape,int frame,
                                   const u8 colors[3][3]){
    if(forward<m->near||forward>GG_EXP_NEAR_RADIUS*256)return;
    GgMeshCameraVertex base={forward,side,ground,0,0};
    GgMeshCameraVertex crown={forward,side,ground+25,0,0};
    GgMeshProjected a=ggMeshProject(m,base),b=ggMeshProject(m,crown);
    int size=ggMeshClamp((a.y-b.y)*m->scale,8,104);
    int step=m->scale;
    size=(size+step-1)/step*step;
    // Some original icons have transparent bottom rows or off-center
    // feet. Anchor their actual lowest ink to the physical ground point.
    int soleRow=-1,soleFirst=0,soleLast=7;
    for(int yy=7;yy>=0&&soleRow<0;yy--){
        const char*row=shpRow(shape,frame,yy);
        for(int xx=0;xx<8;xx++)if(row[xx]!='.'){
            if(soleRow<0){soleRow=yy;soleFirst=xx;}
            soleLast=xx;
        }
    }
    if(soleRow<0)return;
    int center=a.x*step,feet=a.y*step;
    int footHeight=(soleRow+1)*size/8/step*step;
    int left=center-(soleFirst+soleLast+1)*size/16;
    int top=feet-footHeight;
    if(step>1){
        left=left>=0?left/step*step:-((-left+step-1)/step)*step;
        top=top>=0?top/step*step:-((-top+step-1)/step)*step;
    }
    if(left>=400||left+size<=0)return;
    // The authored sole rows keep their silhouette on flat terrain.
    // On a grade each foot follows its own column of the same ground.
    int pivot=footHeight/2/step*step;
    int lower=footHeight-pivot;
    int rough=lower>0&&ggMeshBillboardUneven(m,x,y,ground,forward,size);
    short soleOffset[GG_MESH_MAX_W];
    int rise=0,lowest=feet;
    if(rough){
        const char*sole=shpRow(shape,frame,soleRow);
        int highest=feet;
        int x0=left<0?0:left,x1=left+size>400?400:left+size;
        for(int px=x0;px<x1;px+=step){
            int sx=((px-left+step)*8-1)/size;
            if(sole[sx]=='.'){soleOffset[px]=0;continue;}
            int row=ggMeshBillboardFloorY(m,x,y,forward,px-center);
            soleOffset[px]=(short)(row-feet);
            if(row<highest)highest=row;
            if(row>lowest)lowest=row;
        }
        int room=lower/2/step*step;
        rise=ggMeshClamp(feet-highest-room,0,feet-highest);
    }
    top-=rise;
    if(top>=240||lowest<=0)return;
    ggMeshContactShadow(m,x,y,32,4);
    int light=ggMeshClamp(m->ambientLight+12,128,256);
    int haze=ggMeshFogAmount(forward,m->fogMax);
    u8 ink[4][3];
    for(int k=0;k<4;k++)for(int channel=0;channel<3;channel++){
        int color=k==3?236:colors[k][channel]*8;
        int fog=channel==0?m->fogR:channel==1?m->fogG:m->fogB;
        ink[k][channel]=(u8)((color*light/256*(256-haze)+fog*haze)/256);
    }
    int inverse=65535/forward;
    for(int yy=0;yy<8;yy++){
        const char*row=shpRow(shape,frame,yy);
        int from=(yy*size/8)/step*step;
        int to=((yy+1)*size/8)/step*step;
        for(int xx=0;xx<8;xx++){
            int k=row[xx]=='b'?0:row[xx]=='d'?1:
                  row[xx]=='a'?2:row[xx]=='e'?3:-1;
            if(k<0)continue;
            int x0=left+(xx*size/8)/step*step;
            int x1=left+((xx+1)*size/8)/step*step;
            if(x0<0)x0=0;if(x1>400)x1=400;
            for(int px=x0;px<x1;px+=step){
                int y0=top+from,y1=top+to;
                if(rough){
                    int change=rise+soleOffset[px];
                    if(from>pivot)y0+=(from-pivot)*change/lower/step*step;
                    if(to>pivot)y1+=(to-pivot)*change/lower/step*step;
                }
                if(y0<0)y0=0;if(y1>240)y1=240;
                for(int py=y0;py<y1;py+=step){
                    int z=(py/step)*m->w+px/step;
                    if(inverse+1<m->depth[z])continue;
                    for(int oy=0;oy<step;oy++)for(int ox=0;ox<step;ox++){
                        u8*dst=m->rgb+((py+oy)*400+px+ox)*3;
                        dst[0]=ink[k][0];dst[1]=ink[k][1];dst[2]=ink[k][2];
                    }
                    if(m->depth[z]<(uint16_t)inverse)
                        m->depth[z]=(uint16_t)inverse;
                }
            }
        }
    }
}
static void ggExpRenderCreatures(u8 *top){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    int yaw=ggExplore.angle&255;
    int forwardX=ggExpCos[yaw],forwardY=ggExpSin[yaw];
    int eye=ggExpGround(world[py][px])+GG_WORLD_EYE_HEIGHT;
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
        int ground=ggExpGround(world[c->y][c->x]);
        if(ggExpMeshActive){
            // Person: authored 32x48 procedural anatomical sprite with
            // stable clothing, complexion, facial features and eight
            // facing directions. Other species use their authored shape for closed
            // bird, fish, crawler, serpent and mammal volumes. Both share real terrain, physical z depth.
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
                int kind=ggMeshCreatureKind(shape);
                if(kind==GG_CREATURE_ORIGINAL){
                    ggMeshOriginalCreature(&ggMeshContext,
                        movement.x,movement.y,bodyGround,motionF,motionSide,
                        shape,movement.walking?((ggExplore.frame>>3)&1):0,
                        famCol[fam]);
                    ggExpNpcsDrawn++;
                    continue;
                }
                int animated=movement.walking||kind==GG_CREATURE_BIRD||
                    kind==GG_CREATURE_FISH||kind==GG_CREATURE_WINGED||
                    kind==GG_CREATURE_BLOB;
                ggMeshActorSpecies(&ggMeshContext,
                                     movement.x,movement.y,bodyGround,
                                     movement.heading,animated?phase:0,fam,shape,
                                     famCol[fam],
                                     ggExpScenicQuality>=2?2:1);
            }
            ggExpNpcsDrawn++;
            continue;
        }
        int scale=ggExpClamp(20*256/(npc.forward+256),1,9);
        int height=scale*8,width=height;
        if(arch[si].diet==4){
            height=ggFilmProjectHeight(npc.forward,GG_WORLD_PERSON_HEIGHT);
            // The legacy 8x8 figure fills more of its canvas than the
            // native 32x48 cel, so its projected panel is narrower.
            width=height/2;
        }
        int walkBob=ggExplore.stepPhase?
                    ((ggExplore.stepPhase/5)&1):0;
        int sx,feet;
        ggExpNpcProject(npc.forward,npc.side,eye-GG_WORLD_EYE_HEIGHT,
                        ground,walkBob,0,&sx,&feet);
        sx-=width/2;
        if(width<3||height<5||sx>=400||sx+width<0||feet-height>=240||feet<0)
            continue;
        ggExpFilmDrawDepth=npc.forward;
        ggExpNpcContactShade(top,sx+width/2,feet,width*3/4);
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
                int xx=sx+x*width/8,yy=feet-height+y*height/8;
                int w=sx+(x+1)*width/8-xx,h=feet-height+(y+1)*height/8-yy;
                if(!w||!h)continue;
                ggExpRect(top,xx,yy,w,h,r,g,b);
                // Two subtle world-light edges at high magnification,
                // preserving the source game's real 8x8 creature shape
                // without ballooning it into a featureless colored cube.
                if(w>=4&&h>=4&&col!=3){
                    ggExpRect(top,xx,yy,1,h,
                              r*5/6,g*5/6,b*5/6);
                    ggExpRect(top,xx+w-1,yy,1,h,
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

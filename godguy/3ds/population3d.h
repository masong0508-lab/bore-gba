// GodGuy 3DS: project the existing *2D game creature sprites* into the
// live first-person world. Keep pixel-art individuality and animation;
// do not introduce block-headed replacement models, heap allocations or
// full-map per-frame creature scans. First-person actor never duplicates.
#ifndef GG_POPULATION3D_H
#define GG_POPULATION3D_H
enum { GG_EXP_MAX_VISIBLE=12, GG_EXP_NEAR_RADIUS=11 };
typedef struct {int index,forward,side;} GgExpNPC;
static int ggExpNpcsDrawn;
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
        int sx=200+npc.side*140/npc.forward-4*scale;
        int ground=ggExpGround(world[c->y][c->x]);
        int d=npc.forward/256+1;
        int feet=(58-(ground-eye)*6/d)*2;
        if(sx>=400||sx+8*scale<0||feet-8*scale>=240||feet<0)
            continue;
        ggExpRect(top,sx+scale,feet-2,scale*6,2,24,35,40);
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
                ggExpRect(top,sx+x*scale,feet-(8-y)*scale,
                          scale,scale,r,g,b);
            }
        }
        ggExpNpcsDrawn++;
    }
}
#endif

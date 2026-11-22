// Procedural human sprites composited into the ACTUAL 3D world.
// Individual 32x48 generated cels follow perspective and the same world
// z-buffer as architecture and vegetation. One reusable 1.6 KiB scratch
// cel, no heap, no full-frame sprites/atlas or population-cache arrays.
// All world encounters are depth-tested; never paint through buildings.
#ifndef GG_MESH_PEOPLE_BILLBOARD3D_H
#define GG_MESH_PEOPLE_BILLBOARD3D_H
#include "procedural_people3d.h"
static GgPersonCel ggMeshPersonCel;
static int ggMeshPersonCount;
static int ggMeshPersonPixels;
static int ggMeshPersonWalkingDrawn; // source-rendered walk visibility QC
static inline int ggMeshPersonFacing(int heading,int cameraAngle){
    // Front when citizen's forward vector points AT the camera.
    return ((heading-cameraAngle-128+16)&255)>>5;
}
static inline int ggMeshPersonHeight(const GgMeshContext*m,
                                      int forward,int ground){
    GgMeshCameraVertex bottom={forward,0,ground,0,0};
    GgMeshCameraVertex crown={forward,0,ground+GG_WORLD_PERSON_HEIGHT,0,0};
    GgMeshProjected a=ggMeshProject(m,bottom),b=ggMeshProject(m,crown);
    return (a.y-b.y)*m->scale;
}
// The source cel is 32x48; the old 32/80 billboard aspect ratio
// squeezed shoulders, faces and legs into conspicuously thin cutouts.
// Keep both axes proportional during a dolly. The raster loop clips
// the projected rectangle to the screen, including at the near plane.
static inline int ggMeshPersonWidth(int pixelH){
    return pixelH*GG_PERSON_W/GG_PERSON_H;
}
static inline int ggMeshPersonInView(const GgMeshContext*m,int forward,int side){
    int reach=GG_WORLD_PERSON_HEIGHT*m->vertical*256/3;
    return forward>=m->near&&forward<=11*256&&
           abs(side)*m->focal<=(m->w/2+2)*forward+reach;
}
static int ggMeshBillboardUneven(const GgMeshContext*m,int x,int y,
                                 int ground,int forward,int span){
    int extent=span*forward/(m->focal*m->scale);
    return !ggMeshShadowFlat((x-extent)>>8,(x+extent)>>8,
                             (y-extent)>>8,(y+extent)>>8,ground)&&
           ggMeshGroundAtQ8(x,y)==ground;
}
static int ggMeshBillboardFloorY(const GgMeshContext*m,int x,int y,
                                 int forward,int screenOffset){
    int cross=screenOffset*forward/(m->focal*m->scale);
    int wx=x-m->sine*cross/4096,wy=y+m->cosine*cross/4096;
    GgMeshCameraVertex sole={forward,0,ggMeshGroundAtQ8(wx,wy),0,0};
    return ggMeshProject(m,sole).y*m->scale;
}
static int ggMeshPersonDrawGenerated(GgMeshContext*m,int worldX,int worldY,
                                     int ground,int forward,int side,
                                     int heading,int active){
    if(forward<m->near||forward>11*256)return 0;
    int pixelH=ggMeshPersonHeight(m,forward,ground);
    if(pixelH<7)return 0;
    int width=ggMeshPersonWidth(pixelH);
    GgMeshCameraVertex base={forward,side,ground,0,0};
    GgMeshProjected projected=ggMeshProject(m,base);
    int center=projected.x*m->scale,feet=projected.y*m->scale;
    int top=feet-pixelH,left=center-width/2;
    // Software 200x120 depth and terrain resolve in 2x2 RGB blocks.
    // Snap the billboard to the same grid: 1-pixel facial/limb fragments
    // on Old 3DS looked cut-out and caused four redundant depth checks.
    int pixelStep=m->scale;
    if(pixelStep>1){
        width=(width+pixelStep-1)/pixelStep*pixelStep;
        pixelH=(pixelH+pixelStep-1)/pixelStep*pixelStep;
        left=left>=0?left/pixelStep*pixelStep:
              -((-left+pixelStep-1)/pixelStep)*pixelStep;
        top=top>=0?top/pixelStep*pixelStep:
             -((-top+pixelStep-1)/pixelStep)*pixelStep;
        feet=top+pixelH;
    }
    if(left>=400||left+width<=0)return 0;
    int fromX=left<0?-left:0,toX=400-left;
    if(toX>width)toX=width;
    unsigned char celColumn[GG_MESH_MAX_W];
    for(int dx=fromX;dx<toX;dx+=pixelStep)
        celColumn[left+dx]=(unsigned char)(dx*GG_PERSON_W/width);

    // The cel's vertical plane is one depth, but the visible boots
    // span several world points across that plane, each on its own
    // terrain grade. Only the lower legs follow those contacts.
    enum { KNEE_ROW=32 };
    int knee=pixelH*KNEE_ROW/GG_PERSON_H/pixelStep*pixelStep;
    int rough=ggMeshBillboardUneven(m,worldX,worldY,ground,forward,
                                    width/2+pixelStep);
    short soleOffset[GG_MESH_MAX_W];
    int rise=0,lowest=feet;
    if(rough){
        uint32_t legs=0;
        for(int sy=GG_PERSON_H-8;sy<GG_PERSON_H;sy++)
            for(int sx=0;sx<GG_PERSON_W;sx++)
                if(ggMeshPersonCel.pixels[sy*GG_PERSON_W+sx])
                    legs|=1u<<sx;
        int highest=feet;
        for(int dx=fromX;dx<toX;dx+=pixelStep){
            int x=left+dx,sx=celColumn[x];
            soleOffset[x]=0;
            if(!(legs&(1u<<sx)))continue;
            int y=ggMeshBillboardFloorY(m,worldX,worldY,forward,x-center);
            soleOffset[x]=(short)(y-feet);
            if(y<highest)highest=y;
            if(y>lowest)lowest=y;
        }
        // Give the uphill knee room to bend even on a steep bank.
        // The rigid torso rises only enough to keep the shin visible.
        int room=(pixelH-knee)/2/pixelStep*pixelStep;
        rise=ggMeshClamp(feet-highest-room,0,feet-highest);
    }
    top-=rise;
    if(top>=240||lowest<=0)return 0;
    // Contact follows sloped world terrain, with hardware-appropriate
    // cost and occlusion; the sprite body owns no fake screen shadow.
    if(m->scale==1&&forward<5*256)
        ggMeshContactShadow(m,worldX,worldY,24,4);
    // The same air and ambient light as the surrounding geometry keep
    // distant residents inside the scene instead of bright cutouts.
    // Grade the 24 reusable inks ONCE, not three multiplies and a clamp
    // for every covered pixel. The cel retains its painted local light.
    int shade=ggPersonClamp(m->ambientLight+12,128,256);
    GgMeshRgb gain={shade,shade,shade};
    if(m->daylight.cover>=0){
        int angle=heading&255;
        GgMeshNormal face={ggExpCos[angle]*3,ggExpSin[angle]*3,1400};
        GgMeshRgb light=ggMeshDaylightGain(m,face);
        gain=(GgMeshRgb){(light.r*3+256)/4,(light.g*3+256)/4,
                         (light.b*3+256)/4};
    }
    int haze=ggMeshFogAmount(forward,m->fogMax);
    for(int ink=1;ink<GG_PERSON_INKS;ink++){
        unsigned char *color=ggMeshPersonCel.palette[ink];
        int red=ggPersonClamp(color[0]*gain.r/256,0,255);
        int green=ggPersonClamp(color[1]*gain.g/256,0,255);
        int blue=ggPersonClamp(color[2]*gain.b/256,0,255);
        color[0]=(unsigned char)((red*(256-haze)+m->fogR*haze)/256);
        color[1]=(unsigned char)((green*(256-haze)+m->fogG*haze)/256);
        color[2]=(unsigned char)((blue*(256-haze)+m->fogB*haze)/256);
    }
    int inverse=65535/forward;
    int drawn=0;
    // Closeup screen source remains 32x48 with stable wardrobe. Bilinear
    // filtering would smear the tiny facial features; sample the painted
    // anatomy at integer centers with bounded rectangular footprints.
    int fromY=top<0?-top:0,toY=(lowest<240?lowest:240)-top;
    for(int dy=fromY;dy<toY;dy+=pixelStep){
        int y=top+dy,row=dy*GG_PERSON_H/pixelH;
        int adjustLegs=rough&&dy>=knee;
        int slot=(y/m->scale)*m->w;
        for(int dx=fromX;dx<toX;dx+=pixelStep){
            int x=left+dx,sourceY=row;
            if(adjustLegs){
                int change=rise+soleOffset[x],height=pixelH+change;
                if(dy>=height)continue;
                if(change){
                    int shin=height-knee-pixelStep;
                    sourceY=shin>0?KNEE_ROW+(dy-knee)*(GG_PERSON_H-1-KNEE_ROW)/shin:
                                   GG_PERSON_H-1;
                }
            }
            unsigned char ink=ggMeshPersonCel.pixels[
                sourceY*GG_PERSON_W+celColumn[x]];
            if(!ink)continue;
            int z=slot+x/m->scale;
            if(inverse+1<m->depth[z])continue;
            const unsigned char *color=ggMeshPersonCel.palette[ink];
            int i=(y*400+x)*3;
            for(int oy=0;oy<pixelStep;oy++){
                unsigned char*dst=m->rgb+i+oy*400*3;
                for(int ox=0;ox<pixelStep;ox++){
                    dst[ox*3]=color[0];
                    dst[ox*3+1]=color[1];
                    dst[ox*3+2]=color[2];
                }
            }
            // Keep screen-space people in the physical depth hierarchy:
            // a nearer polygon animal cannot be hidden by a farther
            // sprite and a distant citizen cannot be painted over walls.
            if(m->depth[z]<(uint16_t)inverse)
                m->depth[z]=(uint16_t)inverse;
            drawn+=pixelStep*pixelStep;
        }
    }
    if(drawn){
        ggMeshPersonPixels+=drawn;
        ggMeshPersonCount++;
        if(active)ggMeshPersonWalkingDrawn++;
    }
    return drawn;
}
static void ggMeshPersonDraw(GgMeshContext*m,int worldX,int worldY,
                             int ground,int forward,int side,
                             uint32_t identity,int family,int heading,
                             int tick,int active,
                             const unsigned char colors[3][3]){
    if(!ggMeshPersonInView(m,forward,side))return;
    int turn=ggMeshPersonFacing(heading,ggExplore.angle);
    ggPersonGenerate(&ggMeshPersonCel,identity,family,turn,tick,active,colors);
    ggMeshPersonDrawGenerated(m,worldX,worldY,ground,forward,side,heading,active);
}
#endif

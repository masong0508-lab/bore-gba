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
    GgMeshCameraVertex crown={forward,0,ground+30,0,0};
    GgMeshProjected a=ggMeshProject(m,bottom),b=ggMeshProject(m,crown);
    int h=(a.y-b.y)*m->scale;
    return ggPersonClamp(h,0,152);
}
// The source cel is 32x48; the old 32/80 billboard aspect ratio
// squeezed shoulders, faces and legs into conspicuously thin cutouts.
// Keep true cel proportions in world projection and bound close-up cost.
static inline int ggMeshPersonWidth(int pixelH){
    return ggPersonClamp(pixelH*GG_PERSON_W/GG_PERSON_H,4,96);
}
static void ggMeshPersonDraw(GgMeshContext*m,int worldX,int worldY,
                             int ground,int forward,int side,
                             uint32_t identity,int family,int heading,
                             int tick,int active,
                             const unsigned char colors[3][3]){
    if(forward<m->near||forward>11*256)return;
    int pixelH=ggMeshPersonHeight(m,forward,ground);
    if(pixelH<7)return;
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
    if(left>=400||left+width<=0||top>=240||feet<=0)return;
    int turn=ggMeshPersonFacing(heading,ggExplore.angle);
    ggPersonGenerate(&ggMeshPersonCel,identity,family,turn,tick,
                     active,colors);
    // Contact follows sloped world terrain, with hardware-appropriate
    // cost and occlusion; the sprite body owns no fake screen shadow.
    if(m->scale==1&&forward<5*256)
        ggMeshContactShadow(m,worldX,worldY,24,4);
    int shade=ggPersonClamp(m->ambientLight+20,160,256);
    int inverse=65535/forward;
    int drawn=0;
    // Closeup screen source remains 32x48 with stable wardrobe. Bilinear
    // filtering would smear the tiny facial features; sample the painted
    // anatomy at integer centers with bounded rectangular footprints.
    int fromY=top<0?-top:0,toY=240-top;
    if(toY>pixelH)toY=pixelH;
    int fromX=left<0?-left:0,toX=400-left;
    if(toX>width)toX=width;
    for(int dy=fromY;dy<toY;dy+=pixelStep){
        int sourceY=dy*GG_PERSON_H/pixelH;
        for(int dx=fromX;dx<toX;dx+=pixelStep){
            int sourceX=dx*GG_PERSON_W/width;
            unsigned char ink=ggMeshPersonCel.pixels[
                sourceY*GG_PERSON_W+sourceX];
            if(!ink)continue;
            int x=left+dx,y=top+dy;
            int z=(y/m->scale)*m->w+(x/m->scale);
            if(inverse+1<m->depth[z])continue;
            const unsigned char *color=ggMeshPersonCel.palette[ink];
            int i=(y*400+x)*3;
            int light=ggPersonClamp(shade+(sourceX<GG_PERSON_W/2?8:-9),
                                    144,256);
            unsigned char red=(unsigned char)(color[0]*light/256);
            unsigned char green=(unsigned char)(color[1]*light/256);
            unsigned char blue=(unsigned char)(color[2]*light/256);
            for(int oy=0;oy<pixelStep;oy++){
                unsigned char*dst=m->rgb+i+oy*400*3;
                for(int ox=0;ox<pixelStep;ox++){
                    dst[ox*3]=red;
                    dst[ox*3+1]=green;
                    dst[ox*3+2]=blue;
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
}
#endif

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
static void ggMeshPersonDraw(GgMeshContext*m,int worldX,int worldY,
                             int ground,int forward,int side,
                             uint32_t identity,int family,int heading,
                             int tick,int active,
                             const unsigned char colors[3][3]){
    if(forward<m->near||forward>11*256)return;
    int pixelH=ggMeshPersonHeight(m,forward,ground);
    if(pixelH<7)return;
    int width=ggPersonClamp(pixelH*32/80,4,64);
    GgMeshCameraVertex base={forward,side,ground,0,0};
    GgMeshProjected projected=ggMeshProject(m,base);
    int center=projected.x*m->scale,feet=projected.y*m->scale;
    int top=feet-pixelH,left=center-width/2;
    if(left>=400||left+width<0||top>=240||feet<0)return;
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
    for(int dy=fromY;dy<toY;dy++){
        int sourceY=dy*GG_PERSON_H/pixelH;
        for(int dx=fromX;dx<toX;dx++){
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
            m->rgb[i]=(unsigned char)(color[0]*light/256);
            m->rgb[i+1]=(unsigned char)(color[1]*light/256);
            m->rgb[i+2]=(unsigned char)(color[2]*light/256);
            // Keep screen-space people in the physical depth hierarchy:
            // a nearer polygon animal cannot be hidden by a farther
            // sprite and a distant citizen cannot be painted over walls.
            if(m->depth[z]<(uint16_t)inverse)
                m->depth[z]=(uint16_t)inverse;
            drawn++;
        }
    }
    if(drawn){
        ggMeshPersonPixels+=drawn;
        ggMeshPersonCount++;
    }
}
#endif

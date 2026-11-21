// GodGuy 3DS story film: world-space camera composition for readable casts.
// Candidate scoring runs only when entering a paired take (<=24 candidates).
// No heap, per-frame ray searches, texture assets or second framebuffer.
#ifndef GG_FILM_FRAMING3D_H
#define GG_FILM_FRAMING3D_H
#include "film_projection3d.h"
// Match the existing 3DS actor projector's perspective in a tighter,
// subtitle-safe composition. Reject actors clipped by either picture edge.
static int ggFilmFrameProject(int cameraX,int cameraY,int cosA,int sinA,
                              int worldX,int worldY,int *screenX,int *depth){
    int dx=worldX-cameraX,dy=worldY-cameraY;
    int forward=(dx*cosA+dy*sinA)/4096;
    int side=(-dx*sinA+dy*cosA)/4096;
    if(forward<=80||forward>16*256||
       (side<0?-side:side)*4>forward*5)return 0;
    int sx=ggFilmProjectScreenX(side,forward);
    if(sx<65||sx>335)return 0;
    *screenX=sx;*depth=forward;
    return 1;
}
// A camera far above/below the actors flattens their silhouettes
// against tall terrain columns. Prefer shared elevations; unlike an
// absolute rejection, a lone cliff-edge scene can still be filmed.
static int ggFilmFrameHeightPenalty(int stageHeight,int cameraHeight){
    int diff=stageHeight-cameraHeight;
    return (diff<0?-diff:diff)*18;
}
// Larger scores represent stronger compositions: hero always on screen,
// first guest distinguishable beside them, second guest visible if available.
// Readable player/cast spacing outranks arbitrary rotating camera angles.
static int ggFilmFrameScore(int cameraX,int cameraY,int cosA,int sinA,
                            int stageX,int stageY,
                            int plusX,int plusY,int minusX,int minusY,
                            int plusValid,int minusValid){
    int hx,hDepth,firstX,firstDepth,otherX,otherDepth;
    if(!ggFilmFrameProject(cameraX,cameraY,cosA,sinA,
                           stageX,stageY,&hx,&hDepth))return -10000;
    int center=hx-200;if(center<0)center=-center;
    int score=140-center/2;
    // Nearby grounded actors read more clearly than tiny distant figures;
    // retain some horizon in the frame rather than forcing zero distance.
    int distance=hDepth-576;if(distance<0)distance=-distance;
    score-=distance/44;
    if(plusValid||minusValid){
        int gx=plusValid?plusX:minusX,gy=plusValid?plusY:minusY;
        if(!ggFilmFrameProject(cameraX,cameraY,cosA,sinA,
                               gx,gy,&firstX,&firstDepth))return score-220;
        int separation=firstX-hx;if(separation<0)separation=-separation;
        // Gaps below 35 px make two authored human cels merge together.
        if(separation<35)score-=110;
        else if(separation<64)score+=separation*2-50;
        else if(separation<=120)score+=80;
        else score+=35;
        score+=120;
        int midpoint=(firstX+hx)/2-200;
        if(midpoint<0)midpoint=-midpoint;
        score-=midpoint/2;
        if(plusValid&&minusValid){
            if(!ggFilmFrameProject(cameraX,cameraY,cosA,sinA,
                                   minusX,minusY,&otherX,&otherDepth))
                score-=160;
            else{
                int gap=otherX-hx;if(gap<0)gap=-gap;
                if(gap<35)score-=100;
                else score+=85;
                // Two named partners should flank the protagonist,
                // not land in a single overlapping silhouette.
                if((firstX-hx)*(otherX-hx)>=0)score-=95;
            }
        }
    }else score-=120;
    return score;
}
#endif

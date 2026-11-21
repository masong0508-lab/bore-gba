// GodGuy 3DS - foreground geometry mask for cinematic actor layering.
// Captures only the TWO nearest drawn wall/cliff spans per ray column;
// 400 columns x 8 bytes = 3,200 bytes. Not a framebuffer, z image,
// extra raycast, allocation, or change to DS/DSi gameplay.
// Coordinates use the actual 400x240 cinema pixels and Q8 camera depth.
#ifndef GG_FILM_OCCLUSION3D_H
#define GG_FILM_OCCLUSION3D_H
#include <stdint.h>
enum { GG_FILM_OCCLUSION_COLS=400,GG_FILM_OCCLUSION_LAYERS=2 };
typedef struct {
    uint16_t depth[GG_FILM_OCCLUSION_LAYERS];
    uint8_t top[GG_FILM_OCCLUSION_LAYERS];
    uint8_t bottom[GG_FILM_OCCLUSION_LAYERS];
} GgFilmOcclusionColumn;
static inline void ggFilmOcclusionClear(GgFilmOcclusionColumn *columns){
    for(int x=0;x<GG_FILM_OCCLUSION_COLS;x++){
        columns[x].depth[0]=0;
        columns[x].depth[1]=0;
    }
}
// Record only pixels the live raycaster really painted, not a guessed
// "whole building" rectangle. Camera rays run near -> far, so each new
// occupied slot is deeper; exact foreground colors remain untouched.
static inline void ggFilmOcclusionAdd(GgFilmOcclusionColumn *columns,
                                     int x,int top,int bottom,int depth){
    if((unsigned)x>=GG_FILM_OCCLUSION_COLS||depth<=0||depth>65535)return;
    if(top<0)top=0;
    if(bottom>240)bottom=240;
    if(top>=bottom)return;
    GgFilmOcclusionColumn *c=&columns[x];
    for(int i=0;i<GG_FILM_OCCLUSION_LAYERS;i++){
        if(c->depth[i]==depth){
            if(top<c->top[i])c->top[i]=(uint8_t)top;
            if(bottom>c->bottom[i])c->bottom[i]=(uint8_t)bottom;
            return;
        }
        if(!c->depth[i]){
            c->depth[i]=(uint16_t)depth;
            c->top[i]=(uint8_t)top;
            c->bottom[i]=(uint8_t)bottom;
            return;
        }
    }
}
static inline int ggFilmOcclusionHidden(const GgFilmOcclusionColumn *columns,
                                         int x,int y,int actorDepth){
    if((unsigned)x>=GG_FILM_OCCLUSION_COLS||
       (unsigned)y>=240||actorDepth<=0)return 0;
    const GgFilmOcclusionColumn *c=&columns[x];
    for(int i=0;i<GG_FILM_OCCLUSION_LAYERS;i++){
        // 96 Q8 units of tolerance avoid clipping boots/arms against
        // faces in their own plane, where ray steps are quantized.
        if(c->depth[i]&&actorDepth>c->depth[i]+96&&
           y>=c->top[i]&&y<c->bottom[i])return 1;
    }
    return 0;
}
#endif

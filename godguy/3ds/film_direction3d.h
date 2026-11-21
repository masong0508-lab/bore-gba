// Deterministic direction keys for GodGuy's 3DS dialogue shots.
// A guest reply invites a small continuous camera turn; characters
// turn toward one another without instantly spinning or sliding boots.
// No frame history, heap, trig, data arrays or per-frame terrain search.
#ifndef GG_FILM_DIRECTION3D_H
#define GG_FILM_DIRECTION3D_H
static inline int ggFilmDirectionWeight(int local,int beatTicks){
    if(beatTicks<8)return 0;
    if(local<0)local=0;
    if(local>=beatTicks)return 0;
    int enter=beatTicks/4,exit=beatTicks/5;
    int q8=256;
    if(local<enter)q8=local*256/enter;
    if(local>beatTicks-exit){
        int leave=(beatTicks-local)*256/exit;
        if(leave<q8)q8=leave;
    }
    return q8;
}
static inline int ggFilmFocusYaw(int local,int beatTicks,int speakerSide){
    if(!speakerSide)return 0;
    int weight=ggFilmDirectionWeight(local,beatTicks);
    return (speakerSide<0?-1:1)*(weight*3/256);
}
static inline int ggFilmFigureTurn(int local,int beatTicks,int toward){
    if(!toward)return 0;
    int weight=ggFilmDirectionWeight(local,beatTicks);
    return (toward<0?-1:1)*(weight*4/256);
}
#endif
